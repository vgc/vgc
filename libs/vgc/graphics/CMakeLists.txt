vgc_add_library(graphics
    VGC_DEPENDENCIES
        core

    HEADER_FILES
        api.h
        engine.h

    CPP_FILES
        engine.cpp

    RESOURCE_FILES
        opengl/shader.f.glsl
        opengl/shader.v.glsl
)
