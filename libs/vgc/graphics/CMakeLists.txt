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
        batch.h
        blendstate.h
        buffer.h
        cliprect.h
        colorgradient.h
        constants.h
        engine.h
        enums.h
        exceptions.h
        font.h
        framebuffer.h
        geometryview.h
        idgenerator.h
        image.h
        imageview.h
        logcategories.h
        program.h
        rasterizerstate.h
        resource.h
        richtext.h
        samplerstate.h
        strings.h
        swapchain.h
        text.h

        d3d11/d3d11engine.h

        detail/command.h
        detail/comptr.h
        detail/pipelinestate.h
        detail/pixelformats.h

    CPP_FILES
        engine.cpp
        exceptions.cpp
        font.cpp
        idgenerator.cpp
        image.cpp
        logcategories.cpp
        richtext.cpp
        strings.cpp
        text.cpp

        d3d11/d3d11engine.cpp

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
        shaders/opengl/shader.f.glsl
        shaders/opengl/shader.v.glsl
        shaders/opengl/simple.f.glsl
        shaders/opengl/simple.v.glsl
        shaders/opengl/simple_textured.f.glsl
        shaders/opengl/simple_textured.v.glsl
        shaders/d3d11/simple.f.hlsl
        shaders/d3d11/simple.v.hlsl
        shaders/d3d11/simple_textured.f.hlsl
        shaders/d3d11/simple_textured.v.hlsl
)

add_subdirectory(wraps)
add_subdirectory(tests)
