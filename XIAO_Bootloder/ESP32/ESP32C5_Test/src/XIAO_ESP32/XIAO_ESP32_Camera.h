#ifndef XIAO_ESP32_CAMERA_H
#define XIAO_ESP32_CAMERA_H

#ifdef ENABLE_CAMERA

#include <Arduino.h>
#include <WiFi.h>
#include "esp_camera.h"

// XIAO ESP32S3 Sense Camera Pin Definitions
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     10
#define SIOD_GPIO_NUM     40
#define SIOC_GPIO_NUM     39
#define Y9_GPIO_NUM       48
#define Y8_GPIO_NUM       11
#define Y7_GPIO_NUM       12
#define Y6_GPIO_NUM       14
#define Y5_GPIO_NUM       16
#define Y4_GPIO_NUM       18
#define Y3_GPIO_NUM       17
#define Y2_GPIO_NUM       15
#define VSYNC_GPIO_NUM    38
#define HREF_GPIO_NUM     47
#define PCLK_GPIO_NUM     13

static const uint8_t EXT_INPUT_PINS[] = {13, 12, 11, 10, 14, 16, 42, 40, 38};
static const uint8_t EXT_OUTPUT_PINS[] = {D2, D8, D9, D10, 15, 17, 41, 39, 47};

extern void startCameraServer(); // External from app_httpd.cpp

class XIAO_ESP32_Camera {
public:
    static bool begin(const char* ssid, const char* password);
    static void printConnectionInfo(Stream& serial);
    static int testEXTGPIO(Stream* debugSerial = &Serial);
private:
    static bool initCamera();
    static bool connectWiFi(const char* ssid, const char* password);
};

#endif // ENABLE_CAMERA
#endif
