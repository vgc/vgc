vgc_add_library(geometry
    THIRD_DEPENDENCIES

    VGC_DEPENDENCIES
        core

    CPP_FILES
        point.cpp
        vec2d.h
        vector2d.h
        bezierspline.h
)

add_subdirectory(wraps)
