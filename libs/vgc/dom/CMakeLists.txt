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
        pathelement.h
        positionsattribute.h
        vgcelement.h
        widthsattribute.h

    CPP_FILES
        attribute.cpp
        colorattribute.cpp
        document.cpp
        element.cpp
        node.cpp
        pathelement.cpp
        positionsattribute.cpp
        vgcelement.cpp
        widthsattribute.cpp

    COMPILE_DEFINITIONS
)

add_subdirectory(wraps)
