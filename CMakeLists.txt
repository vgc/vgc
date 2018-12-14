cmake_minimum_required(VERSION 3.1.0)
project(vgc)

# Always generate unit tests
enable_testing()

# Have "make check" run the unit tests in --verbose mode.
# This is more useful than the built-in "make test" which would run ctest
# without --output-on-failure or --verbose, that is, with no indications
# of why a test failed, or details from the Python's unittest module about
# which tests were run.
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --verbose)

# Import our helper functions defined in cmake/VgcTools.cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(VgcTools)

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

# Find, configure, and include Qt.
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
set(CMAKE_AUTOMOC ON)
find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)

# Disable usage of emit/signals/slots because they collide with <Python.h>.
# Instead, we use the more explicit Q_EMIT/Q_SIGNALS/Q_SLOTS.
add_definitions(-DQT_NO_KEYWORDS)

# Explicitly include Qt directories. According to the Qt doc, this is redundant
# with find_package(Qt5). In practice, we need this for python wrappers.
# See https://stackoverflow.com/questions/29613423/cmake-include-files-not-found-with-object
include_directories(${Qt5Core_INCLUDE_DIRS})
include_directories(${Qt5Gui_INCLUDE_DIRS})
include_directories(${Qt5Widgets_INCLUDE_DIRS})

# Add third-party shipped libraries. In particular, this adds pybind11 which
# takes care of finding and configuring Python.
add_subdirectory(third)

# Include Python
include_directories("${PYTHON_INCLUDE_DIRS}")

# Include third-party shipped libs
include_directories("${CMAKE_CURRENT_LIST_DIR}/third/Eigen/include")
include_directories("${CMAKE_CURRENT_LIST_DIR}/third/pybind11/include")

# Include internal libs
include_directories("${CMAKE_CURRENT_LIST_DIR}/libs")

# Add internal libs and apps
add_subdirectory(libs)
add_subdirectory(apps)

# Run "make linuxdeploy" to create an AppImage
add_custom_target(linuxdeploy
    COMMAND
        "${CMAKE_CURRENT_LIST_DIR}/tools/linuxdeploy.sh"
        "${CMAKE_BINARY_DIR}"
        "${CMAKE_CURRENT_LIST_DIR}"
        "${QT_QMAKE_EXECUTABLE}"
    WORKING_DIRECTORY
        "${CMAKE_BINARY_DIR}"
)
