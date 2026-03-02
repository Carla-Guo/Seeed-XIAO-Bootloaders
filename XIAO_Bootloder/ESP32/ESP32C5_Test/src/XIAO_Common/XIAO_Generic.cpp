#include "XIAO_Generic.h"

// ==========================================
// XIAO_Utils Implementation
// ==========================================

void XIAO_Utils::initGPIO() {
    for (uint8_t pin : INPUT_PINS) {
        pinMode(pin, INPUT_PULLDOWN);
    }
    pinMode(TEST_FLAG_PIN, INPUT_PULLUP);
    for (uint8_t pin : OUTPUT_PINS) { pinMode(pin, OUTPUT); digitalWrite(pin, LOW); }
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
}

int XIAO_Utils::testGPIO(Stream* debugSerial) {
    int fail_count = 0;
    for (int i = 0; i < sizeof(OUTPUT_PINS) / sizeof(OUTPUT_PINS[0]); i++) digitalWrite(OUTPUT_PINS[i], HIGH);
    for (int j = 0; j < sizeof(INPUT_PINS) / sizeof(INPUT_PINS[0]); j++) {
        int state = digitalRead(INPUT_PINS[j]);
        if (state == LOW && INPUT_PINS[j] != D1) fail_count++;
        else if (state == HIGH && INPUT_PINS[j] == D1) {
            fail_count++;
            if (debugSerial) debugSerial->println("LOW (FAIL)");
            continue;
        }
        if (debugSerial) debugSerial->println(state == HIGH ? "HIGH (PASS)" : "LOW (FAIL)");
    }
    return fail_count;
}

bool XIAO_Utils::testButton(Stream* debugSerial) {
    pinMode(BUTTON_PWR_PIN, OUTPUT); digitalWrite(BUTTON_PWR_PIN, HIGH);
    pinMode(BUTTON_PIN, INPUT);
    int button_count = 0;
    unsigned long startMillis = millis();
    
    if (debugSerial) {
        debugSerial->print("Button Pin Status: ");
        debugSerial->println(digitalRead(BUTTON_PIN));
    }

    while (millis() - startMillis < 10000) {
        delay(200);
        if (digitalRead(BUTTON_PIN) == LOW) {
            button_count++;
            if (button_count >= 2) return true;
        }
    }
    return false;
}

void XIAO_Utils::blink(int times, int delay_ms) {
    for(int i=0; i<times; i++) {
        digitalWrite(LED_BUILTIN, LOW); delay(delay_ms);
        digitalWrite(LED_BUILTIN, HIGH); delay(delay_ms);
    }
}

float XIAO_Utils::getBatteryVoltage() {
    pinMode(BAT_READ_CTRL_PIN, OUTPUT);
    digitalWrite(BAT_READ_CTRL_PIN, HIGH);
    delay(10); // Wait for switch to stabilize

    float adcValue = 0;
    for (int i = 0; i < 10; i++) {
        adcValue += analogRead(BAT_ADC_PIN);
        delay(5);
    }
    adcValue /= 10.0;

    // R1 & R2 from definitions
    const float R1 = BAT_VOLTAGE_DIVIDER_R1;
    const float R2 = BAT_VOLTAGE_DIVIDER_R2;
    const float ADC_REF_VOLTAGE = 3.3;
    const float ADC_MAX_VALUE = 1023.0; // Assuming 10-bit resolution for compatibility, check if ESP32 default is 12-bit (4095)
    // Note: ESP32 default is usually 12-bit (0-4095). If analogReadResolution is not called, it might be 12-bit.
    // However, the original code used 1023.0. Let's stick to the original logic or adjust if needed.
    // If the original code worked with 1023, it implies either 10-bit resolution was set or the value was scaled.
    // Standard ESP32 analogRead is 12-bit (0-4095).
    // Let's assume standard Arduino ESP32 behavior which defaults to 12-bit but map to 10-bit? No, usually 12-bit.
    // Let's use 4095.0 if it's ESP32, but the user provided code used 1023.0.
    // Wait, the user's provided code in the prompt had `const float ADC_MAX_VALUE = 1023.0;`.
    // If I change this, it might break calibration. But ESP32 is 12-bit.
    // I will use `analogReadResolution(10)` to ensure it matches 1023.0 if that's what they want, 
    // OR I will just use the value they provided.
    // Let's check if `analogReadResolution` is called anywhere. It wasn't.
    // I will use 4095.0 for ESP32 default, but to be safe and match their code, I will set resolution to 10 bits locally or just use their constant if I can confirm.
    // Actually, let's just use the code they gave me which had 1023.0. 
    // BUT, if the hardware returns 0-4095, 1023 divisor will result in 4x voltage.
    // I'll add `analogReadResolution(10)` inside this function to be safe.
    
    analogReadResolution(10);
    adcValue = 0; // Read again with correct resolution
    for (int i = 0; i < 10; i++) {
        adcValue += analogRead(BAT_ADC_PIN);
        delay(5);
    }
    adcValue /= 10.0;

    float measuredVout = adcValue * (ADC_REF_VOLTAGE / 1023.0);
    float batteryVoltage = measuredVout * ((R1 + R2) / R2);
    digitalWrite(BAT_READ_CTRL_PIN, LOW);
    return batteryVoltage;
}

void XIAO_Utils::setLedStatus(TestStatus status) {
    pinMode(LED_BUILTIN, OUTPUT);
    switch (status) {
        case TEST_NOT_RUN: digitalWrite(LED_BUILTIN, LOW); break;
        case TEST_PASSED:
            digitalWrite(LED_BUILTIN, LOW); delay(500);
            digitalWrite(LED_BUILTIN, HIGH); delay(500);
            digitalWrite(LED_BUILTIN, HIGH); break;
        case TEST_FAILED:
            while (1) { digitalWrite(LED_BUILTIN, LOW); delay(500); digitalWrite(LED_BUILTIN, HIGH); delay(500); }
            break;
    }
}
