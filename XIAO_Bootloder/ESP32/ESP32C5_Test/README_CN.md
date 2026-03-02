# Seeed Studio XIAO ESP32C5 工厂测试固件

本项目包含 Seeed Studio XIAO ESP32C5 开发板的工厂测试固件。该固件旨在制造过程中验证硬件功能（GPIO、无线功能、电池、按键），并为最终用户提供友好的开箱体验。

## 项目结构

```
ESP32C5_Test/
├── ESP32C5_Test.ino          # Arduino 主程序
├── src/
│   ├── XIAO_Common/          # 通用 XIAO 工具库
│   │   ├── XIAO_Generic.h    # 引脚定义和工具类头文件
│   │   └── XIAO_Generic.cpp  # GPIO、电池和 LED 实现
│   └── XIAO_ESP32/           # ESP32 特定功能库
│       ├── XIAO_ESP32_Lib.h  # WiFi、BLE 和 NVS 头文件
│       └── XIAO_ESP32_Lib.cpp# WiFi/BLE 扫描和 NVS 存储实现
└── README.md                 # 说明文档 (英文)
└── README_CN.md              # 说明文档 (中文)
```

## 运行模式

固件根据启动时 **D1** 引脚的状态，在两种不同的模式下运行：

### 1. 工厂测试模式 (Factory Test Mode)
*   **触发条件**：在上电或复位前，将 **D1** 引脚连接到 **GND** (拉低)。
*   **目的**：在工厂测试架上进行自动化的硬件验证。
*   **行为**：
    *   初始化 UART0 (`MySerial0`) 用于与测试架通信。
    *   初始化 USB Serial (`Serial`) 用于调试输出。
    *   **双重日志 (Dual Logging)**：所有测试日志会同时发送到 UART0 和 USB Serial。
    *   **测试序列**：
        1.  **按键测试 (Button Test)**：检查 BOOT 按键是否功能正常。
        2.  **无线测试 (Wireless Test)**：扫描 WiFi 网络和蓝牙设备。
        3.  **GPIO 测试 (GPIO Test)**：检查输入/输出引脚（需要在测试架上进行回环连接）。
        4.  **电池测试 (Battery Test)**：通过板载分压电路测量电池电压。
    *   **结果存储**：最终测试结果 (PASS/FAIL) 将保存到非易失性存储器 (NVS) 中。
    *   **LED 指示**：
        *   **通过 (PASS)**：LED 常亮（闪烁结束后）。
        *   **失败 (FAIL)**：LED 持续闪烁。

### 2. 正常模式 / 出货模式 (Normal Mode)
*   **触发条件**：启动时 **D1** 引脚悬空 (高电平)。
*   **目的**：最终用户的默认行为。
*   **行为**：
    *   从 NVS 读取保存的测试结果。
    *   **如果测试通过 (PASSED)**：向 USB Serial 打印 "Hello from Seeed Studio XIAO ESP32-C5" 并闪烁 LED（心跳灯效果）。
    *   **如果测试失败 (FAILED)**：打印 "Device test FAILED"。
    *   **如果未测试 (NOT RUN)**：打印 "Device not tested"。

## 测试硬件要求

*   **测试架**：一个可以将 D1 连接到 GND 以触发测试模式的治具。
*   **回环连接**：为了通过 GPIO 测试，特定的输出引脚必须连接到输入引脚（具体定义见 `XIAO_Generic.h`）。
*   **电池**：连接到电池焊盘的电池或模拟电压源，用于电池测试。

## 编译与上传

1.  在 Arduino IDE 中打开 `ESP32C5_Test.ino`。
2.  选择开发板：**Seeed Studio XIAO ESP32C5**。
3.  选择端口：设备的 COM 端口。
4.  点击 **上传 (Upload)**。

## 调试指南

*   **上传失败**：如果上传时出现 "Fatal error occurred"，请尝试：
    *   降低上传速度 (Upload Speed)，例如降至 115200。
    *   按住 BOOT 键的同时连接 USB。
    *   更换一根质量更好的 USB 数据线。
*   **查看日志**：打开串口监视器 (Serial Monitor)，波特率设置为 **115200**。
