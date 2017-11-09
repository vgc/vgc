cmake_minimum_required(VERSION 3.1.0)
project(vgc)

# Import our helper functions defined in cmake/VgcTools.cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(VgcTools)

# Use C++11
set(CMAKE_CXX_STANDARD 11)

# Find, configure, and include Qt.
set(CMAKE_INCLUDE_CURRENT_DIR ON)
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
include_directories("${CMAKE_CURRENT_LIST_DIR}/third")
include_directories("${CMAKE_CURRENT_LIST_DIR}/third/pybind11/include")

# Include internal libs
include_directories("${CMAKE_CURRENT_LIST_DIR}/libs")

# Add internal libs and apps
add_subdirectory(libs)
add_subdirectory(apps)
