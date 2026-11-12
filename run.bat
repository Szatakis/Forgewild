@echo off
title VoidTerminal Launcher
cd /d "%~dp0"

echo.
echo ========================================
echo        VOIDTERMINAL LAUNCHER
echo ========================================
echo.

echo [1/5] Checking Python 3.14...

py -3.14 --version

if errorlevel 1 (
    echo [ERROR] Python 3.14 not installed!
    pause
    exit /b 1
)

echo.
echo [2/5] Creating virtual environment...

if not exist ".venv\Scripts\python.exe" (
    py -3.14 -m venv .venv
)

if not exist ".venv\Scripts\python.exe" (
    echo [ERROR] Could not create virtual environment!
    pause
    exit /b 1
)

echo.
echo [3/5] Updating pip...

".venv\Scripts\python.exe" -m pip install --upgrade pip setuptools wheel

if errorlevel 1 (
    echo [ERROR] Pip update failed!
    pause
    exit /b 1
)

echo.
echo [4/5] Installing requirements...

".venv\Scripts\python.exe" -m pip install -r requirements.txt

if errorlevel 1 (
    echo [ERROR] Requirements installation failed!
    pause
    exit /b 1
)

echo.
echo [5/5] Checking Pygame...

".venv\Scripts\python.exe" -c "import pygame; print('Pygame version:', pygame.version.ver)"

if errorlevel 1 (
    echo [ERROR] Pygame import failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo       STARTING VOIDTERMINAL
echo ========================================
echo.

".venv\Scripts\python.exe" main.py

echo.
echo Terminal closed.
pause
