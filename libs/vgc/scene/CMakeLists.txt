vgc_add_library(scene
    THIRD_DEPENDENCIES
        Qt5::Core

    VGC_DEPENDENCIES
        geometry

    CPP_FILES
        scene.cpp
)

add_subdirectory(wraps)
