@echo off
REM Build script for the MapGen Engine (Windows, MinGW-w64 / MSYS2 g++).
setlocal

where g++ >nul 2>nul
if errorlevel 1 (
    echo g++ not found on PATH. Install MSYS2 or MinGW-w64 and retry.
    exit /b 1
)

set CXXFLAGS=-std=c++17 -O2 -Wall -Wextra -Iinclude
set SRCS=src\CampusMap.cpp src\MapGenerator.cpp src\Navigation.cpp src\Menu.cpp

set WEBSRCS=%SRCS% src\web\HttpServer.cpp src\web\Json.cpp src\web\ApiController.cpp

REM Console app
g++ %CXXFLAGS% src\main.cpp %SRCS% -o mapgen.exe
if errorlevel 1 (
    echo Build failed.
    exit /b 1
)

REM Web server (link ws2_32 for Winsock)
g++ %CXXFLAGS% src\web_main.cpp %WEBSRCS% -o mapgen_web.exe -lws2_32
if errorlevel 1 (
    echo Web build failed.
    exit /b 1
)

g++ %CXXFLAGS% tests\self_test.cpp %SRCS% -o mapgen_tests.exe
if errorlevel 1 (
    echo Test build failed.
    exit /b 1
)

g++ %CXXFLAGS% tests\web_test.cpp %WEBSRCS% -o mapgen_web_tests.exe -lws2_32
if errorlevel 1 (
    echo Web test build failed.
    exit /b 1
)

echo Build complete: mapgen.exe, mapgen_web.exe, mapgen_tests.exe, mapgen_web_tests.exe
endlocal
