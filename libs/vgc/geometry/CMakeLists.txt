vgc_add_library(geometry
    THIRD_DEPENDENCIES

    VGC_DEPENDENCIES
        core

    CPP_FILES
        point.cpp
        vector2d.h
)

add_subdirectory(wraps)
