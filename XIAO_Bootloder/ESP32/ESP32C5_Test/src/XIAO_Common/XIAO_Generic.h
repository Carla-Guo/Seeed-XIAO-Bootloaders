#ifndef XIAO_GENERIC_H
#define XIAO_GENERIC_H

#include <Arduino.h>

// ==========================================
// 1. Common Definitions (From XIAO_Common.h)
// ==========================================

// Test Status Enum
enum TestStatus {
    TEST_NOT_RUN = 0,
    TEST_PASSED = 1,
    TEST_FAILED = 2
};

// Pin Definitions
static const uint8_t INPUT_PINS[] = {D0, D3, D5, D4};
static const uint8_t OUTPUT_PINS[] = {D2, D10, D8, D9}; 

static const uint8_t TEST_FLAG_PIN = D1;

// Battery & Button Definitions
#ifndef Statsu_Battery
#define Statsu_Battery 6
#endif
#ifndef Battery_Read_SW
#define Battery_Read_SW 26
#endif
#ifndef Boot
#define Boot 28
#endif

static const uint8_t BAT_ADC_PIN = Statsu_Battery;
static const uint8_t BAT_READ_CTRL_PIN = Battery_Read_SW;
static const uint8_t BUTTON_PIN = Boot;
static const uint8_t BUTTON_PWR_PIN = D2;

// Battery Voltage Divider Resistors (Default: R1=1M, R2=510K)
#ifndef BAT_VOLTAGE_DIVIDER_R1
#define BAT_VOLTAGE_DIVIDER_R1 100000.0
#endif
#ifndef BAT_VOLTAGE_DIVIDER_R2
#define BAT_VOLTAGE_DIVIDER_R2 100000.0
#endif

// ==========================================
// 2. IO & Utils Class (Merged XIAO_IO & XIAO_TestUtils)
// ==========================================

class XIAO_Utils {
public:
    // --- GPIO & Hardware ---
    static void initGPIO();
    static int testGPIO(Stream* debugSerial = &Serial);
    static bool testButton(Stream* debugSerial = &Serial);
    static void blink(int times = 1, int delay_ms = 500);
    static float getBatteryVoltage();
    
    // --- System Status ---
    static void setLedStatus(TestStatus status);
};

#endif
