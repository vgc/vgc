vgc_add_library(canvas
    VGC_DEPENDENCIES
        ui
        workspace

    CPP_HEADER_FILES
        api.h
        canvas.h
        canvasmanager.h
        canvastool.h
        documentmanager.h
        logcategories.h
        strings.h
        toolmanager.h
        tooloptionspanel.h
        toolspanel.h

    CPP_SOURCE_FILES
        canvas.cpp
        canvasmanager.cpp
        canvastool.cpp
        documentmanager.cpp
        logcategories.cpp
        strings.cpp
        toolmanager.cpp
        tooloptionspanel.cpp
        toolspanel.cpp
)
