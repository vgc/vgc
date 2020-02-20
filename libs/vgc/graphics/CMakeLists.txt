vgc_add_library(graphics
    VGC_DEPENDENCIES
        core

    HEADER_FILES
        api.h
        engine.h
        idgenerator.h

    CPP_FILES
        engine.cpp
        idgenerator.cpp

    RESOURCE_FILES
        opengl/iv4fcol.f.glsl
        opengl/iv4pos_iv4col_um4proj_um4view_ov4fcol.v.glsl
        opengl/shader.f.glsl
        opengl/shader.v.glsl
)
