@Echo off

ECHO 按任意键开始烧录。
pause
:start_loop
.\JLink\JLink -if swd -commandFile program_test.jlink

timeout /t 2 /nobreak

dfu-util.exe --device 0x2886:0x0049,:0x8049 -D ./XIAO_RA4M1_Test_firmware.bin -a0 -R

ECHO 烧录完毕，请更换下一块待烧录产品。
pause
goto start_loop