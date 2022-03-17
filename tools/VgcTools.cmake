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
    message(STATUS "vgc::${LIB_NAME}")

    set(options "")
    set(oneValueArgs "")
    set(multiValueArgs
            THIRD_DEPENDENCIES
            VGC_DEPENDENCIES
            HEADER_FILES
            CPP_FILES
            COMPILE_DEFINITIONS
            RESOURCE_FILES)
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
    # These are public, that is, they propagate to dependent libraries.
    target_compile_definitions(${TARGET_NAME} PUBLIC ${ARG_COMPILE_DEFINITIONS})

    # Set -DVGC_FOO_EXPORTS when compiling this library (but not when using it)
    # to properly export DLL symbols. See libs/vgc/core/dll.h for details.
    string(TOUPPER VGC_${LIB_NAME}_EXPORTS EXPORTS_COMPILE_DEFINITION)
    target_compile_definitions(${TARGET_NAME} PRIVATE ${EXPORTS_COMPILE_DEFINITION})

    # Add -fvisibility=hidden when compiling on Linux/macOS.
    # Note: This is already the default on Windows.
    set_target_properties(${TARGET_NAME} PROPERTIES CXX_VISIBILITY_PRESET hidden)

    # Add compiler warning flags and macros
    target_compile_options(${TARGET_NAME} PUBLIC ${VGC_COMPILER_WARNING_FLAGS})
    target_compile_definitions(${TARGET_NAME} PUBLIC ${VGC_COMPILER_WARNING_DEFINITIONS})

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
        VERBATIM
        DEPENDS ${RESOURCES_TXT}
        COMMAND ${Python_EXECUTABLE}
            "${CMAKE_SOURCE_DIR}/tools/copy_resources.py"
            "${CMAKE_SOURCE_DIR}"
            "${CMAKE_BINARY_DIR}"
            "$<CONFIG>"
            "lib"
            "${LIB_NAME}"
    )
    add_dependencies(${TARGET_NAME} ${TARGET_NAME}_resources)

endfunction()

# Defines an optional compile definition that can be added. VISIBILITY can be
# either INTERFACE, PUBLIC, or PRIVATE. Example:
#
# vgc_optional_compile_definition(core PUBLIC VGC_CORE_USE_32BIT_INT "Define vgc::Int/UInt as 32-bit integers (default is 64-bit)")
#
function(vgc_optional_compile_definition LIBNAME VISIBILITY NAME DOC)
    option(${NAME} ${DOC})
    if(${NAME})
        target_compile_definitions(vgc_lib_${LIBNAME} ${VISIBILITY} ${NAME})
    endif()
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
    message(STATUS "vgc::${LIB_NAME} wraps")

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
#   ${Python_EXECUTABLE} <vgc-source-dir>/libs/vgc/mylib/tests/test_mytest.py -v'
#
# with the following folder added to PYTHONPATH:
#
#   <vgc-build-dir>/<config>/python
#
function(vgc_test_library LIB_NAME)
    message(STATUS "vgc::${LIB_NAME} tests")

    set(options "")
    set(oneValueArgs "")
    set(multiValueArgs CPP_TESTS PYTHON_TESTS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Add C++ tests
    vgc_prepend_(LIB_TARGET vgc_lib_ ${LIB_NAME})
    foreach(FILENAME ${ARG_CPP_TESTS})
        string(REPLACE "." "_" FILENAME_WITHOUT_DOT ${FILENAME})
        set(TEST_TARGET vgc_${LIB_NAME}_${FILENAME})
        set(EXE_TARGET vgc_${LIB_NAME}_${FILENAME_WITHOUT_DOT})
        if(WIN32)
            # On Windows, we generate the tests in the ./bin folder so that Visual Studio
            # (and perhaps other tools) can find the dependent DLLs. In theory, we'd
            # prefer to keep the tests out of ./bin and provide an appropriate PATH
            # before launching them, but the Test Explorer of VS seems to perform some
            # preprocessing on the tests (e.g., detecting Google Test symbols) which
            # ignores our set_tests_properties(.. ENVIRONMENT "PATH=..."). This means
            # that if our C++ tests are not in ./bin, they do not show up int the Test
            # Explorer and can't be easily run and debugged.
            #
            set(TEST_SUBFOLDER bin)
            set(EXE_TARGET_OUTPUT_NAME ${EXE_TARGET})  # ".exe" suffix automatically added
        else()
            set(TEST_SUBFOLDER tests/vgc/${LIB_NAME})
            set(EXE_TARGET_OUTPUT_NAME ${FILENAME_WITHOUT_DOT}.out)
        endif()
        add_executable(${EXE_TARGET} EXCLUDE_FROM_ALL ${FILENAME})
        target_link_libraries(${EXE_TARGET} ${LIB_TARGET} gtest)
        add_dependencies(tests ${EXE_TARGET})
        set_target_properties(${EXE_TARGET} PROPERTIES
            OUTPUT_NAME ${EXE_TARGET_OUTPUT_NAME}
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/${TEST_SUBFOLDER}
        )
        if(WIN32)            
            add_test(
                NAME ${TEST_TARGET}
                COMMAND ${CMAKE_BINARY_DIR}/$<CONFIG>/${TEST_SUBFOLDER}/${EXE_TARGET_OUTPUT_NAME}.exe
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/${TEST_SUBFOLDER}
            )
            # Note: If we move the tests outside ./bin, we'd need this:
            #   set_tests_properties(${TEST_TARGET} PROPERTIES
            #       ENVIRONMENT "PATH=%PATH%\;${CMAKE_BINARY_DIR}/$<CONFIG>/bin"
            #   )
        else()
            add_test(
                NAME ${TEST_TARGET}
                COMMAND ${CMAKE_BINARY_DIR}/$<CONFIG>/${TEST_SUBFOLDER}/${EXE_TARGET_OUTPUT_NAME}
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/${TEST_SUBFOLDER}
            )
        endif()
        set_property(TEST ${TEST_TARGET} APPEND PROPERTY
            ENVIRONMENT VGCBASEPATH=${CMAKE_BINARY_DIR}/$<CONFIG>
        )
    endforeach()

    # Add python tests
    foreach(PYTHON_TEST_FILENAME ${ARG_PYTHON_TESTS})
        set(PYTHON_TEST_TARGET_NAME vgc_${LIB_NAME}_${PYTHON_TEST_FILENAME})
        if(WIN32)
            add_test(
                NAME ${PYTHON_TEST_TARGET_NAME}
                COMMAND ${CMAKE_BINARY_DIR}/$<CONFIG>/bin/python.exe ${CMAKE_CURRENT_SOURCE_DIR}/${PYTHON_TEST_FILENAME} -v
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            )
            set_tests_properties(${PYTHON_TEST_TARGET_NAME} PROPERTIES
                ENVIRONMENT "PATH=%PATH%\;${CMAKE_BINARY_DIR}/$<CONFIG>/bin"
            )
        else()
            add_test(
                NAME ${PYTHON_TEST_TARGET_NAME}
                COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${PYTHON_TEST_FILENAME} -v
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            )
            set_tests_properties(${PYTHON_TEST_TARGET_NAME} PROPERTIES
                ENVIRONMENT PYTHONPATH=${CMAKE_BINARY_DIR}/$<CONFIG>/python:$ENV{PYTHONPATH}
            )
        endif()
        set_property(TEST ${PYTHON_TEST_TARGET_NAME} APPEND PROPERTY
            ENVIRONMENT VGCBASEPATH=${CMAKE_BINARY_DIR}/$<CONFIG>
        )
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
    message(STATUS "vgc${APP_NAME}")

    set(options "")
    set(oneValueArgs "")
    set(multiValueArgs THIRD_DEPENDENCIES VGC_DEPENDENCIES CPP_FILES COMPILE_DEFINITIONS RESOURCE_FILES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Prepend APP_NAME with "vgc_app_" to get target name.
    vgc_prepend_(TARGET_NAME vgc_app_ ${APP_NAME})

    # Add executable
    add_executable(${TARGET_NAME} WIN32 ${ARG_CPP_FILES})

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
        VERBATIM
        DEPENDS ${RESOURCES_TXT}
        COMMAND ${Python_EXECUTABLE}
            "${CMAKE_SOURCE_DIR}/tools/copy_resources.py"
            "${CMAKE_SOURCE_DIR}"
            "${CMAKE_BINARY_DIR}"
            "$<CONFIG>"
            "app"
            "${APP_NAME}"
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
                ${VGC_QT_ROOT}/bin/windeployqt.exe
                ${CMAKE_BINARY_DIR}/$<CONFIG>/bin/vgc${APP_NAME}.exe
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/deploy/$<CONFIG>
            COMMAND if exist ${CMAKE_BINARY_DIR}/$<CONFIG>/bin/vc_redist.x64.exe
                ${CMAKE_COMMAND} -E rename
                    ${CMAKE_BINARY_DIR}/$<CONFIG>/bin/vc_redist.x64.exe
                    ${CMAKE_BINARY_DIR}/deploy/$<CONFIG>/vc_redist.x64.exe
            VERBATIM
        )

        # Add dependency to copy_python so that pythonXY.dll is copied to the 
        # build folder when building the app.
        #
        add_dependencies(${TARGET_NAME} copy_python)
    endif()

endfunction()

# Print the given property of the given target as
# a STATUS message.
#
# Example:
# vgc_print_target_property(Freetype::Freetype INTERFACE_INCLUDE_DIRECTORIES)
#
# Output:
# -- Freetype::Freetype INTERFACE_INCLUDE_DIRECTORIES: /usr/include/freetype2;/usr/include/x86_64-linux-gnu/freetype2
#
function(vgc_print_target_property target prop)
    get_target_property(propval ${target} ${prop})
    if(propval)
        message(STATUS "${target} ${prop}: ${propval}")
    else()
        message(STATUS "${target} ${prop}: NOTFOUND")
    endif()
endfunction()

# Print useful info about the target library, such as
# its include dirs and location.
#
# Example:
# vgc_print_library_info(Freetype::Freetype)
#
# Output:
# -- Freetype::Freetype TYPE: IMPORTED_LIBRARY
# -- Freetype::Freetype INTERFACE_INCLUDE_DIRECTORIES: C:/Users/Boris/vcpkg/installed/x64-windows/include
# -- Freetype::Freetype IMPORTED_IMPLIB_RELEASE: C:/Users/Boris/vcpkg/installed/x64-windows/lib/freetype.lib
# -- Freetype::Freetype IMPORTED_LOCATION_RELEASE: C:/Users/Boris/vcpkg/installed/x64-windows/bin/freetype.dll
# -- Freetype::Freetype IMPORTED_IMPLIB_DEBUG: C:/Users/Boris/vcpkg/installed/x64-windows/debug/lib/freetyped.lib
# -- Freetype::Freetype IMPORTED_LOCATION_DEBUG: C:/Users/Boris/vcpkg/installed/x64-windows/debug/bin/freetyped.dll
#
function(vgc_print_library_info target)
    if(NOT TARGET ${target})
        message(WARNING "No target named '${target}'")
    else()
        vgc_print_target_property(${target} TYPE)
        vgc_print_target_property(${target} INTERFACE_INCLUDE_DIRECTORIES)
        get_target_property(type ${target} TYPE)
        if(${type} STREQUAL "IMPORTED_LIBRARY")
            get_target_property(configs ${target} IMPORTED_CONFIGURATIONS)
            if(configs)
                set(props "IMPORTED_IMPLIB;IMPORTED_LOCATION")
                foreach(config ${configs})
                    foreach(prop ${props})
                        vgc_print_target_property(${target} ${prop}_${config})
                    endforeach()
                endforeach()
            else()
                vgc_print_target_property(${target} IMPORTED_LOCATION)
            endif()
        endif()
    endif()

endfunction()
