vgc_add_library(tools
    VGC_DEPENDENCIES
        canvas

    CPP_HEADER_FILES
        api.h
        colorpalette.h
        logcategories.h
        paintbucket.h
        sculpt.h
        select.h
        sketch.h
        strings.h

    CPP_SOURCE_FILES
        colorpalette.cpp
        logcategories.cpp
        paintbucket.cpp
        sculpt.cpp
        select.cpp
        sketch.cpp
        strings.cpp
)
