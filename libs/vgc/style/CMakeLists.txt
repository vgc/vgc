vgc_add_library(style
    THIRD_DEPENDENCIES

    VGC_DEPENDENCIES
        core

    HEADER_FILES
        api.h
        parse.h
        strings.h
        stylableobject.h
        style.h
        token.h
        types.h

    CPP_FILES
        parse.cpp
        strings.cpp
        stylableobject.cpp
        style.cpp
        token.cpp
        types.cpp

    COMPILE_DEFINITIONS
)

add_subdirectory(wraps)
add_subdirectory(tests)
