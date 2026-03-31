@echo off
setlocal

set "BUILD_DIR=build"
set "BUILD_TYPE=Debug"

if not "%~1"=="" set "BUILD_DIR=%~1"
if not "%~2"=="" set "BUILD_TYPE=%~2"

cmake -S . -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if errorlevel 1 (
    exit /b 1
)
endlocal
