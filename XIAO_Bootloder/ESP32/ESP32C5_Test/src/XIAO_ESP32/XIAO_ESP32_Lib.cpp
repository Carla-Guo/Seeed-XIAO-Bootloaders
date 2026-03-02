#include "XIAO_ESP32_Lib.h"

// ==========================================
// Wireless Implementation
// ==========================================

int XIAO_ESP32_Wireless::scanWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    return WiFi.scanNetworks();
}

int XIAO_ESP32_Wireless::scanBluetooth() {
    BLEDevice::init("");
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
    BLEScanResults* foundDevices = pBLEScan->start(5);
    int devicesFound = foundDevices->getCount();
    pBLEScan->clearResults();
    return devicesFound;
}

// ==========================================
// NVS Implementation
// ==========================================

static const char* NVS_NAMESPACE = "board_status";
static const char* NVS_KEY_TEST_PASSED = "test_passed";

bool XIAO_ESP32_NVS::init(Stream* debugSerial) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        if(debugSerial) debugSerial->println("Erasing NVS partition...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    return (ret == ESP_OK);
}

TestStatus XIAO_ESP32_NVS::readTestResult(Stream* debugSerial) {
    nvs_handle_t nvs_handle;
    uint8_t stored_value = TEST_NOT_RUN;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle) != ESP_OK) return TEST_NOT_RUN;
    esp_err_t ret = nvs_get_u8(nvs_handle, NVS_KEY_TEST_PASSED, &stored_value);
    nvs_close(nvs_handle);
    if (ret != ESP_OK || stored_value > TEST_FAILED) return TEST_NOT_RUN;
    return (TestStatus)stored_value;
}

bool XIAO_ESP32_NVS::writeTestResult(TestStatus result, Stream* debugSerial) {
    nvs_handle_t nvs_handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) != ESP_OK) return false;
    esp_err_t set_ret = nvs_set_u8(nvs_handle, NVS_KEY_TEST_PASSED, result);
    esp_err_t commit_ret = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    if (set_ret == ESP_OK && commit_ret == ESP_OK) {
        if(debugSerial) debugSerial->println("Test result saved to NVS");
        return true;
    }
    return false;
}
