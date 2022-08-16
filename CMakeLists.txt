# Required:
# - CMake 3.10: Freetype::Freetype target
# - CMake 3.12: New version of FindPython
#
# Useful:
# - CMake 3.15: FindPython gives higher priority to Python_ROOT_DIR
# - CMake 3.16: Capability to specify which Python to use with Python_EXECUTABLE
#
# Nice overview of important features introduced by each version of CMake:
# - https://cliutils.gitlab.io/modern-cmake/chapters/intro/newcmake.html
#
cmake_minimum_required(VERSION 3.12...3.22)

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

# Commit branch. This value is written to resources/core/version.txt and used
# to generate appropriate metadata and program name. See ./tools/version.py for
# more info.
#
# By default, the current commit branch is automatically detected at build time
# via the following command:
#
#   git rev-parse --abbrev-ref HEAD
#
# Unfortunately, the command above doesn't work well when you're in a detached
# HEAD state, which is often the case in CI builds. In this case, you can
# manually specify the branch by setting an explicit value for
# VGC_COMMIT_BRANCH. See for example the various GitHub Actions files in
# .github/workflows
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

# Target architecture. Currently, we only support x64, and this value is hard
# coded. In the future, we may want to support more architectures (e.g., arm64).
#
# TODO: Perform more checks to actually verify that we're compiling to x64.
# 
if(WIN32)
    set(VGC_ARCHITECTURE x64 CACHE STRING "Target architecture. We only support x64 for now.")
    if(CMAKE_GENERATOR MATCHES "Visual Studio")
        if (NOT ${CMAKE_GENERATOR_PLATFORM} MATCHES "x64")
            message(FATAL_ERROR "Detected incorrect architecture: '${CMAKE_GENERATOR_PLATFORM}'. Did you forget '-A x64'?")
        endif()
    endif()
else()
    set(VGC_ARCHITECTURE x86_64 CACHE STRING "Target architecture. We only support x86_64 for now.")
endif()

# List of private compile definitions. These are applied when compiling VGC
# libraries, but not transitively applied for libraries linking to VGC.
#
set(VGC_PRIVATE_COMPILE_DEFINITIONS "")

# Compiler warning flags.
#
option(VGC_PEDANTIC "Enable extra warnings" OFF)
option(VGC_WERROR "Treat compiler warnings as errors" OFF)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(VGC_COMPILER_WARNING_FLAGS "")
    set(VGC_COMPILER_WARNING_DEFINITIONS "")
    set(VGC_PEDANTIC_COMPILE_FLAGS -Wall -Wextra)
    set(VGC_WERROR_FLAG -Werror)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(VGC_COMPILER_WARNING_FLAGS "")
    set(VGC_COMPILER_WARNING_DEFINITIONS "")
    set(VGC_PEDANTIC_COMPILE_FLAGS -Wall -Wextra)
    set(VGC_WERROR_FLAG -Werror)
elseif(MSVC)
    # /wd4251: class 'Foo' needs to have dll-interface to be used by clients of class 'Bar'
    #          https://github.com/vgc/vgc/issues/377
    set(VGC_COMPILER_WARNING_FLAGS /wd4251)
    # C4996: 'strerror'/'getenv'/etc: This function or variable may be unsafe. Consider using 'strerror_s'/_dupenv_s'/etc instead.
    set(VGC_COMPILER_WARNING_DEFINITIONS _CRT_SECURE_NO_WARNINGS)
    set(VGC_PEDANTIC_COMPILE_FLAGS /W4 /w45038)
    set(VGC_WERROR_FLAG /WX)
endif ()

# Import helper CMake functions
include(tools/VgcTools.cmake)

# Use C++17
set(CMAKE_CXX_STANDARD 17)

# Tell CMake where to find our FindFoobar.cmake modules.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/tools")

# Find Python. The recommended way to specify a Python installation is to use
#
#   cmake .. -DPython_ROOT_DIR="%UserProfile%\AppData\Local\Programs\Python\Python37"
#
# If you use CMake < 3.15, be aware that the above isn't very reliable, and CMake
# might prefer a higher version of Python rather than the one explicitly provided.
# The only solution is to upgrade your CMake version.
#
# Starting with CMake 3.16, you can also choose to explicitly point to the desired
# Python executable with -DPython_EXECUTABLE. You can try this if -DPython_ROOT_DIR
# doesn't work as expected.
#
message(STATUS "") # newline
message(STATUS "[Python]")
find_package(Python 3.6 COMPONENTS Interpreter Development REQUIRED)
execute_process(COMMAND ${Python_EXECUTABLE} -c "import sys; print(sys.prefix, end='')" OUTPUT_VARIABLE VGC_PYTHON_PREFIX)
execute_process(COMMAND ${Python_EXECUTABLE} -c "import sys; print(sys.exec_prefix, end='')" OUTPUT_VARIABLE VGC_PYTHON_EXEC_PREFIX)
if(VGC_PYTHON_PREFIX STREQUAL VGC_PYTHON_EXEC_PREFIX)
    set(VGC_PYTHON_HOME ${VGC_PYTHON_PREFIX})
else()
    set(VGC_PYTHON_HOME ${VGC_PYTHON_PREFIX}:${VGC_PYTHON_EXEC_PREFIX})
endif()
message(STATUS "Version: ${Python_VERSION}")
message(STATUS "Python Prefix:        ${VGC_PYTHON_PREFIX}")
message(STATUS "Python Exec Prefix:   ${VGC_PYTHON_EXEC_PREFIX}")
message(STATUS "Python Home:          ${VGC_PYTHON_HOME}")
message(STATUS "Python Executable:    ${Python_EXECUTABLE}")
message(STATUS "Python Libraries:     ${Python_LIBRARIES}")
message(STATUS "Python Library Dirs:  ${Python_LIBRARY_DIRS}")
message(STATUS "Python Include Dirs:  ${Python_INCLUDE_DIRS}")
message(STATUS "") # newline

# Always generate unit tests
enable_testing()

# Define a vgc_tests target that builds all tests (in particular, C++ tests)
add_custom_target(vgc_tests)
set_target_properties(vgc_tests PROPERTIES FOLDER misc)

# Define a `check` target that builds and run all tests.
# Unlike `make test`, running `make check` builds the tests and all its
# dependencies so you don't need to run `make` beforehand. Also, unlike
# `make test`, which doesn't print what went wrong after a test fails, we
# use --output-on-failure for `make check` which is much more useful.
#
if (CMAKE_CONFIGURATION_TYPES)
    add_custom_target(check
        COMMAND ${CMAKE_CTEST_COMMAND}
            --force-new-ctest-process --output-on-failure
            --build-config "$<CONFIGURATION>"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
else()
    add_custom_target(check
        COMMAND ${CMAKE_CTEST_COMMAND}
            --force-new-ctest-process --output-on-failure
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
endif()
add_dependencies(check vgc_tests)
set_target_properties(check PROPERTIES FOLDER misc)

# Set "third" as default FOLDER target property
set(VGC_CMAKE_FOLDER_OLD ${CMAKE_FOLDER})
set(CMAKE_FOLDER third)

# Google Test.
#
# Note: this is why we need gtest_force_shared_crt ON:
# https://github.com/vgc/vgc/pull/295
#
# Note: it is important to add Google Test after Python, because Google test
# also adds Python but may find the wrong version.
#
message(STATUS "[Google Test]")
set(GoogleTest ${CMAKE_CURRENT_LIST_DIR}/third/googletest CACHE FILEPATH "Path to the root of the Google Test distribution")
if (EXISTS ${GoogleTest})
    message(STATUS "Found Google Test: ${GoogleTest}")
    file(READ ${GoogleTest}/CMakeLists.txt gtestLists)
    if (gtestLists MATCHES "GOOGLETEST_VERSION ([0-9]+\.[0-9]+\.[0-9]+)")
        message(STATUS "Version: ${CMAKE_MATCH_1}")
    else()
       message(STATUS "Version: Unknown")
    endif()
    set(gtest_force_shared_crt ON CACHE BOOL
      "Use shared (DLL) run-time lib even when Google Test is built as static lib.")
    add_subdirectory(${GoogleTest})
else()
    message(WARNING "Can't find Google Test")
endif()
message(STATUS "") # newline

# Find, configure, and include Qt. The recommended way to make sure that the
# desired version of Qt is found is to pass -DQt5_DIR to cmake, e.g.:
#
#   cmake .. -DQt5_DIR="C:\Qt\5.15.2\msvc2019_64\lib\cmake\Qt5"
#
# Note that we add -DQT_NO_KEYWORDS as a private compile definition to all
# VGC libs, which forces them to use Q_EMIT/Q_SIGNALS/Q_SLOTS, but doesn't
# force their clients to do the same.
#
# If necessary, note that it is also possible to include private Qt headers,
# for example:
#
#  vgc/CMakeLists.txt:
#    find_package(Qt5Core COMPONENTS Private)
#    set(VGC_QT_CORE_PRIVATE Qt5::CorePrivate)
#
#  libs/vgc/graphics/CMakeLists.txt:
#    vgc_add_library(graphics
#         THIRD_DEPENDENCIES
#             ${VGC_QT_CORE}
#             ${VGC_QT_CORE_PRIVATE}
#
#  libs/vgc/graphics/text.cpp:
#    #include <QtCore/private/qunicodetools_p.h>
#
set(VGC_QT_VERSION "" CACHE STRING "Which Qt major version to use (latest by default)")
message(STATUS "[Qt]")
if(NOT VGC_QT_VERSION)
    find_package(Qt6 COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets QUIET)
    if(NOT Qt6_FOUND OR Qt6_DIR EQUAL "")
        find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)
    endif()
elseif(VGC_QT_VERSION EQUAL 6)
    find_package(Qt6 COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets REQUIRED)
elseif(VGC_QT_VERSION EQUAL 5)
    find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)
else()
    message(FATAL_ERROR "Given VGC_QT_VERSION (${VGC_QT_VERSION}) is not supported.")
endif()

if(Qt5_FOUND)
    list(APPEND VGC_PRIVATE_COMPILE_DEFINITIONS "QT_NO_KEYWORDS")
    get_filename_component(VGC_QT_ROOT ${Qt5_DIR}/../../.. ABSOLUTE)
    message(STATUS "Found Qt5: ${VGC_QT_ROOT}")
    message(STATUS "Version: ${Qt5_VERSION}")
    set(VGC_QT_CORE Qt5::Core)
    set(VGC_QT_GUI Qt5::Gui)
    set(VGC_QT_WIDGETS Qt5::Widgets)
    set(VGC_QT_OPENGL "")
    set(VGC_QT_OPENGLWIDGETS "")
elseif(Qt6_FOUND)
    list(APPEND VGC_PRIVATE_COMPILE_DEFINITIONS "QT_NO_KEYWORDS")
    get_filename_component(VGC_QT_ROOT ${Qt6_DIR}/../../.. ABSOLUTE)
    message(STATUS "Found Qt6: ${VGC_QT_ROOT}")
    message(STATUS "Version: ${Qt6_VERSION}")
    set(VGC_QT_CORE Qt::Core)
    set(VGC_QT_GUI Qt::Gui)
    set(VGC_QT_WIDGETS Qt::Widgets)
    set(VGC_QT_OPENGL Qt::OpenGL)
    set(VGC_QT_OPENGLWIDGETS Qt::OpenGLWidgets)
else()
    message(FATAL_ERROR "Qt not found")
endif()
message(STATUS "") # newline

# pybind11
message(STATUS "[pybind11]")
set(pybind11_DIR ${CMAKE_CURRENT_LIST_DIR}/third/pybind11 CACHE FILEPATH "Path to the root of the pybind11 distribution")
add_subdirectory(${pybind11_DIR})
if(${pybind11_FOUND})
    message(STATUS "Found pybind11: ${pybind11_DIR}")
else()
    message(SEND_ERROR "Can't find pybind11")
endif()
message(STATUS "") # newline

# {fmt}
#
# The CMakeLists of {fmt} provides two targets:
# - fmt::fmt              Static lib by default, or shared lib if BUILD_SHARED_LIBS=ON
# - fmt::fmt-header-only  Header-only version
#
# Which method we use can be configured with the following CMake variable:
#
#     VGC_FMT_LIB_TYPE = header-only | shared | static
#
# We measured no significant performance difference with either method. As of
# 2020-02-12, total code size (vgc libs + libfmt.so if shared) was smaller
# when using header-only, so it would make sense to use this method, as it
# also simplifies deployment. However, as we start using {fmt} more, total
# code size is expected to become smaller by compiling {fmt} as a shared
# library. Also, using a shared library will almost certainly decrease compile
# time, and on Windows it avoids having <windows.h> included in all
# translation units (there is a configurable FMT_USE_WINDOWS_H flag which we
# can set to 0, but keeping the Windows-specific features could be useful, and
# besides the flag doesn't seem properly supported in future versions of
# {fmt}, see: https://github.com/fmtlib/fmt/issues/1508). Therefore, we use
# shared lib as the default.
#
# Note that the lead author of {fmt} recommends in fact compiling it as a
# static lib, but since in our case we have multiple shared libs depending on
# it, it makes more sense for {fmt} to be a shared lib too. Another option
# might be to directly copy the code and compile it as part of vgc::core, but
# it would make updating the lib more complicated.
#
# Finally, note that by default, CMake builds static libs without -fPIC. This
# makes sense for libs which are later linked into a final application, but
# doesn't work in our case where we need to link them into our shared libs,
# which are themselves built with -fPIC (as usual for shared libs). This is
# why in the configuration step below, we explicitly tell CMake to use -fPIC
# when compiling {fmt} as a static lib. See:
#
# https://cmake.org/cmake/help/latest/command/add_library.html
# https://cmake.org/cmake/help/v3.0/variable/BUILD_SHARED_LIBS.html
#
message(STATUS "[{fmt}]")
set(VGC_FMT_LIB_TYPE shared CACHE STRING "How should we build/use fmt? Options: 'header-only', 'shared', 'static'")
set(FMT_DIR ${CMAKE_CURRENT_LIST_DIR}/third/fmt CACHE FILEPATH "Path to the root of the {fmt} distribution")
if(EXISTS ${FMT_DIR})
    message(STATUS "Found {fmt}: ${FMT_DIR}")
    message(STATUS "Library type: ${VGC_FMT_LIB_TYPE}")
    set(BUILD_SHARED_LIBS_OLD ${BUILD_SHARED_LIBS})
    if(VGC_FMT_LIB_TYPE STREQUAL "header-only")
        set(VGC_FMT fmt::fmt-header-only)
    else()
        set(VGC_FMT fmt::fmt)
        if(VGC_FMT_LIB_TYPE STREQUAL "shared")
            set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
        elseif(VGC_FMT_LIB_TYPE STREQUAL "static")
            if(DEFINED BUILD_SHARED_LIBS)
                set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
            endif()
        else()
            message(SEND_ERROR "Unknown library type for {fmt}.")
        endif()
    endif()
    add_subdirectory(${FMT_DIR})
    if(MSVC)
        # disable warning C4275: non dll-interface class
        # 'std::runtime_error' used as base for dll-interface class
        # 'fmt::v6::format_error' / 'fmt::v6::system_error'
        target_compile_options(fmt PRIVATE /wd4275)
        # disable warning C4251 (since fmt 8.0.0): 'fmt::v8::ostream::file_':
        # class 'fmt::v8::file' needs to have dll-interface to be used by
        # clients of class 'fmt::v8::ostream'
        # See: https://github.com/fmtlib/fmt/issues/2796
        # We may remove this if is a fix is merged for the above issue
        target_compile_options(fmt PRIVATE /wd4251)
    endif()
    if(VGC_FMT_LIB_TYPE STREQUAL "static")
        set_target_properties(fmt PROPERTIES POSITION_INDEPENDENT_CODE ON)
    elseif(VGC_FMT_LIB_TYPE STREQUAL "shared")
        set_target_properties(fmt PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/lib
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/bin)
    endif()
    if(BUILD_SHARED_LIBS_OLD STREQUAL "")
        if(DEFINED BUILD_SHARED_LIBS)
            unset(BUILD_SHARED_LIBS CACHE)
        endif()
    else()
        set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_OLD} CACHE BOOL "" FORCE)
    endif()
else()
    message(SEND_ERROR "Can't find {fmt}")
endif()
message(STATUS "") # newline

# Eigen
#
# Note: we don't use add_subdirectory(${Eigen_DIR}) because:
# - it generates an error due to their target 'check' conflicting with ours
# - it is too verbose anyway (~120 lines of output)
#
# Therfore, we manually create an INTERFACE target.
#
set(Eigen_DIR ${CMAKE_CURRENT_LIST_DIR}/third/eigen CACHE FILEPATH "Path to the root of the Eigen distribution")
message(STATUS "[Eigen]")
if (EXISTS ${Eigen_DIR}/Eigen/src/Core/util/Macros.h)
    add_library(eigen INTERFACE)
    add_library(Eigen3::Eigen ALIAS eigen)
    target_include_directories(eigen INTERFACE ${Eigen_DIR})
    message(STATUS "Found Eigen: ${Eigen_DIR}")
    file(READ "${Eigen_DIR}/Eigen/src/Core/util/Macros.h" _eigen_version_header)
    string(REGEX MATCH "define[ \t]+EIGEN_WORLD_VERSION[ \t]+([0-9]+)" _eigen_world_version_match "${_eigen_version_header}")
    set(EIGEN_WORLD_VERSION "${CMAKE_MATCH_1}")
    string(REGEX MATCH "define[ \t]+EIGEN_MAJOR_VERSION[ \t]+([0-9]+)" _eigen_major_version_match "${_eigen_version_header}")
    set(EIGEN_MAJOR_VERSION "${CMAKE_MATCH_1}")
    string(REGEX MATCH "define[ \t]+EIGEN_MINOR_VERSION[ \t]+([0-9]+)" _eigen_minor_version_match "${_eigen_version_header}")
    set(EIGEN_MINOR_VERSION "${CMAKE_MATCH_1}")
    set(EIGEN_VERSION_NUMBER )
    message(STATUS "Version: ${EIGEN_WORLD_VERSION}.${EIGEN_MAJOR_VERSION}.${EIGEN_MINOR_VERSION}")
else()
    message(SEND_ERROR "Can't find Eigen")
endif()
message(STATUS "") # newline

# libtess2
# https://github.com/memononen/libtess2
message(STATUS "[libtess2]")
set(VGC_LIBTESS2_LIB_TYPE static CACHE STRING "How should we build/use libtess2? Options: 'shared', 'static'")
set(VGC_LIBTESS2_DIR ${CMAKE_CURRENT_LIST_DIR}/third/libtess2 CACHE FILEPATH "Path to libtess2")
if(EXISTS ${VGC_LIBTESS2_DIR})
    message(STATUS "Found libtess2: ${VGC_LIBTESS2_DIR}")
    message(STATUS "Library type: ${VGC_LIBTESS2_LIB_TYPE}")
    set(BUILD_SHARED_LIBS_OLD ${BUILD_SHARED_LIBS})
    if(VGC_LIBTESS2_LIB_TYPE STREQUAL "shared")
        set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
    elseif(VGC_LIBTESS2_LIB_TYPE STREQUAL "static")
        if(DEFINED BUILD_SHARED_LIBS)
            set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
        endif()
    else()
        message(SEND_ERROR "Unknown library type for libtess2.")
    endif()
    add_subdirectory(${VGC_LIBTESS2_DIR})
    if(VGC_LIBTESS2_LIB_TYPE STREQUAL "static")
        set_target_properties(libtess2 PROPERTIES POSITION_INDEPENDENT_CODE ON)
    elseif(VGC_LIBTESS2_LIB_TYPE STREQUAL "shared")
        set_target_properties(libtess2 PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/lib
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/bin)
    endif()
    if(BUILD_SHARED_LIBS_OLD STREQUAL "")
        if(DEFINED BUILD_SHARED_LIBS)
            unset(BUILD_SHARED_LIBS CACHE)
        endif()
    else()
        set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_OLD} CACHE BOOL "" FORCE)
    endif()
    message(STATUS "Version: ${libtess2_VERSION}")
else()
    message(SEND_ERROR "Can't find libtess2")
endif()
message(STATUS "") # newline

# Git
message(STATUS "[Git]")
if (GIT_EXECUTABLE)
    # find_package won't reprint this on subsequent CMake calls, so we do it ourselves.
    message(STATUS "Found Git: ${GIT_EXECUTABLE}")
endif()
find_package(Git REQUIRED)
message(STATUS "Version: ${GIT_VERSION_STRING}")
message(STATUS "") # newline

# Freetype
message(STATUS "[FreeType]")
find_package(Freetype REQUIRED)
get_target_property(FREETYPE_INCLUDE_DIRS Freetype::Freetype INTERFACE_INCLUDE_DIRECTORIES)
if(NOT FREETYPE_VERSION_STRING)
    # Sometimes, VERSION_STRING isn't defined, so we do it ourselves. See:
    # https://github.com/microsoft/vcpkg/issues/12848
    # https://github.com/vgc/vgc/pull/413
    set(FREETYPE_H "")
    foreach(h ${FREETYPE_INCLUDE_DIRS})
        if(NOT FREETYPE_H)
            # Yes, FreeType's include hierarchy is a mess, and not all
            # configurations provide the same include dirs.
            if (EXISTS "${h}/freetype/freetype.h")
                set(FREETYPE_H "${h}/freetype/freetype.h")
            elseif (EXISTS "${h}/freetype.h")
                set(FREETYPE_H "${h}/freetype.h")
            endif()
        endif()
    endforeach()
    if(FREETYPE_H)
        file(READ "${FREETYPE_H}" FREETYPE_H_STR)
        string(REGEX MATCH "define[ \t]+FREETYPE_MAJOR[ \t]+([0-9]+)" FREETYPE_MAJOR_MATCH "${FREETYPE_H_STR}")
        set(FREETYPE_VERSION_MAJOR "${CMAKE_MATCH_1}")
        string(REGEX MATCH "define[ \t]+FREETYPE_MINOR[ \t]+([0-9]+)" FREETYPE_MINOR_MATCH "${FREETYPE_H_STR}")
        set(FREETYPE_VERSION_MINOR "${CMAKE_MATCH_1}")
        string(REGEX MATCH "define[ \t]+FREETYPE_PATCH[ \t]+([0-9]+)" FREETYPE_PATCH_MATCH "${FREETYPE_H_STR}")
        set(FREETYPE_VERSION_PATCH "${CMAKE_MATCH_1}")
        set(FREETYPE_VERSION_STRING "${FREETYPE_VERSION_MAJOR}.${FREETYPE_VERSION_MINOR}.${FREETYPE_VERSION_PATCH}")
    endif()
endif()
message(STATUS "Version: ${FREETYPE_VERSION_STRING}")
vgc_print_library_info(Freetype::Freetype)
message(STATUS "") # newline

# HarfBuzz
# Note: As of CMake 3.18.1 (Jul 2020), there is still no official "FindHarBuzz" module
# On Windows, we use vcpkg which provides a harfbuzz::harfbuzz target
# On Linux/macOS, we are still looking for the best approach
message(STATUS "[HarfBuzz]")
find_package(harfbuzz CONFIG QUIET)
if(NOT harfbuzz_FOUND)
    find_package(harfbuzz MODULE REQUIRED)
endif()
get_target_property(HARFBUZZ_INCLUDE_DIRS harfbuzz::harfbuzz INTERFACE_INCLUDE_DIRECTORIES)
if(DEFINED VCPKG_TARGET_TRIPLET AND NOT HARFBUZZ_INCLUDE_DIRS)
    # INTERFACE_INCLUDE_DIRECTORIES is missing in vcpkg <= 2020.06
    # See: https://github.com/microsoft/vcpkg/issues/12847
    set(HARFBUZZ_INCLUDE_DIRS "${_VCPKG_ROOT_DIR}/installed/${VCPKG_TARGET_TRIPLET}/include/harfbuzz")
    set_target_properties(harfbuzz::harfbuzz PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${HARFBUZZ_INCLUDE_DIRS}")
endif()
if(NOT HARFBUZZ_VERSION_STRING)
    # Sometimes, VERSION_STRING isn't defined, so we do it ourselves. See:
    # https://github.com/microsoft/vcpkg/issues/12848
    # https://github.com/vgc/vgc/pull/413
    if (EXISTS "${HARFBUZZ_INCLUDE_DIRS}/hb-version.h")
        file(READ "${HARFBUZZ_INCLUDE_DIRS}/hb-version.h" _HB_VERSION_H)
        string(REGEX MATCH "define[ \t]+HB_VERSION_MAJOR[ \t]+([0-9]+)" _HARFBUZZ_MAJOR_MATCH "${_HB_VERSION_H}")
        set(HARFBUZZ_VERSION_MAJOR "${CMAKE_MATCH_1}")
        string(REGEX MATCH "define[ \t]+HB_VERSION_MINOR[ \t]+([0-9]+)" _HARFBUZZ_MINOR_MATCH "${_HB_VERSION_H}")
        set(HARFBUZZ_VERSION_MINOR "${CMAKE_MATCH_1}")
        string(REGEX MATCH "define[ \t]+HB_VERSION_MICRO[ \t]+([0-9]+)" _HARFBUZZ_MICRO_MATCH "${_HB_VERSION_H}")
        set(HARFBUZZ_VERSION_MICRO "${CMAKE_MATCH_1}")
        set(HARFBUZZ_VERSION_STRING "${HARFBUZZ_VERSION_MAJOR}.${HARFBUZZ_VERSION_MINOR}.${HARFBUZZ_VERSION_MICRO}")
    endif()
endif()
message(STATUS "Version: ${HARFBUZZ_VERSION_STRING}")
vgc_print_library_info(harfbuzz::harfbuzz)
message(STATUS "") # newline

# Restore default value of FOLDER target property
set(CMAKE_FOLDER ${VGC_CMAKE_FOLDER_OLD})

# Generate the file resources/core/version.txt with versioning info.
# See vgc/tools/version.py for details.
#
add_custom_target(vgc_version
    VERBATIM
    COMMAND ${Python_EXECUTABLE}
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
set_target_properties(vgc_version PROPERTIES FOLDER misc)

# Generate the vgc.conf file, telling VGC executables about the location
# of various runtime resources.
#
add_custom_target(vgc_conf
    VERBATIM
    COMMAND ${Python_EXECUTABLE}
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/conf.py"
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}"
        "$<CONFIG>"
        "${VGC_PYTHON_HOME}"
)
set_target_properties(vgc_conf PROPERTIES FOLDER misc)

# Copy Python dependencies
#
# On Windows we cannot easily use the system Python to import our Python wrappers
# (including for running our Python unit tests), because the system Python wouldn't
# find our DLLs even if we add them to the PATH.
#
# For example, for our python tests, if we used:
#   add_test(... COMMAND ${Python_EXECUTABLE} ...)
#   set_tests_properties(... ENVIRONMENT "PATH=${CMAKE_BINARY_DIR}/$<CONFIG>/bin;PYTHONPATH=${CMAKE_BINARY_DIR}/$<CONFIG>/python") 
#
# Then ${Python_EXECUTABLE} would correctly find <build>/<config>/python/vgc/core.*.pyd
# thanks to the provided PYTHONPATH, but would still fail with:
#
#   ImportError: DLL load failed while importing core: The specified module could not be found.
#
# The reason is that python/vgc/core.*.pyd cannot find bin/vgccore.dll, even if the
# bin folder is in the PATH. Indeed, since Python 3.8, Python completely ignores
# the PATH variable for DLL resolution. Instead, it is required to use
# os.add_dll_directory('path/to/dll'), see:
#
#   - https://docs.python.org/3/library/os.html#os.add_dll_directory
#   - https://docs.python.org/3/whatsnew/3.8.html#bpo-36085-whatsnew
#
# Therefore, for now, our solution is to copy the whole Python distribution to our build 
# folder and use this copy of Python instead of the system Python whenever we need to
# import our Python modules (for example, in our Python tests, or in the embedded interpreter
# of our apps).
#
# A more long-term solution might be to rename our pyd module `core_`, and have a
# pure-Python module called `core` that calls os.add_dll_directory() before 
# importing core_ and transferring all symbols from core_ to core.
#
if(WIN32)
    add_custom_target(vgc_copy_python
        VERBATIM
        COMMAND ${Python_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/windows/copy_python.py"
            "${CMAKE_CURRENT_BINARY_DIR}"
            "$<CONFIG>"
    )
    set_target_properties(vgc_copy_python PROPERTIES FOLDER misc)
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
        COMMAND ${Python_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/windows/deploy.py"
            "${CMAKE_CURRENT_SOURCE_DIR}"
            "${CMAKE_CURRENT_BINARY_DIR}"
            "$<CONFIG>"
            "${WiX}"
    )
elseif(APPLE)
    add_custom_target(deploy
        VERBATIM
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${Python_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/macos/deploy.py"
            "${CMAKE_CURRENT_SOURCE_DIR}"
            "${CMAKE_CURRENT_BINARY_DIR}"
            "$<CONFIG>"
            "${VGC_QT_ROOT}"
    )
else()
    add_custom_target(deploy
        VERBATIM
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${Python_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/linux/deploy.py"
            "${CMAKE_CURRENT_SOURCE_DIR}"
            "${CMAKE_CURRENT_BINARY_DIR}"
            "$<CONFIG>"
            "${VGC_QT_ROOT}"
    )
endif()
set_target_properties(deploy PROPERTIES FOLDER misc)

# Add internal libs and apps
message(STATUS "[VGC]")
add_subdirectory(libs)
add_subdirectory(apps)
message(STATUS "") # newline
