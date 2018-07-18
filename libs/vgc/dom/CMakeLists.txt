vgc_add_library(dom
    THIRD_DEPENDENCIES

    VGC_DEPENDENCIES
        core

    HEADER_FILES
        api.h
        attribute.h
        document.h
        element.h
        io.h
        node.h
        notaliveexception.h
        value.h
        xmlformattingstyle.h

    CPP_FILES
        attribute.cpp
        document.cpp
        element.cpp
        io.cpp
        node.cpp
        notaliveexception.cpp
        value.cpp
        xmlformattingstyle.cpp

    COMPILE_DEFINITIONS
)

add_subdirectory(wraps)
add_subdirectory(tests)
