// 引入必要的头文件
#include "MainBox.h"      // 包含自定义的 CTROL_BOX 类定义
#include <RTTStream.h>    // 用于 RTT 调试输出
#include "qspi_flash.h"   // 确保包含 qspi_flash 的头文件，以便直接调用其函数

// RTTStream 实例，用于调试输出（如果您的开发环境支持 RTT）
RTTStream rtt;

// CTROL_BOX 类的全局实例
CTROL_BOX ctrol_box;
#define TEST_RESULT_FLASH_ADDR 0x0 

// 全局变量声明
// 这些引脚定义现在只在 .ino 文件中使用，因为 MainBox.cpp 中的 IO_Test 有自己的内部定义。
// 如果 MainBox.cpp 中的 IO_Test 需要使用这些全局定义，则需要调整其实现。
const uint8_t INPUT_PINS[] = {D0, D3, D5, D4};
const uint8_t OUTPUT_PINS[] = {D2, D10, D8, D9}; 

// 测试模式触发引脚
const uint8_t test_flag_pin = D1;

// 全局变量用于存储测试模式标志和测试 LED 状态
bool test_flag;      // 用于判断是否进入测试模式
bool TestLED_sta = HIGH; // 用于控制 Test_LED 的状态，默认高电平（LED OFF）

// 函数原型声明 (在 setup() 和 loop() 之前声明，确保编译器可见)
void test_mode();
void test_GPIO_init();
bool USER_BUTTOM(); // 声明 USER_BUTTOM 函数，因为 Battery_Test 依赖它


void setup()
{
  // 初始化串口通信，用于调试输出和与上位机交互
  Serial.begin(115200);
  Serial1.begin(9600); // 统一在 setup 中初始化 Serial1，避免重复初始化和波特率冲突
  
  // 初始化测试模式触发引脚
  pinMode(test_flag_pin, INPUT_PULLUP); // 设置为带上拉的输入，按钮按下时为 LOW

  // 等待一段时间，确保引脚状态稳定
  delay(1000); 

  // 读取测试模式触发引脚的状态
  int test_flag_state = digitalRead(test_flag_pin); // 修正变量名，避免与全局 test_flag 冲突
  // 如果 test_flag_pin 为 LOW (按钮按下)，则进入测试模式
  if(test_flag_state == LOW) 
  {
    Serial.println("进入测试模式...");
    test_mode();
  }
  else
  {
    // Serial.println("正常运行模式（未进入测试模式）。");
    ctrol_box.SeeedXiaoBleInit();

    // 定义预期的 Pass/Fail 消息
    const char* pass_message = "Overall Test: PASS";
    const char* fail_message = "Overall Test: FAIL";
    // 确定读取的最大长度，以确保能捕获整个消息（包括 null 终止符）
    size_t expected_read_len = std::max(strlen(pass_message), strlen(fail_message)) + 1;

    // 确保 QSPI 已初始化 - 直接调用 qspi_flash_init
    if (qspi_flash_init() != NRFX_SUCCESS) { // 直接调用 qspi_flash_init()
      // Serial.println("Error: QSPI initialization failed!");
      return ;
      // 这里不 return，让程序可以进入失败闪烁状态
    }

    char read_back_buffer[64]; // Buffer to read back the result, ensure it's large enough
    memset(read_back_buffer, 0, sizeof(read_back_buffer)); // Clear buffer
    
    // 使用 qspi_flash_read_string 函数读取 Flash 数据
    qspi_flash_read_string(read_back_buffer, TEST_RESULT_FLASH_ADDR, sizeof(read_back_buffer));
    // if (qspi_flash_read_string(read_back_buffer, TEST_RESULT_FLASH_ADDR, sizeof(read_back_buffer))) {
    //     Serial.print("Read back from Flash: ");
    //     Serial.println(read_back_buffer);
    // } else {
    //     Serial.println("Failed to read back test result from Flash!");
    // }
    // 比较读取到的结果是 Pass 还是 Fail
    if (strcmp(read_back_buffer, pass_message) == 0) {
        delay(3000);
        Serial.println("\nHello from Seeed Studio XIAO NRF52840 Sense");
        
        while (true) {
            // 进入死循环，保持 Test_LED 常亮，表示通过状态
          ctrol_box.RGB_BLINK(); 
          delay(200);
        }
    } 
    // else if (strcmp(read_back_buffer, fail_message) == 0) {
    //     Serial.println("之前的整体测试结果：失败。进入失败闪烁状态。");
    else {
        // 如果读取到的结果既不是 Pass 也不是 Fail，或者数据损坏
        Serial.println("Flash 中的测试结果未知或已损坏。默认为失败闪烁。");
        Serial.print("Read back from Flash: ");
        Serial.println(read_back_buffer);
        while (true) {
            // 进入死循环，保持 Test_LED 常亮，表示通过状态
          digitalWrite(RED_LED, LOW); // Turn off Red
          delay(500);
          digitalWrite(RED_LED, HIGH); // Turn off Red
          delay(500);
        }
    }
  }
}

void loop()
{ 
  // 主循环，如果进入测试模式，程序会在 test_mode() 中进入死循环
  // 如果在正常模式下，这里可以添加其他应用程序的循环逻辑
  // 例如：传感器读取、通信处理等
}

/**
 * @brief 测试模式主函数，按顺序执行各项硬件测试。
 */
void test_mode()
{
  bool ALL_STA = true; // 整体测试状态标志，默认为 true
  
  // 初始化通用 GPIO 引脚
  test_GPIO_init();
  
  // 初始化 CTROL_BOX 实例中的 Seeed Xiao BLE 相关引脚和 LED
  ctrol_box.SeeedXiaoBleInit();

  Serial.println("--- 开始各项模块测试 ---");
  ctrol_box.RGB_BLINK(); 

    // Battery (电池) 测试
  Serial.println("\n--- Battery Test ---");
  if (!ctrol_box.Battery_Test()) { // 如果 Battery_Test 返回 false (测试失败)
      ALL_STA = false;
      Serial.println("Battery Test Failed!");
  } else {
      Serial.println("Battery Test Passed.");
  }

  // IO (输入/输出) 测试
  Serial.println("\n--- IO Test ---");
  if (!ctrol_box.IO_Test()) { // 如果 IO_Test 返回 false (测试失败)
      ALL_STA = false;
      Serial.println("IO Test Failed!");
  } else {
      Serial.println("IO Test Passed.");
  }

  // BLE (蓝牙低功耗) 扫描测试
  Serial.println("\n--- BLE Scan Test ---");
  if (!ctrol_box.BLE_Scan()) { // 如果 BLE_Scan 返回 false (测试失败)
      ALL_STA = false;
      Serial.println("BLE Scan Test Failed!");
  } else {
      Serial.println("BLE Scan Test Passed.");
  }

  // FLASH (QSPI 闪存) 测试 - ctrol_box.Flash_test() 内部会调用 qspi_flash 的函数
  Serial.println("\n--- Flash Test ---");
  if (!ctrol_box.Flash_test()) { // 如果 Flash_test 返回 false (测试失败)
      ALL_STA = false;
      Serial.println("Flash Test Failed!");
  } else {
      Serial.println("Flash Test Passed.");
  }

  // MIC (麦克风) 测试
  Serial1.println("MIC_Test Start"); // 发送信号给上位机
  Serial.println("\n--- MIC Test ---");
  if (!ctrol_box.MIC_TEST()) { // 如果 MIC_TEST 返回 false (测试失败)
      ALL_STA = false;
      Serial.println("MIC Test Failed!");
  } else {
      Serial.println("MIC Test Passed.");
  }

  // DOF (自由度传感器，IMU) 测试
  Serial.println("\n--- LSM6DS3TR (IMU) Test ---");
  if (!ctrol_box.LSM6DS3TR_Test()) { // 如果 LSM6DS3TR_Test 返回 false (测试失败)
      ALL_STA = false;
      Serial.println("LSM6DS3TR (IMU) Test Failed!");
  } else {
      Serial.println("LSM6DS3TR (IMU) Test Passed.");
  }
  
  Serial.println("\n--- 所有模块测试完成 ---");
  Serial1.println("Test Passed");
  Serial.println("Test Passed");
    // Write overall test results to Flash
  const char* result_message;
  if (ALL_STA) {
    Serial.println("All test items PASSED.");
    result_message = "Overall Test: PASS";
  } else {
    Serial.println("Some test items FAILED.");
    result_message = "Overall Test: FAIL";
  } 

  // Erase the relevant Flash page before writing - 直接调用 qspi_flash_erase_block
  Serial.println("Erasing 64KB block at address: 0x0");
  nrfx_err_t erase_status = qspi_flash_erase_block(TEST_RESULT_FLASH_ADDR); // **修改为 qspi_flash_erase_block**
  if (erase_status != NRFX_SUCCESS) {
      Serial.print("QSPI Erase FAILED! Error Code: ");
      Serial.println(erase_status);
      // Handle error, perhaps return or indicate failure
  } else {
      Serial.println("Erase successful.");
  }

  // Write the result message to Flash - 直接调用 qspi_flash_write_string
  if (qspi_flash_write_string(result_message, TEST_RESULT_FLASH_ADDR, strlen(result_message) + 3)) { // 直接调用 qspi_flash_write_string
      Serial.println("Overall test result written to Flash successfully.");
  } else {
      Serial.println("Failed to write overall test result to Flash!");
  }
  // Read back the result to verify (optional) - 直接调用 qspi_flash_read_string
  char read_back_buffer[64]; // Buffer to read back the result, ensure it's large enough
  memset(read_back_buffer, 0, sizeof(read_back_buffer)); // Clear buffer
  if (qspi_flash_read_string(read_back_buffer, TEST_RESULT_FLASH_ADDR, sizeof(read_back_buffer))) { // 直接调用 qspi_flash_read_string
      Serial.print("Read back from Flash: ");
      Serial.println(read_back_buffer);
  } else {
      Serial.println("Failed to read back test result from Flash!");
  }
  Serial1.println("Test_Results_Saved");
  Serial.println("Test_Results_Saved");
  // The original loop for ALL_STA should remain, but the name is misleading as it's for keeping the LED on.
  while (ALL_STA) {
    ctrol_box.RGB_BLINK();
  }

}

/**
 * @brief 初始化测试模式所需的 GPIO 引脚。
 */
void test_GPIO_init()
{
  // 初始化 INPUT_PINS
  for (uint8_t pin : INPUT_PINS) {
      pinMode(pin, INPUT_PULLDOWN); // 将输入引脚初始化为带下拉电阻的输入模式
  }

  // 初始化 OUTPUT_PINS
  for (uint8_t pin : OUTPUT_PINS) {
      pinMode(pin, OUTPUT);     // 将输出引脚初始化为输出模式
      digitalWrite(pin, LOW);   // 设置初始状态为低电平
  }

  // Battery_Read_SW 引脚已在 MainBox.h 中移除，因此此处也应移除其相关代码。
  // 如果此引脚仍有用途，请在 MainBox.h 中重新定义并在此处使用。
  pinMode(Battery_Read_SW, OUTPUT);
  digitalWrite(Battery_Read_SW, LOW);
  pinMode(HiCHG, OUTPUT);
  // 以下是示例引脚，如果您的硬件有这些引脚并且需要初始化，请取消注释
  // pinMode(29, OUTPUT);
  // pinMode(30, OUTPUT);
  
  delay(200); // 延时等待引脚稳定
  Serial1.println("Init Ready"); // 发送初始化完成信号给上位机
  Serial.println("Init Ready"); // 可以发送两次，确保上位机接收到
}