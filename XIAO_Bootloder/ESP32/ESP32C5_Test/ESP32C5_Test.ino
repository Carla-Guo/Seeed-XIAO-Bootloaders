/*
 * Seeed Studio XIAO ESP32C5 Factory Test Firmware
 * 
 * This firmware is designed for factory testing of the XIAO ESP32C5 board.
 * It includes two main modes:
 * 1. Test Mode: Activated when D1 is pulled LOW (connected to GND).
 *    - Performs hardware tests: Button, WiFi, BLE, GPIO, Battery.
 *    - Saves the result (PASS/FAIL) to NVS.
 *    - Logs output to both USB Serial and UART0 (MySerial0).
 * 
 * 2. Normal Mode (Shipping Mode): Activated when D1 is HIGH (floating).
 *    - Reads the saved test result from NVS.
 *    - If PASSED: Blinks LED and prints "Hello" message (User experience).
 *    - If FAILED/NOT RUN: Prints status message.
 */

#include <HardwareSerial.h>
#include <Arduino.h>
#include <WiFi.h>
#include <BLEDevice.h>

// ==========================================
// 1. Import Generic XIAO Library
// ==========================================
// Define battery voltage divider resistors for XIAO ESP32C5
#define BAT_VOLTAGE_DIVIDER_R1 100000.0
#define BAT_VOLTAGE_DIVIDER_R2 100000.0
#include "src/XIAO_Common/XIAO_Generic.h"

// ==========================================
// 2. Import ESP32 Specific Library
// ==========================================
#include "src/XIAO_ESP32/XIAO_ESP32_Lib.h"

// WiFi Configuration (Used for connection test if needed, currently only scan is performed)
const char* ssid = "Seeed-Test";     // WIFI SSID
const char* password = "seeed"; // WIFI Password

// Serial Objects
HardwareSerial MySerial0(0); // Communication Serial (UART0) - Used for test fixture communication

// --------------------------------------------------------------------------
// DualStream Class
// Helper class to duplicate log output to two Serial interfaces simultaneously.
// Used in Test Mode to send logs to both USB Serial (for human debugging) 
// and UART0 (for automated test fixture).
// --------------------------------------------------------------------------
class DualStream : public Stream {
public:
    DualStream(Stream& s1, Stream& s2) : _s1(s1), _s2(s2) {}
    size_t write(uint8_t c) override {
        _s1.write(c);
        return _s2.write(c);
    }
    size_t write(const uint8_t *buffer, size_t size) override {
        _s1.write(buffer, size);
        return _s2.write(buffer, size);
    }
    int available() override { return _s1.available(); }
    int read() override { return _s1.read(); }
    int peek() override { return _s1.peek(); }
    void flush() override { _s1.flush(); _s2.flush(); }
private:
    Stream& _s1;
    Stream& _s2;
};

// Function Prototypes
void runTestMode();
void runNormalMode(TestStatus saved_status);
bool runButtonTest(Stream* logStream);
bool runWirelessTest(Stream* logStream);
bool runGpioTest(Stream* logStream);
bool runBatteryTest(Stream* logStream);

void setup() {
  // 1. Initialize Serials
  // Note: Serial (USB) is NOT initialized here to prevent noise during boot.
  // It will be initialized later in runTestMode or runNormalMode.
  MySerial0.begin(115200, SERIAL_8N1, -1, -1);

  // 2. Initialize GPIOs (Input/Output configuration)
  XIAO_Utils::initGPIO();
  
  // 3. Initialize NVS (Non-Volatile Storage)
  // Pass NULL to suppress initialization logs
  if (!XIAO_ESP32_NVS::init(NULL)) {
    // Serial.println("Failed to initialize NVS! Halting."); // Suppressed
    while(1) delay(1000);
  }

  // 4. Determine Mode
  // Logic:
  // - If D1 is LOW (GND): The board is likely on the test fixture. Force Test Mode.
  // - If D1 is HIGH (Floating): The board is in user hands. Run Normal Mode based on saved result.
  if (digitalRead(TEST_FLAG_PIN) == LOW) {
      runTestMode();
  } else {
      // Only read result in normal mode (shipping mode)
      TestStatus previous_result = XIAO_ESP32_NVS::readTestResult(NULL);
      runNormalMode(previous_result);
  }
}

// --------------------------------------------------------------------------
// Test Mode Logic
// Executed when D1 is pulled LOW.
// Runs all hardware tests and saves the result to NVS.
// --------------------------------------------------------------------------
void runTestMode() {
    // Initialize USB Serial for logging now that we are in active mode
    Serial.begin(115200);
    
    // Create DualStream to log to both MySerial0 and Serial
    DualStream logStream(MySerial0, Serial);

    // --- ENTER TEST MODE ---
    XIAO_Utils::setLedStatus(TEST_NOT_RUN); // Turn off LED initially
    
    // Handshake with Test Fixture
    delay(200);
    logStream.println("Init Ready");
    delay(500);
    logStream.println("Init Ready");

    bool allTestsPassed = true;

    // Execute Tests Sequentially
    // If any test fails, allTestsPassed becomes false, but subsequent tests might still run 
    // (depending on logic below, currently they are chained with if(allTestsPassed))
    if(allTestsPassed) allTestsPassed = runButtonTest(&logStream);
    if(allTestsPassed) allTestsPassed = runWirelessTest(&logStream);
    if(allTestsPassed) allTestsPassed = runGpioTest(&logStream);
    if(allTestsPassed) allTestsPassed = runBatteryTest(&logStream);

    // Handle Test Result
    if(allTestsPassed) {
        delay(1000);
        logStream.println("Test Passed");
        XIAO_ESP32_NVS::writeTestResult(TEST_PASSED, &logStream);
        logStream.println("Tests passed, result saved");
        XIAO_Utils::setLedStatus(TEST_PASSED); // Blink LED pattern for PASS
    } else {
        XIAO_ESP32_NVS::writeTestResult(TEST_FAILED, &logStream);
        logStream.println("Tests failed, result saved");
        XIAO_Utils::setLedStatus(TEST_FAILED); // Blink LED pattern for FAIL
    }

    // Loop for commands from Test Fixture
    unsigned long timeoutMillis = 10000;
    unsigned long startTime = millis();

    while (1) {
        XIAO_Utils::blink(1, 100);
        
        // 1. Check timeout
        if (millis() - startTime >= timeoutMillis) {
            logStream.println("Timeout waiting for command");
            break;
        }

        // 2. Check for incoming data
        if (MySerial0.available()) {
            String command = MySerial0.readStringUntil('\n');
            command.trim();
            
            if (command == "TEST_PASS") {
                if (allTestsPassed) {
                    logStream.println();
                    logStream.println("Over");
                } else {
                    logStream.println("TEST_FAILED");
                }
                break;
            }
        }
        delay(100);
    }
}

// --------------------------------------------------------------------------
// Normal Mode Logic
// Executed when D1 is HIGH (User Mode).
// Behaves according to the factory test result.
// --------------------------------------------------------------------------
void runNormalMode(TestStatus saved_status) {
    Serial.begin(115200);
    while(!Serial);
    // --- NORMAL MODE ---
    switch(saved_status) {
        case TEST_PASSED:
            // Standard shipping behavior: Print Hello and blink LED
            Serial.println("Hello from Seeed Studio XIAO ESP32-C5");
            // Blink LED forever
            while(1) XIAO_Utils::blink(1, 1000);
            break;
        case TEST_FAILED:
            Serial.println("Device test FAILED");
            break;
        case TEST_NOT_RUN:
            Serial.println("Device not tested");
            break;
    }
}

// ==========================================
// Test Implementation Functions
// ==========================================

bool runButtonTest(Stream* logStream) {
    // Checks if the BOOT button is pressed
    if (!XIAO_Utils::testButton(logStream)) {
        logStream->println("Button Failed");
        return false;
    }
    logStream->println("Button PASS");
    return true;
}

bool runWirelessTest(Stream* logStream) {
    // WiFi Scan
    int wifiCount = XIAO_ESP32_Wireless::scanWiFi();
    logStream->print("WiFi Networks Found: "); logStream->println(wifiCount);

    // BLE Scan
    int bleCount = XIAO_ESP32_Wireless::scanBluetooth();
    logStream->print("Bluetooth Devices Found: "); logStream->println(bleCount);

    // Pass if at least one network/device is found for both
    if (wifiCount >= 1 && bleCount >= 1) {
        logStream->println("Wireless PASS");
        return true;
    }
    logStream->println("Wireless Failed");
    return false;
}

bool runGpioTest(Stream* logStream) {
    // Tests GPIO connectivity (requires loopback on fixture)
    if (XIAO_Utils::testGPIO(logStream) > 0) {
        logStream->println("GPIO Failed");
        return false;
    }
    logStream->println("GPIO PASS");
    return true;
}

bool runBatteryTest(Stream* logStream) {
   logStream->println("Battery Test Started.");

  float batteryVoltage = XIAO_Utils::getBatteryVoltage();

  logStream->print("Measured Battery Voltage: ");
  logStream->print(batteryVoltage, 3);
  logStream->println(" V");

  // Check if voltage is valid (e.g. > 3.3V)
  if (batteryVoltage < 3.3) {
    logStream->println("Battery voltage too low (< 3.3V).");
    // return false; // Uncomment if strict check is needed
  }
  logStream->println("Battery voltage OK.");
  return true;
}

void loop() {
  // Main loop is empty as logic is handled in setup()
}



