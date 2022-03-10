![VGC](https://github.com/vgc/vgc/blob/master/hero.png)

[![Windows Build](https://github.com/vgc/vgc/actions/workflows/windowsbuild.yml/badge.svg?branch=master&event=push)](https://github.com/vgc/vgc/actions/workflows/windowsbuild.yml)
[![macOS Build](https://github.com/vgc/vgc/actions/workflows/macosbuild.yml/badge.svg?branch=master&event=push)](https://github.com/vgc/vgc/actions/workflows/macosbuild.yml)
[![Linux Build](https://github.com/vgc/vgc/actions/workflows/linuxbuild.yml/badge.svg?branch=master&event=push)](https://github.com/vgc/vgc/actions/workflows/linuxbuild.yml)

VGC is an upcoming suite of applications for graphic design and 2D animation,
in which the lines and shapes you draw are connected to each others both in
space and time, allowing for faster editing and inbetweening.

VGC is licensed under the **[Apache 2.0 License](https://github.com/vgc/vgc/blob/master/LICENSE)**.

More info: **[www.vgc.io](https://www.vgc.io)**

## Disclaimer

This project is still in very early development (pre-alpha) and doesn't yet
have any of the innovative features which make VGC unique. If you are curious,
you can try our earlier research prototype [VPaint](https://www.vpaint.org).

## Dependencies

- **CMake 3.1+**: We recommend the latest version of CMake.
- **C++17**: We recommend Visual Studio 2019 on Windows, Clang 5+ on macOS, and GCC 7+ on Linux.
- **Python 3.6+**: We recommend the latest Python 3.7.x version via the [official installer](https://www.python.org/downloads/).
- **Qt 5.15+**: We recommend Qt 5.15.2 via the [official installer](https://www.qt.io/download-qt-installer).
- **FreeType 2+**: We recommend the latest version of FreeType.
- **HarfBuzz 2+**: We recommend the latest version of HarfBuzz.
- **OpenGL Dev Tools**: Already installed on Windows, macOS, and many Linux distributions. On Ubuntu, you need `sudo apt install libglu1-mesa-dev`.

VGC also depends on the following libraries, but these are already included in
the `third` folder, so there is no need to pre-install them:
- **Eigen**
- **{fmt}**
- **Google Test**
- **libtess2**
- **pybind11**

VGC follows the [VFX Reference Platform](http://www.vfxplatform.com/)
recommendations for library versions.

## Build instructions for Windows

### Install dependencies (Windows 8/10/11)

Manually install the latest versions of Git, CMake, and Powershell, then the desired versions of Visual Studio, Python, and Qt. Make sure to have installed a recent version of Powershell because it is required by vcpkg.

You can then install vcpkg and the other dependencies of VGC by entering these instructions on a command prompt:

```
cd %UserProfile%
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg install freetype:x64-windows harfbuzz:x64-windows
```

Skip the next section and go to "build and run VGC".

### Install dependencies (Windows 7)

First, install Windows Management Platform 5.1, which is required for Powershell.

Then, manually install the latest versions of Git, CMake, and Powershell, then the desired versions of Visual Studio, Python, and Qt.

We then need to install vcpkg. Unfortunately, the latest versions of vcpkg use Python 3.9+ which isn't supported on Windows 7. Therefore, we need to patch it to use Python 3.8.10 instead, as explained below.

First, clone vcpkg by entering these instructions on a command prompt:

```
cd %UserProfile%
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
```

Then, open the file vcpkg\ports\python3\portfile.cmake with a text editor and perform the following changes around lines 5-10:

```
set(PYTHON_VERSION_MAJOR 3)
set(PYTHON_VERSION_MINOR 8)
set(PYTHON_VERSION_PATCH 10)
```

Then, open the file vcpkg\ports\python3\portfile.cmake and perform the following changes around lines 250-260:

```
            set(program_name python)
            set(program_version 3.8.10)
            if(VCPKG_TARGET_ARCHITECTURE STREQUAL "x86")
                set(tool_subdirectory "python-${program_version}-x86")
                set(download_urls "https://www.python.org/ftp/python/${program_version}/python-${program_version}-embed-win32.zip")
                set(download_filename "python-${program_version}-embed-win32.zip")
                set(download_sha512 a6f0c35ba37c07c6e8815fb43c20095541117f3b6cc034b8ef0acfc908de8951bdffa38706bac802f820290b39ae84f934f27a8e32f548735f470277f7a70550)
            else()
                set(tool_subdirectory "python-${program_version}-x64")
                set(download_urls "https://www.python.org/ftp/python/${program_version}/python-${program_version}-embed-amd64.zip")
                set(download_filename "python-${program_version}-embed-amd64.zip")
                set(download_sha512 86e55911be78205a61f886feff2195c78a6f158a760cc1697ce4340dcb5ca118360251de2f707b6d2a78b7469d92c87b045b7326d6f194bfa92e665af1cd55a5)
            endif()
            set(paths_to_search "${DOWNLOADS}/tools/python/${tool_subdirectory}")
            vcpkg_list(SET post_install_command "${CMAKE_COMMAND}" -E rm python38._pth)
```

You can now finish the installation of vcpkg and of VGC dependencies with the following commands:

```
.\bootstrap-vcpkg.bat
.\vcpkg install freetype:x64-windows harfbuzz:x64-windows
```

### Build and run VGC (Windows 7/8/10/11)


Enter the following on a command prompt.

Note that in this example, we assume that you installed Visual Studio 2019, Python 3.7 and Qt 5.15.2 in their default paths. If you installed different versions, or if you installed them in different paths, change the `cmake` command below accordingly.


```
cd %UserProfile%
git clone --recurse-submodules https://github.com/vgc/vgc.git
mkdir build && cd build
cmake .. ^
    -DCMAKE_TOOLCHAIN_FILE="%UserProfile%\vcpkg\scripts\buildsystems\vcpkg.cmake" ^
    -DVCPKG_MANIFEST_MODE=OFF ^
    -G "Visual Studio 16 2019" -A x64 ^
    -DPython="%UserProfile%\AppData\Local\Programs\Python\Python37"
    -DQt5_DIR="C:\Qt\5.15.2\msvc2019_64\lib\cmake\Qt5"
.\make.bat
\Release\bin\vgcillustration.exe
```

Note: `make.bat` is a convenient custom script which sets up a Visual Studio environment by calling the appropriate `VsDevCmd.bat` file, then call `cmake --build . --config Release`.

### Generate an installer (optional)

If you'd like to generate an installer, you will need to install WiX 3.11, add its path to the cmake command, then run make deploy, as such:

```
cmake ..\vgc ^
    [other cmake variables here]
    -DWiX="C:/Program Files (x86)/WiX Toolset v3.11"
make
make deploy
```

## Build instructions for macOS

(Note: if you're using different software versions, change commands accordingly)

Manually install Xcode, Python, Qt, then:

```
# Download and install FreeType and HarfBuzz via homebrew
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install.sh)"
brew install freetype harfbuzz

# Download, build, and run VGC
git clone --recurse-submodules https://github.com/vgc/vgc.git
mkdir build && cd build
cmake ../vgc \
    -DCMAKE_BUILD_TYPE=Release \
    -DPython="/Library/Frameworks/Python.framework/Versions/3.7" \
    -DQt5_DIR="~/Qt/5.15.2/clang_64/lib/cmake/Qt5"
make
./Release/bin/vgcillustration
```

## Build instructions for Ubuntu 18.04+

(Note: if you're using different software versions, change commands accordingly)

Manually install Qt, then:

```
# Install remaining dependencies
sudo apt install git cmake build-essential python3-dev libglu1-mesa-dev libfreetype6-dev libharfbuzz-dev

# Download, build, and run VGC
git clone --recurse-submodules https://github.com/vgc/vgc.git
mkdir build && cd build
cmake ../vgc \
    -DCMAKE_BUILD_TYPE=Release \
    -DQt5_DIR="~/Qt/5.15.2/gcc_64/lib/cmake/Qt5"
make
./Release/bin/vgcillustration
```
