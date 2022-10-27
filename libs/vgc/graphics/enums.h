// Copyright 2022 The VGC Developers
// See the COPYRIGHT file at the top-level directory of this distribution
// and at https://github.com/vgc/vgc/blob/master/COPYRIGHT
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef VGC_GRAPHICS_ENUMS_H
#define VGC_GRAPHICS_ENUMS_H

#include <type_traits>

#include <vgc/core/arithmetic.h>
#include <vgc/core/enum.h>
#include <vgc/core/flags.h>

namespace vgc::graphics {

// clang-format off

enum class FrameKind : UInt8 {
    Window,
    QWidget,
};

enum class PresentFlag : UInt32 {
    None,
};
VGC_DEFINE_FLAGS(PresentFlags, PresentFlag)

// See https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_bind_flag
//
enum class BindFlag : UInt16 {
    None            = 0x00,
    VertexBuffer    = 0x01,
    IndexBuffer     = 0x02,
    ConstantBuffer  = 0x04,
    ShaderResource  = 0x08,
    RenderTarget    = 0x10,
    DepthStencil    = 0x20,
    StreamOutput    = 0x40,
    UnorderedAccess = 0x80,
};
VGC_DEFINE_FLAGS(BindFlags, BindFlag)

/// Subset of `BindFlags` compatible with images.
//
enum class ImageBindFlag : UInt16 {
    None            = 0x00,
    ShaderResource  = 0x08,
    RenderTarget    = 0x10,
    DepthStencil    = 0x20,
    UnorderedAccess = 0x80,
};
VGC_DEFINE_FLAGS(ImageBindFlags, ImageBindFlag)

// See https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_resource_misc_flag
//
enum class ResourceMiscFlag : UInt32 {
    None = 0x00,

    /// Enables generation of mipmaps.
    ///
    GenerateMips = 0x01,

    /// Enables resource sharing between compatible engines.
    /// Unsupported at the moment.
    ///
    Shared = 0x02,

    // requires OpenGL 4.0 / ES 3.1
    //DrawIndirectArgs = 0x10,

    // requires OpenGL 4.3 / ES 3.1
    //BufferRaw = 0x20,

    // requires OpenGL 4.3 / ES 3.1
    //BufferStructured = 0x40,

    //ResourceClamp = 0x80,
    //SharedKeyedMutex = 0x100,
};
VGC_DEFINE_FLAGS(ResourceMiscFlags, ResourceMiscFlag)

// See https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_cpu_access_flag
//
enum class CpuAccessFlag : UInt8 {
    None  = 0,
    Read  = 1,
    Write = 2,
};
VGC_DEFINE_FLAGS(CpuAccessFlags, CpuAccessFlag)

// See https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_usage
//
enum class Usage : UInt8 {
    Default,
    Immutable,
    Dynamic,
    Staging,
    VGC_ENUM_ENDMAX
};

// See https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_cpu_access_flag
//
enum class Mapping : UInt8 {
    None,
    Read,
    Write,
    ReadWrite,
    WriteDiscard,
    WriteNoOverwrite,
};

enum class PrimitiveType : UInt8 {
    Undefined,
    Point,
    LineList,
    LineStrip,
    TriangleList,
    TriangleStrip,
    VGC_ENUM_ENDMAX
};

enum class ImageRank : UInt8 {
    _1D,
    _2D,
    // XXX future
    //_3D,
    //_CubeMap, // OpenGL doesn't support cubemap textures from 2d image array.
    VGC_ENUM_ENDMAX
};

enum class PixelFormat : UInt8 {
#define VGC_PIXEL_FORMAT_MACRO_(Enumerator, ElemSizeInBytes, DXGIFormat, OpenGLInternalFormat, OpenGLPixelType, OpenGLPixelFormat) \
    Enumerator,
#include <vgc/graphics/detail/pixelformats.h>
    VGC_ENUM_ENDMAX
};

namespace detail {

    template <PixelFormat Format>
    struct pixelFormatElementSizeInBytes_;

#define VGC_PIXEL_FORMAT_MACRO_(Enumerator, ElemSizeInBytes, DXGIFormat, OpenGLInternalFormat, OpenGLPixelType, OpenGLPixelFormat) \
    template <> \
    struct pixelFormatElementSizeInBytes_<PixelFormat::Enumerator>\
        : std::integral_constant<UInt8, ElemSizeInBytes> {};
#include <vgc/graphics/detail/pixelformats.h>

} // namespace detail

template<PixelFormat Format>
inline constexpr UInt8 pixelFormatElementSizeInBytes = detail::pixelFormatElementSizeInBytes_<Format>::value;

enum class WindowPixelFormat : UInt8 {
    RGBA_8_UNORM = core::toUnderlying(PixelFormat::RGBA_8_UNORM),
    RGBA_8_UNORM_SRGB = core::toUnderlying(PixelFormat::RGBA_8_UNORM_SRGB),
};

inline constexpr PixelFormat windowPixelFormatToPixelFormat(WindowPixelFormat format) {
    return static_cast<PixelFormat>(core::toUnderlying(format));
}

enum class ImageWrapMode : UInt8 {
    Undefined,
    Repeat,
    MirrorRepeat,
    Clamp,
    ClampToConstantColor,
    // requires OpenGL 4.4
    //MirrorClamp,
    VGC_ENUM_ENDMAX
};

enum class ComparisonFunction : UInt8 {
    Undefined,
    Disabled,
    Always,
    Never,
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    VGC_ENUM_ENDMAX
};

enum class BlendFactor : UInt8 {
    Undefined,
    One,
    Zero,
    SourceColor,
    OneMinusSourceColor,
    SourceAlpha,
    OneMinusSourceAlpha,
    TargetColor,
    OneMinusTargetColor,
    TargetAlpha,
    OneMinusTargetAlpha,
    SourceAlphaSaturated,
    Constant,
    OneMinusConstant,
    SecondSourceColor,
    OneMinusSecondSourceColor,
    SecondSourceAlpha,
    OneMinusSecondSourceAlpha,
    VGC_ENUM_ENDMAX
};

enum class BlendOp : UInt8 {
    Undefined,
    Add,
    SourceMinusTarget,
    TargetMinusSource,
    Min,
    Max,
    VGC_ENUM_ENDMAX
};

enum class BlendWriteMaskBit : UInt8 {
    None = 0,
    R = 1,
    G = 2,
    B = 4,
    A = 8,
    RGB = R | G | B,
    RGBA = RGB | A,
    All = RGBA,
};
VGC_DEFINE_FLAGS(BlendWriteMask, BlendWriteMaskBit)

enum class FillMode : UInt8 {
    Undefined,
    Solid,
    Wireframe,
    VGC_ENUM_ENDMAX
};

enum class CullMode : UInt8 {
    Undefined,
    None,
    Front,
    Back,
    VGC_ENUM_ENDMAX
};

enum class FilterMode : UInt8 {
    Undefined,
    Point,
    Linear,
    VGC_ENUM_ENDMAX
};

enum class ShaderStage : Int8 {
    None = -1,
    Vertex = 0,
    Geometry,
    Pixel,
    //Hull,
    //Domain,
    //Compute,
    VGC_ENUM_ENDMAX
};
inline constexpr auto numShaderStages = VGC_ENUM_COUNT(ShaderStage);

enum class BuiltinProgram : Int8 {
    NotBuiltin = -1,
    Simple = 0,
    SimpleTextured,
    SreenSpaceDisplacement,
    // XXX publicize ?
    //GlyphAtlas,
    //IconsAtlas,
    //RoundedRectangle,
    VGC_ENUM_ENDMAX
};
inline constexpr auto numBuiltinPrograms = VGC_ENUM_COUNT(BuiltinProgram);

enum class BuiltinGeometryLayout : Int8 {
    NotBuiltin = -1,
    XY = 0,
    XYRGB,
    XYRGBA,
    XYUVRGBA,
    XY_iRGBA,
    XYUV_iRGBA,
    // Rot is a "float" boolean to enable the rotation of the displacement by the view matrix.
    XYDxDy_iXYRotRGBA,
    VGC_ENUM_ENDMAX
};
inline constexpr auto numBuiltinGeometryLayouts = VGC_ENUM_COUNT(BuiltinGeometryLayout);

enum class IndexFormat : UInt8 {
    Undefined,
    None,
    UInt16,
    UInt32,
    VGC_ENUM_ENDMAX
};

namespace detail {

template<IndexFormat Format>
struct IndexFormatType_ {};
template<>
struct IndexFormatType_<IndexFormat::UInt16> {
    using type = UInt16;
};
template<>
struct IndexFormatType_<IndexFormat::UInt32> {
    using type = UInt32;
};

} // namespace detail

template<IndexFormat Format>
using IndexFormatType = typename detail::IndexFormatType_<Format>::type;

enum class PipelineParameter : UInt32 {
    None = 0,

    Framebuffer                     = 0x00000001,
    Viewport                        = 0x00000002,
    Program                         = 0x00000004,
    BlendState                      = 0x00000008,
    DepthStencilState               = 0x00000010,
    RasterizerState                 = 0x00000020,
    ScissorRect                     = 0x00000040,

    VertexShaderConstantBuffers     = 0x00001000,
    VertexShaderImageViews          = 0x00010000,
    VertexShaderSamplers            = 0x00100000,

    GeometryShaderConstantBuffers   = 0x00002000,
    GeometryShaderImageViews        = 0x00020000,
    GeometryShaderSamplers          = 0x00200000,

    PixelShaderConstantBuffers      = 0x00004000,
    PixelShaderImageViews           = 0x00040000,
    PixelShaderSamplers             = 0x00400000,

    VertexShaderResources       = VertexShaderConstantBuffers   | VertexShaderImageViews    | VertexShaderSamplers,
    GeometryShaderResources     = GeometryShaderConstantBuffers | GeometryShaderImageViews  | GeometryShaderSamplers,
    PixelShaderResources        = PixelShaderConstantBuffers    | PixelShaderImageViews     | PixelShaderSamplers,

    AllShadersConstantBuffers   = VertexShaderConstantBuffers   | GeometryShaderConstantBuffers | PixelShaderConstantBuffers,
    AllShadersImageViews        = VertexShaderImageViews        | GeometryShaderImageViews      | PixelShaderImageViews,
    AllShadersSamplers          = VertexShaderSamplers          | GeometryShaderSamplers        | PixelShaderSamplers,

    AllShadersResources = AllShadersConstantBuffers | AllShadersImageViews | AllShadersSamplers,

    All = Framebuffer | Viewport | Program | BlendState | DepthStencilState | RasterizerState | ScissorRect | AllShadersResources,
};
VGC_DEFINE_FLAGS(PipelineParameters, PipelineParameter)

// clang-format on

} // namespace vgc::graphics

#endif // VGC_GRAPHICS_ENUMS_H