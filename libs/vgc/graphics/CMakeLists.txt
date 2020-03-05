vgc_add_library(graphics
    THIRD_DEPENDENCIES
        ${FREETYPE_LIBRARIES}

    VGC_DEPENDENCIES
        core

    HEADER_FILES
        api.h
        engine.h
        exceptions.h
        font.h
        idgenerator.h

    CPP_FILES
        engine.cpp
        font.cpp
        idgenerator.cpp

    RESOURCE_FILES
        opengl/iv4fcol.f.glsl
        opengl/iv4pos_iv4col_um4proj_um4view_ov4fcol.v.glsl
        opengl/shader.f.glsl
        opengl/shader.v.glsl
)

add_subdirectory(wraps)
add_subdirectory(tests)
