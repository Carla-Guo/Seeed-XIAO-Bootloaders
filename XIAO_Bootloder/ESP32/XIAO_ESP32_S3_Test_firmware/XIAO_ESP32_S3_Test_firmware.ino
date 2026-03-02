#include <HardwareSerial.h>
#include <WiFi.h>
#include <BLEDevice.h>
#include <Arduino.h>
#include "esp_camera.h"
#include <nvs_flash.h>
#include <nvs.h>

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

// 硬件配置
#define UART_BAUD_RATE 115200
#define CAMERA_MODEL_AI_THINKER // Has PSRAM
#include "camera_pins.h"

// 硬件引脚定义
HardwareSerial MySerial0(0);
const uint8_t INPUT_PINS[] = {D0, D3, D5, D4,};
const uint8_t OUTPUT_PINS[] = {D2, D10, D8, D9}; 
const uint8_t EXT_INPUT_PINS[] = {13, 12, 11, 10, 14, 16, 42, 40, 38};
const uint8_t EXT_OUTPUT_PINS[] = {D2, D8, D9, D10, 15, 17, 41, 39, 47};
const uint8_t test_flag = D1;

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
// 测试模式相关函数
void test_GPIO_init();
int test_mode(int Ext_flag);
int testButton();
int testEXTGPIO();
int testGPIO();
bool scanWiFi();
bool scanBluetooth();

void setup() {
  Serial.begin(115200);
  // while(!Serial);
  MySerial0.begin(115200, SERIAL_8N1, -1, -1);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // 初始化NVS
  if (!init_nvs()) {
    Serial.println("Failed to initialize NVS");
    while(1) delay(1000);
  }

  // 状态检测引脚
  pinMode(18, INPUT_PULLUP);
  pinMode(48, INPUT_PULLDOWN);
  pinMode(test_flag, INPUT_PULLUP);

  if(!digitalRead(test_flag)) {
        int EXT_test_flag = (digitalRead(18) == LOW && digitalRead(48) == HIGH);//有无接入测试拓展板
        TestStatus test_result = (test_mode(EXT_test_flag) == 1) ? TEST_PASSED : TEST_FAILED;
        Serial.println(test_result);
        // 无论成功失败都写入结果
        if (test_result == TEST_PASSED) {
            write_test_result_to_nvs(test_result);
            Serial.println(test_result == TEST_PASSED ? "Test PASSED" : "Test FAILED");
            while(1)
            {
              blink();
            }
        } else {
            write_test_result_to_nvs(test_result);
            Serial.println("Failed to save test result");
            blink();
            delay(1000);
            blink();
        }
    } else {
        // 读取并验证测试结果
        TestStatus saved_status = read_test_result_from_nvs();
        // Serial.println(saved_status);
        switch(saved_status) {
            case TEST_PASSED:
                // Serial.println("Validated test PASSED");
                Serial.println("Hello from Seeed Studio XIAO ESP32-S3");
                while(1) blink();
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

// 扫描Wi-Fi列表的函数
bool scanWiFi() {
  int networksFound = WiFi.scanNetworks();  // 扫描Wi-Fi网络
  MySerial0.print("WiFi Networks Found: ");
  MySerial0.println(networksFound);
  Serial.print("WiFi Networks Found: ");
  Serial.println(networksFound);
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
  Serial.print("Bluetooth Devices Found: ");
  Serial.println(devicesFound);
  return devicesFound >= 1;
}

void blink()
{
  digitalWrite(LED_BUILTIN,LOW);
  delay(500);
  digitalWrite(LED_BUILTIN,HIGH);
  delay(500);
}

//测试函数
int testButton()
{
  int count = 0;

  pinMode(D2,OUTPUT);
  digitalWrite(D2,HIGH);
  pinMode(0,INPUT);
  // Serial.print("GPIO0:");
  int gpio0_status=digitalRead(0);
  // Serial.println(gpio0_status);
  int button_count=0;
  unsigned long startMillis = millis();
  while(millis() - startMillis < 10000)
  {
    delay(200);
    int gpio0_status=digitalRead(0);
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

int testEXTGPIO()
{
  int count=0;
  for (int i = 0; i < sizeof(EXT_OUTPUT_PINS) / sizeof(EXT_OUTPUT_PINS[0]); i++)
  {
    digitalWrite(EXT_OUTPUT_PINS[i], HIGH);  // 上拉当前输出引脚
  }

  for (int j = 0; j < sizeof(EXT_INPUT_PINS) / sizeof(EXT_INPUT_PINS[0]); j++) {
      int state = digitalRead(EXT_INPUT_PINS[j]);

        Serial.println(state == HIGH ? "HIGH (PASS)" : "LOW (FAIL)");
        if (state == LOW) {
          count++;
        }
    }
  return count;
}

// GPIO测试函数
int testGPIO() {
  int count = 0;

  for (int i = 0; i < sizeof(OUTPUT_PINS) / sizeof(OUTPUT_PINS[0]); i++)
  {
    digitalWrite(OUTPUT_PINS[i], HIGH);  // 上拉当前输出引脚
  }

  for (int j = 0; j < sizeof(INPUT_PINS) / sizeof(INPUT_PINS[0]); j++) {
      int state = digitalRead(INPUT_PINS[j]);

        
        if (state == LOW && INPUT_PINS[j] != D1) {
          count++;
        }
        else if(state == HIGH && INPUT_PINS[j] == D1)
        {
          count++;
          Serial.println( "LOW (FAIL)");
          continue;
        }
        Serial.println(state == HIGH ? "HIGH (PASS)" : "LOW (FAIL)");
    }
  return count;
}

//#########################################################################################


/*Test模式GPIO初始化*/
void test_GPIO_init()
{

  // 初始化GPIO模式
  for (uint8_t pin : INPUT_PINS) {
    if (pin == D1) {
      pinMode(pin, INPUT_PULLUP);  // D1接地，初始化为下拉输入
    } else {
      pinMode(pin, INPUT_PULLDOWN);  // 其余引脚初始化为输入模式
    }
  }
  for (uint8_t pin : EXT_INPUT_PINS) {
      pinMode(pin, INPUT_PULLDOWN);  // 其余引脚初始化为输入模式
  }

  for (uint8_t pin : OUTPUT_PINS) {
    pinMode(pin, OUTPUT);          // 初始化为输出模式
    digitalWrite(pin, LOW);        // 设置初始状态为低电平
  }
  for (uint8_t pin : EXT_OUTPUT_PINS) {
    pinMode(pin, OUTPUT);          // 初始化为输出模式
    digitalWrite(pin, LOW);        // 设置初始状态为低电平
  }

  delay(200);
  MySerial0.println("Init Ready");
  MySerial0.println("Init Ready");

}

bool waitForTestPassWithTimeout(unsigned long timeoutMillis) {
  unsigned long startTime = millis(); // 获取当前时间作为开始时间

  while (1) {
    // 1. 检查是否超时
    if (millis() - startTime >= timeoutMillis) {
      MySerial0.println("Timeout waiting for TEST_PASS");
      digitalWrite(LED_BUILTIN, LOW); // 例如，超时后关闭LED
      return false; // 返回0表示超时或其他错误
    }

    // 2. 检查串口是否有数据
    if (MySerial0.available()) {
      String command = MySerial0.readStringUntil('\n');
      command.trim(); // 去除多余的空格
      MySerial0.println(command); // 调试时可以取消注释
      Serial.println(command);
      if (command == "TEST_PASS") {
        digitalWrite(LED_BUILTIN,HIGH); // 点亮LED表示成功
        MySerial0.println("Over");
        return true; // 返回1表示成功接收到命令
      }
    }
  }
}

int test_mode(int Ext_flag)
{
  test_GPIO_init();
  if(testButton() == false)
  {
    MySerial0.println("Button Failed");
    return 0;
  }
  MySerial0.println("Button PASS");
  if (Ext_flag)
  {
    if(testEXTGPIO() > 0 )
    {
      MySerial0.println("EXT_GPIO Failed");
      return 0;
    }
    MySerial0.println("EXT_GPIO PASS");
    Serial.println("EXT_GPIO PASS");
  }
  delay(100);
  
   // 执行GPIO测试
  if (testGPIO() > 0) {
    MySerial0.println("GPIO Failed");
    Serial.println("GPIO Failed");
    return 0;
  }
  MySerial0.println("GPIO PASS");
  Serial.println("GPIO PASS");
  // 扫描Wi-Fi和蓝牙
  bool wifiFound = scanWiFi();
  bool bluetoothFound = scanBluetooth();
  delay(500);
  if (wifiFound && bluetoothFound) {
    MySerial0.println("Wireless PASS");
    MySerial0.println("Test Passed");
    Serial.println("Test Passed");
  } else {
    MySerial0.println("Wireless Failed");
    return 0;
  }
  blink();
  digitalWrite(LED_BUILTIN, HIGH);
  return waitForTestPassWithTimeout(30000);

}
