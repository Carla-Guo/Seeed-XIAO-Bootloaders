@ECHO OFF
ECHO start to auto processing and exit

 JFlash.exe -openbootloader-XIAO.bin -openprjATSAMD21G18.jflash -connect  -auto -startapp -exit
IF ERRORLEVEL 1 goto ERROR

ECHO OK!  flash upload complete!
echo Reset the board and Upload the fileware

@echo.
seeeduino_xiao_test_program.exe
IF ERRORLEVEL 1 goto ERROR
ECHO Successful QVQ
exit

:ERROR
ECHO  Program Error!!
pause