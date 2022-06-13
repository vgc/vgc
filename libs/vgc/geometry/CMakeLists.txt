vgc_add_library(geometry
    THIRD_DEPENDENCIES
        libtess2

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
        curvecommand.h
        curves2d.h
        logcategories.h
        mat.h
        mat3d.h
        mat3f.h
        mat4d.h
        mat4f.h
        rect2d.h
        rect2f.h
        stride.h
        vec.h
        vec2d.h
        vec2f.h

    CPP_FILES
        camera2d.cpp
        curve.cpp
        curves2d.cpp
        logcategories.cpp
        mat3d.cpp
        mat3f.cpp
        mat4d.cpp
        mat4f.cpp
        rect2d.cpp
        rect2f.cpp
        vec2d.cpp
        vec2f.cpp

    NATVIS_FILES
        tools/vgcgeometry.natvis
)

add_subdirectory(wraps)
add_subdirectory(tests)
