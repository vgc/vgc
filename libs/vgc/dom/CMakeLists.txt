vgc_add_library(dom
    THIRD_DEPENDENCIES

    VGC_DEPENDENCIES
        core

    HEADER_FILES
        api.h
        attribute.h
        document.h
        element.h
        node.h
        path.h
        value.h
        vgc.h

    CPP_FILES
        attribute.cpp
        document.cpp
        element.cpp
        node.cpp
        path.cpp
        value.cpp
        vgc.cpp

    COMPILE_DEFINITIONS
)

add_subdirectory(wraps)
