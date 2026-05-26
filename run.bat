@echo off
chcp 65001 >nul 2>nul
title 校园二手书交易管理系统

:: ============================================================
:: 校园二手书交易管理系统 - 一键启动脚本
:: 流程：编译 server.exe → stats_report.exe → welcome.exe → 启动 welcome.exe
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
echo [1/3] Compiling server.exe ...
%GCC% server.c simple_database.c data_persistence.c -o server.exe -lsqlite3 -lws2_32 -O2 -Wall
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] server.exe compilation failed! Please check the error messages.
    pause
    exit /b 1
)
echo server.exe compiled successfully!
echo.

:: —— Compile Stats Report ——
echo [2/3] Compiling stats_report.exe ...
%GCC% stats_report.c -o stats_report.exe -lsqlite3 -O2 -Wall
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] stats_report.exe compilation failed! Please check the error messages.
    pause
    exit /b 1
)
echo stats_report.exe compiled successfully!
echo.

:: —— Compile Welcome ——
echo [3/3] Compiling welcome.exe ...
%GCC% welcome.c -o welcome.exe -O2 -Wall
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] welcome.exe compilation failed! Please check the error messages.
    pause
    exit /b 1
)
echo welcome.exe compiled successfully!
echo.

:: —— Start Welcome (Launcher) ——
echo ============================================================
echo  Starting System Launcher...
echo ============================================================
echo.

welcome.exe

pause