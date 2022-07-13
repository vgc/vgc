vgc_add_library(style
    THIRD_DEPENDENCIES

    VGC_DEPENDENCIES
        core

    HEADER_FILES
        api.h
        strings.h
        stylableobject.h
        style.h
        token.h

    CPP_FILES
        strings.cpp
        stylableobject.cpp
        style.cpp
        token.cpp

    COMPILE_DEFINITIONS
)

add_subdirectory(wraps)
add_subdirectory(tests)
