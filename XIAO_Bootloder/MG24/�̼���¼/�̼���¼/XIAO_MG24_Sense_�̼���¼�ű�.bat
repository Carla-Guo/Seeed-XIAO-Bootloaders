@Echo off

pause

:StartLoop

%~dp0\JLink\JLink -if swd -commandFile %~dp0\program_test.jlink
timeout /t 2 /nobreak >NUL
%~dp0openocd -s %~dp0scripts/ -f %~dp0scripts/interface/cmsis-dap.cfg -f %~dp0scripts/target/efm32s2_g23.cfg -c "init; reset_config srst_nogate; reset halt; program {%~dp0XIAO_MG24_Sense_Test_firmware.ino.hex.}; reset; exit


pause
goto StartLoop
