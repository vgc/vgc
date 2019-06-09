:: Don't print commands before executing them and keep variables
:: and working directory local to this script
@echo off
setlocal

:: Setup Visual Studio development environment
call "${CMAKE_GENERATOR_INSTANCE}\Common7\Tools\VsDevCmd.bat" -arch=amd64

:: Build the given target with the default config
cmake --build . --target %* --config ${DefaultConfig}
