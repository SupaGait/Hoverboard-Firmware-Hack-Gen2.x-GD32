@echo off
setlocal

REM Flash master half: 4 KiB UART IAP BL @ 0x08000000, then app @ 0x08001000.
set "SCRIPT_DIR=%~dp0"
set "STFLASH=%SCRIPT_DIR%st-flash.exe"
set "ROOT=%SCRIPT_DIR%.."
set "BL=%ROOT%\Bootloader\build\master\bootloader-master.bin"
set "APP=%ROOT%\build\carrier-master.bin"

color 08
cls

if not exist "%STFLASH%" (
  echo st-flash.exe not found: %STFLASH%
  goto :failure
)
if not exist "%BL%" (
  echo Bootloader not found: %BL%
  echo Build with: make -C Bootloader
  goto :failure
)
if not exist "%APP%" (
  echo App not found: %APP%
  echo Build with: make carrier-master
  goto :failure
)

echo Flashing master bootloader @ 0x08000000 ...
"%STFLASH%" --connect-under-reset write "%BL%" 0x08000000 || goto :failure

echo:
echo Flashing master app @ 0x08001000 ...
"%STFLASH%" --connect-under-reset write "%APP%" 0x08001000 || goto :failure

color 0A
echo:
echo ================================
echo ^|       FLASH SUCCESSFUL       ^|
echo ================================

goto :exit

:failure

color 09
echo:
echo ================================
echo ^|       FLASH FAILED :-(      ^|
echo ================================
echo:

:exit
endlocal
