vgc_add_library(geometry
    THIRD_DEPENDENCIES

    VGC_DEPENDENCIES
        core

    CPP_FILES
        bezierspline.h
        bezierspline1d.h
        bezierspline2d.h
        curve.cpp
        vec2d.h
)

add_subdirectory(wraps)
