#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "r_lpm.h"

lpm_ctrl_t* const p_api_ctrl = NULL;

// 定义输入和输出引脚
const uint8_t INPUT_PINS[] = {D0, D10, D8, D4}; // 包含 D4
const uint8_t OUTPUT_PINS[] = {D2, D3, D5, D9}; // 包含 D9

const float referenceVoltage = 3.3;  // Reference voltage for the ADC
const float voltageDivider = 2.0;    // Voltage divider factor

const int Power = 21;
const int LED_PIN  = 20;
#define NUMPIXELS 1

#define Test_Flag D1
const int RGB_Y = 19;  
const int ADC_PIN = 22; // Corresponds to A3 on XIAO RP2040
const int ENABLE_PIN = 23;
const int Button = 26;

Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

void turn_off_light() {
  // pixels.end();
  Serial1.end();
  Serial.end();
  digitalWrite(Power, LOW);
  digitalWrite(ENABLE_PIN, LOW);  
  digitalWrite(RGB_Y, HIGH); 
  pinMode(Button, OUTPUT);
  digitalWrite(Button, HIGH);
  pinMode(Test_Flag, OUTPUT);
  // digitalWrite(Test_Flag, HIGH);
  for (uint8_t pin : OUTPUT_PINS) {
    digitalWrite(pin, LOW);        // 设置初始状态为低
  }  
}

void set_system_clock() {
  cgc_divider_cfg_t dividers;
  dividers.sckdivcr_b.pclkb_div = CGC_SYS_CLOCK_DIV_64;
  dividers.sckdivcr_b.pclkd_div = CGC_SYS_CLOCK_DIV_64;
  dividers.sckdivcr_b.iclk_div = CGC_SYS_CLOCK_DIV_1;
  dividers.sckdivcr_b.pclka_div = CGC_SYS_CLOCK_DIV_1;
  dividers.sckdivcr_b.pclkc_div = CGC_SYS_CLOCK_DIV_1;
  dividers.sckdivcr_b.fclk_div = CGC_SYS_CLOCK_DIV_1;
  dividers.sckdivcr_b.bclk_div = CGC_SYS_CLOCK_DIV_1;

  fsp_err_t err = R_CGC_SystemClockSet(&g_cgc0_ctrl, CGC_CLOCK_LOCO, &dividers);

  assert(FSP_SUCCESS == err);
}

void config_beforeLPM() {
  lpm_cfg_t *p_cfg;
  p_cfg->low_power_mode = LPM_MODE_SLEEP;
  p_cfg->standby_wake_sources = 0;
  p_cfg->snooze_request_source = LPM_SNOOZE_REQUEST_RXD0_FALLING;
  p_cfg->snooze_end_sources = LPM_SNOOZE_END_STANDBY_WAKE_SOURCES;
  p_cfg->snooze_cancel_sources = LPM_SNOOZE_CANCEL_SOURCE_NONE;
  p_cfg->dtc_state_in_snooze = LPM_SNOOZE_DTC_DISABLE;
  p_cfg->p_extend = NULL;

  R_LPM_Open(p_api_ctrl, p_cfg);
  // R_LPM_LowPowerReconfigure(p_api_ctrl, p_cfg);
}

void Sleep_mode() {
  turn_off_light();

  delay(1000);

  set_system_clock();

  config_beforeLPM();

  // Serial.println("1 second to enter sleep mode");

  R_LPM_LowPowerModeEnter(p_api_ctrl);
}

void setup() {
  // 初始化串口
  Serial.begin(115200);  // 默认 USB 串口用于调试
  // while (!Serial);

  Serial1.begin(115200); // 初始化 Serial2，用于与 D6（RX）和 D7（TX）通信
  while (!Serial1);

  Serial1.println("Starting GPIO Test...");
  // Serial.println("Starting GPIO Test...");
  pinMode(Test_Flag, INPUT_PULLUP);  // D1 设置为下拉输入
  GPIO_Init();

  pixels.begin();
  pinMode(Power,OUTPUT);
  pinMode(RGB_Y,OUTPUT);
  pinMode(ENABLE_PIN,OUTPUT);
  digitalWrite(Power, HIGH);
  digitalWrite(ENABLE_PIN, HIGH);  
  digitalWrite(RGB_Y, HIGH);  

  delay(1000);
  Serial1.println("Init Ready");
  delay(10);
  Serial1.println("Init Ready");

  int Test_mode_Detect = digitalRead(Test_Flag);
  
  // Serial.println(Test_mode_Detect);
  if(Test_mode_Detect == HIGH)
  {
    // Serial.println("no in test mode");
    Serial.println("Welcome to the Seeed XIAO RA4M1!");
    digitalWrite(RGB_Y, LOW);  
    while(1)
    {
      RGB_Blink();
    }
  }
  test_mode();
  
}

void loop() {
  
}

void test_mode()
{
  if(!testButton())
  {
    Serial1.println("Button Failed");
    return ;
  }
  Serial1.println("Button Passed");
  // 执行 GPIO 测试
  if (testGPIO() == 1) {
      Serial1.println("GPIO Failed");
      while(1)
      {
        digitalWrite(RGB_Y, HIGH);
        delay(50);
        digitalWrite(RGB_Y, LOW);
        delay(50);
      }
      // Serial.println("GPIO Failed");
    }
    delay(200);
    Serial1.println("GPIO PASS");
    digitalWrite(RGB_Y, LOW);
    int read_battery_count = 20;
    int battery_pass_count = 0;
    float battery_voltage_norm = 3.6;
    while(read_battery_count > 0)
    {
      read_battery_count--;
      if(readBatteryVoltage() >= battery_voltage_norm)
      {
        battery_pass_count++;
        if(battery_pass_count >= 5)
        {
          delay(100);
          Serial1.println("Read Battery voltage Passed");
          break;
        }

      }
      delay(100);
    }
    if(read_battery_count > 0 )
    {
      delay(300);
      Serial1.println("Test Passed");
      
        // RGB_test_Blink();
      Sleep_mode();

    }
      //测试失败USER灯闪烁
      while(1)
      {
        digitalWrite(RGB_Y, HIGH);
        delay(50);
        digitalWrite(RGB_Y, LOW);
        delay(50);
      }


}

void GPIO_Init()
{
    // 初始化 GPIO 模式
  for (uint8_t pin : INPUT_PINS) {
      pinMode(pin, INPUT);  // 其他引脚设置为输入
    }

  for (uint8_t pin : OUTPUT_PINS) {
    pinMode(pin, OUTPUT);          // 设置为输出模式
    digitalWrite(pin, LOW);        // 设置初始状态为低
  }  
  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, HIGH); // Set the pin to HIGH to enable battery voltage reading
  pinMode(Button, INPUT);
}

void RGB_Blink()
{
  
      // Transition from Red to Green
  for (int i = 0; i <= 255; i++) {
    pixels.setPixelColor(0, pixels.Color(255 - i, i, 0));  // Red decreases, Green increases
    pixels.show();
    delay(10);  // Adjust delay for smoothness
  }

  // Transition from Green to Blue
  for (int i = 0; i <= 255; i++) {
    pixels.setPixelColor(0, pixels.Color(0, 255 - i, i));  // Green decreases, Blue increases
    pixels.show();
    delay(10);  // Adjust delay for smoothness
  }

  // Transition from Blue to Red
  for (int i = 0; i <= 255; i++) {
    pixels.setPixelColor(0, pixels.Color(i, 0, 255 - i));  // Blue decreases, Red increases
    pixels.show();
    delay(10);  // Adjust delay for smoothness
  }
}

void RGB_test_Blink()
{
  digitalWrite(RGB_Y, HIGH);
  delay(200);
  digitalWrite(RGB_Y, LOW);
  delay(200);
  
  pixels.clear();
  pixels.setPixelColor(0, pixels.Color(255, 0, 0));
  delay(200);
  pixels.show();
  pixels.clear();

  digitalWrite(RGB_Y, HIGH);
  delay(200);
  digitalWrite(RGB_Y, LOW);
  delay(200);

  pixels.setPixelColor(0, pixels.Color(0, 255, 0));
  delay(200);
  pixels.show();
  pixels.clear();

  digitalWrite(RGB_Y, HIGH);
  delay(200);
  digitalWrite(RGB_Y, LOW);
  delay(200);

  pixels.setPixelColor(0, pixels.Color(0, 0, 255));
  delay(200);
  pixels.show();
  pixels.clear();
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
      }  else {  // 其他非对应引脚
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

float readBatteryVoltage() {
  // Read the raw analog value from the ADC pin
  // analogRead() returns a 10-bit value (0-1023) by default on most Arduinos.
  // The RP2040 ADC is 12-bit, so analogRead() will return 0-4095.
  int rawValue = analogRead(ADC_PIN);

  // Conversion factor for 3.3V reference and 12-bit ADC (4095 max value)
  // 3.3V / 4095 = voltage per raw unit
  float conversionFactor = referenceVoltage / (1023);

  // Calculate the voltage at the ADC pin
  float adcVoltage = rawValue * conversionFactor;

  // The original MicroPython code used a voltage divider with a factor of 2.
  // This means the voltage at the battery is twice the voltage measured at the ADC pin.
  float batteryVoltage = adcVoltage * voltageDivider;
  Serial1.println(batteryVoltage, 2);
  Serial.println(batteryVoltage, 2);
  return batteryVoltage;
}