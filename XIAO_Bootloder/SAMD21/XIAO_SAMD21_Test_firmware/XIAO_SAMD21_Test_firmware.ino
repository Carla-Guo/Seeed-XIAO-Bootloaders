#include <Arduino.h>
#include <sam.h> // 引入SAM D21相关的头文件



// 定义输入和输出引脚
const uint8_t INPUT_PINS[] = {D0, D10, D8, D4, D1}; // 包含 D4
const uint8_t OUTPUT_PINS[] = {D2, D3, D5, D9}; // 包含 D9

const int ledPin =  13;      // the number of the LED pin

void software_reset() {
    // 设置AIRCR寄存器进行复位
    SCB->AIRCR = (0x5FA << SCB_AIRCR_VECTKEY_Pos) | SCB_AIRCR_SYSRESETREQ_Msk;
    __DSB(); // 数据同步屏障
    __ISB(); // 指令同步屏障
    while(1); // 确保复位请求后进入死循环
}

void setup() {
  // 初始化串口
  Serial.begin(115200);  // 默认 USB 串口用于调试
  Serial1.begin(115200); // 初始化 Serial1，用于与 D6（RX）和 D7（TX）通信
  Serial1.println("Starting GPIO Test...");
  pinMode(ledPin, OUTPUT); 
  // 初始化 GPIO 模式
  for (uint8_t pin : INPUT_PINS) {
    if (pin == D1) {
      pinMode(pin, INPUT_PULLUP);  // D1 设置为下拉输入
    } else {
      pinMode(pin, INPUT);  // 其他引脚设置为输入
    }
  }

  for (uint8_t pin : OUTPUT_PINS) {
    pinMode(pin, OUTPUT);          // 设置为输出模式
    digitalWrite(pin, LOW);        // 设置初始状态为低
  }  
  
  
  // 执行 GPIO 测试

}

void loop() {

  if (Serial1.available()) {
    String command = Serial1.readStringUntil('\n'); // 读取命令直到换行
    command.trim();  // 去除多余空格
    if (command.equalsIgnoreCase("Test")) {  // 检查是否是 "Test" 命令
      Serial1.println("Test Executed");
      // Serial1.println("Init Ready");
        if (testGPIO() == 0) {
          Serial1.println("GPIO PASS");
          Serial1.println("Test Passed");
          // Serial.println("GPIO PASS");
        } else {
          Serial1.println("GPIO Failed");
        }
    }
  }

  digitalWrite(ledPin,!digitalRead(ledPin));
  delay(500);
}

// GPIO 测试函数
int testGPIO() {
  int count = 0;

  // 遍历输出引脚进行测试
  for (int i = 0; i < sizeof(OUTPUT_PINS) / sizeof(OUTPUT_PINS[0]); i++) {
    digitalWrite(OUTPUT_PINS[i], HIGH);  // 设置当前输出引脚为高
    delay(100);                           // 等待 0.5 秒

    // 检查对应输入引脚的状态
    for (int j = 0; j < sizeof(INPUT_PINS) / sizeof(INPUT_PINS[0]); j++) {
      int state = digitalRead(INPUT_PINS[j]);
      if (i == j) {  // 如果是当前对应的输入引脚
        if (state != HIGH) {
          count++; // 如果状态不为高，计数
        }
      } else if (INPUT_PINS[j] == D1) {  // 特殊情况处理 D1
        if (state != LOW) {
          count++; // 如果 D1 状态不为低，计数
        }
      } else {  // 其他非对应引脚
        if (state != LOW) {
          count++;  // 如果非对应引脚检测到高信号，计数
        }
      }
    }

    // 将当前输出引脚设置回低电平
    digitalWrite(OUTPUT_PINS[i], LOW);
    delay(100);  // 等待 0.2 秒再测试下一个引脚
  }
  return count;  // 返回失败计数
}
