@echo off

echo =========================================================
echo     按任意键开始烧录
echo =========================================================
pause

:START_LOOP
cd openocd
echo.
echo =========================================================
echo       正在烧录 SAMD11 (J-Link)...
echo =========================================================
.\JLink\JLink -if swd -commandFile .\program_test_nrf54.jlink
timeout /t 1 /nobreak > nul

cd ..
echo.
echo =========================================================
echo       正在烧录 NRF54L (OpenOCD)...
echo =========================================================

openocd\openocd.exe  -f openocd\interface/cmsis-dap.cfg -f openocd\nrf54l.cfg -c "init; mww 0x5004b500 0x101; load_image merged_CPUFLPR.hex; reset run; exit"
openocd\openocd.exe  -f openocd\interface/cmsis-dap.cfg -f openocd\nrf54l.cfg -c "targets nrf54l.cpu" -c "init; mww 0x5004b500 0x101; load_image merged.hex; reset run; exit"


echo.
echo =========================================================
echo       当前烧录完成，按任意键等待下一轮...
echo =========================================================
pause
goto START_LOOP