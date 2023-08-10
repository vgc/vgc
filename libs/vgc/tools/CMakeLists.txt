vgc_add_library(tools
    THIRD_DEPENDENCIES
        ${VGC_QT_SVG}

    VGC_DEPENDENCIES
        canvas

    CPP_HEADER_FILES
        api.h
        colorpalette.h
        copypaste.h
        logcategories.h
        paintbucket.h
        sculpt.h
        select.h
        sketch.h
        strings.h
        topology.h
        transform.h

    CPP_SOURCE_FILES
        colorpalette.cpp
        copypaste.cpp
        logcategories.cpp
        paintbucket.cpp
        sculpt.cpp
        select.cpp
        sketch.cpp
        strings.cpp
        topology.cpp
        transform.cpp

    RESOURCE_FILES
        cursors/rotation.svg
        cursors/scaling.svg
        icons/paintBucket.svg
        icons/sculpt.svg
        icons/select.svg
        icons/sketch.svg
)
