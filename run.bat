@echo off
chcp 65001 >nul 2>nul
title Campus Bookstore Management System

:: ============================================================
:: Campus Second-hand Book Trading Management System - Start Script
:: ============================================================

set PROJECT_DIR=%~dp0
cd /d "%PROJECT_DIR%"

:: —— Find GCC Compiler ——
set GCC=
for %%G in (
    "C:\msys64\mingw64\bin\gcc.exe"
    "C:\msys64\mingw32\bin\gcc.exe"
    "C:\mingw64\bin\gcc.exe"
    "C:\MinGW\bin\gcc.exe"
) do if exist %%G set GCC=%%G

if "%GCC%"=="" (
    echo [ERROR] GCC compiler not found.
    echo Please install MSYS2 and run: pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-sqlite3
    echo.
    pause
    exit /b 1
)

echo Compiler: %GCC%
echo.

:: —— Shutdown previous server instance ——
taskkill /f /im server.exe >nul 2>nul

:: —— Compile Server ——
echo --- Compiling server.exe ...
%GCC% server.c simple_database.c -o server.exe -lsqlite3 -lws2_32 -O2 -Wall
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Compilation failed! Please check the error messages.
    pause
    exit /b 1
)
echo Compilation successful!
echo.

:: —— Start Server ——
echo ============================================================
echo  Campus Second-hand Book Trading Management System
echo ============================================================
echo  Server starting on port: 8080
echo  Open browser: http://localhost:8080
echo ============================================================
echo.
echo  Press Ctrl+C to stop the server.
echo.

start http://localhost:8080
server.exe

pause