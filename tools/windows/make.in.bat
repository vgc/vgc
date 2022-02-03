:: Don't print commands before executing them and keep variables
:: and working directory local to this script
@echo on
setlocal

:: Setup Visual Studio development environment
call "${CMAKE_GENERATOR_INSTANCE}\Common7\Tools\VsDevCmd.bat" -arch=amd64

if "%~1"=="" (target="") else (target="--target %1")

echo %*
echo %0
echo %1
echo %2
echo %target%

:: Build the given target with the default config
cmake --build . %target% --config ${DefaultConfig}
