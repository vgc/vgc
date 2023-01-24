vgc_add_library(style
    THIRD_DEPENDENCIES

    VGC_DEPENDENCIES
        core

    HEADER_FILES
        api.h
        logcategories.h
        metrics.h
        parse.h
        sheet.h
        spec.h
        strings.h
        stylableobject.h
        token.h
        types.h
        value.h

    CPP_FILES
        logcategories.cpp
        parse.cpp
        sheet.cpp
        spec.cpp
        strings.cpp
        stylableobject.cpp
        token.cpp
        types.cpp
        value.cpp

    COMPILE_DEFINITIONS
)

add_subdirectory(wraps)
add_subdirectory(tests)
