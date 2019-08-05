:: Don't print commands before executing them and keep variables
:: and working directory local to this script
@echo off
setlocal

:: Build the given target with the default config
ctest -C ${DefaultConfig} %*
