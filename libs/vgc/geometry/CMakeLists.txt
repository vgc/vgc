vgc_add_library(geometry
    THIRD_DEPENDENCIES

    VGC_DEPENDENCIES
        core

    CPP_FILES
        bezier.h
        bezierspline.h
        bezierspline1d.h
        bezierspline2d.h
        curve.cpp
        epsilon.h
        vec2d.h
)

add_subdirectory(wraps)
