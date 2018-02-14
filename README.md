![VGC](https://github.com/vgc/vgc/blob/master/logo.png)

# About Me

My name is [Boris Dalstein](http://www.borisdalstein.com/), I am a French
mathematician, computer scientist, and software engineer, but most importantly,
I am an *animation lover*. I hold a PhD in Computer Science from
[UBC](https://www.ubc.ca/), and I have worked at various companies and research
institutes such as [Inria](https://www.inria.fr/), [Disney
Research](https://www.disneyresearch.com/labs/), and more recently [Pixar
Animation Studios](https://www.pixar.com/).

# About VGC

VGC is a startup I founded in October 2017 to develop commercial (yet
open-source!) next-generation tools for graphic design and 2D animation. Visit
[www.vgc.io](https://www.vgc.io) for more details. Currently, this project is
entirely funded by voluntary donations from awesome people like yourself, many
of whom are open-source software enthusiasts. The best way to help this project
reach maturity is to support me on
[Tipeee](https://www.tipeee.com/borisdalstein) (preferred), or
[Patreon](https://www.patreon.com/borisdalstein) (preferred for US residents only).

# Products

I am planning to release two products in July 2020:

![VGC](https://github.com/vgc/vgc/blob/master/products.png)

VGC Illustration will be a vector graphics editor, that is, a competitor of
products such as Adobe Illustrator, Autodesk Graphic, CorelDRAW, or Inkscape.
The difference is that unlike these existing programs, VGC Illustration will be
based on vector graphics complexes, a technology that we developed and presented
at SIGGRAPH 2014.

VGC Animation will be a vector-based 2D animation system, that is, a competitor
of products such as Adobe Animate (formerly Adobe/Macromedia Flash), ToonBoom
Harmony, CACANi, Synfig, or OpenToonz. To some extent, VGC Animation will also
be a competitor to raster-based 2D animation systems such as TVPaint or Krita.
VGC Animation will be based on vector animation complexes, an extension of
vector graphics complexes that supports animation, presented at SIGGRAPH 2015.

# Licensing

Starting July 2020, VGC Illustration and VGC Animation will be released yearly
(example: VGC Animation 2020, VGC Animation 2021, etc.), under a commercial
open-source model.

A license key for VGC Illustration 2020 will cost **$39**, while a license key for
VGC Animation 2020 will cost **$79**. Each license key allows a single user to use
the software on any number of devices, on any number of platforms (Windows,
MacOS X, and Linux). License keys are perpetual, that is, they never expire.
This means that you will still be able to use VGC Illustration 2020 five years
later without paying any extra fee. However, you will need to pay for an upgrade
if you want to use newer versions. The cost of the upgrade from one year to the
next will be **$19** for VGC Illustration, and **$39** for VGC Animation.

Despite being distributed commercially, both apps will in fact be open-source
and publicly developed here on this git repository under the [Apache
2.0](https://github.com/vgc/vgc/blob/master/README.md) license. Also, no
license key will be required to use the software on Linux, that is, the software
will be completely **free of charge for Linux users**. This is my way to say thank
you and give back the open-source community (my work relies heavily on
open-source software), and also a way to encourage more users to try Linux.

If you wish, you can obtain early license keys by supporting me on
[Tipeee](https://www.tipeee.com/borisdalstein) or
[Patreon](https://www.patreon.com/borisdalstein), starting at $3 per month. You
can stop your donations at any time and keep your license keys, although the
idea behind Tipeee/Patreon is to have a stable monthly revenue, which is
critical in this early development stage.

# About this Git Repository

This is the main git repository where all software development happens. At the
moment, development is at a very early planning stage with no consideration for
backward compatibility and subject to frequent and significant refactoring
without notice. For this reason, I do not really recommend contributing code,
but of course any contribution may be discussed and considered on a case-by-case
basis. I'm more than happy to hear any advice or comments you may have, if you
happen to be an expert in relevant areas, just submit an "issue" here.

External contributions will be more than welcome starting January 2019, when the
software architecture is expected to be more stable.

# Dependencies

- C++11
- CMake 3.1.0+
- Qt 5.6+
- Python 3.5+

It is recommended to use Qt 5.6 instead of newer version due to PySide2
not properly supporting Qt 5.7+ at the moment. Currently, C++14 support is
not required, but it is likely to be required in the near future.

VGC loosely follows the [VFX Reference Platform](http://www.vfxplatform.com/)
recommendations for library versions, but since VGC apps are only to be
released in 2020, we already jumped ahead to Python 3.x.

VGC also depends on the following third-party libraries, but these are shipped
and installed alongside this repository (="vendored") so you don't need to have
them already installed your system:
- pybind11
- Eigen

# Build Instructions

Tested platform: Ubuntu 16.04 LTS

Install GCC, CMake, and Python:

```
~$ sudo apt-get install build-essential
~$ sudo apt-get install python3-dev
```

Install Qt if you don't have it yet:

1. Download [Qt 5.6.3](http://download.qt.io/official_releases/qt/5.6/5.6.3/qt-opensource-linux-x64-5.6.3.run).

2. Execute the binary and follow the instructions to install Qt at the desired
   location (e.g., ~/Qt5.6.3). You only need to install the components
   "Desktop gcc 64-bit", "Qt Source Package", and "Tools", but these are the
   largest components anyway (2.02GiB), so I suggest just installing
   everything (2.31GiB).

3. Tell cmake where to find Qt5. You can either do it once and for all by adding the path to CMAKE_PREFIX_PATH:
   ```
   echo 'export CMAKE_PREFIX_PATH="$HOME/Qt5.6.3/5.6.3/gcc_64/lib/cmake/Qt5:$CMAKE_PREFIX_PATH"' >> ~/.bashrc
   source ~/.bashrc
   ```
   Or you can pass the argument `-DQt5_DIR=~/Qt5.6.3/5.6.3/gcc_64/lib/cmake/Qt5`
   when you run cmake for the first time in a given directory.

Get VGC:

```
git clone https://github.com/vgc/vgc.git
```

Build VGC:
```
mkdir build-vgc
cd build-vgc
cmake ../vgc          # Optional: -DQt5_DIR=~/Qt5.6.3/5.6.3/gcc_64/lib/cmake/Qt5
make
```

Run VGC:
```
./bin/vgcillustration
```
