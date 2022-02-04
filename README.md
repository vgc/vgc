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

## Build Instructions

Prerequisites:
- **CMake 3.1+**: We recommend the latest version of CMake.
- **C++17**: We recommend Visual Studio 2017 on Windows, and any recent version of Clang/GCC on macOS/Linux.
- **Python 3.6+**: We recommend the latest Python 3.7.x version via the [official installer](https://www.python.org/downloads/).
- **Qt 5.12+**: We recommend the latest Qt 5.12.x version via the [official installer](https://www.qt.io/download-qt-installer).
- **FreeType**: We recommend the latest FreeType 2.x.
- **HarfBuzz**: We recommend the latest HarfBuzz 2.x.
- **OpenGL Dev Tools**: Already installed on Windows, macOS, and many Linux distributions. On Ubuntu, you need `sudo apt install libglu1-mesa-dev`.

VGC also depends on the following libraries, but these are already included in
the `third` folder, so there is no need to pre-install them:
- **Eigen**
- **{fmt}**
- **Google Test**
- **pybind11**

VGC follows the [VFX Reference Platform](http://www.vfxplatform.com/)
recommendations for library versions.

#### Windows 7/8/10, Visual Studio 2017 64bit, Python 3.7, Qt 5.12.6

Manually install Git, CMake, Visual Studio, Python, Qt, then:

```
# Download and build FreeType and HarfBuzz via vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
.\vcpkg install freetype:x64-windows harfbuzz:x64-windows
cd ..

# Download, build, and run VGC
git clone https://github.com/vgc/vgc.git
mkdir build && cd build
cmake ..\vgc ^
    -DCMAKE_TOOLCHAIN_FILE="%UserProfile%\vcpkg\scripts\buildsystems\vcpkg.cmake" ^
    -G "Visual Studio 15 2017" -A x64 ^
    -DPython="%UserProfile%\AppData\Local\Programs\Python\Python37" ^
    -DQt="C:\Qt\5.12.6\msvc2017_64"
make
\Release\bin\vgcillustration.exe
```

#### macOS 10.14, Xcode 10.3 (=> Clang 10.0.1), Python 3.7, Qt 5.12.6

Manually install Xcode, Python, Qt, then:

```
# Download and install FreeType and HarfBuzz via homebrew
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install.sh)"
brew install freetype harfbuzz

# Download, build, and run VGC
git clone https://github.com/vgc/vgc.git
mkdir build && cd build
cmake ../vgc \
    -DCMAKE_BUILD_TYPE=Release \
    -DPython="/Library/Frameworks/Python.framework/Versions/3.7" \
    -DQt="~/Qt/5.12.6/clang_64"
make
./Release/bin/vgcillustration
```

#### Ubuntu 18.04 (=> GCC 7.4, Python 3.6), Qt 5.12.6

Manually install Qt, then:

```
# Install remaining dependencies
sudo apt install git cmake build-essential python3-dev libglu1-mesa-dev libfreetype6-dev libharfbuzz-dev

# Download, build, and run VGC
git clone https://github.com/vgc/vgc.git
mkdir build && cd build
cmake ../vgc \
    -DCMAKE_BUILD_TYPE=Release \
    -DQt="~/Qt/5.12.6/gcc_64"
make
./Release/bin/vgcillustration
```
