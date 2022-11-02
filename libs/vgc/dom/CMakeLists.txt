vgc_add_library(dom
    THIRD_DEPENDENCIES

    VGC_DEPENDENCIES
        geometry

    HEADER_FILES
        api.h
        attribute.h
        document.h
        element.h
        exceptions.h
        io.h
        logcategories.h
        node.h
        operation.h
        path.h
        schema.h
        strings.h
        value.h
        xmlformattingstyle.h

    CPP_FILES
        attribute.cpp
        document.cpp
        element.cpp
        exceptions.cpp
        io.cpp
        logcategories.cpp
        node.cpp
        operation.cpp
        path.cpp
        schema.cpp
        strings.cpp
        value.cpp
        xmlformattingstyle.cpp

    COMPILE_DEFINITIONS
)

add_subdirectory(wraps)
add_subdirectory(tests)
