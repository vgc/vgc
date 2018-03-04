vgc_add_library(dom
    THIRD_DEPENDENCIES

    VGC_DEPENDENCIES
        core

    HEADER_FILES
        api.h
        document.h

    CPP_FILES
        document.cpp

    COMPILE_DEFINITIONS
)

add_subdirectory(wraps)
