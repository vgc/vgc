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

:: Get VGC
:: XXX We temporarily use the dalboris:gh62 branch for testing
:: XXX TODO Change to git clone https://github.com/vgc/vgc.git
git clone -b gh62 https://github.com/dalboris/vgc.git

:: Setup VS 2017 development environment. Importantly:
:: - Add msbuild to the PATH
:: - Set VCINSTALLDIR so that windeployqt finds the vc_redist.exe file
:: - Make sure that the proper d3dcompiler DLL is found
:: - Probably other useful variables
::
set vsdir=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community
call "%vsdir%\Common7\Tools\VsDevCmd.bat" -arch=amd64

:: Build VGC
mkdir build
cd build
cmake ../vgc ^
	-A x64 ^
	-G "Visual Studio 15 2017" ^
    -DQt="C:/Qt/5.12.3/msvc2017_64" ^
    -DWiX="C:/Program Files (x86)/WiX Toolset v3.11"
msbuild vgc.sln /p:Configuration=Release

:: Generate MSI file
msbuild deploy.vcxproj /p:Configuration=Release

:: TODO Upload MSI file to server
