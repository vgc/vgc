vgc_add_library(graphics
    THIRD_DEPENDENCIES
        ${VGC_QT_CORE}
        Freetype::Freetype
        harfbuzz::harfbuzz

    VGC_DEPENDENCIES
        geometry
        style

    CPP_HEADER_FILES
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
        icon.h
        image.h
        imageview.h
        logcategories.h
        program.h
        rasterizerstate.h
        resource.h
        richtext.h
        samplerstate.h
        strings.h
        svg.h
        swapchain.h
        text.h

        d3d11/d3d11engine.h

        detail/command.h
        detail/comptr.h
        detail/pipelinestate.h
        detail/pixelformats.h
        detail/shapeutil.h

    CPP_SOURCE_FILES
        engine.cpp
        exceptions.cpp
        font.cpp
        icon.cpp
        image.cpp
        logcategories.cpp
        richtext.cpp
        strings.cpp
        svg.cpp
        text.cpp

        d3d11/d3d11engine.cpp

        detail/shapeutil.cpp

    RESOURCE_FILES
        fonts/SourceSansPro/LICENSE.md
        fonts/SourceSansPro/TTF/SourceSansPro-Regular.ttf # Kept for test_font.py
        fonts/Roboto/LICENSE.txt
        fonts/Roboto/Roboto-BlackItalic.ttf
        fonts/Roboto/Roboto-Black.ttf
        fonts/Roboto/Roboto-BoldItalic.ttf
        fonts/Roboto/Roboto-Bold.ttf
        fonts/Roboto/Roboto-Italic.ttf
        fonts/Roboto/Roboto-LightItalic.ttf
        fonts/Roboto/Roboto-Light.ttf
        fonts/Roboto/Roboto-MediumItalic.ttf
        fonts/Roboto/Roboto-Medium.ttf
        fonts/Roboto/Roboto-Regular.ttf
        fonts/Roboto/Roboto-ThinItalic.ttf
        fonts/Roboto/Roboto-Thin.ttf
        shaders/d3d11/screen_space_displacement.v.hlsl
        shaders/d3d11/simple.f.hlsl
        shaders/d3d11/simple.v.hlsl
        shaders/d3d11/simple_preview.f.hlsl
        shaders/d3d11/simple_textured.f.hlsl
        shaders/d3d11/simple_textured.v.hlsl
        shaders/d3d11/simple_textured_debug.f.hlsl
        shaders/opengl/screen_space_displacement.v.glsl
        shaders/opengl/shader.f.glsl
        shaders/opengl/shader.v.glsl
        shaders/opengl/simple.f.glsl
        shaders/opengl/simple.v.glsl
        shaders/opengl/simple_preview.f.glsl
        shaders/opengl/simple_textured.f.glsl
        shaders/opengl/simple_textured.v.glsl
        shaders/opengl/simple_textured_debug.f.glsl
)

add_subdirectory(wraps)
add_subdirectory(tests)
