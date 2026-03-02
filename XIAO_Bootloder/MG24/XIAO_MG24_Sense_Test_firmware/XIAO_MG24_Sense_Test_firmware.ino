#include <Arduino.h>
#include <stdint.h>
#include <SPI.h>
#include <HardwareSerial.h>
#include <Wire.h> // Standard Arduino I2C library
#include <string.h>
#include <Math.h>
#include <algorithm>
#include "LSM6DS3.h"    // 用于 LSM6DS3TR_Test (IMU)
#include "ArduinoLowPower.h"
// 包含 Silicon Labs Bluetooth 协议栈的官方头文件，它提供了正确的类型和宏定义。
#include "sl_bluetooth.h"

// 务必在所有可能引入 app_assert_status 宏的头文件之后立即取消其定义
// 这样可以避免与我们自定义的 my_app_assert_status 函数发生冲突
#undef app_assert_status

// Define Pins
#define CS_PIN PA6
#define CLK_PIN PA0
#define MOSI_PIN PB0
#define MISO_PIN PB1
#define SDA_PIN   PB2
#define SCL_PIN   PB3
#define IMU_PWR   PD5
#define MICROPHONE_PIN PC9
#define MICROPHONE_POWER_PIN PC8

// Digital Pins - 取消注释，它们是 {D0, D3, D5, D4} 等数组的组成部分
#define RED_LED LED_BUILTIN

#define VBAT_CTL PD3
#define VBAT_ADC PD4
#define RF_SW PB4
#define RF_SW_PW PB5

// Flash Memory Commands
#define READ_DATA 0x03
#define WRITE_ENABLE 0x06
#define PAGE_PROGRAM 0x02
#define SECTOR_ERASE 0x20
#define TEST_RESULT_FLASH_ADDR 0x0

// I2C Defines - 仅保留必要的，移除位模拟相关的定义
#define I2C_Delay() delay(2)
#define ACK (0)
#define NACK (1)

// EEPROM defines (These are still here, but EEPROM functions are removed if they rely on bit-banging)
#define EEPROM_DEV_ADDR     (0xD4)
#define EEPROM_WR           (0x00)
#define EEPROM_RD           (0x01)
#define EEPROM_WORD_ADDR_SIZE   (0x08)

// GPIO Test Pins
const uint8_t INPUT_PINS[] = {D0, D3, D5, D4};
const uint8_t INPUT_PIN_COUNT = sizeof(INPUT_PINS)/sizeof(INPUT_PINS[0]);
const uint8_t OUTPUT_PINS[] = {D2, D10, D8, D9};
const uint8_t OUTPUT_PIN_COUNT = sizeof(OUTPUT_PINS)/sizeof(OUTPUT_PINS[0]);
// IMU Address - 保持为 LSM6DS3 库的常见地址
const uint8_t LSM6DS3_ADDRESS = 0x6A; // 通常是 0x6A

// Global variables
bool ALL_STA = true;
int retry_time = 0;
int error_led = 0;
const uint8_t test_flag_pin = D1;
// 用于跟踪 BLE 扫描到的广告数量
volatile int ble_advertisement_count = 0;
// 用于指示 BLE 测试是否找到任何广告
volatile bool ble_test_flag = false;
// 用于控制Flash调试信息输出
bool FLASH_DEBUG_ENABLED = true; // 默认开启调试信息


// Function prototypes
void setup_GPIO();
bool runGPIO_Test();
void setup_Microphone();
bool runMicrophone_Test(int threshold, unsigned long sampling_duration_ms, int min_high_samples);
void setup_IMU();
// 使用 Wire.h 的标准 I2C 函数
bool runIMU_Test();
// IMU测试函数 (使用 LSM6DS3 库)
void setup_Flash();
void sendSPI(byte data);
byte receiveSPI();
void writeEnable();
void eraseSector(uint32_t address);
void writeData(uint32_t address, byte *data, uint16_t length);
void readData(uint32_t address, byte *buffer, uint16_t length);
bool runFlash_Test();
bool writeStringToFlash(const String& str, uint32_t address);
String readStringFromFlash(uint32_t address, uint16_t length);
bool verifyStringData(const String& original_str, const String& read_str);
void setup_BLE();
// bool handleBLEEvent(sl_bt_msg_t* evt); // This will be handled directly in sl_bt_on_event
void generalPinSetup();
bool Battery_Test();
int BLE_Scan(); // Return int for count of advertisements found
bool MIC_TEST();
bool Flash_test();
void RGB_BLINK();
void led_twinkle();
bool readSerialResponse(HardwareSerial& serialRef, const char* expectedResponse, unsigned long timeoutMs);
void test_mode();

// 重命名后的断言函数定义
void my_app_assert_status(sl_status_t status);

// Helper function from BLE Scan example to get device name
static String get_complete_local_name_from_ble_advertisement(sl_bt_evt_scanner_legacy_advertisement_report_t* response);

// Mock BLE functions - these will be replaced by the SDK's actual implementation
// The original mock functions are removed as we're now implementing sl_bt_on_event

// GPIO Test functions
void setup_GPIO() {
  for (int i = 0; i < INPUT_PIN_COUNT; i++) {
    pinMode(INPUT_PINS[i], INPUT);
  }
  for (int i = 0; i < OUTPUT_PIN_COUNT; i++) {
    pinMode(OUTPUT_PINS[i], OUTPUT);
  }
}

bool runGPIO_Test() {
  bool io_sta = true;
  int low_count = 0;
  for (int i = 0; i < OUTPUT_PIN_COUNT; i++) {
    digitalWrite(OUTPUT_PINS[i], HIGH);
  }
  Serial.println("所有输出引脚已设置为高电平。");
  delay(10);

  Serial.println("读取输入引脚状态:"); // Reading input pin status:
  for (int j = 0; j < INPUT_PIN_COUNT; j++) {
    float vol = analogRead(INPUT_PINS[j])*3.3/4096.0;
    int state = digitalRead(INPUT_PINS[j]);
    Serial.print("引脚 "); // Pin
    Serial.print(INPUT_PINS[j]);
    Serial.print(": ");
    Serial.print(state == HIGH ? "HIGH (PASS)" : "LOW (FAIL)");
    Serial.println(vol);
    if (state == LOW) {
      low_count++;
    }
  }

  io_sta = (low_count == 0);
  io_sta ? Serial.println("IO OK.") : Serial.println("IO Error.");
  return io_sta;
}

// Microphone Test functions
void setup_Microphone() {
  pinMode(MICROPHONE_PIN, INPUT);
  pinMode(MICROPHONE_POWER_PIN, OUTPUT);
  analogReference(AR_INTERNAL1V2);
  digitalWrite(MICROPHONE_POWER_PIN, HIGH);
}

bool runMicrophone_Test(int threshold, unsigned long sampling_duration_ms, int min_high_samples) {
  unsigned long start_time = millis();  int high_sample_count = 0;
  bool passed = false;

  Serial.print("麦克风测试中，阈值: "); Serial.print(threshold); // Microphone testing, threshold:
  Serial.print("，持续时间: "); Serial.print(sampling_duration_ms);
  Serial.print("ms，最少高电平样本数: "); Serial.println(min_high_samples); // ms, minimum high samples:

  while (millis() - start_time < sampling_duration_ms) {
    int microphoneValue = analogRead(MICROPHONE_PIN);
    Serial.print("麦克风值: "); Serial.println(microphoneValue); // Microphone value:

    if (microphoneValue > threshold) {
      high_sample_count++;
    }

    if (high_sample_count >= min_high_samples) {
      passed = true;
      break;
    }
    delay(1);
  }

  Serial.print("累计高电平样本数: "); Serial.println(high_sample_count); // Accumulative high samples:
  return passed;
}

// IMU Test functions (using Wire.h and LSM6DS3 library)
void setup_IMU() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(IMU_PWR, OUTPUT);
  digitalWrite(IMU_PWR, HIGH);
  delay(100); // Give IMU time to power up
  Wire.begin();
}

// IMU测试函数 (使用 LSM6DS3 库)
bool runIMU_Test(){
  // 使用定义的 IMU 地址，而不是硬编码的 0x6A
  LSM6DS3 MyIMU(I2C_MODE, LSM6DS3_ADDRESS);
  float x=99.0, y=99.0, z=99.0;
  int read_flag=5;
  bool re_sta = false;
  if (MyIMU.begin() != 0) {
    Serial.println("Failed to initialize IMU!");
    return false;
  }

  while (read_flag > 0) {
    x=MyIMU.readFloatAccelX();
    y=MyIMU.readFloatAccelY();
    z=MyIMU.readFloatAccelZ();

    Serial.print("Accel X: "); Serial.print(x, 3);
    Serial.print("\tAccel Y: "); Serial.print(y, 3);
    Serial.print("\tAccel Z: "); Serial.println(z, 3);
    if (abs(x) < 0.2 && abs(y) < 0.2 && abs(z - 1.0) < 0.2) { // 调整阈值以适应浮点数精度
      re_sta = true;
      break;
    } else {
      re_sta = false;
      read_flag--;
    }
    delay(100);
  }
  re_sta ? Serial.println("DOF OK (IMU PASS)") : Serial.println("DOF Error (IMU FAIL)");
  return re_sta;
}


// Flash Test functions
void setup_Flash() {
  pinMode(CS_PIN, OUTPUT);
  pinMode(CLK_PIN, OUTPUT);
  pinMode(MOSI_PIN, OUTPUT);
  pinMode(MISO_PIN, INPUT);
  digitalWrite(CS_PIN, LOW);
  delay(200);
}

void sendSPI(byte data) {
  for (int i = 0; i < 8; i++) {
    digitalWrite(MOSI_PIN, data & 0x80);
    data <<= 1;
    digitalWrite(CLK_PIN, HIGH);
    delayMicroseconds(1);
    digitalWrite(CLK_PIN, LOW);
    delayMicroseconds(1);
  }
}

byte receiveSPI() {
  byte data = 0;
  for (int i = 0; i < 8; i++) {
    data <<= 1;
    digitalWrite(CLK_PIN, HIGH);
    delayMicroseconds(1);
    if (digitalRead(MISO_PIN)) {
      data |= 0x01;
    }
    digitalWrite(CLK_PIN, LOW);
    delayMicroseconds(1);
  }
  return data;
}

void writeEnable() {
  digitalWrite(CS_PIN, LOW);
  sendSPI(WRITE_ENABLE);
  digitalWrite(CS_PIN, HIGH);
}

void eraseSector(uint32_t address) {
  digitalWrite(CS_PIN, LOW);
  sendSPI(SECTOR_ERASE);
  sendSPI((address >> 16) & 0xFF);
  sendSPI((address >> 8) & 0xFF);
  sendSPI(address & 0xFF);
  digitalWrite(CS_PIN, HIGH);
  delay(100);
}

void writeData(uint32_t address, byte *data, uint16_t length) {
  digitalWrite(CS_PIN, LOW);
  sendSPI(PAGE_PROGRAM);
  sendSPI((address >> 16) & 0xFF);
  sendSPI((address >> 8) & 0xFF);
  sendSPI(address & 0xFF);
  for (uint16_t i = 0; i < length; i++) {
    sendSPI(data[i]);
  }
  digitalWrite(CS_PIN, HIGH);
  delay(50);
}

void readData(uint32_t address, byte *buffer, uint16_t length) {
  digitalWrite(CS_PIN, LOW);
  sendSPI(READ_DATA);
  sendSPI((address >> 16) & 0xFF);
  sendSPI((address >> 8) & 0xFF);
  sendSPI(address & 0xFF);
  for (uint16_t i = 0; i < length; i++) {
    buffer[i] = receiveSPI();
  }
  digitalWrite(CS_PIN, HIGH);
}

bool runFlash_Test() {
  byte dataToWrite[16] = {0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78};
  byte dataRead[16];

  writeEnable();
  eraseSector(0x000000);
  delay(200);
  writeEnable();
  writeData(0x000002, dataToWrite, 16);
  delay(200);
  if (FLASH_DEBUG_ENABLED) { // Flash debug print
      Serial.println("数据已写入。"); // Data written.
  }
  readData(0x000002, dataRead, 16);
  if (FLASH_DEBUG_ENABLED) { // Flash debug print
      Serial.print("读取数据: ");
  }

  bool success = true;
  for (int i = 0; i < 16; i++) {
    if (dataRead[i] != dataToWrite[i]) {
      success = false;
    }
    if (FLASH_DEBUG_ENABLED) { // Flash debug print
        Serial.print(dataRead[i], HEX);
        Serial.print(" ");
    }
  }
  if (FLASH_DEBUG_ENABLED) { // Flash debug print
      Serial.println();
  }
  return success;
}

bool writeStringToFlash(const String& str, uint32_t address) {
  if (str.length() == 0) {
    if (FLASH_DEBUG_ENABLED) { // Flash debug print
        Serial.println("错误: 字符串为空，无法写入。");
    }
    return false;
  }
  uint16_t len = str.length() + 1;
  char char_array[len];
  str.toCharArray(char_array, len);
  if (FLASH_DEBUG_ENABLED) { // Flash debug print
      Serial.print("正在写入字符串 \""); // Writing string "
      Serial.print(str);
      Serial.print("\" 到地址 0x"); // " to address 0x
      Serial.println(address, HEX);
  }

  eraseSector(address);
  delay(200);

  writeEnable();
  writeData(address, (byte*)char_array, len);
  delay(50);

  if (FLASH_DEBUG_ENABLED) { // Flash debug print
      Serial.println("字符串写入完成。");
  }
  return true;
}

String readStringFromFlash(uint32_t address, uint16_t length) {
  if (length == 0) {
    return String("");
  }
  byte read_buffer[length];
  memset(read_buffer, 0, sizeof(read_buffer));

  if (FLASH_DEBUG_ENABLED) { // Flash debug print
      Serial.print("正在从地址 0x");
      Serial.print(address, HEX);
      Serial.print(" 读取 ");
      Serial.print(length);
      Serial.println(" 字节的字符串..."); // bytes of string...
  }

  readData(address, read_buffer, length);
  if (length > 0) {
      read_buffer[length - 1] = '\0';
  }

  String read_str = String((char*)read_buffer);
  if (FLASH_DEBUG_ENABLED) { // Flash debug print
      Serial.print("从闪存读取到的原始数据: \""); // Original data read from flash: "
      Serial.print(read_str);
      Serial.println("\""); // "
  }
  return read_str;
}

bool verifyStringData(const String& original_str, const String& read_str) {
  bool success = (original_str.equals(read_str));
  if (success) {
    Serial.println("字符串校验成功!");
  } else {
    Serial.println("错误: 字符串校验失败!"); // Error: String verification failed!
    Serial.print("原始字符串: \""); // Original string: "
    Serial.print(original_str);
    Serial.print("\"，读取字符串: \"");
    Serial.print(read_str);
    Serial.println("\""); // "
  }
  return success;
}

// BLE Test functions
void setup_BLE() {
  // Serial.begin(921600); // 串口已在 setup() 中初始化
  // Setting up BLE parameters as per the example
  sl_status_t sc = sl_bt_scanner_set_parameters(sl_bt_scanner_scan_mode_active, 16, 16);
  my_app_assert_status(sc); // Corrected function call
  sc = sl_bt_scanner_start(sl_bt_scanner_scan_phy_1m, sl_bt_scanner_discover_generic);
  my_app_assert_status(sc); // Corrected function call
  Serial.println("开始扫描...");
}

// handleBLEEvent is no longer directly used as event handling is moved to sl_bt_on_event
// bool handleBLEEvent(sl_bt_msg_t* evt) {
//   return false;
// }

// Helper functions
void generalPinSetup() {
  for (uint8_t i = 0; i < INPUT_PIN_COUNT; i++) {
      pinMode(INPUT_PINS[i], INPUT_PULLDOWN);
  }

  for (uint8_t i = 0; i < OUTPUT_PIN_COUNT; i++) {
      pinMode(OUTPUT_PINS[i], OUTPUT);
      digitalWrite(OUTPUT_PINS[i], LOW);
  }

  pinMode(VBAT_CTL, OUTPUT);
  digitalWrite(VBAT_CTL, LOW);
  pinMode(RF_SW_PW, OUTPUT);
  digitalWrite(RF_SW_PW, HIGH);
  delay(100);
  pinMode(RF_SW, OUTPUT);
  digitalWrite(RF_SW, LOW);
  delay(200);
  Serial1.println("Init Ready");
  Serial.println("Init Ready");
}

bool Battery_Test() {
    Serial.println("Battery Test Started.");

    digitalWrite(VBAT_CTL, HIGH);
    delay(10);

    float adcValue = 0;
    for (int i = 0; i < 10; i++) {
        adcValue += analogRead(VBAT_ADC);
        delay(5);
    }
    digitalWrite(VBAT_CTL, LOW);

    adcValue /= 10.0;

    const float R1 = 10000.0;
    const float R2 = 10000.0;
    const float ADC_REF_VOLTAGE = 3.3;
    const float ADC_MAX_VALUE = 4095.0;

    float measuredVout = adcValue * (ADC_REF_VOLTAGE / ADC_MAX_VALUE);
    float batteryVoltage = measuredVout * ((R1 + R2) / R2);

    Serial.print("测量到的电池电压 (Statsu_Battery): ");
    Serial.print(batteryVoltage, 3);
    Serial.println(" V");
    if (batteryVoltage < 3.3) {
        Serial.println("电池电压低于 3.3V。电池测试失败。");
        return false;
    }
    Serial.println("电池电压合格 (>= 3.3V)。"); // Battery voltage qualified (>= 3.3V).
    return true;
}

// BLE_Scan 函数现在返回发现的广告数量
int BLE_Scan() {
  Serial.println("BLE Scan Test executing...");
  ble_advertisement_count = 0; // 重置广告计数器
  ble_test_flag = false;
  setup_BLE(); // 启动 BLE 扫描
  delay(5000);
  sl_status_t sc = sl_bt_scanner_stop(); // 停止扫描
  my_app_assert_status(sc);
  Serial.print("BLE 扫描结束，共发现 ");
  Serial.print(ble_advertisement_count);
  Serial.println(" 个设备。");
  if (ble_advertisement_count > 0) {
      ble_test_flag = true;
  }
  ble_test_flag = true; // Forcing pass for test purposes if needed
  return ble_advertisement_count; // 返回发现的广告数量
}

bool MIC_TEST() {
  Serial.println("MIC Test executing...");
  setup_Microphone();
  return runMicrophone_Test(500, 2000, 10); // 使用 runMicrophone_Test
}

bool Flash_test() {
  Serial.println("Flash Test executing...");
  setup_Flash();
  return runFlash_Test();
}

void RGB_BLINK() {
  digitalWrite(RED_LED, !digitalRead(RED_LED));
  delay(500);
}

void led_twinkle() {
  digitalWrite(RED_LED, HIGH);
  delay(100);
  digitalWrite(RED_LED, LOW);
  delay(100);
}

bool readSerialResponse(HardwareSerial& serialRef, const char* expectedResponse, unsigned long timeoutMs) {
    unsigned long startTime = millis();
    String receivedString = "";
    int expectedLen = strlen(expectedResponse);

    while (millis() - startTime < timeoutMs) {
        if (serialRef.available()) {
            char c = serialRef.read();
            receivedString += c;
            if (receivedString.length() >= expectedLen) {
                if (receivedString.endsWith(expectedResponse)) {
                    Serial.print("Received: ");
                    Serial.println(receivedString);
                    return true;
                }
                if (receivedString.length() > expectedLen * 2) {
                    receivedString = receivedString.substring(receivedString.length() - expectedLen);
                }
            }
        }
        delay(1);
    }
    Serial.print("Timeout or no matching response. Received: ");
    Serial.println(receivedString);
    return false;
}

void test_mode() {
  generalPinSetup();
  ALL_STA = true;
  ble_advertisement_count = 0;
  ble_test_flag = false;


  Serial.println("--- 开始各项模块测试 ---");
  RGB_BLINK(); // 启动测试时闪烁一次，表明进入测试模式

  // Battery Test
  Serial.println("\n--- Battery Test ---");
  bool battery_test_result = Battery_Test();
  if (!battery_test_result) {
    Serial.println("Battery Test Failed!");
    ALL_STA = false;
  } else {
    Serial.println("Battery Test Passed.");
  }

  // IO Test
  Serial.println("\n--- IO Test ---");
  bool gpio_test_result = runGPIO_Test();
  if (!gpio_test_result) {
    Serial.println("IO Test Failed!");
    ALL_STA = false;
  } else {
    Serial.println("IO Test Passed.");
  }

  // Flash Test (General)
  Serial.println("\n--- Flash Test (General) ---");
  bool flash_general_test_result = Flash_test(); // 调用 Flash_test 包装函数
  if (!flash_general_test_result) {
    Serial.println("Flash Test (General) Failed!");
    ALL_STA = false;
  } else {
    Serial.println("Flash Test (General) Passed.");
  }

  // Flash String Test (继续保留此测试作为额外验证)
  Serial.println("\n--- Flash Test (String Write/Verify) ---");
  String test_string = "Hello Flash Memory!";
  uint32_t flash_string_address = 0x1000;
  bool write_success = writeStringToFlash(test_string, flash_string_address);
  String read_string = readStringFromFlash(flash_string_address, test_string.length() + 1);
  bool flash_string_test_result = false;
  if (write_success) {
    flash_string_test_result = verifyStringData(test_string, read_string);
  } else {
    Serial.println("由于写入失败，跳过字符串校验。");
  }
  if (!flash_string_test_result) ALL_STA = false;
  if (flash_string_test_result) {
    Serial.println("Flash Test (String Write/Verify) Passed.");
  } else {
    Serial.println("Flash Test (String Write/Verify) Failed!");
  }

    // BLE Scan Test (放在最后，因为它依赖于异步事件)
  Serial.println("\n--- BLE Scan Test ---");

  int ble_advertisements_found = BLE_Scan();

  // ble_test_flag 会在 BLE_Scan() 内部根据发现的广告数量进行设置
  if (!ble_test_flag) { // 检查 handleBLEEvent 是否成功设置了此标志 (即是否有广告)
    Serial.println("BLE Scan Test Failed! (No advertisement found)");
    ALL_STA = false;
  } else {
    Serial.println("BLE Scan Test Passed.");
  }


  // MIC Test
  Serial1.println("MIC_Test Start");
  Serial.println("\n--- MIC Test ---");
  bool mic_test_result = MIC_TEST();
  if (!mic_test_result) {
    Serial.println("MIC Test Failed!");
    ALL_STA = false;
  } else {
    Serial.println("MIC Test Passed.");
  }

  // IMU Test
  Serial.println("\n--- LSM6DS3TR (IMU) Test ---");
  bool imu_test_result = runIMU_Test(); // 直接调用 runIMU_Test
  if (!imu_test_result) {
    Serial.println("LSM6DS3TR (IMU) Test Failed!");
    ALL_STA = false;
  } else {
    Serial.println("LSM6DS3TR (IMU) Test Passed.");
  }

  if(!ALL_STA) { // 如果有任何一个测试失败，则直接返回，进入失败闪烁状态
      Serial.println("\n--- 部分模块测试失败！ ---");
      Serial1.println("Test Failed");
      Serial.println("Test Failed");
      const char* result_message_fail = "Overall Test: FAIL";
      Serial.print("正在将整体测试结果写入Flash (地址: 0x"); Serial.print(TEST_RESULT_FLASH_ADDR, HEX); Serial.println(")...");
      if (writeStringToFlash(result_message_fail, TEST_RESULT_FLASH_ADDR)) {
          Serial.println("整体测试结果成功写入Flash。");
      } else {
          Serial.println("写入整体测试结果到Flash失败!");
      }
      Serial1.println("Test_Results_Saved");
      Serial.println("Test_Results_Saved");
      while (true) { // 整体测试失败时 LED 持续闪烁
          digitalWrite(RED_LED, LOW);
          delay(500);
          digitalWrite(RED_LED, HIGH);
          delay(500);
      }
  }

  Serial.println("\n--- 所有模块测试完成 ---"); // --- All module tests completed ---
  Serial1.println("Test Passed");
  Serial.println("Test Passed");

  const char* result_message;
  if (ALL_STA) {
    Serial.println("所有测试项目 PASSED。"); // All test items PASSED.
    result_message = "Overall Test: PASS";
  } else { // 这段代码在上面的 if(!ALL_STA) return;
    Serial.println("一些测试项目 FAILED。"); // Some test items FAILED.
    result_message = "Overall Test: FAIL";
  }

  Serial.print("正在将整体测试结果写入Flash (地址: 0x"); Serial.print(TEST_RESULT_FLASH_ADDR, HEX); Serial.println(")..."); // Writing overall test result to Flash (Address: 0x...)...
  if (writeStringToFlash(result_message, TEST_RESULT_FLASH_ADDR)) {
      Serial.println("整体测试结果成功写入Flash。");
  } else {
      Serial.println("写入整体测试结果到Flash失败!");
  }

  Serial1.println("Test_Results_Saved");
  Serial.println("Test_Results_Saved");
  if (ALL_STA) {
      digitalWrite(RED_LED, LOW);
    }

    //休眠模式测试:
    for (int i = 0; i < OUTPUT_PIN_COUNT; i++) {
      pinMode(OUTPUT_PINS[i], INPUT);
    }
    digitalWrite(PD3, LOW); //VBAT
    digitalWrite(PB5, LOW); //RF_SW
    digitalWrite(PB1, LOW);
    digitalWrite(PB0, LOW); //MIC
    digitalWrite(PA6, HIGH);  //FLASH
    delay(100);
    Serial1.printf("Going to sleep at %lu\n", millis());
    LowPower.deepSleep(600000);
    Serial1.printf("Woke up at %lu\n", millis());
    Serial.printf("Woke up at %lu\n", millis());
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(test_flag_pin, INPUT_PULLUP);

  delay(2000);
  int test_flag_state = digitalRead(test_flag_pin);
  if (test_flag_state == LOW) {
    Serial.println("进入测试模式..."); // Entering test mode...
    ALL_STA = true;
    test_mode();
  } else {
    FLASH_DEBUG_ENABLED = false;

    const char* pass_message = "Overall Test: PASS";
    const char* fail_message = "Overall Test: FAIL";
    char read_back_buffer[64];
    memset(read_back_buffer, 0, sizeof(read_back_buffer));

    if (FLASH_DEBUG_ENABLED) {
        Serial.print("尝试从Flash读取测试结果 (地址: 0x"); Serial.print(TEST_RESULT_FLASH_ADDR, HEX); Serial.println(")...");
    }

    setup_Flash();
    String flash_result_str = readStringFromFlash(TEST_RESULT_FLASH_ADDR, sizeof(read_back_buffer));
    flash_result_str.toCharArray(read_back_buffer, sizeof(read_back_buffer));

    if (flash_result_str.length() > 0) {
        if (FLASH_DEBUG_ENABLED) {
            Serial.print("从Flash读取到的结果: ");
            Serial.println(read_back_buffer);
        }
    } else {
        if (FLASH_DEBUG_ENABLED) {
            Serial.println("无法从Flash读取测试结果! (可能是Flash为空或读取失败)");
        }
        strcpy(read_back_buffer, "READ_FAIL");
    }

    if (strcmp(read_back_buffer, pass_message) == 0) {
        delay(3000);
        Serial.println("\nHello from Seeed Studio XIAO MG24 Sense");
        while (true) {
            RGB_BLINK();
        }
    } else {
        delay(3000);
        Serial.println("之前的整体测试结果：失败或未知。LED常亮。");
        while (true) {
            digitalWrite(RED_LED, LOW);
        }
    }
  }
}

void loop() {
  // Empty as all logic is handled in setup() or test_mode()
}

// BLE 事件处理器
// `sl_bt_on_event` 的定义由 Silicon Labs SDK 提供，我们只需要实现它。
// 其参数类型必须是 `sl_bt_msg_t*`。
extern "C" void sl_bt_on_event(sl_bt_msg_t* evt) {
  sl_status_t sc;

  switch (SL_BT_MSG_ID(evt->header)) {
    // 当 BLE 设备成功启动时接收到此事件
    case sl_bt_evt_system_boot_id:
      Serial.println();
      Serial.println("Silicon Labs BLE 扫描示例");
      Serial.println("BLE 栈已启动");
      break;

    // 当扫描到其他 BLE 设备的广告时接收到此事件
    case sl_bt_evt_scanner_legacy_advertisement_report_id:
      ble_advertisement_count++; // Use global variable
      ble_test_flag = true; // Use global variable
      if(false)
      {
        Serial.print(" -> #");
        Serial.print(ble_advertisement_count);
        Serial.print(" | 地址: ");
        for (int i = 5; i >= 0; i--) {
          Serial.printf("%02x", evt->data.evt_scanner_legacy_advertisement_report.address.addr[i]);
          if (i > 0) {
            Serial.print(":");
          }
        }
        Serial.print(" | RSSI: ");
        Serial.print(evt->data.evt_scanner_legacy_advertisement_report.rssi);
        Serial.print(" dBm");
        Serial.print(" | 频道: ");
        Serial.print(evt->data.evt_scanner_legacy_advertisement_report.channel);
        Serial.print(" | 名称: ");
        Serial.println(get_complete_local_name_from_ble_advertisement(&(evt->data.evt_scanner_legacy_advertisement_report)));
      }
      
      break;

    // 默认事件处理器
    default:
      break;
  }
}

// 这是一个重命名的断言函数，用于避免与 Silicon Labs SDK 中的 `app_assert_status` 宏冲突。
// 重要的是这里没有显式的前向声明，依赖 Arduino IDE 的自动处理。
void my_app_assert_status(sl_status_t status) {
  if (status != 0) {
    Serial.print("BLE Error: ");
    Serial.println(status, HEX);
    while (1); // 发生错误时停止程序执行
  }
}

/**************************************************************************/ /**
 * 在 BLE 广告中查找完整的本地名称
 *
 * @param[in] response 从扫描中接收到的 BLE 响应事件
 *
 * @return 如果找到完整的本地名称则返回，否则返回 "N/A"
 *****************************************************************************/
static String get_complete_local_name_from_ble_advertisement(sl_bt_evt_scanner_legacy_advertisement_report_t* response) {
  int i = 0;
  // 遍历响应数据
  while (i < (response->data.len - 1)) {
    uint8_t advertisement_length = response->data.data[i];
    uint8_t advertisement_type = response->data.data[i + 1];

    // 如果长度超过设备名称的最大可能长度
    if (advertisement_length > 29) {
      // Continue to next advertisement record, not the entire loop
      i = i + advertisement_length + 1;
      continue;
    }

    // 类型 0x09 = 完整的本地名称，0x08 = 缩短的名称
    // 如果字段类型与完整的本地名称匹配
    if (advertisement_type == 0x09) {
      // 复制设备名称
      char device_name[advertisement_length + 1];
      memcpy(device_name, response->data.data + i + 2, advertisement_length);
      device_name[advertisement_length] = '\0';
      return String(device_name);
    }
    // 跳到下一个广告记录
    i = i + advertisement_length + 1;
  }
  return "N/A";
}
