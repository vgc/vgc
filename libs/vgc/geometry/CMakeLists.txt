vgc_add_library(geometry
    THIRD_DEPENDENCIES

    VGC_DEPENDENCIES
        core

    CPP_FILES
        bezier.h
        bezierspline.h
        bezierspline1d.h
        bezierspline2d.h
        camera2d.cpp
        catmullrom.h
        curve.cpp
        epsilon.h
        mat4d.cpp
        vec2d.h
)

add_subdirectory(wraps)
