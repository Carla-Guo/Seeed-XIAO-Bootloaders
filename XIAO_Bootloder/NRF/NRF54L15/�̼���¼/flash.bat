openocd\openocd.exe  -f openocd\interface/cmsis-dap.cfg -f openocd\nrf54l.cfg -c "init; mww 0x5004b500 0x101; load_image merged_CPUFLPR.hex; reset run; exit"
openocd\openocd.exe  -f openocd\interface/cmsis-dap.cfg -f openocd\nrf54l.cfg -c "targets nrf54l.cpu" -c "init; mww 0x5004b500 0x101; load_image merged.hex; reset run; exit"

