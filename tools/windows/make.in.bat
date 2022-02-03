:: Don't print commands before executing them and keep variables
:: and working directory local to this script
@echo off
setlocal

:: Setup Visual Studio development environment
call "${CMAKE_GENERATOR_INSTANCE}\Common7\Tools\VsDevCmd.bat" -arch=amd64

:: Build the given target with the default config
if "%~1"=="" (
    cmake --build . --config ${DefaultConfig}
) else (
    cmake --build . --target %1 --config ${DefaultConfig}
)
