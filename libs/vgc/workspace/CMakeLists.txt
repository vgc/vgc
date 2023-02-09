vgc_add_library(workspace
    VGC_DEPENDENCIES
        dom
        graphics
        topology

    HEADER_FILES
        api.h
        edge.h
        element.h
        layer.h
        logcategories.h
        vertex.h
        workspace.h

    CPP_FILES
        edge.cpp
        element.cpp
        layer.cpp
        logcategories.cpp
        vertex.cpp
        workspace.cpp
)
