@echo off
setlocal

set "BUILD_DIR=build"
set "CONFIG=Debug"

if not "%~1"=="" set "BUILD_DIR=%~1"
if not "%~2"=="" set "CONFIG=%~2"

if not exist "%BUILD_DIR%\CMakeCache.txt" (
    exit /b 1
)

cmake --build "%BUILD_DIR%" --config %CONFIG%
if errorlevel 1 (
    exit /b 1
)

endlocal
