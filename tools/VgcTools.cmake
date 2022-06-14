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

# Prefixes and suffixes all strings of a comma-separated list of string,
# and stores the result in a new variable.
#
# Usage:
# vgc_add_prefix_suffix_(s "vgc_" "core;ui" "_lib")
# message(${s}) # -> "vgc_core_lib;vgc_ui_lib"
#
function(vgc_add_prefix_suffix_ var prefix middle suffix)
   set(listVar "")
   foreach(f ${middle})
      list(APPEND listVar "${prefix}${f}${suffix}")
   endforeach()
   set(${var} "${listVar}" PARENT_SCOPE)
endfunction()

# Sets the desired warning flags for a VGC lib, test, or app.
#
function(vgc_set_warning_flags target)
    target_compile_options(${target} PUBLIC ${VGC_COMPILER_WARNING_FLAGS})
    target_compile_definitions(${target} PUBLIC ${VGC_COMPILER_WARNING_DEFINITIONS})
    if(VGC_WERROR)
        target_compile_options(${target} PRIVATE ${VGC_WERROR_FLAG})
    endif ()
    if(VGC_PEDANTIC)
        target_compile_options(${target} PRIVATE ${VGC_PEDANTIC_COMPILE_FLAGS})
    endif()
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
    set(BASE_TARGET      vgc_${LIB_NAME})
    set(LIB_TARGET       vgc_${LIB_NAME}_lib)
    set(RESOURCES_TARGET vgc_${LIB_NAME}_resources)
    message(STATUS ${LIB_TARGET})

    set(options "")
    set(oneValueArgs "")
    set(multiValueArgs
            THIRD_DEPENDENCIES
            VGC_DEPENDENCIES
            HEADER_FILES
            CPP_FILES
            COMPILE_DEFINITIONS
            RESOURCE_FILES
            NATVIS_FILES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Create library target "vgc_<libname>_lib"
    add_library(${LIB_TARGET} SHARED ${ARG_HEADER_FILES} ${ARG_CPP_FILES} ${ARG_NATVIS_FILES})

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
    set_target_properties(${LIB_TARGET}
        PROPERTIES
            OUTPUT_NAME vgc${LIB_NAME}
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/lib
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/bin
            FOLDER libs/${LIB_NAME}
    )

    # Include dir ( = ${CMAKE_CURRENT_SOURCE_DIR}/../.. )
    get_filename_component(LIB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
    get_filename_component(LIB_INCLUDE_DIR ${LIB_INCLUDE_DIR} DIRECTORY)
    target_include_directories(${LIB_TARGET} PUBLIC ${LIB_INCLUDE_DIR})

    # Add dependencies to other VGC libraries
    vgc_add_prefix_suffix_(VGC_LIB_DEPENDENCIES vgc_ "${ARG_VGC_DEPENDENCIES}" _lib)
    target_link_libraries(${LIB_TARGET} PUBLIC ${VGC_LIB_DEPENDENCIES})

    # Add dependencies to third-party dependencies
    target_link_libraries(${LIB_TARGET} PUBLIC ${ARG_THIRD_DEPENDENCIES})

    # Set compile definitions, that is, values given to preprocessor variables
    # These are public, that is, they propagate to dependent libraries.
    target_compile_definitions(${LIB_TARGET} PUBLIC ${ARG_COMPILE_DEFINITIONS})

    # Set -DVGC_FOO_EXPORTS when compiling this library (but not when using it)
    # to properly export DLL symbols. See libs/vgc/core/dll.h for details.
    string(TOUPPER VGC_${LIB_NAME}_EXPORTS EXPORTS_COMPILE_DEFINITION)
    target_compile_definitions(${LIB_TARGET} PRIVATE ${EXPORTS_COMPILE_DEFINITION})

    # Add -fvisibility=hidden when compiling on Linux/macOS.
    # Note: This is already the default on Windows.
    set_target_properties(${LIB_TARGET} PROPERTIES CXX_VISIBILITY_PRESET hidden)

    # Add private compile definitions
    target_compile_definitions(${LIB_TARGET} PUBLIC ${VGC_PRIVATE_COMPILE_DEFINITIONS})

    # Set compiler warning flags
    vgc_set_warning_flags(${LIB_TARGET})

    # Copy resources to the build folder whenever necessary
    if(ARG_RESOURCE_FILES)
        
        # Write list of resources as comma-separated string.
        # We use this in copy_resources.py to know which resources require to be updated.
        set(RESOURCES_TXT ${CMAKE_CURRENT_BINARY_DIR}/resources.txt)
        vgc_prepend_(RESOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/ ${ARG_RESOURCE_FILES})
        add_custom_command(
            COMMENT ""
            OUTPUT ${RESOURCES_TXT}
            DEPENDS ${RESOURCE_FILES}
            COMMAND ${CMAKE_COMMAND} -E echo "${ARG_RESOURCE_FILES}" > ${RESOURCES_TXT}
            VERBATIM
        )
                
        # Create target executing our copy_resources.py script.
        add_custom_target(${RESOURCES_TARGET}
            VERBATIM
            SOURCES ${ARG_RESOURCE_FILES}
            DEPENDS ${RESOURCES_TXT}
            COMMAND ${Python_EXECUTABLE}
                "${CMAKE_SOURCE_DIR}/tools/copy_resources.py"
                "${CMAKE_SOURCE_DIR}"
                "${CMAKE_BINARY_DIR}"
                "$<CONFIG>"
                "lib"
                "${LIB_NAME}"
        )
        set_target_properties(${RESOURCES_TARGET} PROPERTIES FOLDER libs/${LIB_NAME})
        add_dependencies(${LIB_TARGET} ${RESOURCES_TARGET})
    endif()

    # Create vgc_<libname> base target, which is useful for:
    # - building both the lib and its Python wrappers (vgc_<appname>_app needs this)
    # - storing internal dependencies (vgc_<libname>_wraps needs this)
    add_custom_target(${BASE_TARGET})
    set_target_properties(${BASE_TARGET}
        PROPERTIES
            FOLDER misc/libs
            VGC_DEPENDENCIES "${ARG_VGC_DEPENDENCIES}"
    )
    add_dependencies(${BASE_TARGET} ${LIB_TARGET})
    vgc_prepend_(BASE_TARGET_DEPENDENCIES vgc_ ${ARG_VGC_DEPENDENCIES})
    if(BASE_TARGET_DEPENDENCIES)
        add_dependencies(${BASE_TARGET} ${BASE_TARGET_DEPENDENCIES})
    endif()

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
    set(BASE_TARGET   vgc_${LIB_NAME})
    set(LIB_TARGET    vgc_${LIB_NAME}_lib)
    set(WRAPS_TARGET  vgc_${LIB_NAME}_wraps)
    message(STATUS ${WRAPS_TARGET})

    cmake_parse_arguments(ARG "" "" "" ${ARGN})    
    
    # Use pybind11 helper function. This calls add_library(${TARGET_NAME} ...)
    # and sets all required include dirs and libs to link.
    pybind11_add_module(${WRAPS_TARGET} ${ARG_UNPARSED_ARGUMENTS})

    # Set the output name. This name must match how python users
    # import the library, e.g., "from vgc.geometry import Point"
    set_target_properties(${WRAPS_TARGET}
        PROPERTIES
            OUTPUT_NAME ${LIB_NAME}
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/python/vgc
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/python/vgc
            FOLDER libs/${LIB_NAME}
    )
    
    # Link to the C++ library this Python module is wrapping
    target_link_libraries(${WRAPS_TARGET} PRIVATE ${LIB_TARGET})

    # Set compiler warning flags
    vgc_set_warning_flags(${WRAPS_TARGET})

    # Add vgc_<libname>_wraps target as dependency to vgc_<libname> target
    add_dependencies(${BASE_TARGET} ${WRAPS_TARGET})

    # Add dependencies to the wrappers that this wrapper depends to. For
    # example, when we build vgc_dom_wraps, we also need vgc_core_wraps to be
    # built. We do this indirectly by adding a dependency to vgc_core which
    # depends itself on vgc_wraps, which nicely handles the case where a lib
    # doesn't have wrappers.
    get_target_property(VGC_DEPENDENCIES ${BASE_TARGET} VGC_DEPENDENCIES)
    vgc_prepend_(WRAPS_TARGET_DEPENDENCIES vgc_ ${VGC_DEPENDENCIES})
    if(WRAPS_TARGET_DEPENDENCIES)
        add_dependencies(${WRAPS_TARGET} ${WRAPS_TARGET_DEPENDENCIES})
    endif()

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
    set(BASE_TARGET         vgc_${LIB_NAME})
    set(LIB_TARGET          vgc_${LIB_NAME}_lib)
    set(WRAPS_TARGET        vgc_${LIB_NAME}_wraps)
    set(CHECK_TARGET        vgc_${LIB_NAME}_check)
    set(TESTS_TARGET        vgc_${LIB_NAME}_tests)
    set(CPP_TESTS_TARGET    vgc_${LIB_NAME}_cpp_tests)
    set(PYTHON_TESTS_TARGET vgc_${LIB_NAME}_python_tests)
    message(STATUS ${TESTS_TARGET})

    set(options "")
    set(oneValueArgs "")
    set(multiValueArgs CPP_TESTS PYTHON_TESTS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Create target vgc_<libname>_tests
    add_custom_target(${TESTS_TARGET} SOURCES ${ARG_CPP_TESTS} ${ARG_PYTHON_TESTS})
    set_target_properties(${TESTS_TARGET} PROPERTIES FOLDER libs/${LIB_NAME}/tests)
    add_dependencies(vgc_tests ${TESTS_TARGET})

    # C++ tests
    if(ARG_CPP_TESTS) # If there is at least one C++ test
        # Create target vgc_<libname>_cpp_tests
        add_custom_target(${CPP_TESTS_TARGET} SOURCES ${ARG_CPP_TESTS})
        set_target_properties(${CPP_TESTS_TARGET} PROPERTIES FOLDER libs/${LIB_NAME}/tests)
        add_dependencies(${TESTS_TARGET} ${CPP_TESTS_TARGET})
    endif()
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
        target_link_libraries(${EXE_TARGET} PRIVATE ${LIB_TARGET} gtest)
        vgc_set_warning_flags(${EXE_TARGET})
        add_dependencies(${CPP_TESTS_TARGET} ${EXE_TARGET})
        set_target_properties(${EXE_TARGET} PROPERTIES
            OUTPUT_NAME ${EXE_TARGET_OUTPUT_NAME}
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/${TEST_SUBFOLDER}
            FOLDER libs/${LIB_NAME}/tests/cpp_tests
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

    # Python tests
    if(ARG_PYTHON_TESTS) # If there is at least one C++ test
        # Create target vgc_<libname>_python_tests and add the python files
        # as SOURCES to make them show up on IDEs
        add_custom_target(${PYTHON_TESTS_TARGET} SOURCES ${ARG_PYTHON_TESTS})
        set_target_properties(${PYTHON_TESTS_TARGET} PROPERTIES FOLDER libs/${LIB_NAME}/tests)
        add_dependencies(${TESTS_TARGET} ${PYTHON_TESTS_TARGET})
        add_dependencies(${PYTHON_TESTS_TARGET} ${BASE_TARGET})
        if(WIN32)
            add_dependencies(${PYTHON_TESTS_TARGET} vgc_copy_python)
        endif()
    endif()
    foreach(FILENAME ${ARG_PYTHON_TESTS})
        set(TEST_TARGET vgc_${LIB_NAME}_${FILENAME})
        if(WIN32)
            add_test(
                NAME ${TEST_TARGET}
                COMMAND ${CMAKE_BINARY_DIR}/$<CONFIG>/bin/python.exe ${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME} -v
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            )
            set_tests_properties(${TEST_TARGET} PROPERTIES
                ENVIRONMENT "PATH=%PATH%\;${CMAKE_BINARY_DIR}/$<CONFIG>/bin"
            )
        else()
            add_test(
                NAME ${TEST_TARGET}
                COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME} -v
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            )
            set_tests_properties(${TEST_TARGET} PROPERTIES
                ENVIRONMENT PYTHONPATH=${CMAKE_BINARY_DIR}/$<CONFIG>/python:$ENV{PYTHONPATH}
            )
        endif()
        set_property(TEST ${TEST_TARGET} APPEND PROPERTY
            ENVIRONMENT VGCBASEPATH=${CMAKE_BINARY_DIR}/$<CONFIG>
        )
    endforeach()

    # Create a target vgc_<libname>_check that builds and runs all the tests in
    # this lib, re-building test dependencies as needed.
    if (CMAKE_CONFIGURATION_TYPES)
        add_custom_target(${CHECK_TARGET} SOURCES ${ARG_CPP_TESTS} ${ARG_PYTHON_TESTS}
            COMMAND ${CMAKE_CTEST_COMMAND}
                --force-new-ctest-process --output-on-failure
                --build-config "$<CONFIGURATION>"
                -R vgc_${LIB_NAME}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    else()
        add_custom_target(${CHECK_TARGET} SOURCES ${ARG_CPP_TESTS} ${ARG_PYTHON_TESTS}
            COMMAND ${CMAKE_CTEST_COMMAND}
                --force-new-ctest-process --output-on-failure
                -R vgc_${LIB_NAME}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    endif()
    set_target_properties(${CHECK_TARGET} PROPERTIES FOLDER libs/${LIB_NAME})
    add_dependencies(${CHECK_TARGET} ${TESTS_TARGET})

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
    set(APP_TARGET        vgc_${APP_NAME}_app)
    set(RESOURCES_TARGET  vgc_${APP_NAME}_resources)
    message(STATUS ${APP_TARGET})

    set(options "")
    set(oneValueArgs "")
    set(multiValueArgs THIRD_DEPENDENCIES VGC_DEPENDENCIES CPP_FILES COMPILE_DEFINITIONS RESOURCE_FILES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    # Add executable
    add_executable(${APP_TARGET} WIN32 ${ARG_CPP_FILES})

    # VGC dependencies. We need to:
    # - link to all dependent libs
    # - build all dependent Python wrappers (the app requires them at runtime)
    vgc_add_prefix_suffix_(VGC_LIB_DEPENDENCIES vgc_ "${ARG_VGC_DEPENDENCIES}" _lib)
    vgc_prepend_(APP_TARGET_DEPENDENCIES vgc_ ${ARG_VGC_DEPENDENCIES})
    target_link_libraries(${APP_TARGET} PRIVATE ${VGC_LIB_DEPENDENCIES})
    add_dependencies(${APP_TARGET} ${APP_TARGET_DEPENDENCIES})

    # Third-party dependencies
    target_link_libraries(${APP_TARGET} PRIVATE ${ARG_THIRD_DEPENDENCIES})

    # Compile definitions, that is, values given to preprocessor variables
    target_compile_definitions(${APP_TARGET} PRIVATE ${ARG_COMPILE_DEFINITIONS})

    # Set compiler warning flags
    vgc_set_warning_flags(${APP_TARGET})

    # Set the output name. Example (for now): vgcillustration
    # Under Windows, we may want to call it "VGC_Illustration_2020.exe"
    # Under Linux, we may want to call it vgc-illustration-2020
    set_target_properties(${APP_TARGET}
        PROPERTIES
            OUTPUT_NAME vgc${APP_NAME}
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/bin
            FOLDER apps/${APP_NAME}
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
    if(ARG_RESOURCE_FILES)
        add_custom_target(${RESOURCES_TARGET}
            VERBATIM
            SOURCES ${ARG_RESOURCE_FILES}
            DEPENDS ${RESOURCES_TXT}
            COMMAND ${Python_EXECUTABLE}
                "${CMAKE_SOURCE_DIR}/tools/copy_resources.py"
                "${CMAKE_SOURCE_DIR}"
                "${CMAKE_BINARY_DIR}"
                "$<CONFIG>"
                "app"
                "${APP_NAME}"
        )
        set_target_properties(${RESOURCES_TARGET}
            PROPERTIES
                FOLDER apps/${APP_NAME}
        )
        add_dependencies(${APP_TARGET} ${RESOURCES_TARGET})
    endif()    
    
    # Ensures the vgc.conf file is generated next to the app
    add_dependencies(${APP_TARGET} vgc_conf)
    
    # Add the app to the deploy target
    add_dependencies(deploy ${APP_TARGET})

    if(WIN32)
        # Copy dependent DLLs to the bin folder. Unlike for other platforms,
        # we do this as part of the app target rather than the deploy target
        # because we needs the DLLs even just to run the app.
        #
        add_custom_command(TARGET ${APP_TARGET} POST_BUILD
            COMMAND ${Python_EXECUTABLE}
                "${CMAKE_SOURCE_DIR}/tools/windows/windeployqt.py"
                "${CMAKE_SOURCE_DIR}"
                "${CMAKE_BINARY_DIR}"
                "$<CONFIG>"
                "${VGC_QT_ROOT}/bin/windeployqt.exe"
                "${APP_NAME}"
        )

        # Add dependency to vgc_copy_python so that pythonXY.dll is copied to the 
        # build folder when building the app.
        #
        add_dependencies(${APP_TARGET} vgc_copy_python)
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
