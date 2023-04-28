vgc_add_library(workspace
    VGC_DEPENDENCIES
        dom
        graphics
        vacomplex

    CPP_HEADER_FILES
        api.h
        colors.h
        edge.h
        element.h
        face.h
        layer.h
        logcategories.h
        vertex.h
        workspace.h

    CPP_SOURCE_FILES
        edge.cpp
        element.cpp
        face.cpp
        layer.cpp
        logcategories.cpp
        vertex.cpp
        workspace.cpp
)
