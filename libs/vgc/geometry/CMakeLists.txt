vgc_add_library(geometry
    THIRD_DEPENDENCIES

    VGC_DEPENDENCIES
        core

    HEADER_FILES
        api.h
        bezier.h
        bezierspline.h
        bezierspline1d.h
        bezierspline2d.h
        camera2d.h
        catmullrom.h
        curve.h
        epsilon.h
        mat4d.h
        vec2d.h

    CPP_FILES
        camera2d.cpp
        curve.cpp
        mat4d.cpp
)

add_subdirectory(wraps)
