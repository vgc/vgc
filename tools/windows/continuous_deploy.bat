:: Don't print commands before executing them and keep variables
:: and working directory local to this script
@echo off
setlocal

:: Get datetime in a locale-independent way
for /f %%a in ('Powershell -Command "Get-Date -format yyyy-MM-dd_HHmm.ss"') do set datetime=%%a

:: Go to deploy folder
cd %UserProfile%
if not exist vgcdeploy mkdir vgcdeploy
cd vgcdeploy
mkdir %datetime%
cd %datetime%

:: Get source code
:: XXX We temporarily use the dalboris:gh62 branch for testing
:: XXX TODO Change to git clone https://github.com/vgc/vgc.git
git clone -b gh62 https://github.com/dalboris/vgc.git

:: Compile and deploy
mkdir build
cd build
cmake ../vgc ^
    -G "Visual Studio 15 2017" -A x64 ^
    -DQt="C:/Qt/5.12.3/msvc2017_64" ^
    -DWiX="C:/Program Files (x86)/WiX Toolset v3.11"
make.bat
make.bat deploy

:: TODO Upload MSI file to server
