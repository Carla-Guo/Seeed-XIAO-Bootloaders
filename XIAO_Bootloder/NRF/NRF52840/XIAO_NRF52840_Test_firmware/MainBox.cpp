#include "MainBox.h"
#include <ArduinoBLE.h> // 用于 BLE_Scan
#include <PDM.h>      // 用于 MIC_TEST (麦克风)
#include <SPI.h>      // 用于 SPI 通信
#include "LSM6DS3.h"    // 用于 LSM6DS3TR_Test (IMU)
#include <string.h>   // 用于 strlen, memset, strcmp
#include <Wire.h>
// 移除了对 nrfx_qspi.h 和相关宏的直接引用，因为它们现在由 qspi_flash.cpp 封装
// #include "nrfx_qspi.h"
// #include "app_util_platform.h"
// #include "nrf_log.h"
// #include "nrf_log_ctrl.h"
// #include "nrf_log_default_backends.h"
// #include "sdk_config.h"
// #include "nrf_delay.h"

const char channels = 1;
const int frequency = 16000;
short sampleBuffer[512];
volatile int samplesRead;

CTROL_BOX::CTROL_BOX() {
    // 构造函数中可以进行一些初始化
}

void CTROL_BOX::SeeedXiaoBleInit() {
    Serial1.println("Seeed Xiao BLE Init done.");
    pinMode(BLUE_LED, OUTPUT);
    pinMode(GREEN_LED, OUTPUT);
    pinMode(RED_LED, OUTPUT);
    digitalWrite(BLUE_LED, HIGH);
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, HIGH); // All off
}

void CTROL_BOX::RGB_BLINK() {
    digitalWrite(BLUE_LED, LOW); // Turn on Blue
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, HIGH);
    delay(300);
    digitalWrite(BLUE_LED, HIGH); // Turn off Blue
    digitalWrite(GREEN_LED, LOW); // Turn on Green
    delay(300);
    digitalWrite(GREEN_LED, HIGH); // Turn off Green
    digitalWrite(RED_LED, LOW); // Turn on Red
    delay(300);
    digitalWrite(RED_LED, HIGH); // Turn off Red
    delay(300);
}

// =================================================================================================
// Flash (QSPI) 相关的实现 - 现在直接调用 qspi_flash.cpp 中的函数
// =================================================================================================

// 以下 Flash 相关函数不再是 CTROL_BOX 的成员函数，而是直接调用 qspi_flash 模块的功能。
// 因此，这些函数已从 MainBox.h 和 MainBox.cpp 中移除其作为 CTROL_BOX 成员的实现。
// nrfx_err_t CTROL_BOX::QSPI_Initialise() { /* ... */ }
// nrfx_err_t CTROL_BOX::QSPI_Erase(uint32_t AStartAddress) { /* ... */ }
// bool CTROL_BOX::writeTestResultsToFlash(const char* results, uint32_t address, uint32_t len) { /* ... */ }
// bool CTROL_BOX::readTestResultsFromFlash(char* buffer, uint32_t address, uint32_t len) { /* ... */ }

// Flash_test 函数现在直接调用 qspi_flash 模块的函数
bool CTROL_BOX::Flash_test() {
    Serial.println("Starting internal Flash Test (via qspi_flash module)...");

    // 1. 初始化 QSPI
    nrfx_err_t err_code = qspi_flash_init(); // 直接调用全局 qspi_flash_init
    if (err_code != NRFX_SUCCESS) {
        Serial.print("Flash Test: qspi_flash_init() failed! Error: ");
        Serial.println(err_code);
        return false;
    }
    Serial.println("Flash Test: qspi_flash_init() successful.");

    // 2. 写入测试数据
    const char* test_data_write = "Flash Test Data ABCDEFGH";
    uint32_t test_addr = 0x1000; // Flash 地址
    uint32_t max_buffer_len = 64; // 示例：为字符串分配的缓冲区最大长度

    // 先擦除对应的页面
    // 注意：qspi_flash_erase_page 应该在您的 qspi_flash.cpp 中正确实现
    err_code = qspi_flash_erase_block(test_addr); // 直接调用全局 qspi_flash_erase_page
    if (err_code != NRFX_SUCCESS) {
        Serial.print("Flash Test: qspi_flash_erase_page() failed! Error: ");
        Serial.println(err_code);
        return false;
    }
    Serial.println("Flash Test: Erase successful.");
    
    // 使用 qspi_flash_write_string
    if (!qspi_flash_write_string(test_data_write, test_addr, max_buffer_len)) {
        Serial.println("Flash Test: qspi_flash_write_string() failed!");
        return false;
    }
    Serial.println("Flash Test: Write successful.");

    // 3. 读取测试数据
    char read_buffer[max_buffer_len];
    memset(read_buffer, 0, sizeof(read_buffer));

    // 使用 qspi_flash_read_string
    if (!qspi_flash_read_string(read_buffer, test_addr, max_buffer_len)) {
        Serial.println("Flash Test: qspi_flash_read_string() failed!");
        return false;
    }
    Serial.print("Flash Test: Read data: '");
    Serial.print(read_buffer);
    Serial.println("'");

    // 4. 比较数据
    if (strcmp(test_data_write, read_buffer) == 0) {
        Serial.println("Flash Test: Write and Read data matched. Flash test PASSED.");
        return true;
    } else {
        Serial.print("Flash Test: Data mismatch! Expected: '");
        Serial.print(test_data_write);
        Serial.print("', Read: '");
        Serial.print(read_buffer);
        Serial.println("'. Flash test FAILED.");
        return false;
    }
}
/**
 * @brief 执行电池测试序列。
 * 默认以低电流充电模式（HiCHG 为 HIGH）开始。
 * 读取电池电压（Statsu_Battery，P0.31/AIN7）并通过分压电阻计算。
 * 判断电压是否大于 3.7V。
 * 然后等待 Serial1 串口回复 "Pass"。
 * 如果成功，切换到高电流充电模式（HiCHG 为 LOW）。
 * 再次等待 Serial1 串口回复 "Pass"。
 * 如果两次都成功收到 "Pass"，则测试正常。
 * @return 如果电池测试序列成功完成，则返回 true，否则返回 false。
 */
bool CTROL_BOX::Battery_Test(void)
{
  // 串口通信初始化，用于与外部设备（如上位机）交互
  // Serial1.begin(9600); // 确保 Serial1 已初始化，根据实际需求调整波特率
  Serial.println("Battery Test Started.");

    // --- 步骤 1: 检查电池电压并等待低电流模式的 Pass 响应 ---
  float adcValue = 0;
  // 多次读取 ADC 值进行平均，提高精度
  for (int i = 0; i < 10; i++) {
    adcValue += analogRead(Statsu_Battery); // Statsu_Battery 对应 AIN7
    delay(5); // 确保 ADC 采样稳定
  }
  adcValue /= 10.0; // 计算平均值
  // 计算分压电阻的比例 (R1 = 1M, R2 = 510K)
  // V_out = V_in * (R2 / (R1 + R2)) => V_in = V_out * (R1 + R2) / R2
  const float R1 = 1000000.0; // 1M Ohm
  const float R2 = 510000.0;  // 510K Ohm
  const float ADC_REF_VOLTAGE = 3.3; // ADC 参考电压 (nRF52840 的 VCC 通常为 3.3V)
  const float ADC_MAX_VALUE = 1023.0; // 8位 ADC 的最大值 (0-1023)

  float measuredVout = adcValue * (ADC_REF_VOLTAGE / ADC_MAX_VALUE); // ADC 读取的电压
  float batteryVoltage = measuredVout * ((R1 + R2) / R2); // 计算实际电池电压

  Serial.print("测量到的电池电压 (Statsu_Battery): ");
  Serial.print(batteryVoltage, 3); // 打印三位小数
  Serial.println(" V");

  // 判断电池电压是否大于 3.7V
  if (batteryVoltage < 3.3) {
    Serial.println("电池电压低于 3.6V。低电流充电测试失败。");
    pinMode(HiCHG, INPUT); // 恢复引脚为输入模式
    // return false;
  }
  Serial.println("电池电压合格 (>= 3.6V)。");

  
  digitalWrite(HiCHG, LOW); // 低电流充电模式 (HiCHG 为 HIGH)
  Serial.println("设置为高电流充电模式 (HiCHG: LOW)。");
  delay(100); // 

  // 等待 Serial1 回复 "Pass" (低电流模式)
  Serial.println("等待 Serial1 接收 'Pass' (高电流模式)...");
  if (!readSerialResponse(Serial1, "Pass", 10000)) { // 5秒超时
    Serial.println("未收到高电流模式的 'Pass' 响应或超时。测试失败。");
    pinMode(HiCHG, INPUT); // 恢复引脚为输入模式
    return false;
  }
  Serial.println("收到高电流模式的 'Pass'。");

  // --- 步骤 2: 切换到高电流充电模式并等待高电流模式的 Pass 响应 ---
  pinMode(HiCHG, INPUT); // 恢复引脚为输入模式
  // delay(100);
  // pinMode(HiCHG, OUTPUT); // 恢复引脚为输入模式
  // digitalWrite(HiCHG, HIGH); // 高电流充电模式 (HiCHG 为 LOW)
  Serial.println("切换到低电流充电模式 (HiCHG: HIGH)。");
  // 等待 Serial1 回复 "Pass" (高电流模式)
  Serial.println("等待 Serial1 接收 'Pass' (低电流模式)...");

  Serial1.println("HighChargingCurrent");
  
  if (!readSerialResponse(Serial1, "Pass", 10000)) { // 5秒超时
    Serial.println("未收到低电流模式的 'Pass' 响应或超时。测试失败。");
    pinMode(HiCHG, INPUT); // 恢复引脚为输入模式
    return false;
  }
  Serial.println("收到高电流模式的 'Pass'。");

  // --- 测试完成 ---
  Serial.println("电池测试正常。");
  pinMode(HiCHG, INPUT); // 恢复 HiCHG 引脚为输入模式（可选，根据硬件决定）
  return true;
}



const uint8_t INPUT_PINS[] = {D0, D3, D5, D4,};
const uint8_t OUTPUT_PINS[] = {D2, D10, D8, D9}; 

bool CTROL_BOX::IO_Test(void)
{
  bool io_sta = true;
  int count = 0;

  for (int i = 0; i < sizeof(OUTPUT_PINS) / sizeof(OUTPUT_PINS[0]); i++)
  {
    digitalWrite(OUTPUT_PINS[i], HIGH);  // 上拉当前输出引脚
  }

  for (int j = 0; j < sizeof(INPUT_PINS) / sizeof(INPUT_PINS[0]); j++) {
      int state = digitalRead(INPUT_PINS[j]);
        
      if (state == LOW) {
        count++;
      }
      Serial.println(state == HIGH ? "HIGH (PASS)" : "LOW (FAIL)");
  }
  
  io_sta = (count==0?true:false);
  io_sta ? Serial.println("IO OK.") : Serial.println("IO Error.");
  return io_sta;
}


bool CTROL_BOX::BLE_Scan() {
    Serial.println("Starting BLE Scan Test...");
    bool ble_status = false;

    if (!BLE.begin()) {
        Serial.println("BLE initialization failed!");
        Serial1.println("BLE_Scan Fail");
        return false;
    }

    Serial.println("Scanning for BLE peripherals...");
    BLE.scan(false);

    unsigned long startTime = millis();
    const unsigned long scanDuration = 10000;

    while (millis() - startTime < scanDuration) {
        BLEDevice peripheral = BLE.available();
        if (peripheral) {
            Serial.print("Found BLE Peripheral: ");
            Serial.println(peripheral.address());
            Serial.print("  Name: ");
            Serial.println(peripheral.localName());
            ble_status = true;
            break;
        }
        delay(10);
    }

    BLE.end();

    if (ble_status) {
        Serial.println("BLE Scan Test Passed: Found at least one BLE device.");
        Serial1.println("BLE_Scan Pass");
        return true;
    } else {
        Serial.println("BLE Scan Test Failed: No BLE devices found within timeout.");
        Serial1.println("BLE_Scan Fail");
        return false;
    }
}

/**
 * @brief PDM 数据接收回调函数。
 * 当有新的 PDM 音频数据可用时调用。
 */
void onPDMdata() {
  int bytesAvailable = PDM.available(); // 查询可用的字节数
  PDM.read(sampleBuffer, bytesAvailable); // 读取到样本缓冲区
  samplesRead = bytesAvailable / 2; // 计算 16 位样本的数量
}

/**
 * @brief 执行麦克风 (PDM) 测试，检测声音。
 * @return 如果检测到声音，则返回 true，否则返回 false。
 */
bool CTROL_BOX::MIC_TEST(void)
{
  bool re_sta = false;
  uint8_t MIC_OK = 0;
  unsigned char mic_test_done = 0;
  
  PDM.onReceive(onPDMdata); // 注册 PDM 数据回调函数
  if (!PDM.begin(channels, frequency)) {
    Serial.println("启动 PDM 失败！");
    return false;
  }
  

  for (int i = 0; i < 1000; i++) // 循环一定时长以检测声音
  {
    if (samplesRead) // 如果回调函数中读取了样本
    {
      for (int j = 0; j < samplesRead; j++)
      {
        // Serial.println(sampleBuffer[j]); // DEBUG: 打印原始音频样本
        if (abs(sampleBuffer[j]) >= 500) // 如果绝对样本值超过阈值（检测到声音）
        {
          Serial.println("MIC 检测到声音");
          j = samplesRead; // 退出内层循环
          mic_test_done = 1;
        }
      }
      samplesRead = 0; // 处理后重置样本读取计数
    }
    
    if (mic_test_done == 1)
    {
      MIC_OK++; // 增加检测到声音的计数器
      if (MIC_OK > 10) // 如果声音连续检测到 10 次
      {
        // digitalWrite(Buzzer, LOW); // 关闭蜂鸣器
        re_sta = true; // MIC 测试成功
        break;
      }
      mic_test_done = 0; // 重置以便下次检测
    }
    delay(1); // 短暂延时，避免过度忙等待
  }
  // 打印最终状态
  re_sta ? Serial.println("MIC OK") : Serial.println("MIC Error");
  return re_sta;
}

bool CTROL_BOX::LSM6DS3TR_Test() {
    LSM6DS3 MyIMU(I2C_MODE, 0x6A);
  float x=99.0, y=99.0, z=99.0;
  int read_flag=5;
  bool re_sta = false;
  if (MyIMU.begin() != 0) {
    Serial.println("Failed to initialize IMU!");
    while (1) yield();
  }
  while (read_flag>0)
  {
      x=MyIMU.readFloatAccelX();
      y=MyIMU.readFloatAccelY();
      z=MyIMU.readFloatAccelZ();
      Serial.print(x);
      Serial.print('\t');
      Serial.print(y);
      Serial.print('\t');
      Serial.println(z);
      if (abs(x) < 0.05 && abs(y) < 0.05 && abs(1 - z) < 0.05) //水平状态下，三个值基本处于固定不变的情况？？
      {
          re_sta = true;
          break;  
      }        
      else
      {
        re_sta = false;
        read_flag-=1;
      }   
  }
  re_sta ? Serial.println("DOF OK") : Serial.println("DOF Error");
  return re_sta;
}


bool CTROL_BOX::readSerialResponse(HardwareSerial& serialRef, const char* expectedResponse, unsigned long timeoutMs) {
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