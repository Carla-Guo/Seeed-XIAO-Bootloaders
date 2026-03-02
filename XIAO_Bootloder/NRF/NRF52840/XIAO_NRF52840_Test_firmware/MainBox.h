#ifndef MAINBOX_h
#define MAINBOX_h

#include <Arduino.h>     // 包含 Arduino 基础库，提供 uint32_t 等类型和 pinMode, digitalWrite 等函数
#include <stdint.h>      // 明确包含 uint32_t 等整数类型定义
#include <SPI.h>         // 用于 SPI 通信
#include <HardwareSerial.h> // 引入 HardwareSerial 以便在类中引用 Serial1

#define BLUE_LED          14
#define GREEN_LED         13
#define RED_LED           12
#define HiCHG             P0_13
#define Statsu_Battery    P0_31
#define Battery_Read_SW   P0_14

// 引入您新的 QSPI Flash 封装库的头文件
// 注意：这里仍然需要包含 qspi_flash.h，因为它提供了 Flash_test() 中将直接调用的函数。
#include "qspi_flash.h"

class CTROL_BOX {
public:
    CTROL_BOX();

    void SeeedXiaoBleInit();
    void RGB_BLINK();
    
    // Test functions
    bool Battery_Test();
    bool IO_Test();
    bool BLE_Scan();
    bool Flash_test(); // Flash Test函数仍然保留，但其内部将直接调用 qspi_flash 封装的函数
    bool MIC_TEST();
    bool LSM6DS3TR_Test();

    // 移除了以下函数声明，因为它们现在由 qspi_flash.h 提供
    // nrfx_err_t QSPI_Initialise();
    // nrfx_err_t QSPI_Erase(uint32_t AStartAddress);
    // bool writeTestResultsToFlash(const char* results, uint32_t address, uint32_t len);
    // bool readTestResultsFromFlash(char* buffer, uint32_t address, uint32_t len);

private:
    /**
     * @brief 从指定 HardwareSerial 端口读取字符串，并带超时功能，检查是否匹配预期响应。
     * @param serialRef HardwareSerial 对象的引用 (例如 Serial1)。
     * @param expectedResponse 期望的 C 字符串响应。
     * @param timeoutMs 等待响应的最大超时时间 (毫秒)。
     * @return 如果在超时时间内收到预期响应，则返回 true，否则返回 false。
     */
    bool readSerialResponse(HardwareSerial& serialRef, const char* expectedResponse, unsigned long timeoutMs);
};

#endif