vgc_add_library(geometry
    THIRD_DEPENDENCIES

    VGC_DEPENDENCIES
        core

    CPP_FILES
        vec2d.h
        bezierspline.h
)

add_subdirectory(wraps)
