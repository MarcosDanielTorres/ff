@echo off
setlocal enabledelayedexpansion
set root=%cd%

call build.bat || (
    echo Build failed with error %ERRORLEVEL%.
    exit /b %ERRORLEVEL%
)
.\build\physics_example_v2.exe