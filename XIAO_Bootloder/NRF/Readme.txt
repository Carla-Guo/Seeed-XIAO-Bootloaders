1. bootloader均在固件烧录文件夹下，
注意：(1).NRF52840的bootloader没有独立保存，是用Arduino烧录整个固件后用jilink读取出来保存的
(2). NRF54L15需要先给SAMD11烧录固件后再烧录NRF的固件：
echo =========================================================
echo       正在烧录 NRF54L (OpenOCD)...
echo =========================================================

openocd\openocd.exe  -f openocd\interface/cmsis-dap.cfg -f openocd\nrf54l.cfg -c "init; mww 0x5004b500 0x101; load_image merged_CPUFLPR.hex; reset run; exit"
openocd\openocd.exe  -f openocd\interface/cmsis-dap.cfg -f openocd\nrf54l.cfg -c "targets nrf54l.cpu" -c "init; mww 0x5004b500 0x101; load_image merged.hex; reset run; exit"

