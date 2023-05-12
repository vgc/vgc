vgc_add_library(canvas
    VGC_DEPENDENCIES
        ui
        workspace

    CPP_HEADER_FILES
        api.h
        canvas.h
        canvastool.h
        logcategories.h
        strings.h

    CPP_SOURCE_FILES
        canvas.cpp
        canvastool.cpp
        logcategories.cpp
        strings.cpp
)
