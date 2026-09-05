@echo off
setlocal
cd /d "%~dp0"

echo =====================================================================
echo   TAMAGOTCHI P1 (1996) - 1:1 PURE C FIRMWARE BUILD (E0C6S46)
echo =====================================================================
echo.

echo [1/2] Compiling pure C sources (main.c) with option --match-rom ...
..\..\c_compiler.exe main.c -o tamagotchi_p1.bin -s tamagotchi_p1.s -l tamagotchi_p1.lst --match-rom
if errorlevel 1 (
    echo.
    echo [ERROR] C compilation failed!
    exit /b 1
)

echo.
echo [2/2] Running bit-by-bit differential verification...
python verify.py
if errorlevel 1 (
    echo [ERROR] Binaries do not match!
    exit /b 1
)

echo.
echo Build completed successfully: tamagotchi_p1.bin is 100%% byte-identical!


rem --- Beispiel ausfuehren (Emulator aus C:\Users\James\tools) ---
set "EMU=%USERPROFILE%\tools\tamagotcha\tamagotcha_display.exe"
if exist "%EMU%" (
    echo. & echo Starte tamagotchi_p1.bin im Tamagotchi-Emulator ...
    "%EMU%" tamagotchi_p1.bin
) else (
    echo [INFO] Emulator nicht unter %EMU% - nur kompiliert.
)