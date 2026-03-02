📦 **Seeed Studio XIAO 全系列 Bootloader 资源库**
本仓库致力于收集并分类整理 Seeed Studio XIAO 全系列开发板的 Bootloader 源代码及预编译二进制文件。无论你是需要修复“砖头”板，还是进行底层开发，这里都能提供官方标准的恢复文件。

---

## 📊 XIAO 系列全型号对比表 (Specifications)

| Board Model | Chipset | Bootloader Type | Flash Size | Download |
| --- | --- | --- | --- | --- |
| **XIAO ESP32S3 Plus** | ESP32-S3 |  | 16 MB | [Link] |
| **XIAO ESP32S3** | ESP32-S3 |  | 8 MB | [Link] |
| **XIAO ESP32S3 Sense** | ESP32-S3 |  | 8 MB | [Link] |
| **XIAO ESP32C3** | ESP32-C3 |  | 4 MB | [Link] |
| **XIAO ESP32C5** | ESP32-C5 |  | 4 MB | [Link] |
| **XIAO ESP32C6** | ESP32-C6 |  | 4 MB | [Link] |
| **XIAO RA4M1** | Renesas RA4M1 |  | 256 KB | [Link] |
| **XIAO MG24** | EFR32MG24 |  | 1.5 MB | [Link] |
| **XIAO MG24 Sense** | EFR32MG24 |  | 1.5 MB | [Link] |
| **XIAO nRF54L15** | nRF54L15 |  | 1.5 MB | [Link] |
| **XIAO nRF54L15 Sense** | nRF54L15 |  | 1.5 MB | [Link] |
| **XIAO nRF52840** | nRF52840 |  | 1 MB | [Link] |
| **XIAO nRF52840 Sense** | nRF52840 |  | 1 MB | [Link] |
| **XIAO RP2040** | RP2040 |  | 2 MB | [Link] |
| **XIAO RP2350** | RP2350 |  | 2 MB | [Link] |
| **XIAO SAMD21** | ATSAMD21G18 |  | 256 KB | [Link] |

---

## 🛠️ Bootloader 烧录与恢复方法分类

根据 Seeed Studio Wiki 官方指南，不同芯片架构的进入模式与烧录工具如下：

### 1. ESP32 系列 (S3 / C3 / C5 / C6)

* **进入 Bootloader 模式：** 1. 按住 **BOOT** 键。
2. 按一下 **RESET** 键（或重新插拔 USB）。
3. 松开 BOOT 键。
* **烧录工具：** 使用 `esptool.py`。
* **注意：** ESP32S3 Plus 与 Sense 版本的分区表和 Bootloader 偏移地址可能因 Flash 大小不同而有差异。

### 2. Renesas 系列 (RA4M1)

* **进入方式：** 快速 **短接 RST 两次** 或使用板载按键进入内置的 Boot ROM 模式。
* **烧录工具：** 使用 Renesas Flash Programmer 或 Arduino IDE 串口烧录。

### 3. nRF 系列 (nRF52840 / nRF54L15)

* **进入方式：** 快速 **双击 RESET 按钮**。
* 此时电脑应出现一个名为 `XIAO-nRFxx` 的 Mass Storage 磁盘。


* **底层恢复：** 若无法进入磁盘模式，需通过背面的 **SWD 焊盘** 使用 J-Link 或 DAP-Link 进行烧录。

### 4. MG24 系列 (Matter / Zigbee)

* **进入方式：** 遵循 Silicon Labs 的通用烧录逻辑，通常通过串口进入监控模式或使用 Simplicity Studio 进行恢复。
* **特点：** MG24 专注于 Matter 和 AI 加速，其 Bootloader 包含安全启动（Secure Boot）相关逻辑。

### 5. RP 系列 (RP2040 / RP2350)

* **进入方式：** 按住 **BOOT** 键后插入 USB。
* 电脑会识别为 `RPI-RP2` 或 `RPI-RP3` 磁盘。


* **烧录方式：** 将 `.uf2` 文件直接拖入磁盘即可。

### 6. SAMD 系列 (SAMD21)

* **进入方式：** 快速 **短接 RST 焊盘两次**。
* **状态指示：** 板载橙色 LED 进入呼吸/闪烁状态即代表成功进入。

---

## 📂 资源获取

* **`/Source`**: 各型号 Bootloader 源码，包含编译环境配置说明。
* **`/Binaries`**: 已编译完成的 `.bin` / `.hex` / `.uf2` 文件，可直接下载使用。

---

## ⚠️ 免责声明

烧录 Bootloader 存在风险，可能导致硬件损坏。请在操作前确保您已了解相关风险并备份数据。
