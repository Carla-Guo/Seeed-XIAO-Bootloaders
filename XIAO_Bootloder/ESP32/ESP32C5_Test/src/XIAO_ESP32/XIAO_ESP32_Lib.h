#ifndef XIAO_ESP32_LIB_H
#define XIAO_ESP32_LIB_H

#include <Arduino.h>
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <nvs_flash.h>
#include <nvs.h>
#include "../XIAO_Common/XIAO_Generic.h" // For TestStatus enum

// ==========================================
// 1. Wireless (WiFi & BLE)
// ==========================================

class XIAO_ESP32_Wireless {
public:
    static int scanWiFi();
    static int scanBluetooth();
};

// ==========================================
// 2. NVS (ESP32 Specific)
// ==========================================

class XIAO_ESP32_NVS {
public:
    static bool init(Stream* debugSerial = &Serial);
    static TestStatus readTestResult(Stream* debugSerial = &Serial);
    static bool writeTestResult(TestStatus result, Stream* debugSerial = &Serial);
};

#endif
