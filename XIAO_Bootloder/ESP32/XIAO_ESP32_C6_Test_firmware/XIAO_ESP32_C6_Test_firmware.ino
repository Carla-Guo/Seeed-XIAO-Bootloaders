#include <HardwareSerial.h>
#include <WiFi.h>
#include <BLEDevice.h>
#include <Arduino.h>
#include <nvs_flash.h>
#include <nvs.h>

#define UART_BAUD_RATE 115200

HardwareSerial MySerial0(0);
//注，D9连接BOOT，下拉会使C3进BOOT模式

// 增加测试结果状态枚举
enum TestStatus {
    TEST_NOT_RUN = 0,
    TEST_PASSED = 1,
    TEST_FAILED = 2
};


// NVS配置
const char* NVS_NAMESPACE = "board_status";
const char* NVS_KEY_TEST_PASSED = "test_passed";

// 函数原型声明
bool init_nvs();
TestStatus read_test_result_from_nvs();
void write_test_result_to_nvs(bool result);

// 定义输入引脚数组
const uint8_t INPUT_PINS[] = {D0, D10, D8, D4 };// D1最后单独处理
// 定义输出引脚数组（与输入引脚一一对应）
const uint8_t OUTPUT_PINS[] = {D2, D3, D5, D9};  
const uint8_t Button = 9;
const uint8_t test_flag = D1;
const uint8_t User_LED = 15;

// 扫描Wi-Fi列表的函数
bool scanWiFi() {
  int networksFound = WiFi.scanNetworks();  // 扫描Wi-Fi网络
  MySerial0.print("WiFi Networks Found: ");
  MySerial0.println(networksFound);
  return networksFound >= 1;
}

// 扫描蓝牙列表的函数
bool scanBluetooth() {
  BLEDevice::init("");  // 初始化BLE设备
  BLEScan* pBLEScan = BLEDevice::getScan();  // 获取扫描对象
  pBLEScan->setActiveScan(true);  // 启用主动扫描
  pBLEScan->setInterval(100);  // 设置扫描间隔
  pBLEScan->setWindow(99);  // 设置扫描窗口
  BLEScanResults* foundDevices = pBLEScan->start(10);  // 获取扫描结果

  int devicesFound = foundDevices->getCount();  // 获取扫描到的蓝牙设备数量
  MySerial0.print("Bluetooth Devices Found: ");
  MySerial0.println(devicesFound);
  return devicesFound >= 1;
}

// NVS初始化函数
bool init_nvs() {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    Serial.println("Erasing NVS partition...");
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  if (ret != ESP_OK) {
    Serial.printf("NVS init failed: %s\n", esp_err_to_name(ret));
    return false;
  }
  return true;
}


// 改进后的NVS读取函数
TestStatus read_test_result_from_nvs() {
    nvs_handle_t nvs_handle;
    uint8_t stored_value = TEST_NOT_RUN;

    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (ret != ESP_OK) {
        Serial.printf("NVS open failed: 0x%x\n", ret);
        return TEST_NOT_RUN;
    }

    ret = nvs_get_u8(nvs_handle, NVS_KEY_TEST_PASSED, &stored_value);
    nvs_close(nvs_handle);

    if (ret != ESP_OK) {
        if (ret == ESP_ERR_NVS_NOT_FOUND) {
            Serial.println("No previous test record");
        } else {
            Serial.printf("NVS read error: 0x%x\n", ret);
        }
        return TEST_NOT_RUN;
    }

    // 验证存储值的有效性
    if (stored_value > TEST_FAILED) {
        Serial.println("Invalid test result value");
        return TEST_NOT_RUN;
    }

    return (TestStatus)stored_value;
}

// 改进后的NVS写入函数
bool write_test_result_to_nvs(TestStatus result) {
    nvs_handle_t nvs_handle;
    
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) != ESP_OK) {
        Serial.println("Failed to open NVS for writing");
        return false;
    }

    esp_err_t set_ret = nvs_set_u8(nvs_handle, NVS_KEY_TEST_PASSED, result);
    esp_err_t commit_ret = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);

    if (set_ret != ESP_OK || commit_ret != ESP_OK) {
        Serial.printf("NVS write failed: set=0x%x, commit=0x%x\n", set_ret, commit_ret);
        return false;
    }

    Serial.println("Test result saved to NVS");
    return true;
}

void setup() {
  // 初始化串口
  Serial.begin(115200);  // 用于调试的串口（默认的USB串口）
  MySerial0.begin(115200);
  // MySerial0.begin(115200, SERIAL_8N1, -1, -1); // RX, TX
  MySerial0.println("Starting GPIO Test...");
  pinMode(User_LED, OUTPUT);
  digitalWrite(User_LED, HIGH);
  pinMode(test_flag, INPUT_PULLUP);          // 初始化为输出模式
  delay(100);
  if(digitalRead(test_flag) == 0)
  {
    TestStatus test_result = (test_mode() == true) ? TEST_PASSED : TEST_FAILED;
    Serial.println(test_result);

    if (test_result == TEST_PASSED) {
            digitalWrite(User_LED, LOW);
            write_test_result_to_nvs(test_result);
            Serial.println(test_result == TEST_PASSED ? "Test PASSED" : "Test FAILED");
            MySerial0.println(test_result == TEST_PASSED ? "Test PASSED" : "Test FAILED");
        } else {
            write_test_result_to_nvs(test_result);
            Serial.println("Failed to save test result");
        }
    delay(1000); // 延时1秒后进入休眠
    MySerial0.println("Entering Sleep mode in 1S...");
    Serial.println("Entering Sleep mode in 1S...");
    
    esp_deep_sleep_start();

  } 
  else 
  { 
    TestStatus saved_status = read_test_result_from_nvs();
        // Serial.println(saved_status);
        switch(saved_status) {
            case TEST_PASSED:
                // Serial.println("Validated test PASSED");
                Serial.println("Hello from Seeed Studio XIAO ESP32-C6");
                while(1)
                {
                  blink();
                }
                break;
            case TEST_FAILED:
                // Serial.println("Device test FAILED");
                // indicate_error_state();
                break;
            case TEST_NOT_RUN:
                // Serial.println("Device not tested");
                // indicate_untested_state();
                break;
        }
   
  }
}

void loop() {
  
}

void blink()
{
  digitalWrite(User_LED, LOW);
  delay(500);
  digitalWrite(User_LED, HIGH);
  delay(500);
}

void test_GPIO_Init()
{
  
    // 初始化GPIO模式
  for (uint8_t pin : INPUT_PINS) {
      pinMode(pin, INPUT);  // 其余引脚初始化为输入模式
  }
  pinMode(8, INPUT_PULLDOWN);
  for (uint8_t pin : OUTPUT_PINS) {
    pinMode(pin, OUTPUT);          // 初始化为输出模式
    digitalWrite(pin, LOW);        // 设置初始状态为低电平
  }
  pinMode(Button, INPUT);

  delay(200);
  MySerial0.println("Init Ready");
  delay(200);
  MySerial0.println("Init Ready");
}

bool test_mode()
{
  test_GPIO_Init();
  if(!testButton())
  {
    MySerial0.println("Button Failed");
    return false;
  }
  MySerial0.println("Button Passed");
  // 执行GPIO测试
  if (testGPIO() == 0) {
    MySerial0.println("GPIO PASS");
    // Serial.println("GPIO PASS");
  } else {
    MySerial0.println("GPIO Failed");
    return false;
  }

  // 扫描Wi-Fi和蓝牙
  bool wifiFound = scanWiFi();
  bool bluetoothFound = scanBluetooth();
  delay(500);
  if (wifiFound || bluetoothFound) {
    MySerial0.println("Wireless PASS");
    delay(200);
    MySerial0.println("Test Passed");
    // Serial.println("Wireless PASS");
    return true;
  } else {
    MySerial0.println("Wireless Failed.");
    return false;
  }
  
}

//测试函数
bool testButton()
{
  int count = 0;

  int gpio0_status=digitalRead(Button);
  // Serial.println(gpio0_status);
  int button_count=0;
  unsigned long startMillis = millis();
  while(millis() - startMillis < 10000)
  {
    delay(20);
    int gpio0_status=digitalRead(Button);
    // Serial.println(gpio0_status);
    if(gpio0_status == LOW)
    {
      button_count++;
      if(button_count >= 2)
      {
        return true;
      }
    }
  }
  return false;
}

// GPIO测试函数
int testGPIO() {
  int count = 0;

  // 遍历输出引脚，依次上拉
  for (int i = 0; i < sizeof(OUTPUT_PINS) / sizeof(OUTPUT_PINS[0]); i++) {
    digitalWrite(OUTPUT_PINS[i], HIGH);  // 上拉当前输出引脚
    delay(500);                          // 等待0.5秒

    // 检查对应输入引脚的状态
    for (int j = 0; j < sizeof(INPUT_PINS) / sizeof(INPUT_PINS[0]); j++) {
      int state = digitalRead(INPUT_PINS[j]);
      // Serial.print("Checking INPUT Pin ");
      // Serial.print(INPUT_PINS[j]);
      // Serial.print(" with OUTPUT Pin ");
      // Serial.print(OUTPUT_PINS[i]);
      // Serial.print(": ");
      if (i == j) {  // 如果是当前对应的输入引脚
        // Serial.println(state == HIGH ? "HIGH (PASS)" : "LOW (FAIL)");
        if (state != HIGH) {
          count++;
        }
      } else if (INPUT_PINS[j] == D1 || INPUT_PINS[j] == D4 ) {  // D1特殊情况，始终接地
        // Serial.println(state == LOW ? "LOW (Expected)" : "HIGH (Unexpected)");
        if (state != LOW) {
          count++;
        }
      } else {  // 其他非对应引脚
        // Serial.println(state == LOW ? "LOW (PASS)" : "HIGH (FAIL)");
        if (state != LOW) {
          count++;  // 如果非对应引脚检测到上拉信号，计数+1
        }
      }
    }

    // 恢复当前输出引脚为低电平
    digitalWrite(OUTPUT_PINS[i], LOW);
    delay(200);  // 等待0.5秒再测试下一个引脚
  }
  if(digitalRead(8)==LOW)
    count++;
  // Serial.print(count);
  return count;
}

