vgc_add_library(style
    THIRD_DEPENDENCIES

    VGC_DEPENDENCIES
        core

    HEADER_FILES
        api.h
        logcategories.h
        metrics.h
        parse.h
        strings.h
        stylableobject.h
        style.h
        token.h
        types.h

    CPP_FILES
        logcategories.cpp
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
