![VGC](https://github.com/vgc/vgc/blob/master/hero.png)

[![TravisCI Build Status](https://travis-ci.org/vgc/vgc.svg?branch=master)](https://travis-ci.org/vgc/vgc)
[![AppVeyor Build status](https://ci.appveyor.com/api/projects/status/3tasnhbrlucfltp5?svg=true)](https://ci.appveyor.com/project/vgc/vgc)

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
- **C++11**: We recommend Visual Studio 2017 on Windows, and any recent version of Clang/GCC on macOS/Linux.
- **Python 3.6+**: We recommend the latest Python 3.7.x version via the [official installer](https://www.python.org/downloads/).
- **Qt 5.12+**: We recommend the latest Qt 5.12.x version via the [official installer](https://www.qt.io/download-qt-installer).
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

(Manually installed: Git, CMake, Visual Studio, Python, Qt)

```
git clone https://github.com/vgc/vgc.git
mkdir build && cd build
cmake ..\vgc ^
    -G "Visual Studio 15 2017" -A x64 ^
    -DPython="%UserProfile%\AppData\Local\Programs\Python\Python37" ^
    -DQt="C:\Qt\5.12.6\msvc2017_64"
make
\Release\bin\vgcillustration.exe
```

#### macOS 10.14, Xcode 10.3 (=> Clang 10.0.1), Python 3.7, Qt 5.12.6

(Manually installed: Xcode, Python, Qt)

```
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

(Manually installed: Qt)

```
sudo apt install git cmake build-essential python3-dev libglu1-mesa-dev
git clone https://github.com/vgc/vgc.git
mkdir build && cd build
cmake ../vgc \
    -DCMAKE_BUILD_TYPE=Release \
    -DQt="~/Qt/5.12.6/gcc_64"
make
./Release/bin/vgcillustration
```
