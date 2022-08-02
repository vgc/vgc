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

#include <vgc/core/arithmetic.h>
#include <vgc/core/enum.h>

namespace vgc::graphics {

// See https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_bind_flag
//
enum class BindFlag : UInt16 {
    None = 0,
    VertexBuffer = 1,
    IndexBuffer = 2,
    ConstantBuffer = 4,
    ShaderResource = 8,
    RenderTarget = 0x10,
    DepthStencil = 0x20,
    StreamOutput = 0x40,
    UnorderedAccess = 0x80,
};
VGC_DEFINE_FLAGS(BindFlags, BindFlag)

/// Subset of `BindFlags` compatible with images.
//
enum class ImageBindFlag : UInt16 {
    None = 0,
    ShaderResource = 8,
    RenderTarget = 0x10,
    DepthStencil = 0x20,
    UnorderedAccess = 0x80,
};
VGC_DEFINE_FLAGS(ImageBindFlags, ImageBindFlag)

// See https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_resource_misc_flag
//
enum class ResourceMiscFlag : UInt32 {
    None = 0,

    /// Enables resource sharing between compatible engines.
    /// Unsupported at the moment.
    ///
    Shared = 2,

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
    None = 0,
    Read = 1,
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
    Point,
    LineList,
    LineStrip,
    TriangleList,
    TriangleStrip,
};

enum class PresentFlag : UInt32 {
    None,
};
VGC_DEFINE_FLAGS(PresentFlags, PresentFlag)

enum class ImageRank : UInt8 {
    _1D,
    _2D,
    // XXX future
    //_3D,
    //_CubeMap, // OpenGL doesn't support cubemap textures from 2d image array.
};

enum class ImageFormat : UInt8 {
    Unknown,
    // Depth
    D_16_UNORM,
    D_32_FLOAT,
    // Depth + Stencil
    DS_24_UNORM_8_UINT,
    DS_32_FLOAT_8_UINT_24_X,
    // Red
    R_8_UNORM,
    R_8_SNORM,
    R_8_UINT,
    R_8_SINT,
    R_16_UNORM,
    R_16_SNORM,
    R_16_UINT,
    R_16_SINT,
    R_16_FLOAT,
    R_32_UINT,
    R_32_SINT,
    R_32_FLOAT,
    // RG
    RG_8_UNORM,
    RG_8_SNORM,
    RG_8_UINT,
    RG_8_SINT,
    RG_16_UNORM,
    RG_16_SNORM,
    RG_16_UINT,
    RG_16_SINT,
    RG_16_FLOAT,
    RG_32_UINT,
    RG_32_SINT,
    RG_32_FLOAT,
    // RGB
    RGB_11_11_10_FLOAT,
    RGB_32_UINT,
    RGB_32_SINT,
    RGB_32_FLOAT,
    // RGBA
    RGBA_8_UNORM,
    RGBA_8_UNORM_SRGB,
    RGBA_8_SNORM,
    RGBA_8_UINT,
    RGBA_8_SINT,
    RGBA_10_10_10_2_UNORM,
    RGBA_10_10_10_2_UINT,
    RGBA_16_UNORM,
    RGBA_16_UINT,
    RGBA_16_SINT,
    RGBA_16_FLOAT,
    RGBA_32_UINT,
    RGBA_32_SINT,
    RGBA_32_FLOAT,
    // XXX future
    // Compressed
    //BC1_UNORM,
    //BC1_UNORM_SRGB,
    //BC2_UNORM,
    //BC2_UNORM_SRGB,
    //BC3_UNORM,
    //BC3_UNORM_SRGB,
    //BC4_UNORM,
    //BC4_SNORM,
    //BC5_UNORM,
    //BC5_SNORM,
    //BC7_UNORM,
    //BC7_UNORM_SRGB,
};

enum class SwapChainTargetFormat : UInt8 {
    RGBA_8_UNORM = core::toUnderlying(ImageFormat::RGBA_8_UNORM),
    RGBA_8_UNORM_SRGB = core::toUnderlying(ImageFormat::RGBA_8_UNORM_SRGB),
};

inline constexpr ImageFormat swapChainTargetFormatToImageFormat(SwapChainTargetFormat format)
{
    return static_cast<ImageFormat>(core::toUnderlying(format));
}

inline constexpr UInt8 imageFormatToElementSizeInBytes(ImageFormat format)
{
    switch (format) {
    case ImageFormat::D_16_UNORM:               return 2;
    case ImageFormat::D_32_FLOAT:               return 4;
    case ImageFormat::DS_24_UNORM_8_UINT:       return 4;
    case ImageFormat::DS_32_FLOAT_8_UINT_24_X:  return 8;
    case ImageFormat::R_8_UNORM:                return 1;
    case ImageFormat::R_8_SNORM:                return 1;
    case ImageFormat::R_8_UINT:                 return 1;
    case ImageFormat::R_8_SINT:                 return 1;
    case ImageFormat::R_16_UNORM:               return 2;
    case ImageFormat::R_16_SNORM:               return 2;
    case ImageFormat::R_16_UINT:                return 2;
    case ImageFormat::R_16_SINT:                return 2;
    case ImageFormat::R_16_FLOAT:               return 2;
    case ImageFormat::R_32_UINT:                return 4;
    case ImageFormat::R_32_SINT:                return 4;
    case ImageFormat::R_32_FLOAT:               return 4;
    case ImageFormat::RG_8_UNORM:               return 2;
    case ImageFormat::RG_8_SNORM:               return 2;
    case ImageFormat::RG_8_UINT:                return 2;
    case ImageFormat::RG_8_SINT:                return 2;
    case ImageFormat::RG_16_UNORM:              return 4;
    case ImageFormat::RG_16_SNORM:              return 4;
    case ImageFormat::RG_16_UINT:               return 4;
    case ImageFormat::RG_16_SINT:               return 4;
    case ImageFormat::RG_16_FLOAT:              return 4;
    case ImageFormat::RG_32_UINT:               return 8;
    case ImageFormat::RG_32_SINT:               return 8;
    case ImageFormat::RG_32_FLOAT:              return 8;
    case ImageFormat::RGB_11_11_10_FLOAT:       return 12;
    case ImageFormat::RGB_32_UINT:              return 12;
    case ImageFormat::RGB_32_SINT:              return 12;
    case ImageFormat::RGB_32_FLOAT:             return 12;
    case ImageFormat::RGBA_8_UNORM:             return 4;
    case ImageFormat::RGBA_8_UNORM_SRGB:        return 4;
    case ImageFormat::RGBA_8_SNORM:             return 4;
    case ImageFormat::RGBA_8_UINT:              return 4;
    case ImageFormat::RGBA_8_SINT:              return 4;
    case ImageFormat::RGBA_10_10_10_2_UNORM:    return 4;
    case ImageFormat::RGBA_10_10_10_2_UINT:     return 4;
    case ImageFormat::RGBA_16_UNORM:            return 8;
    case ImageFormat::RGBA_16_UINT:             return 8;
    case ImageFormat::RGBA_16_SINT:             return 8;
    case ImageFormat::RGBA_16_FLOAT:            return 8;
    case ImageFormat::RGBA_32_UINT:             return 16;
    case ImageFormat::RGBA_32_SINT:             return 16;
    case ImageFormat::RGBA_32_FLOAT:            return 16;
    default:
        break;
    }
    return 1;
}

enum class ImageWrapMode : UInt8 {
    Undefined,
    Repeat,
    MirrorRepeat,
    Clamp,
    ClampToConstantColor,
    // requires OpenGL 4.4
    //MirrorClamp,
    Max_ = ClampToConstantColor,
};
inline constexpr UInt8 numImageWrapModes = static_cast<UInt8>(ImageWrapMode::Max_) + 1;

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
    Max_ = GreaterEqual,
};
inline constexpr UInt8 numComparisonFunctions = static_cast<UInt8>(ComparisonFunction::Max_) + 1;

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
    Max_ = OneMinusSecondSourceAlpha,
};
inline constexpr UInt8 numBlendFactors = static_cast<UInt8>(BlendFactor::Max_) + 1;

enum class BlendOp : UInt8 {
    Undefined,
    Add,
    SourceMinusTarget,
    TargetMinusSource,
    Min,
    Max,
    Max_ = Max,
};
inline constexpr UInt8 numBlendOps = static_cast<UInt8>(BlendOp::Max_) + 1;

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
    Max_ = Wireframe,
};
inline constexpr UInt8 numFillModes = static_cast<UInt8>(FillMode::Max_) + 1;

enum class CullMode : UInt8 {
    Undefined,
    None,
    Front,
    Back,
    Max_ = Back,
};
inline constexpr UInt8 numCullModes = static_cast<UInt8>(CullMode::Max_) + 1;

enum class FilterMode : UInt8 {
    Undefined,
    Point,
    Linear,
    Max_ = Linear,
};
inline constexpr UInt8 numFilterModes = static_cast<UInt8>(FilterMode::Max_) + 1;

enum class ShaderStage : Int8 {
    None = -1,
    Vertex = 0,
    Geometry = 1,
    Pixel = 2,
    //Hull,
    //Domain,
    //Compute,
    Max_ = Pixel,
};
inline constexpr UInt8 numShaderStages = static_cast<UInt8>(ShaderStage::Max_) + 1;

enum class BuiltinProgram : UInt8 {
    Simple,
    // XXX publicize ?
    //GlyphAtlas,
    //IconsAtlas,
    //RoundedRectangle,
};

enum class BuiltinGeometryLayout : Int8 {
    None = -1,
    XY = 0,
    XYRGB = 1,
    XYZ = 2,
    Max_ = XYZ,
};

enum class PipelineParameter : UInt32 {
    None = 0,

    Framebuffer                     = 0x00000001,
    Viewport                        = 0x00000002,
    Program                         = 0x00000004,
    BlendState                      = 0x00000008,
    DepthStencilState               = 0x00000010,
    RasterizerState                 = 0x00000020,

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

    All = Viewport | Program | BlendState | DepthStencilState | RasterizerState | AllShadersResources,
};
VGC_DEFINE_FLAGS(PipelineParameters, PipelineParameter)

} // namespace vgc::graphics

#endif // VGC_GRAPHICS_ENUMS_H
