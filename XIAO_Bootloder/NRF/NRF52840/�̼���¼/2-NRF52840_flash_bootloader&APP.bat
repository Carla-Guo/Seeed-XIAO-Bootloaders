@ECHO OFF
ECHO start to auto processing and exit
ECHO 按任意键开始烧录。
pause

:StartLoop
REM Always start in the JFlash directory for the JFlash.exe command
cd JFlash
ECHO Starting J-Flash process...

REM The J-Flash command
JFlash.exe -open"Bootloader&APP.jflash" -open"XIAO_NRF52840_Test_firmware.ino.with_bootloader.hex" -auto -exit

REM Check the error level of the last command (JFlash.exe)
IF ERRORLEVEL 1 goto ERROR_HANDLER
goto SUCCESS_HANDLER

:ERROR_HANDLER
ECHO J-Flash program Error!!
REM Return to the original directory before pausing, if needed for context
cd ..

pause
goto StartLoop

:SUCCESS_HANDLER
ECHO /*****************************************/
ECHO /* */
ECHO /* NRF52840 bootloader flash done     */
ECHO /* */
ECHO /*****************************************/
REM Return to the original directory before pausing, if needed for context
cd ..
ECHO 烧录完成。请更换下一个板子，然后按任意键继续。
pause
goto StartLoop