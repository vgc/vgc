# Prefixes all strings of a comma-separated list of string with a common
# string, and stores the result in a new variable.
#
# Usage:
# vgc_prepend_(myVariable "prefix_" "string1;string2")
# message(${myVariable}) # -> "prefix_string1;prefix_string2"
#
function(vgc_prepend_ var prefix)
   set(listVar "")
   foreach(f ${ARGN})
      list(APPEND listVar "${prefix}${f}")
   endforeach()
   set(${var} "${listVar}" PARENT_SCOPE)
endfunction()

# Defines a command that copies a file from src to dest.
#
# Use of generator expressions
# ----------------------------
#
# Unfortunately, the destination cannot use generator expressions (e.g.,
# $<Config>), because it is used as OUTPUT of add_custom_command, which does
# not support generator expressions.
#
# If you need to generate a file whose path depends on a generator expression,
# then your only possible workaround is to generate it within a COMMAND of an
# add_custom_target(). The drawback is that the COMMAND will *always* be
# called, which may not be efficient, unless the COMMAND is smart enough and
# includes logic to avoid regenerating the file if it knows somehow that it
# isn't out of date (e.g., rolling out its own timestamp-based mechanism).
#
function(vgc_add_copy_command src dest)
    add_custom_command(
        COMMENT ""
        OUTPUT ${dest}
        DEPENDS ${src}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${src} ${dest}
        VERBATIM
    )
endfunction()

# Defines a new VGC library. This calls add_library under the hood.
#
# Usage:
# vgc_add_library(myvgclib1
#     THIRD_DEPENDENCIES
#         thirdlib1
#         thirdlib2
#
#     VGC_DEPENDENCIES
#         myvgclib2
#
#     HEADER_FILES
#         class1.h
#         class2.h
#         headeronlyclass.h
#
#     CPP_FILES
#         class1.cpp
#         class2.cpp
#
#     RESOURCE_FILES
#         resource1.png
#         resource2.ttf
# )
#
function(vgc_add_library LIB_NAME)
    message("-- VGC Library: ${LIB_NAME}")

    set(options "")
    set(oneValueArgs "")
    set(multiValueArgs THIRD_DEPENDENCIES VGC_DEPENDENCIES HEADER_FILES CPP_FILES COMPILE_DEFINITIONS RESOURCE_FILES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Add library using target name "vgc_lib_mylib"
    set(TARGET_NAME vgc_lib_${LIB_NAME})
    add_library(${TARGET_NAME} SHARED ${ARG_HEADER_FILES} ${ARG_CPP_FILES})

    # Set library output directory and filenames.
    #
    # Note that on non-DLL platforms (macOS, Linux), the output directory is
    # specified by LIBRARY_OUTPUT_DIRECTORY, while on DLL platforms (Windows),
    # the output directory is specified by RUNTIME_OUTPUT_DIRECTORY. See:
    #
    # https://cmake.org/cmake/help/latest/manual/cmake-buildsystem.7.html#output-artifacts
    #
    # Also, note that on multi-configuration generators (VS, Xcode), a
    # per-configuration subdirectory ("Release", "Debug", etc.) is automatically
    # appended unless a generator expression is used. For example, with a Visual
    # Studio generator, the property `RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin`
    # would output a file such as `<build>/bin/Release/vgccore.dll`. See:
    #
    # https://cmake.org/cmake/help/latest/prop_tgt/RUNTIME_OUTPUT_DIRECTORY.html
    #
    # In our case, we prefer to have the per-configuration subdirectory
    # prepended rather than appended, such as `<build>/Release/bin/vgccore.dll`,
    # so that the directory structure within the per-configuration subdirectory
    # is identitical than those of single-configuration. Therefore, we use
    # the generator expression $<Config> to achieve this.
    #
    # Finally, note that prefixes and suffixes are automatically added based
    # on ${CMAKE_SHARED_LIBRARY_PREFIX} and ${CMAKE_SHARED_LIBRARY_SUFFIX}.
    #
    # Examples:
    #   LIB_NAME:                            core
    #   Output on Linux with Make generator: <build>/lib/libvgccore.so
    #   Output on Windows with VS generator: <build>/<Config>/bin/vgccore.dll
    #
    set_target_properties(${TARGET_NAME}
        PROPERTIES
            OUTPUT_NAME vgc${LIB_NAME}
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/lib
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/bin
    )

    # Add dependencies to other VGC libraries
    vgc_prepend_(VGC_DEPENDENCIES vgc_lib_ ${ARG_VGC_DEPENDENCIES})
    target_link_libraries(${TARGET_NAME} ${VGC_DEPENDENCIES})

    # Add dependencies to third-party dependencies
    target_link_libraries(${TARGET_NAME} ${ARG_THIRD_DEPENDENCIES})

    # Set compile definitions, that is, values given to preprocessor variables
    # In addition to those provided to vgc_add_library, we add -DVGC_FOO_EXPORTS
    # to properly export DLL symbols (see libs/vgc/core/dll.h for details)
    target_compile_definitions(${TARGET_NAME} PRIVATE ${ARG_COMPILE_DEFINITIONS})
    string(TOUPPER VGC_${LIB_NAME}_EXPORTS EXPORTS_COMPILE_DEFINITION)
    target_compile_definitions(${TARGET_NAME} PRIVATE ${EXPORTS_COMPILE_DEFINITION})

    # Add -fvisibility=hidden when compiling on Linux/macOS.
    # Note: This is already the default on Windows.
    set_target_properties(${TARGET_NAME} PROPERTIES CXX_VISIBILITY_PRESET hidden)

    # Copy the 'copy_resources.py' script to the current build directory.
    #
    # Why making a copy instead of running directly from source dir? Because we
    # prefer to run scripts from the build directory to avoid polluting the
    # source directory with output (e.g., __pycache__).
    #
    # Why not creating only ony copy in ${CMAKE_BINARY_DIR}, instead of one copy
    # per library in ${CMAKE_CURRENT_BINARY_DIR}? Because add_custom_target only
    # allows to define dependencies generated by the same CMakeLists.txt.
    # Besides, this makes it an implementation detail of vgc_add_library hidden
    # to the parent directory scope, which increases maintainability.
    #
    set(COPY_RESOURCES_PY ${CMAKE_CURRENT_BINARY_DIR}/copy_resources.py)
    vgc_add_copy_command(
        ${CMAKE_SOURCE_DIR}/tools/copy_resources.py
        ${COPY_RESOURCES_PY}
    )

    # Write list of resources as comma-separated string
    set(RESOURCES_TXT ${CMAKE_CURRENT_BINARY_DIR}/resources.txt)
    vgc_prepend_(RESOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/ ${ARG_RESOURCE_FILES})
    add_custom_command(
        COMMENT ""
        OUTPUT ${RESOURCES_TXT}
        DEPENDS ${RESOURCE_FILES}
        COMMAND ${CMAKE_COMMAND} -E echo "${ARG_RESOURCE_FILES}" > ${RESOURCES_TXT}
        VERBATIM
    )

    # Update build copy of resources whenever necessary
    add_custom_target(${TARGET_NAME}_resources ALL
        DEPENDS ${RESOURCES_TXT} ${COPY_RESOURCES_PY}
        COMMAND ${PYTHON_EXECUTABLE} -c
            "import copy_resources; copy_resources.run('${CMAKE_SOURCE_DIR}', '${CMAKE_BINARY_DIR}', '$<CONFIG>', '${LIB_NAME}')"
        WORKING_DIRECTORY
            ${CMAKE_CURRENT_BINARY_DIR}
        VERBATIM
    )

endfunction()

# Defines a new python extension module that wraps the given
# VGC library. This calls pybind11_add_module() under the hood
#
# Usage:
# vgc_wrap_library(mylib
#     module.cpp
#     wrap_file1.cpp
#     wrap_file2.cpp
# )
#
function(vgc_wrap_library LIB_NAME)
    message("-- VGC Library: ${LIB_NAME} (python wrappers)")

    cmake_parse_arguments(ARG "" "" "" ${ARGN})

    # Get cmake target name for the python module and the C++ lib
    # Example:
    #   LIB_NAME = geometry
    #   LIB_TARGET_NAME = vgc_lib_geometry
    #   MODULE_TARGET_NAME = vgc_wrap_geometry
    vgc_prepend_(LIB_TARGET_NAME vgc_lib_ ${LIB_NAME})
    vgc_prepend_(WRAP_TARGET_NAME vgc_wrap_ ${LIB_NAME})

    # Use pybind11 helper function. This calls add_library(${TARGET_NAME} ...)
    # and sets all required include dirs and libs to link.
    pybind11_add_module(${WRAP_TARGET_NAME} ${ARG_UNPARSED_ARGUMENTS})

    # Set the output name. This name must match how python users
    # import the library, e.g., "from vgc.geometry import Point"
    set_target_properties(${WRAP_TARGET_NAME}
        PROPERTIES
            OUTPUT_NAME ${LIB_NAME}
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/python/vgc
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/python/vgc
    )

    # Link to the C++ library this Python module is wrapping
    target_link_libraries(${WRAP_TARGET_NAME} PRIVATE ${LIB_TARGET_NAME})

endfunction()

# Defines a set of unit tests to be run when calling 'make test'. Under the
# hood, this calls add_test() for each test.
#
# For now, only Python tests are supported, but we are planning to support C++
# tests in the future.
#
# Usage:
# vgc_test_library(mylib
#     PYTHON_TESTS
#         test_file1.py
#         test_file2.py
# )
#
# Python Tests
# ------------
#
# The Python tests are run from the following directory:
#
#   <vgc-build-dir>/libs/vgc/mylib/tests/
#
# By calling the command
#
#   ${PYTHON_EXECUTABLE} <vgc-source-dir>/libs/vgc/mylib/tests/test_mytest.py -v'
#
# with the following folder added to PYTHONPATH:
#
#   <vgc-build-dir>/<config>/python
#
# Currently, the CMake variable ${PYTHON_EXECUTABLE} is set by FindPythonLibsNew
# shipped with third/pybind11.
#
function(vgc_test_library LIB_NAME)
    message("-- VGC Library: ${LIB_NAME} (tests)")

    set(options "")
    set(oneValueArgs "")
    set(multiValueArgs PYTHON_TESTS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Add python tests
    foreach(PYTHON_TEST_FILENAME ${ARG_PYTHON_TESTS})
        set(PYTHON_TEST_TARGET_NAME vgc_${LIB_NAME}_${PYTHON_TEST_FILENAME})
        if(WIN32)
            add_test(
                NAME ${PYTHON_TEST_TARGET_NAME}
                COMMAND ${CMAKE_BINARY_DIR}/$<CONFIG>/bin/python.exe ${CMAKE_CURRENT_SOURCE_DIR}/${PYTHON_TEST_FILENAME} -v
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            )
            set_tests_properties(${PYTHON_TEST_TARGET_NAME}
                PROPERTIES
                    ENVIRONMENT "PATH=%PATH%\;${CMAKE_BINARY_DIR}/$<CONFIG>/bin"
            )
        else()
            add_test(
                NAME ${PYTHON_TEST_TARGET_NAME}
                COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${PYTHON_TEST_FILENAME} -v
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            )
            set_tests_properties(${PYTHON_TEST_TARGET_NAME}
                PROPERTIES
                    ENVIRONMENT PYTHONPATH=${CMAKE_BINARY_DIR}/$<CONFIG>/python:$ENV{PYTHONPATH}
            )
        endif()
    endforeach()

endfunction()

# Defines a new VGC app. This calls add_executable under the hood.
#
# Usage:
# vgc_add_app(myvgcapp
#     THIRD_DEPENDENCIES
#         thirdlib1
#         thirdlib2
#
#     VGC_DEPENDENCIES
#         vgclib1
#
#     CPP_FILES
#         main.cpp
# )
#
function(vgc_add_app APP_NAME)
    message("-- VGC App: ${APP_NAME}")

    set(options "")
    set(oneValueArgs "")
    set(multiValueArgs THIRD_DEPENDENCIES VGC_DEPENDENCIES CPP_FILES COMPILE_DEFINITIONS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Prepend APP_NAME with "vgc_app_" to get target name.
    vgc_prepend_(TARGET_NAME vgc_app_ ${APP_NAME})

    # Add library
    add_executable(${TARGET_NAME} ${ARG_CPP_FILES})

    # VGC dependencies
    vgc_prepend_(VGC_DEPENDENCIES vgc_lib_ ${ARG_VGC_DEPENDENCIES})
    target_link_libraries(${TARGET_NAME} ${VGC_DEPENDENCIES})

    # Third-party dependencies
    target_link_libraries(${TARGET_NAME} ${ARG_THIRD_DEPENDENCIES})

    # Compile definitions, that is, values given to preprocessor variables
    target_compile_definitions(${TARGET_NAME} PRIVATE ${ARG_COMPILE_DEFINITIONS})

    # Set the output name. Example (for now): vgcillustration
    # Under Windows, we may want to call it "VGC_Illustration_2020.exe"
    # Under Linux, we may want to call it vgc-illustration-2020
    set_target_properties(${TARGET_NAME}
        PROPERTIES
            OUTPUT_NAME vgc${APP_NAME}
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/bin
    )

    if(WIN32)
        # Run windeployqt to copy all required Qt dependencies in the bin
        # folder.
        #
        # Note: in theory, a POST_BUILD command will only be executed if the
        # target is rebuilt. However, there a specific bug when using msbuild:
        # POST_BUILD commands are always run regardless of whether the target
        # is rebuilt or not. See:
        #
        # https://gitlab.kitware.com/cmake/cmake/issues/18530
        #
        # In addition, we may not even want to run windeployqt each time the
        # target is rebuilt, since it is extremely rare that new Qt dependencies
        # are added between two builds.
        #
        # Therefore, to avoid running windeployqt unnecessarily, we first check
        # for the existence of *any* Qt DLL in the bin folder, then run
        # windeployqt only if there are none. This would fail to add any newly
        # required Qt dependency, but this should be very rare. If one needs
        # windeployqt to be re-run, simply delete all Qt DLLs from the bin
        # folder and rebuild.
        #
        # In the future, we may want to be a little more conservative, for
        # example we could implement a way to at least automatically re-run
        # windeployqt when a CMake file changes.
        #
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND if not exist ${CMAKE_BINARY_DIR}/$<CONFIG>/bin/Qt*.dll
                ${Qt}/bin/windeployqt.exe
                ${CMAKE_BINARY_DIR}/$<CONFIG>/bin/vgc${APP_NAME}.exe
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/deploy/$<CONFIG>
            COMMAND if exist ${CMAKE_BINARY_DIR}/$<CONFIG>/bin/vc_redist.x64.exe
                ${CMAKE_COMMAND} -E rename
                    ${CMAKE_BINARY_DIR}/$<CONFIG>/bin/vc_redist.x64.exe
                    ${CMAKE_BINARY_DIR}/deploy/$<CONFIG>/vc_redist.x64.exe
            VERBATIM
        )
    endif()

endfunction()
