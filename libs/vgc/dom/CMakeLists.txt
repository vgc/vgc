vgc_add_library(dom
    THIRD_DEPENDENCIES

    VGC_DEPENDENCIES
        core

    HEADER_FILES
        api.h
        attribute.h
        colorattribute.h
        document.h
        element.h
        node.h
        path.h
        positionsattribute.h
        vgc.h
        widthsattribute.h

    CPP_FILES
        attribute.cpp
        colorattribute.cpp
        document.cpp
        element.cpp
        node.cpp
        path.cpp
        positionsattribute.cpp
        vgc.cpp
        widthsattribute.cpp

    COMPILE_DEFINITIONS
)

add_subdirectory(wraps)
