vgc_add_library(graphics
    THIRD_DEPENDENCIES
        ${VGC_QT_CORE}
        ${VGC_QT_OPENGL}
        Freetype::Freetype
        harfbuzz::harfbuzz

    VGC_DEPENDENCIES
        geometry
        style

    HEADER_FILES
        api.h
        engine.h
        exceptions.h
        font.h
        idgenerator.h
        logcategories.h
        text.h

    CPP_FILES
        engine.cpp
        exceptions.cpp
        font.cpp
        idgenerator.cpp
        logcategories.cpp
        text.cpp

    RESOURCE_FILES
        fonts/SourceSansPro/LICENSE.md
        fonts/SourceSansPro/TTF/SourceSansPro-Black.ttf
        fonts/SourceSansPro/TTF/SourceSansPro-BlackIt.ttf
        fonts/SourceSansPro/TTF/SourceSansPro-Bold.ttf
        fonts/SourceSansPro/TTF/SourceSansPro-BoldIt.ttf
        fonts/SourceSansPro/TTF/SourceSansPro-ExtraLight.ttf
        fonts/SourceSansPro/TTF/SourceSansPro-ExtraLightIt.ttf
        fonts/SourceSansPro/TTF/SourceSansPro-It.ttf
        fonts/SourceSansPro/TTF/SourceSansPro-Light.ttf
        fonts/SourceSansPro/TTF/SourceSansPro-LightIt.ttf
        fonts/SourceSansPro/TTF/SourceSansPro-Regular.ttf
        fonts/SourceSansPro/TTF/SourceSansPro-Semibold.ttf
        fonts/SourceSansPro/TTF/SourceSansPro-SemiboldIt.ttf
        opengl/iv4fcol.f.glsl
        opengl/iv4pos_iv4col_um4proj_um4view_ov4fcol.v.glsl
        opengl/shader.f.glsl
        opengl/shader.v.glsl
)

add_subdirectory(wraps)
add_subdirectory(tests)
