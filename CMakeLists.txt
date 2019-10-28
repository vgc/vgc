#
# Build instructions
# ==================
#
# ## On Windows
#
# git clone https://github.com/vgc/vgc.git
# mkdir build
# cd build
# cmake ../vgc
#     -G "Visual Studio 15 2017" -A x64
#     -DPython="C:/Users/Boris/AppData/Local/Programs/Python/Python37"
#     -DQt="C:/Qt/5.12.3/msvc2017_64"
#     -DWiX="C:/Program Files (x86)/WiX Toolset v3.11"
# make
# test [-VV]
# [make deploy]
#
# Note: Wix is only necessary if you plan to deploy the application, that is,
# create a Windows installer.
#
# Note 2: `make` and `test` refer to batch scripts `make.bat` and `test.bat`,
# which CMake copies from tools/windows to the build directory.
#
# ## On Linux
#
# git clone https://github.com/vgc/vgc.git
# mkdir build
# cd build
# cmake ../vgc
#     -DQt="~/Qt/5.12.3/gcc_64"
# make
# ctest [-VV]
# [make deploy]
#
# Branching model
# ===============
#
# We follow a trunk-based [1] branching model that looks like:
#
#                     __ 2020.0    __ 2020.1           <- "stable"
#                ____/____________/___________ 2020    <- "beta"
#               /          /
#              /          / <- cherry-pick
# ____________/__________/____________________ master  <- "alpha"
#  \______/      \______/
#    gh62          gh63
#
# [1] https://trunkbaseddevelopment.com/
#
# Branches like 2020 are called "beta branches", and branches like 2020.0 are
# called "stable branches". Both are specific cases of "release branches".
#
# New features and bug fixes are developed via topic branches off the master
# branch (e.g., "gh62", referring to the issue #62 on GitHub). If we want to
# fix a bug affecting 2020.0, we first fix it in master, then manually
# cherry-pick the fix to 2020. In rare cases, if the bug isn't reproducible in
# master (e.g., due to significant refactoring or removed feature), it is
# allowed to fix a bug directy off a beta branch, not merging back to master.
#
# In fact, unlike in the "GitFlow" branching model, we never merge back or
# cherry-pick changes from release branches to master. We only go the other
# way around: from master to release:
#
#   GitFlow:      release --> master  (merge or cherry-pick)
#   trunk-based:  master  --> release (cherry-pick)
#
# The rationale is that any merge or cherry-pick introduces a small risk of
# (possibly undetected) problems, and we prefer these problems to only affect
# a single release branch rather than all future versions. Philosophically,
# one can view this approach as prioritizing long-term quality over short-term
# quality. Practically, this has many advantages:
# - Conceptually simpler branching model.
# - Makes integrations between master and release branches explicit
#   (and as we know, "explicit is better than implicit").
# - Prevents merge hell.
# - Allows to set VGC_VERSION_* after creating a release branch without
#   worrying that this change could be merged back to master.
# - Allows to fix bugs differently in master and release branches in case
#   these branches have diverged too much.
# - Other advantages well documented in [1].
#

cmake_minimum_required(VERSION 3.1.0)
project(vgc)

# Manufacturer, suite, and commit repository. Customize this if you are
# planning to distribute modified versions of VGC. Please note that "VGC" is a
# registered trademark of VGC Software.
#
# More details:
#
# - It is obviously okay to mention that your derivated work is *based* on
#   VGC, for example in the About dialog, or in your webpage. In fact, the
#   Apache 2.0 license requires you to credit us. Just don't mislead users
#   into thinking that your derivated work is an official version of VGC.
#
# - None of the above applies to minor tweaks you may have to do in order to
#   compile/package VGC for your favorite Linux or BSD open-source distro.
#   Obviously, if you take the time to make VGC easily installable via "apt
#   install vgc" or similar, we're super thankful, and obviously, you can keep
#   calling it VGC :)
#
set(VGC_MANUFACTURER "VGC Software" CACHE STRING "Customize this if you distribute modified versions of VGC.")
set(VGC_SUITE "VGC" CACHE STRING "Customize this if you distribute modified versions of VGC.")
set(VGC_COMMIT_REPOSITORY "https://github.com/vgc/vgc" CACHE STRING "Customize this if you distribute modified versions of VGC.")

# Commit branch. By default, the current commit branch is automatically
# detected at build time and written to resources/core/version.txt via the
# following command:
#
#   git rev-parse --abbrev-ref HEAD
#
# However, you can override this automatic detection by manually specifying a
# non-empty string to VGC_COMMIT_BRANCH. This is useful if you are in a
# detached HEAD state, in which case the concept of branch is ambiguous and
# the above command would return "HEAD". For example, in Travis, one may
# use TRAVIS_BRANCH or TRAVIS_PULL_REQUEST_BRANCH, documented here:
#
#   https://docs.travis-ci.com/user/environment-variables/
#
# Note that automatic detection of the branch happens at build time, since the
# current branch may change from build to build without CMake being re-run.
# This means that you can't know the automatically detected branch in this
# CMakeLists. In particular, the CMake variable VGC_COMMIT_BRANCH stays empty
# unless you manually specify it. If you need to known the automatically
# detected branch, you must read and parse resources/core/version.txt at build
# time.
#
# See ./tools/version.py for more info.
#
set(VGC_COMMIT_BRANCH "" CACHE STRING "Commit branch. Leave empty for automatic detection.")

# Major version. Empty for alpha version. This should be bumped from "" to
# something like "2020" when creating a beta branch.
#
set(VGC_VERSION_MAJOR "")

# Minor version. Empty for alpha and beta versions. This should be bumped from
# "" to something like "0" when creating a stable branch.
#
set(VGC_VERSION_MINOR "")

# Version type. Currently, this is determined from version major and minor,
# but maybe in the future we may want to decorellate those if we change our
# branching model.
#
if(VGC_VERSION_MAJOR STREQUAL "")
    set(VGC_VERSION_TYPE "alpha")
elseif(VGC_VERSION_MINOR STREQUAL "")
    set(VGC_VERSION_TYPE "beta")
else()
    set(VGC_VERSION_TYPE "stable")
endif()

# Target architecture. Currently, we only support x86_64.
# On Windows, this must be set by passing "-A x64" to the CMake command.
# On other platforms, it is automatically set to x86_64.
#
if(WIN32)
    set(VGC_ARCHITECTURE ${CMAKE_GENERATOR_PLATFORM} CACHE STRING "Target architecture. Note: you MUST explicitly pass -A x64 to CMake.")
else()
    set(VGC_ARCHITECTURE x86_64 CACHE STRING "Target architecture. We only support x86_64 for now.")
endif()

# Always generate unit tests
enable_testing()

# Have "make check" run the unit tests in --verbose mode.
# This is more useful than the built-in "make test" which would run ctest
# without --output-on-failure or --verbose, that is, with no indications
# of why a test failed, or details from the Python's unittest module about
# which tests were run.
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --verbose)

# Import helper CMake functions
include(tools/VgcTools.cmake)

# Configure VGC
#
# XXX TODO Implement CMake macros to ease the definition of such CMake options.
#
# Note: All VGC modules (core, dom, etc.) must have the VGC_CORE_OBJECT_DEBUG
# C++ macro defined when the VGC_CORE_OBJECT_DEBUG CMake option is ON. For
# example, the following would NOT work:
#
# vgc_add_library(core
#     ...
#     COMPILE_DEFINITIONS
#         ...
#         -DVGC_CORE_OBJECT_DEBUG
# )
#
# since it would define VGC_CORE_OBJECT_DEBUG only when compiling vgc_core, but not
# when compiling the other modules that depend on it.
#
option(VGC_CORE_OBJECT_DEBUG "Enable debug mode for vgc::core::Object" OFF)
if(VGC_CORE_OBJECT_DEBUG)
     add_definitions(-DVGC_CORE_OBJECT_DEBUG)
endif()

# Use C++11
set(CMAKE_CXX_STANDARD 11)

# Find Python. The following variables are defined (with example value):
#
# PYTHONINTERP_FOUND         TRUE
# PYTHON_EXECUTABLE          C:/Users/Boris/AppData/Local/Programs/Python/Python37/python.exe
# PYTHON_VERSION_STRING      3.7.3
# PYTHON_VERSION_MAJOR       3
# PYTHON_VERSION_MINOR       7
# PYTHON_VERSION_PATCH       3
# PYTHONLIBS_FOUND           TRUE
# PYTHON_LIBRARIES           C:/Users/Boris/AppData/Local/Programs/Python/Python37/libs/python37.lib
# PYTHON_INCLUDE_DIRS        C:/Users/Boris/AppData/Local/Programs/Python/Python37/include
# PYTHONLIBS_VERSION_STRING  3.7.3
#
# Note 1: since CMake 3.12, there is a new FindPython module and the separate
# FindPythonInterp and FindPythonLibs modules are deprecated. For now, we keep
# using the deprecated modules since requiring CMake 3.12 is a bit too
# constraining (e.g., Ubuntu 18.04 ships with CMake 3.10 only). More info:
#
# https://cmake.org/cmake/help/latest/module/FindPython.html
# https://cmake.org/cmake/help/latest/module/FindPythonInterp.html
# https://cmake.org/cmake/help/latest/module/FindPythonLibs.html
#
# Note 2: pybind11 will also search for Python using its own custom module
# called FindPythonLibsNew. It will properly detect that PYTHON_EXECUTABLE
# is already defined and not attempt to search for another Python installation.
# It will define additional PYTHON_* variables useful to pybind11, but these
# variables are not available in this CMakeLists due to CMake scoping rules.
#
if(Python)
    if(WIN32)
        # Explicitly set the path to the Python executable.
        #
        # Note 1: It is important to set it as a CACHE variable otherwise
        # pybind11 fails with:
        #
        #   CMake Error at C:/Program Files/CMake/share/cmake-3.12/Modules/FindPackageHandleStandardArgs.cmake:137 (message):
        #     Could NOT find PythonInterp (missing: PYTHON_EXECUTABLE) (found suitable
        #     version "3.7.3", minimum required is "3.7.3")
        #   Call Stack (most recent call first):
        #     C:/Program Files/CMake/share/cmake-3.12/Modules/FindPackageHandleStandardArgs.cmake:378 (_FPHSA_FAILURE_MESSAGE)
        #     C:/Program Files/CMake/share/cmake-3.12/Modules/FindPythonInterp.cmake:159 (FIND_PACKAGE_HANDLE_STANDARD_ARGS)
        #     third/pybind11/tools/FindPythonLibsNew.cmake:60 (find_package)
        #     third/pybind11/tools/pybind11Tools.cmake:16 (find_package)
        #     third/pybind11/CMakeLists.txt:33 (include)
        #
        # (our own earlier call to find_package(PythonInterp) does work, though)
        #
        # We believe the reason is that find_package(PythonInterp) calls
        # find_program(PYTHON_EXECUTABLE) under the hood [1], which sets
        # PYTHON_EXECUTABLE as a CACHE FILEPATH variable [2, 3] if it doesn't
        # exist yet. It seems that FindPackageHandleStandardArgs is confused if
        # PYTHON_EXECUTABLE does indeed exist, but isn't a CACHE variable as
        # expected. For some reason, it seems less confused if it isn't a CACHE
        # variable but is at least defined in the current scope.
        #
        # [1] https://gitlab.kitware.com/cmake/cmake/blob/master/Modules/FindPythonInterp.cmake
        # [2] https://cmake.org/cmake/help/latest/command/find_program.html
        # [3] https://gitlab.kitware.com/cmake/cmake/blob/master/Source/cmFindProgramCommand.cxx
        #
        # Note 2: We also experimented with using PythonInterp_ROOT instead of
        # the more explicit PYTHON_EXECUTABLE. This is a new feature of
        # find_program() since CMake 3.12, which requires to enable the
        # CMP0074 policy, like so:
        #
        #   if (POLICY CMP0074)
        #       cmake_policy(SET CMP0074 NEW)
        #       set(PythonInterp_ROOT ${Python})
        #   endif()
        #
        # Unfortunately, for some reason, the policy does not propagate to
        # pybind11 (like I think it should [4]), and causes the following
        # warning in pybind11:
        #
        # [4] https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/Policies#policy-stack
        #
        #   CMake Warning (dev) at third/pybind11/tools/FindPythonLibsNew.cmake:60 (find_package):
        #     Policy CMP0074 is not set: find_package uses <PackageName>_ROOT variables.
        #     Run "cmake --help-policy CMP0074" for policy details.  Use the cmake_policy
        #     command to set the policy and suppress this warning.
        #     CMake variable PythonInterp_ROOT is set to:
        #       C:/Users/Boris/AppData/Local/Programs/Python/Python37
        #     For compatibility, CMake is ignoring the variable.
        #   Call Stack (most recent call first):
        #     third/pybind11/tools/pybind11Tools.cmake:16 (find_package)
        #     third/pybind11/CMakeLists.txt:33 (include)
        #   This warning is for project developers.  Use -Wno-dev to suppress it.
        #
        # In order to avoid this warning, and since <PackageName>_ROOT is only
        # available since CMake 3.12, we prefer using the PYTHON_EXECUTABLE
        # approach.
        #
        set(PYTHON_EXECUTABLE ${Python}/python.exe CACHE FILEPATH "Path to the Python executable")
    endif()
endif()
find_package(PythonInterp 3.6 REQUIRED)
find_package(PythonLibs 3.6 REQUIRED)

# Find, configure, and include Qt. The recommended way to make sure that the
# desired version of Qt is found is to pass -DQt to cmake, e.g.:
#
#   cmake -DQt="C:/Qt5.12.3/5.12.3/msvc2017_64"
#
# Note: The Qt documentation also recommends to set the following CMake
# variable:
#
#   set(CMAKE_INCLUDE_CURRENT_DIR ON)
#
# However, this has proven to lead to naming conflicts. More specifically, the
# header file vgc/core/math.h includes <cmath> which includes <math.h>. If
# CMAKE_INCLUDE_CURRENT_DIR is ON, then <math.h> resolves to vgc/core/math.h
# instead of the C standard library header, leading to compilation errors.
# Therefore, we do not set CMAKE_INCLUDE_CURRENT_DIR to ON, forcing libraries
# to use absolute paths, which is better practice anyway.
#
# Here are useful cmake variables that can be queried (with example value):
#   QT_MOC_EXECUTABLE=/home/user/Qt/5.12.0/gcc_64/bin/moc
#   QT_QMAKE_EXECUTABLE=/home/user/Qt/5.12.0/gcc_64/bin/qmake
#   Qt5_DIR=/home/user/Qt/5.12.0/gcc_64/lib/cmake/Qt5
#   Qt5_VERSION=5.12.0
#   Qt5_VERSION_COUNT=3
#   Qt5_VERSION_MAJOR=5
#   Qt5_VERSION_MINOR=12
#   Qt5_VERSION_PATCH=0
#   Qt5_VERSION_TWEAK=0
#
if (Qt)
    set(Qt5_DIR ${Qt}/lib/cmake/Qt5)
endif()
set(CMAKE_AUTOMOC ON)
find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)
get_filename_component(Qt ${Qt5_DIR}/../../.. ABSOLUTE)
message("-- Found Qt ${Qt5_VERSION}: ${Qt}")

# Disable usage of emit/signals/slots because they collide with <Python.h>.
# Instead, we use the more explicit Q_EMIT/Q_SIGNALS/Q_SLOTS.
add_definitions(-DQT_NO_KEYWORDS)

# Explicitly include Qt directories. According to the Qt doc, this is redundant
# with find_package(Qt5). In practice, we need this for python wrappers.
# See https://stackoverflow.com/questions/29613423/cmake-include-files-not-found-with-object
include_directories(${Qt5Core_INCLUDE_DIRS})
include_directories(${Qt5Gui_INCLUDE_DIRS})
include_directories(${Qt5Widgets_INCLUDE_DIRS})

# Add third-party shipped libraries.
add_subdirectory(third)

# Include Python
include_directories(${PYTHON_INCLUDE_DIRS})

# Include third-party shipped libs
include_directories(${CMAKE_CURRENT_LIST_DIR}/third/Eigen/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/third/pybind11/include)

# Include internal libs
include_directories(${CMAKE_CURRENT_LIST_DIR}/libs)

# Add internal libs and apps
add_subdirectory(libs)
add_subdirectory(apps)

# Generate the file resources/core/version.txt with versioning info.
# See vgc/tools/version.py for details.
#
find_package(Git)
add_custom_target(version ALL
    VERBATIM
    COMMAND ${PYTHON_EXECUTABLE}
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/version.py"
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${GIT_EXECUTABLE}"
        "${VGC_MANUFACTURER}"
        "${VGC_SUITE}"
        "${VGC_VERSION_TYPE}"
        "${VGC_VERSION_MAJOR}"
        "${VGC_VERSION_MINOR}"
        "${VGC_COMMIT_REPOSITORY}"
        "${VGC_COMMIT_BRANCH}"
        "${CMAKE_CXX_COMPILER_ID}"
        "${CMAKE_CXX_COMPILER_VERSION}"
        "${VGC_ARCHITECTURE}"
        "$<CONFIG>"
)

# On Windows, create a convenient batch script called 'make.bat' in the build
# folder, which allows you to run the following command in cmd.exe:
#
# make [targetname]
#
# as syntactic sugar for running the following command in the appropriate Visual
# Studio command prompt:
#
# cmake --build . --target [targetname] --config Release
#
# Note: the reason we automatically setup a Visual Studio environment is that
# windeployqt requires it to find the appropriate dependencies.
#
if(WIN32)

    # Set which configuration (Debug, Release, MinSizeRel, or RelWithDebInfo)
    # should be used when calling make.bat.
    #
    set(DefaultConfig Release)

    # Create make.bat.
    #
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/windows/make.in.bat
        ${CMAKE_BINARY_DIR}/make.bat)

    # Create test.bat.
    #
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/windows/test.in.bat
        ${CMAKE_BINARY_DIR}/test.bat)
endif()

# Copy Python dependencies
#
if(WIN32)
    add_custom_target(copy_python ALL
        VERBATIM
        COMMAND ${PYTHON_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/windows/copy_python.py"
            "${CMAKE_CURRENT_BINARY_DIR}"
            "$<CONFIG>"
    )
endif()

# Add deploy target.
#
if(WIN32)
    # On Windows, we assume that windeployqt has already been run (since we need
    # this for running the app, even without deploying it), and that the
    # bin/vc_redist.x64.exe output was moved from bin/ to deploy/. In the
    # future, we may want to directly find the vc_redist without relying on
    # windeployqt, which may be useful for deploying a subset of VGC that does
    # not depend on Qt.
    #
    add_custom_target(deploy
        VERBATIM
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND powershell -NoLogo -NoProfile -Command \(
            Get-Item ${CMAKE_BINARY_DIR}/deploy/$<CONFIG>/vc_redist.x64.exe
            \).VersionInfo.FileVersion
            > ${CMAKE_BINARY_DIR}/deploy/$<CONFIG>/vc_redist.x64.exe.version
        COMMAND ${PYTHON_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/windows/wix.py"
            "${CMAKE_CURRENT_SOURCE_DIR}"
            "${CMAKE_CURRENT_BINARY_DIR}"
            "$<CONFIG>"
            "${WiX}"
    )
elseif(APPLE)
    add_custom_target(deploy
        VERBATIM
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${PYTHON_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/macos/deploy.py"
            "${CMAKE_CURRENT_SOURCE_DIR}"
            "${CMAKE_CURRENT_BINARY_DIR}"
            "$<CONFIG>"
            "${Qt}"
    )
else()
    add_custom_target(deploy
        VERBATIM
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${PYTHON_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/linux/deploy.py"
            "${CMAKE_CURRENT_SOURCE_DIR}"
            "${CMAKE_CURRENT_BINARY_DIR}"
            "$<CONFIG>"
            "${Qt}"
    )
endif()
