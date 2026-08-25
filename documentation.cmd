@echo off
setlocal EnableDelayedExpansion
title DIY TP-7 Field Recorder - Project Documentation Hub

:menu
cls
echo ===============================================================================
echo            DIY TP-7 MOTORIZED FIELD RECORDER - DOCUMENTATION HUB
echo                     Teenage Engineering Inspired Digital Twin
echo ===============================================================================
echo.
echo   [1] Open Digital Twin Simulator in Browser (index.html)
echo   [2] Open Commercialization ^& Legal Compliance PDF Guide
echo   [3] Open Breadboard Prototyping Guide (Markdown)
echo   [4] Open Custom PCB Design ^& Manufacturing Guide (Markdown)
echo   [5] Open Required Parts List ^& Sourcing BOM (Markdown)
echo   [6] Open Arduino IDE ^& ESP32 Setup Guide (Markdown)
echo   [7] Open Original Build Guide (DIY-TP7-Build-Guide.md)
echo   [8] Open Wiring Diagram PDF (DIY-TP7-Wiring-Diagram.pdf)
echo   [9] Launch Arduino IDE with TP-7 Firmware (TP7_Replica.ino)
echo   [10] Launch Arduino IDE with Breadboard Diagnostics (Breadboard_Subsystem_Tester.ino)
echo   [11] Regenerate 3D Printable STL Files (generate_stls.ps1)
echo   [12] Open 3D Models Folder (3D_Models)
echo   [13] View Project Repository on GitHub
echo   [0] Exit
echo.
echo ===============================================================================
set /p choice="Enter your choice (0-13): "

if "%choice%"=="1" (
    echo Launching Digital Twin Simulator...
    start "" "%~dp0index.html"
    goto menu
)
if "%choice%"=="2" (
    echo Opening Commercialization ^& Legal Guide PDF...
    start "" "%~dp0DIY-TP7-Commercialization-And-Legal-Guide.pdf"
    goto menu
)
if "%choice%"=="3" (
    echo Opening Breadboard Prototyping Guide...
    start "" "%~dp0Breadboard-Prototyping-Guide.md"
    goto menu
)
if "%choice%"=="4" (
    echo Opening PCB Design ^& Manufacturing Guide...
    start "" "%~dp0PCB-Design-And-Manufacturing-Guide.md"
    goto menu
)
if "%choice%"=="5" (
    echo Opening Required Parts List ^& Sourcing BOM...
    start "" "%~dp0Required-Parts-List-And-Sourcing.md"
    goto menu
)
if "%choice%"=="6" (
    echo Opening Arduino Setup Guide...
    start "" "%~dp0Arduino-Prototyping-Setup-Guide.md"
    goto menu
)
if "%choice%"=="7" (
    echo Opening Original Build Guide...
    start "" "%~dp0DIY-TP7-Build-Guide.md"
    goto menu
)
if "%choice%"=="8" (
    echo Opening Wiring Diagram PDF...
    start "" "%~dp0DIY-TP7-Wiring-Diagram.pdf"
    goto menu
)
if "%choice%"=="9" (
    echo Opening TP7_Replica.ino in Arduino IDE...
    start "" "%~dp0TP7_Replica\TP7_Replica.ino"
    goto menu
)
if "%choice%"=="10" (
    echo Opening Breadboard_Subsystem_Tester.ino in Arduino IDE...
    start "" "%~dp0TP7_Replica\Breadboard_Subsystem_Tester.ino"
    goto menu
)
if "%choice%"=="11" (
    echo Regenerating 3D STL Models...
    powershell -ExecutionPolicy Bypass -File "%~dp0generate_stls.ps1"
    pause
    goto menu
)
if "%choice%"=="12" (
    echo Opening 3D Models Folder...
    start "" "%~dp03D_Models"
    goto menu
)
if "%choice%"=="13" (
    echo Opening GitHub Repository...
    start "" "https://github.com/praxlabs/tp-7-replica-diy"
    goto menu
)
if "%choice%"=="0" (
    exit /b 0
)

echo Invalid selection. Please try again.
timeout /t 2 >nul
goto menu
