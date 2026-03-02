#include "XIAO_ESP32_Camera.h"

#ifdef ENABLE_CAMERA

// ==========================================
// Camera Implementation
// ==========================================

bool XIAO_ESP32_Camera::begin(const char* ssid, const char* password) {
    if (!initCamera()) return false;
    if (!connectWiFi(ssid, password)) return false;
    startCameraServer();
    return true;
}

bool XIAO_ESP32_Camera::initCamera() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.frame_size = FRAMESIZE_UXGA;
    config.pixel_format = PIXFORMAT_JPEG;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 12;
    config.fb_count = 1;

    if (config.pixel_format == PIXFORMAT_JPEG) {
        if (psramFound()) {
            config.jpeg_quality = 10;
            config.fb_count = 2;
            config.grab_mode = CAMERA_GRAB_LATEST;
        } else {
            config.frame_size = FRAMESIZE_SVGA;
            config.fb_location = CAMERA_FB_IN_DRAM;
        }
    } else {
        config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
        config.fb_count = 2;
#endif
    }

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        return false;
    }

    sensor_t *s = esp_camera_sensor_get();
    if (s->id.PID == OV3660_PID) {
        s->set_vflip(s, 1); s->set_brightness(s, 1); s->set_saturation(s, -2);
    }
    if (config.pixel_format == PIXFORMAT_JPEG) s->set_framesize(s, FRAMESIZE_QVGA);

    return true;
}

bool XIAO_ESP32_Camera::connectWiFi(const char* ssid, const char* password) {
    WiFi.begin(ssid, password);
    WiFi.setSleep(false);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        delay(500); Serial.print(".");
        if (millis() - start > 20000) { Serial.println("\nWiFi Connection Timeout"); return false; }
    }
    Serial.println("\nWiFi connected");
    return true;
}

void XIAO_ESP32_Camera::printConnectionInfo(Stream& serial) {
    serial.print("Camera Ready! Use 'http://");
    serial.print(WiFi.localIP());
    serial.println("' to connect");
}

int XIAO_ESP32_Camera::testEXTGPIO(Stream* debugSerial) {
    // Initialize pins first
    for (uint8_t pin : EXT_INPUT_PINS) pinMode(pin, INPUT_PULLDOWN);
    for (uint8_t pin : EXT_OUTPUT_PINS) { pinMode(pin, OUTPUT); digitalWrite(pin, LOW); }

    int fail_count = 0;
    for (int i = 0; i < sizeof(EXT_OUTPUT_PINS) / sizeof(EXT_OUTPUT_PINS[0]); i++) digitalWrite(EXT_OUTPUT_PINS[i], HIGH);
    for (int j = 0; j < sizeof(EXT_INPUT_PINS) / sizeof(EXT_INPUT_PINS[0]); j++) {
        int state = digitalRead(EXT_INPUT_PINS[j]);
        if (debugSerial) debugSerial->println(state == HIGH ? "HIGH (PASS)" : "LOW (FAIL)");
        if (state == LOW) fail_count++;
    }
    return fail_count;
}

#endif
