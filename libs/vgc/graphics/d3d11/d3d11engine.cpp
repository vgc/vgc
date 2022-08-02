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

#include <vgc/core/compiler.h>
#ifdef VGC_CORE_COMPILER_MSVC

#include <vgc/graphics/d3d11/d3d11engine.h>

#include <algorithm>
#include <array>
#include <chrono>

#include <d3d11.h>
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler")
#pragma comment(lib, "d3d11")

#include <vgc/core/exceptions.h>

namespace vgc::graphics {

namespace {

// core::resourcePath("graphics/d3d11/" + name);

struct XYRGBVertex {
    float x, y, r, g, b;
};

} // namespace

class D3d11ImageView;
class D3d11Framebuffer;

class D3d11Buffer : public Buffer {
protected:
    friend D3d11Engine;
    using Buffer::Buffer;

public:
    ID3D11Buffer* object() const
    {
        return object_.get();
    }

    const D3D11_BUFFER_DESC& desc() const
    {
        return desc_;
    }

protected:
    void release_(Engine* engine) override;

private:
    ComPtr<ID3D11Buffer> object_;
    D3D11_BUFFER_DESC desc_ = {};

    std::array<bool, numShaderStages> isBoundToD3dStage_ = {};

    friend D3d11ImageView;
    core::Array<D3d11ImageView*> dependentD3dImageViews_;
};
using D3d11BufferPtr = ResourcePtr<D3d11Buffer>;

class D3d11Image : public Image {
protected:
    friend D3d11Engine;
    using Image::Image;

public:
    ID3D11Resource* object() const
    {
        return object_.get();
    }

    DXGI_FORMAT dxgiFormat() const
    {
        return dxgiFormat_;
    }

    // resizing a swap chain requires releasing all views to its back buffers.
    void forceReleaseD3dObject()
    {
        object_.reset();
    }

protected:
    void release_(Engine* engine) override
    {
        Image::release_(engine);
        object_.reset();
    }

private:
    ComPtr<ID3D11Resource> object_;
    DXGI_FORMAT dxgiFormat_ = {};
};
using D3d11ImagePtr = ResourcePtr<D3d11Image>;

class D3d11ImageView : public ImageView {
protected:
    friend D3d11Engine;

    D3d11ImageView(ResourceRegistry* registry,
              const ImageViewCreateInfo& createInfo,
              const ImagePtr& image)
        : ImageView(registry, createInfo, image) {
    }

    D3d11ImageView(ResourceRegistry* registry,
                   const ImageViewCreateInfo& createInfo,
                   const BufferPtr& buffer,
                   ImageFormat format,
                   UInt32 numBufferElements)
        : ImageView(registry, createInfo, buffer, format, numBufferElements) {

        d3dBuffer_ = buffer.get_static_cast<D3d11Buffer>();
        d3dBuffer_->dependentD3dImageViews_.append(this);
    }

public:
    ID3D11ShaderResourceView* srvObject() const
    {
        return srv_.get();
    }

    ID3D11RenderTargetView* rtvObject() const
    {
        return rtv_.get();
    }

    ID3D11DepthStencilView* dsvObject() const
    {
        return dsv_.get();
    }

    DXGI_FORMAT dxgiFormat() const
    {
        return dxgiFormat_;
    }

    // resizing a swap chain requires releasing all views to its back buffers.
    // XXX and forceSet after the resize ?
    void forceReleaseD3dObject()
    {
        D3d11Image* d3dImage = viewedImage().get_static_cast<D3d11Image>();
        if (d3dImage) {
            d3dImage->forceReleaseD3dObject();
        }
        srv_.reset();
        rtv_.reset();
        dsv_.reset();
    }

    ID3D11Resource* d3dViewedResource() const
    {
        D3d11Buffer* d3dBuffer = viewedBuffer().get_static_cast<D3d11Buffer>();
        if (d3dBuffer) {
            return d3dBuffer->object();
        }
        else {
            D3d11Image* d3dImage = viewedImage().get_static_cast<D3d11Image>();
            return d3dImage->object();
        }
    }

protected:
    void release_(Engine* engine) override;

private:
    ComPtr<ID3D11ShaderResourceView> srv_;
    ComPtr<ID3D11RenderTargetView> rtv_;
    ComPtr<ID3D11DepthStencilView> dsv_;
    DXGI_FORMAT dxgiFormat_ = {};

    std::array<bool, numShaderStages> isBoundToD3dStage_ = {};

    friend D3d11Buffer;
    D3d11Buffer* d3dBuffer_ = nullptr; // used to clear backpointer at release time

    friend D3d11Framebuffer;
    core::Array<D3d11Framebuffer*> dependentD3dFramebuffers_;
};
using D3d11ImageViewPtr = ResourcePtr<D3d11ImageView>;

class D3d11SamplerState : public SamplerState {
protected:
    friend D3d11Engine;
    using SamplerState::SamplerState;

public:
    ID3D11SamplerState* object() const {
        return object_.get();
    }

protected:
    void release_(Engine* engine) override
    {
        SamplerState::release_(engine);
        object_.reset();
    }

private:
    ComPtr<ID3D11SamplerState> object_;
};
using D3d11SamplerStatePtr = ResourcePtr<D3d11SamplerState>;

class D3d11GeometryView : public GeometryView {
protected:
    friend D3d11Engine;
    using GeometryView::GeometryView;

public:
    D3D_PRIMITIVE_TOPOLOGY topology() const {
        return topology_;
    }

protected:
    void release_(Engine* engine) override
    {
        GeometryView::release_(engine);
    }

private:
    D3D_PRIMITIVE_TOPOLOGY topology_;
};
using D3d11GeometryViewPtr = ResourcePtr<D3d11GeometryView>;

class D3d11Program : public Program {
protected:
    friend D3d11Engine;
    using Program::Program;

public:
    // ...

protected:
    void release_(Engine* engine) override
    {
        Program::release_(engine);
        vertexShader_.reset();
        geometryShader_.reset();
        pixelShader_.reset();
        for (auto& x : builtinLayouts_) {
            x.reset();
        }
    }

private:
    ComPtr<ID3D11VertexShader> vertexShader_;
    ComPtr<ID3D11GeometryShader> geometryShader_;
    ComPtr<ID3D11PixelShader> pixelShader_;
    std::array<ComPtr<ID3D11InputLayout>, core::toUnderlying(BuiltinGeometryLayout::Max_) + 1> builtinLayouts_;
};
using D3d11ProgramPtr = ResourcePtr<D3d11Program>;

class D3d11BlendState : public BlendState {
protected:
    friend D3d11Engine;
    using BlendState::BlendState;

public:
    ID3D11BlendState* object() const
    {
        return object_.get();
    }

protected:
    void release_(Engine* engine) override
    {
        BlendState::release_(engine);
        object_.reset();
    }

private:
    ComPtr<ID3D11BlendState> object_;
};
using D3d11BlendStatePtr = ResourcePtr<D3d11BlendState>;

class D3d11RasterizerState : public RasterizerState {
protected:
    friend D3d11Engine;
    using RasterizerState::RasterizerState;

public:
    ID3D11RasterizerState* object() const
    {
        return object_.get();
    }

protected:
    void release_(Engine* engine) override
    {
        RasterizerState::release_(engine);
        object_.reset();
    }

private:
    ComPtr<ID3D11RasterizerState> object_;
};
using D3d11RasterizerStatePtr = ResourcePtr<D3d11RasterizerState>;

// no equivalent in D3D11, see OMSetRenderTargets
class D3d11Framebuffer : public Framebuffer {
protected:
    friend D3d11Engine;

    D3d11Framebuffer(
        ResourceRegistry* registry,
        const D3d11ImageViewPtr& colorView,
        const D3d11ImageViewPtr& depthStencilView,
        bool isDefault)
        : Framebuffer(registry)
        , colorView_(colorView)
        , depthStencilView_(depthStencilView)
        , isDefault_(isDefault)
    {
        if (colorView_) {
            colorView_->dependentD3dFramebuffers_.append(this);
            d3dColorView_ = colorView_.get();
        }
        if (depthStencilView_) {
            depthStencilView_->dependentD3dFramebuffers_.append(this);
            d3dDepthStencilView_ = depthStencilView_.get();
        }
    }

public:
    bool isDefault() const
    {
        return isDefault_;
    }

    ID3D11RenderTargetView* rtvObject() const
    {
        return colorView_ ?
            static_cast<ID3D11RenderTargetView*>(colorView_->rtvObject()) : nullptr;
    }

    ID3D11DepthStencilView* dsvObject() const
    {
        return depthStencilView_ ?
            static_cast<ID3D11DepthStencilView*>(depthStencilView_->dsvObject()) : nullptr;
    }

    // resizing a swap chain requires releasing all views to its back buffers.
    void forceReleaseColorViewD3dObject()
    {
        colorView_->forceReleaseD3dObject();
    }

protected:
    void releaseSubResources_() override
    {
        colorView_.reset();
        depthStencilView_.reset();
    }

    void release_(Engine* engine) override;

private:
    D3d11ImageViewPtr colorView_;
    D3d11ImageViewPtr depthStencilView_;

    bool isDefault_ = false;

    bool isBoundToD3D_ = false;

    friend D3d11ImageView;
    D3d11ImageView* d3dColorView_ = nullptr; // used to clear backpointer at release time
    D3d11ImageView* d3dDepthStencilView_ = nullptr; // used to clear backpointer at release time
};
using D3d11FramebufferPtr = ResourcePtr<D3d11Framebuffer>;

void D3d11Buffer::release_(Engine* engine)
{
    Buffer::release_(engine);
    object_.reset();
    for (D3d11ImageView* view : dependentD3dImageViews_) {
        view->d3dBuffer_ = nullptr;
    }
    dependentD3dImageViews_.clear();
}

void D3d11ImageView::release_(Engine* engine)
{
    ImageView::release_(engine);
    srv_.reset();
    rtv_.reset();
    dsv_.reset();
    if (d3dBuffer_) {
        d3dBuffer_->dependentD3dImageViews_.removeOne(this);
        d3dBuffer_ = nullptr;
    }
    for (D3d11Framebuffer* framebuffer : dependentD3dFramebuffers_) {
        if (framebuffer->d3dColorView_ == this) {
            framebuffer->d3dColorView_ = nullptr;
        }
        if (framebuffer->d3dDepthStencilView_ == this) {
            framebuffer->d3dDepthStencilView_ = nullptr;
        }
    }
    dependentD3dFramebuffers_.clear();
}

void D3d11Framebuffer::release_(Engine* engine)
{
    Framebuffer::release_(engine);
    if (d3dColorView_) {
        d3dColorView_->dependentD3dFramebuffers_.removeOne(this);
        d3dColorView_ = nullptr;
    }
    if (d3dDepthStencilView_) {
        d3dDepthStencilView_->dependentD3dFramebuffers_.removeOne(this);
        d3dDepthStencilView_ = nullptr;
    }
}

class D3d11SwapChain : public SwapChain {
public:
    D3d11SwapChain(ResourceRegistry* registry,
                   const SwapChainCreateInfo& desc,
                   IDXGISwapChain* dxgiSwapChain)
        : SwapChain(registry, desc)
        , dxgiSwapChain_(dxgiSwapChain) {
    }

    IDXGISwapChain* dxgiSwapChain() const
    {
        return dxgiSwapChain_.get();
    }

    // can't be called from render thread
    void setDefaultFramebuffer(const D3d11FramebufferPtr& defaultFrameBuffer)
    {
        defaultFrameBuffer_ = defaultFrameBuffer;
    }

    // can't be called from render thread
    void clearDefaultFramebuffer()
    {
        if (defaultFrameBuffer_) {
            static_cast<D3d11Framebuffer*>(defaultFrameBuffer_.get())->forceReleaseColorViewD3dObject();
            defaultFrameBuffer_.reset();
        }
    }

protected:
    void release_(Engine* engine) override
    {
        SwapChain::release_(engine);
    }

private:
    ComPtr<IDXGISwapChain> dxgiSwapChain_;
};

// ENUM CONVERSIONS

DXGI_FORMAT imageFormatToDxgiFormat(ImageFormat format)
{
    switch (format) {
        // Depth
    case ImageFormat::D_16_UNORM:               return DXGI_FORMAT_D16_UNORM;
    case ImageFormat::D_32_FLOAT:               return DXGI_FORMAT_D32_FLOAT;
        // Depth + Stencil
    case ImageFormat::DS_24_UNORM_8_UINT:       return DXGI_FORMAT_D24_UNORM_S8_UINT;
    case ImageFormat::DS_32_FLOAT_8_UINT_24_X:  return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
        // Red
    case ImageFormat::R_8_UNORM:                return DXGI_FORMAT_R8_UNORM;
    case ImageFormat::R_8_SNORM:                return DXGI_FORMAT_R8_SNORM;
    case ImageFormat::R_8_UINT:                 return DXGI_FORMAT_R8_UINT;
    case ImageFormat::R_8_SINT:                 return DXGI_FORMAT_R8_SINT;
    case ImageFormat::R_16_UNORM:               return DXGI_FORMAT_R16_UNORM;
    case ImageFormat::R_16_SNORM:               return DXGI_FORMAT_R16_SNORM;
    case ImageFormat::R_16_UINT:                return DXGI_FORMAT_R16_UINT;
    case ImageFormat::R_16_SINT:                return DXGI_FORMAT_R16_SINT;
    case ImageFormat::R_16_FLOAT:               return DXGI_FORMAT_R16_FLOAT;
    case ImageFormat::R_32_UINT:                return DXGI_FORMAT_R32_UINT;
    case ImageFormat::R_32_SINT:                return DXGI_FORMAT_R32_SINT;
    case ImageFormat::R_32_FLOAT:               return DXGI_FORMAT_R32_FLOAT;
        // RG
    case ImageFormat::RG_8_UNORM:               return DXGI_FORMAT_R8G8_UNORM;
    case ImageFormat::RG_8_SNORM:               return DXGI_FORMAT_R8G8_SNORM;
    case ImageFormat::RG_8_UINT:                return DXGI_FORMAT_R8G8_UINT;
    case ImageFormat::RG_8_SINT:                return DXGI_FORMAT_R8G8_SINT;
    case ImageFormat::RG_16_UNORM:              return DXGI_FORMAT_R16G16_UNORM;
    case ImageFormat::RG_16_SNORM:              return DXGI_FORMAT_R16G16_SNORM;
    case ImageFormat::RG_16_UINT:               return DXGI_FORMAT_R16G16_UINT;
    case ImageFormat::RG_16_SINT:               return DXGI_FORMAT_R16G16_SINT;
    case ImageFormat::RG_16_FLOAT:              return DXGI_FORMAT_R16G16_FLOAT;
    case ImageFormat::RG_32_UINT:               return DXGI_FORMAT_R32G32_UINT;
    case ImageFormat::RG_32_SINT:               return DXGI_FORMAT_R32G32_SINT;
    case ImageFormat::RG_32_FLOAT:              return DXGI_FORMAT_R32G32_FLOAT;
        // RGB
    case ImageFormat::RGB_11_11_10_FLOAT:       return DXGI_FORMAT_R11G11B10_FLOAT;
    case ImageFormat::RGB_32_UINT:              return DXGI_FORMAT_R32G32B32_UINT;
    case ImageFormat::RGB_32_SINT:              return DXGI_FORMAT_R32G32B32_SINT;
    case ImageFormat::RGB_32_FLOAT:             return DXGI_FORMAT_R32G32B32_FLOAT;
        // RGBA
    case ImageFormat::RGBA_8_UNORM:             return DXGI_FORMAT_R8G8B8A8_UNORM;
    case ImageFormat::RGBA_8_UNORM_SRGB:        return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    case ImageFormat::RGBA_8_SNORM:             return DXGI_FORMAT_R8G8B8A8_SNORM;
    case ImageFormat::RGBA_8_UINT:              return DXGI_FORMAT_R8G8B8A8_UINT;
    case ImageFormat::RGBA_8_SINT:              return DXGI_FORMAT_R8G8B8A8_SINT;
    case ImageFormat::RGBA_10_10_10_2_UNORM:    return DXGI_FORMAT_R10G10B10A2_UNORM;
    case ImageFormat::RGBA_10_10_10_2_UINT:     return DXGI_FORMAT_R10G10B10A2_UINT;
    case ImageFormat::RGBA_16_UNORM:            return DXGI_FORMAT_R16G16B16A16_UNORM;
    case ImageFormat::RGBA_16_UINT:             return DXGI_FORMAT_R16G16B16A16_UINT;
    case ImageFormat::RGBA_16_SINT:             return DXGI_FORMAT_R16G16B16A16_SINT;
    case ImageFormat::RGBA_16_FLOAT:            return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case ImageFormat::RGBA_32_UINT:             return DXGI_FORMAT_R32G32B32A32_UINT;
    case ImageFormat::RGBA_32_SINT:             return DXGI_FORMAT_R32G32B32A32_SINT;
    case ImageFormat::RGBA_32_FLOAT:            return DXGI_FORMAT_R32G32B32A32_FLOAT;
    default:
        break;
    }
    return DXGI_FORMAT_UNKNOWN;
}

D3D_PRIMITIVE_TOPOLOGY primitiveTypeToD3DPrimitiveTopology(PrimitiveType type)
{
    switch (type) {
    case PrimitiveType::Point:          return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
    case PrimitiveType::LineList:       return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
    case PrimitiveType::LineStrip:      return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
    case PrimitiveType::TriangleList:   return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case PrimitiveType::TriangleStrip:  return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    default:
        break;
    }
    return D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
}

D3D11_USAGE usageToD3DUsage(Usage usage)
{
    switch (usage) {
    case Usage::Default:
        return D3D11_USAGE_DEFAULT;
    case Usage::Immutable:
        return D3D11_USAGE_IMMUTABLE;
    case Usage::Dynamic:
        return D3D11_USAGE_DYNAMIC;
    case Usage::Staging:
        return D3D11_USAGE_STAGING;
    default:
        break;
    }
    throw core::LogicError("D3d11Engine: unsupported usage");
}

UINT resourceMiscFlagsToD3DResourceMiscFlags(ResourceMiscFlags resourceMiscFlags)
{
    UINT x = 0;
    if (!!(resourceMiscFlags & ResourceMiscFlags::Shared)) {
        x |= D3D11_RESOURCE_MISC_SHARED;
    }
    //if (!!(resourceMiscFlags & ResourceMiscFlags::DrawIndirectArgs)) {
    //    x |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
    //}
    //if (!!(resourceMiscFlags & ResourceMiscFlags::BufferRaw)) {
    //    x |= D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
    //}
    //if (!!(resourceMiscFlags & ResourceMiscFlags::BufferStructured)) {
    //    x |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    //}
    //if (!!(resourceMiscFlags & ResourceMiscFlags::ResourceClamp)) {
    //    x |= D3D11_RESOURCE_MISC_RESOURCE_CLAMP;
    //}
    //if (!!(resourceMiscFlags & ResourceMiscFlags::SharedKeyedMutex)) {
    //    x |= D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;
    //}
    return x;
}

D3D11_TEXTURE_ADDRESS_MODE imageWrapModeToD3DTextureAddressMode(ImageWrapMode mode)
{
    static_assert(numImageWrapModes == 5);
    static constexpr std::array<D3D11_TEXTURE_ADDRESS_MODE, numImageWrapModes> map = {
        D3D11_TEXTURE_ADDRESS_MODE(0),  // Undefined
        D3D11_TEXTURE_ADDRESS_WRAP,     // Repeat
        D3D11_TEXTURE_ADDRESS_MIRROR,   // MirrorRepeat
        D3D11_TEXTURE_ADDRESS_CLAMP,    // Clamp
        D3D11_TEXTURE_ADDRESS_BORDER,   // ClampConstantColor
    };

    const UInt index = core::toUnderlying(mode);
    if (index == 0 || index >= numImageWrapModes) {
        throw core::LogicError("D3d11Engine: invalid ImageWrapMode enum value");
    }

    return map[index];
}

D3D11_COMPARISON_FUNC comparisonFunctionToD3DComparisonFunc(ComparisonFunction func)
{
    static_assert(numComparisonFunctions == 10);
    static constexpr std::array<D3D11_COMPARISON_FUNC, numComparisonFunctions> map = {
        D3D11_COMPARISON_FUNC(0),       // Undefined
        D3D11_COMPARISON_NEVER,         // Disabled
        D3D11_COMPARISON_ALWAYS,        // Always
        D3D11_COMPARISON_NEVER,         // Never
        D3D11_COMPARISON_EQUAL,         // Equal
        D3D11_COMPARISON_NOT_EQUAL,     // NotEqual
        D3D11_COMPARISON_LESS,          // Less
        D3D11_COMPARISON_LESS_EQUAL,    // LessEqual
        D3D11_COMPARISON_GREATER,       // Greater
        D3D11_COMPARISON_GREATER_EQUAL, // GreaterEqual
    };

    const UInt index = core::toUnderlying(func);
    if (index == 0 || index >= numComparisonFunctions) {
        throw core::LogicError("D3d11Engine: invalid ComparisonFunction enum value");
    }

    return map[index];
}

D3D11_BLEND blendFactorToD3DBlend(BlendFactor factor)
{
    static_assert(numBlendFactors == 18);
    static constexpr std::array<D3D11_BLEND, numBlendFactors> map = {
        D3D11_BLEND(0),                 // Undefined
        D3D11_BLEND_ONE,                // One
        D3D11_BLEND_ZERO,               // Zero
        D3D11_BLEND_SRC_COLOR,          // SourceColor
        D3D11_BLEND_INV_SRC_COLOR,      // OneMinusSourceColor
        D3D11_BLEND_SRC_ALPHA,          // SourceAlpha
        D3D11_BLEND_INV_SRC_ALPHA,      // OneMinusSourceAlpha
        D3D11_BLEND_DEST_COLOR,         // TargetColor
        D3D11_BLEND_INV_DEST_COLOR,     // OneMinusTargetColor
        D3D11_BLEND_DEST_ALPHA,         // TargetAlpha
        D3D11_BLEND_INV_DEST_ALPHA,     // OneMinusTargetAlpha
        D3D11_BLEND_SRC_ALPHA_SAT,      // SourceAlphaSaturated
        D3D11_BLEND_BLEND_FACTOR,       // Constant
        D3D11_BLEND_INV_BLEND_FACTOR,   // OneMinusConstant
        D3D11_BLEND_SRC1_COLOR,         // SecondSourceColor
        D3D11_BLEND_INV_SRC1_COLOR,     // OneMinusSecondSourceColor
        D3D11_BLEND_SRC1_ALPHA,         // SecondSourceAlpha
        D3D11_BLEND_INV_SRC1_ALPHA,     // OneMinusSecondSourceAlpha
    };

    const UInt index = core::toUnderlying(factor);
    if (index == 0 || index >= numBlendFactors) {
        throw core::LogicError("D3d11Engine: invalid BlendFactor enum value");
    }

    return map[index];
}

D3D11_BLEND_OP blendOpToD3DBlendOp(BlendOp op)
{
    static_assert(numBlendOps == 6);
    static constexpr std::array<D3D11_BLEND_OP, numBlendOps> map = {
        D3D11_BLEND_OP(0),              // Undefined
        D3D11_BLEND_OP_ADD,             // Add
        D3D11_BLEND_OP_SUBTRACT,        // SourceMinusTarget
        D3D11_BLEND_OP_REV_SUBTRACT,    // TargetMinusSource
        D3D11_BLEND_OP_MIN,             // Min
        D3D11_BLEND_OP_MAX,             // Max
    };

    const UInt index = core::toUnderlying(op);
    if (index == 0 || index >= numBlendOps) {
        throw core::LogicError("D3d11Engine: invalid BlendOp enum value");
    }

    return map[index];
}

D3D11_FILL_MODE fillModeToD3DFillMode(FillMode mode)
{
    static_assert(numFillModes == 3);
    static constexpr std::array<D3D11_FILL_MODE, numFillModes> map = {
        D3D11_FILL_MODE(0),             // Undefined
        D3D11_FILL_SOLID,               // Solid
        D3D11_FILL_WIREFRAME,           // Wireframe
    };

    const UInt index = core::toUnderlying(mode);
    if (index == 0 || index >= numFillModes) {
        throw core::LogicError("D3d11Engine: invalid FillMode enum value");
    }

    return map[index];
}

D3D11_CULL_MODE cullModeToD3DCullMode(CullMode mode)
{
    static_assert(numCullModes == 4);
    static constexpr std::array<D3D11_CULL_MODE, numCullModes> map = {
        D3D11_CULL_MODE(0),             // Undefined
        D3D11_CULL_NONE,                // None
        D3D11_CULL_FRONT,               // Front
        D3D11_CULL_BACK,                // Back
    };

    const UInt index = core::toUnderlying(mode);
    if (index == 0 || index >= numCullModes) {
        throw core::LogicError("D3d11Engine: invalid CullMode enum value");
    }

    return map[index];
}

// ENGINE FUNCTIONS

D3d11Engine::D3d11Engine()
{
    // XXX add success checks (S_OK)

    const D3D_FEATURE_LEVEL featureLevels[1] = { D3D_FEATURE_LEVEL_11_0 };
    D3D11CreateDevice(
        NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        D3D11_CREATE_DEVICE_DEBUG |
        0, // could use D3D11_CREATE_DEVICE_SINGLETHREADED
           // if we defer creation of buffers and swapchain.
        featureLevels, 1,
        D3D11_SDK_VERSION,
        device_.releaseAndGetAddressOf(),
        NULL,
        deviceCtx_.releaseAndGetAddressOf());

    // Retrieve DXGI factory from device.
    ComPtr<IDXGIDevice> dxgiDevice;
    ComPtr<IDXGIAdapter> dxgiAdapter;
    device_->QueryInterface(IID_PPV_ARGS(dxgiDevice.releaseAndGetAddressOf()));
    dxgiDevice->GetParent(IID_PPV_ARGS(dxgiAdapter.releaseAndGetAddressOf()));
    dxgiAdapter->GetParent(IID_PPV_ARGS(factory_.releaseAndGetAddressOf()));
}

void D3d11Engine::onDestroyed()
{
    Engine::onDestroyed();
}

/* static */
D3d11EnginePtr D3d11Engine::create()
{
    return D3d11EnginePtr(new D3d11Engine());
}

// USER THREAD pimpl functions

void D3d11Engine::createBuiltinShaders_()
{
    D3d11ProgramPtr simpleProgram(new D3d11Program(resourceRegistry_));
    simpleProgram_ = simpleProgram;

    // Create the simple shader
    {
        static const char* vertexShaderSrc = R"hlsl(

            cbuffer vertexBuffer : register(b0)
            {
                float4x4 projMatrix;
                float4x4 viewMatrix;
                unsigned int frameStartTimeInMs;
            };
            struct VS_INPUT
            {
                float2 pos : POSITION;
                float4 col : COLOR0;
            };
            struct PS_INPUT
            {
                float4 pos : SV_POSITION;
                float4 col : COLOR0;
            };

            PS_INPUT main(VS_INPUT input)
            {
                PS_INPUT output;
                float4 viewPos = mul(viewMatrix, float4(input.pos.xy, 0.f, 1.f));
                output.pos = mul(projMatrix, viewPos);
                output.col = input.col;
                return output;
            }

        )hlsl";

        ComPtr<ID3DBlob> errorBlob;
        ComPtr<ID3DBlob> vertexShaderBlob;
        if (0 > D3DCompile(
            vertexShaderSrc, strlen(vertexShaderSrc),
            NULL, NULL, NULL, "main", "vs_4_0", 0, 0,
            vertexShaderBlob.releaseAndGetAddressOf(), errorBlob.releaseAndGetAddressOf()))
        {
            const char* errString = static_cast<const char*>(errorBlob->GetBufferPointer());
            throw core::RuntimeError(errString);
        }
        errorBlob.reset();

        ComPtr<ID3D11VertexShader> vertexShader;
        device_->CreateVertexShader(
            vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(),
            NULL, vertexShader.releaseAndGetAddressOf());
        simpleProgram->vertexShader_ = vertexShader;

        // Create the input layout
        ComPtr<ID3D11InputLayout> inputLayout;
        D3D11_INPUT_ELEMENT_DESC layout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,    0, (UINT)offsetof(XYRGBVertex, x), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, (UINT)offsetof(XYRGBVertex, r), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        device_->CreateInputLayout(
            layout, 2, vertexShaderBlob->GetBufferPointer(),
            vertexShaderBlob->GetBufferSize(), inputLayout.releaseAndGetAddressOf());
        simpleProgram->builtinLayouts_[core::toUnderlying(BuiltinGeometryLayout::XYRGB)] = inputLayout;
    }

    // Create the paint pixel shader
    {
        static const char* pixelShaderSrc = R"hlsl(
            struct PS_INPUT
            {
                float4 pos : SV_POSITION;
                float4 col : COLOR0;
            };

            float4 main(PS_INPUT input) : SV_Target
            {
                return input.col;
            }

        )hlsl";

        ComPtr<ID3DBlob> errorBlob;
        ComPtr<ID3DBlob> pixelShaderBlob;
        if (0 > D3DCompile(
            pixelShaderSrc, strlen(pixelShaderSrc),
            NULL, NULL, NULL, "main", "ps_4_0", 0, 0,
            pixelShaderBlob.releaseAndGetAddressOf(), errorBlob.releaseAndGetAddressOf()))
        {
            const char* errString = static_cast<const char*>(errorBlob->GetBufferPointer());
            throw core::RuntimeError(errString);
        }
        errorBlob.reset();

        ComPtr<ID3D11PixelShader> pixelShader;
        device_->CreatePixelShader(
            pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(),
            NULL, pixelShader.releaseAndGetAddressOf());
        simpleProgram->pixelShader_ = pixelShader;
    }

    // Create depth-stencil State
    {
        D3D11_DEPTH_STENCIL_DESC desc = {};
        desc.DepthEnable = false;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
        desc.StencilEnable = false;
        desc.FrontFace.StencilFailOp = desc.FrontFace.StencilDepthFailOp = desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        desc.BackFace = desc.FrontFace;
        device_->CreateDepthStencilState(&desc, depthStencilState_.releaseAndGetAddressOf());
    }
}

SwapChainPtr D3d11Engine::constructSwapChain_(const SwapChainCreateInfo& createInfo)
{
    if (!device_) {
        throw core::LogicError("device_ is null.");
    }

    if (createInfo.windowNativeHandleType() != WindowNativeHandleType::Win32) {
        return nullptr;
    }

    ImageFormat colorViewFormat = {};

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = createInfo.numBuffers();
    sd.BufferDesc.Width = createInfo.width();
    sd.BufferDesc.Height = createInfo.height();
    switch(createInfo.format()) {
    case SwapChainTargetFormat::RGBA_8_UNORM: {
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        colorViewFormat = ImageFormat::RGBA_8_UNORM;
        break;
    }
    case SwapChainTargetFormat::RGBA_8_UNORM_SRGB: {
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        colorViewFormat = ImageFormat::RGBA_8_UNORM_SRGB;
        break;
    }
    default:
        throw core::LogicError("D3d11SwapChain: unsupported format");
        break;
    }
    sd.BufferDesc.RefreshRate.Numerator = 0;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    // do we need DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT ?
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = static_cast<HWND>(createInfo.windowNativeHandle());
    sd.SampleDesc.Count = createInfo.numSamples();
    sd.SampleDesc.Quality = 0;
    sd.Windowed = true;
    //sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;

    ComPtr<IDXGISwapChain> dxgiSwapChain;
    if (factory_->CreateSwapChain(device_.get(), &sd, dxgiSwapChain.releaseAndGetAddressOf()) < 0) {
        throw core::LogicError("D3d11Engine: could not create swap chain");
    }

    ComPtr<ID3D11Texture2D> backBuffer;
    dxgiSwapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.releaseAndGetAddressOf()));

    D3D11_TEXTURE2D_DESC backBufferDesc = {};
    backBuffer->GetDesc(&backBufferDesc);

    ImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.setRank(ImageRank::_2D);
    imageCreateInfo.setFormat(colorViewFormat);
    // XXX fill it using backBufferDesc

    D3d11ImagePtr backBufferImage(new D3d11Image(resourceRegistry_, imageCreateInfo));
    backBufferImage->object_ = backBuffer;

    ImageViewCreateInfo viewCreateInfo = {};
    viewCreateInfo.setBindFlags(ImageBindFlags::RenderTarget);
    D3d11ImageViewPtr colorView(new D3d11ImageView(resourceRegistry_, viewCreateInfo, backBufferImage));

    ComPtr<ID3D11RenderTargetView> backBufferView;
    device_->CreateRenderTargetView(backBuffer.get(), NULL, backBufferView.releaseAndGetAddressOf());
    colorView->rtv_ = backBufferView.get();

    D3d11FramebufferPtr newFramebuffer(
        new D3d11Framebuffer(
            resourceRegistry_,
            colorView,
            D3d11ImageViewPtr(),
            true));

    auto swapChain = makeUnique<D3d11SwapChain>(resourceRegistry_, createInfo, dxgiSwapChain.get());
    swapChain->setDefaultFramebuffer(newFramebuffer);

    return SwapChainPtr(swapChain.release());
}

FramebufferPtr D3d11Engine::constructFramebuffer_(const ImageViewPtr& colorImageView)
{
    auto framebuffer = makeUnique<D3d11Framebuffer>(
        resourceRegistry_, static_pointer_cast<D3d11ImageView>(colorImageView), nullptr, false);
    return FramebufferPtr(framebuffer.release());
}

BufferPtr D3d11Engine::constructBuffer_(const BufferCreateInfo& createInfo)
{
    auto buffer = makeUnique<D3d11Buffer>(resourceRegistry_, createInfo);
    D3D11_BUFFER_DESC& desc = buffer->desc_;

    desc.Usage = usageToD3DUsage(createInfo.usage());

    const BindFlags bindFlags = createInfo.bindFlags();
    if (!!(bindFlags & BindFlags::ConstantBuffer)) {
        desc.BindFlags |= D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER;
        if (bindFlags != BindFlags::ConstantBuffer) {
            throw core::LogicError("D3d11Buffer: BindFlags::UniformBuffer cannot be combined with any other bind flag");
        }
    }
    else {
        if (!!(bindFlags & BindFlags::VertexBuffer)) {
            desc.BindFlags |= D3D11_BIND_VERTEX_BUFFER;
        }
        if (!!(bindFlags & BindFlags::IndexBuffer)) {
            desc.BindFlags |= D3D11_BIND_INDEX_BUFFER;
        }
        if (!!(bindFlags & BindFlags::ConstantBuffer)) {
            desc.BindFlags |= D3D11_BIND_CONSTANT_BUFFER;
        }
        if (!!(bindFlags & BindFlags::ShaderResource)) {
            desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
        }
        if (!!(bindFlags & BindFlags::RenderTarget)) {
            desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
        }
        if (!!(bindFlags & BindFlags::DepthStencil)) {
            desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
        }
        if (!!(bindFlags & BindFlags::UnorderedAccess)) {
            desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
        }
        if (!!(bindFlags & BindFlags::StreamOutput)) {
            desc.BindFlags |= D3D11_BIND_STREAM_OUTPUT;
        }
    }

    const ResourceMiscFlags resourceMiscFlags = createInfo.resourceMiscFlags();
    desc.MiscFlags = resourceMiscFlagsToD3DResourceMiscFlags(resourceMiscFlags);

    const CpuAccessFlags cpuAccessFlags = createInfo.cpuAccessFlags();
    if (!!(cpuAccessFlags & CpuAccessFlags::Write)) {
        desc.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;
    }
    if (!!(cpuAccessFlags & CpuAccessFlags::Read)) {
        desc.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;
    }

    return BufferPtr(buffer.release());
}

ImagePtr D3d11Engine::constructImage_(const ImageCreateInfo& createInfo)
{
    DXGI_FORMAT dxgiFormat = imageFormatToDxgiFormat(createInfo.format());
    if (dxgiFormat == DXGI_FORMAT_UNKNOWN) {
        throw core::LogicError("D3d11: unknown image format");
    }

    auto image = makeUnique<D3d11Image>(resourceRegistry_, createInfo);
    image->dxgiFormat_ = dxgiFormat;
    return ImagePtr(image.release());
}

ImageViewPtr D3d11Engine::constructImageView_(const ImageViewCreateInfo& createInfo, const ImagePtr& image)
{
    // XXX should check bind flags compatibility in abstract engine

    auto imageView = makeUnique<D3d11ImageView>(resourceRegistry_, createInfo, image);
    imageView->dxgiFormat_ = static_cast<D3d11Image*>(image.get())->dxgiFormat();
    return ImageViewPtr(imageView.release());
}

ImageViewPtr D3d11Engine::constructImageView_(const ImageViewCreateInfo& createInfo, const BufferPtr& buffer, ImageFormat format, UInt32 numElements)
{
    // XXX should check bind flags compatibility in abstract engine

    DXGI_FORMAT dxgiFormat = imageFormatToDxgiFormat(format);
    if (dxgiFormat == DXGI_FORMAT_UNKNOWN) {
        throw core::LogicError("D3d11: unknown image format");
    }

    auto view = makeUnique<D3d11ImageView>(resourceRegistry_, createInfo, buffer, format, numElements);
    view->dxgiFormat_ = dxgiFormat;
    return ImageViewPtr(view.release());
}

SamplerStatePtr D3d11Engine::constructSamplerState_(const SamplerStateCreateInfo& createInfo)
{
    auto state = makeUnique<D3d11SamplerState>(resourceRegistry_, createInfo);
    return SamplerStatePtr(state.release());
}

GeometryViewPtr D3d11Engine::constructGeometryView_(const GeometryViewCreateInfo& createInfo)
{
    D3D_PRIMITIVE_TOPOLOGY topology = primitiveTypeToD3DPrimitiveTopology(createInfo.primitiveType());
    if (topology == D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED) {
        throw core::LogicError("D3d11: unknown primitive type");
    }

    auto view = makeUnique<D3d11GeometryView>(resourceRegistry_, createInfo);
    view->topology_ = topology;
    return GeometryViewPtr(view.release());
}

BlendStatePtr D3d11Engine::constructBlendState_(const BlendStateCreateInfo& createInfo)
{
    auto state = makeUnique<D3d11BlendState>(resourceRegistry_, createInfo);
    return BlendStatePtr(state.release());
}

RasterizerStatePtr D3d11Engine::constructRasterizerState_(const RasterizerStateCreateInfo& createInfo)
{
    auto state = makeUnique<D3d11RasterizerState>(resourceRegistry_, createInfo);
    return RasterizerStatePtr(state.release());
}

void D3d11Engine::resizeSwapChain_(SwapChain* swapChain, UInt32 width, UInt32 height)
{
    D3d11SwapChain* d3dSwapChain = static_cast<D3d11SwapChain*>(swapChain);
    IDXGISwapChain* dxgiSwapChain = d3dSwapChain->dxgiSwapChain();

    d3dSwapChain->clearDefaultFramebuffer();

    HRESULT hres = dxgiSwapChain->ResizeBuffers(
        0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    if (hres < 0) {
        throw core::LogicError("D3d11Engine: could not resize swap chain buffers");
    }

    ComPtr<ID3D11Texture2D> backBuffer;
    dxgiSwapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.releaseAndGetAddressOf()));

    D3D11_TEXTURE2D_DESC backBufferDesc = {};
    backBuffer->GetDesc(&backBufferDesc);

    ImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.setRank(ImageRank::_2D);
    imageCreateInfo.setFormat(d3dSwapChain->backBufferFormat());
    // XXX fill it using backBufferDesc

    D3d11ImagePtr backBufferImage(new D3d11Image(resourceRegistry_, imageCreateInfo));
    backBufferImage->object_ = backBuffer;

    ImageViewCreateInfo viewCreateInfo = {};
    viewCreateInfo.setBindFlags(ImageBindFlags::RenderTarget);
    D3d11ImageViewPtr colorView(new D3d11ImageView(resourceRegistry_, viewCreateInfo, backBufferImage));

    ComPtr<ID3D11RenderTargetView> backBufferView;
    device_->CreateRenderTargetView(backBuffer.get(), NULL, backBufferView.releaseAndGetAddressOf());
    colorView->rtv_ = backBufferView.get();

    D3d11FramebufferPtr newFramebuffer(
        new D3d11Framebuffer(
            resourceRegistry_,
            colorView,
            D3d11ImageViewPtr(),
            true));

    d3dSwapChain->setDefaultFramebuffer(newFramebuffer);
}

// -- RENDER THREAD functions --

void D3d11Engine::initFramebuffer_(Framebuffer* /*framebuffer*/)
{
    // no-op
}

void D3d11Engine::initBuffer_(Buffer* buffer, const char* data, Int lengthInBytes)
{
    D3d11Buffer* d3dBuffer = static_cast<D3d11Buffer*>(buffer);
    if (lengthInBytes) {
        loadBuffer_(d3dBuffer,
                    data,
                    lengthInBytes);
    }
    d3dBuffer->gpuLengthInBytes_ = lengthInBytes;
}

void D3d11Engine::initImage_(Image* image, const Span<const Span<const char>>* dataSpanSpan)
{
    D3d11Image* d3dImage = static_cast<D3d11Image*>(image);

    // XXX add size checks, see https://docs.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11device-createtexture2d
    core::Array<D3D11_SUBRESOURCE_DATA> initData;
    if (dataSpanSpan) {
        initData.resize(dataSpanSpan->length());
        for (Int i = 0; i < dataSpanSpan->length(); ++i) {
            initData[i].pSysMem = dataSpanSpan->data()[i].data();
        }
    }

    D3D11_USAGE d3dUsage = usageToD3DUsage(d3dImage->usage());

    UINT d3dBindFlags = 0;
    if (!!(d3dImage->bindFlags() & ImageBindFlags::ShaderResource)) {
        d3dBindFlags |= D3D11_BIND_SHADER_RESOURCE;
    }
    if (!!(d3dImage->bindFlags() & ImageBindFlags::RenderTarget)) {
        d3dBindFlags |= D3D11_BIND_RENDER_TARGET;
    }
    if (!!(d3dImage->bindFlags() & ImageBindFlags::DepthStencil)) {
        d3dBindFlags |= D3D11_BIND_DEPTH_STENCIL;
    }

    const CpuAccessFlags cpuAccessFlags = d3dImage->cpuAccessFlags();
    UINT d3dCPUAccessFlags = 0;
    if (!!(cpuAccessFlags & CpuAccessFlags::Write)) {
        d3dCPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;
    }
    if (!!(cpuAccessFlags & CpuAccessFlags::Read)) {
        d3dCPUAccessFlags |= D3D11_CPU_ACCESS_READ;
    }

    UINT d3dMiscFlags = resourceMiscFlagsToD3DResourceMiscFlags(d3dImage->resourceMiscFlags());

    if (d3dImage->isMipGenerationEnabled()) {
        d3dMiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
    }

    if (d3dImage->rank() == ImageRank::_1D) {
        D3D11_TEXTURE1D_DESC desc = {};
        desc.Width = d3dImage->width();
        desc.MipLevels = d3dImage->numMipLevels();
        desc.ArraySize = d3dImage->numLayers();
        desc.Format = d3dImage->dxgiFormat();
        desc.Usage = d3dUsage;
        desc.BindFlags = d3dBindFlags;
        desc.CPUAccessFlags = d3dCPUAccessFlags;
        desc.MiscFlags = d3dMiscFlags;

        ComPtr<ID3D11Texture1D> texture;
        device_->CreateTexture1D(&desc, initData.size() ? initData.data() : nullptr, texture.releaseAndGetAddressOf());
        d3dImage->object_ = texture;
    }
    else {
        VGC_CORE_ASSERT(d3dImage->rank() == ImageRank::_2D);
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = d3dImage->width();
        desc.Height = d3dImage->height();
        desc.MipLevels = d3dImage->numMipLevels();
        desc.ArraySize = std::max<UInt8>(1, d3dImage->numLayers());
        desc.Format = d3dImage->dxgiFormat();
        desc.SampleDesc.Count = d3dImage->numSamples();
        desc.Usage = d3dUsage;
        desc.BindFlags = d3dBindFlags;
        desc.CPUAccessFlags = d3dCPUAccessFlags;
        desc.MiscFlags = d3dMiscFlags;

        ComPtr<ID3D11Texture2D> texture;
        device_->CreateTexture2D(&desc, initData.size() ? initData.data() : nullptr, texture.releaseAndGetAddressOf());
        d3dImage->object_ = texture;
    }
}

void D3d11Engine::initImageView_(ImageView* view)
{
    D3d11ImageView* d3dImageView = static_cast<D3d11ImageView*>(view);
    if (!!(d3dImageView->bindFlags() & ImageBindFlags::ShaderResource)) {
        D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
        desc.Format = d3dImageView->dxgiFormat();
        if (view->isBuffer()) {
            BufferPtr buffer = view->viewedBuffer();
            desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
            desc.Buffer.FirstElement = 0;
            desc.Buffer.NumElements = d3dImageView->numBufferElements();
        }
        else {
            ImagePtr image = view->viewedImage();
            switch (image->rank()) {
            case ImageRank::_1D: {
                if (image->numLayers() > 1) {
                    desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1DARRAY;
                    desc.Texture1DArray.FirstArraySlice = d3dImageView->firstLayer();
                    desc.Texture1DArray.ArraySize = d3dImageView->numLayers();
                    desc.Texture1DArray.MostDetailedMip = d3dImageView->firstMipLevel();
                    desc.Texture1DArray.MipLevels = d3dImageView->numMipLevels();
                }
                else {
                    desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
                    desc.Texture1D.MostDetailedMip = d3dImageView->firstMipLevel();
                    desc.Texture1D.MipLevels = d3dImageView->numMipLevels();
                }
                break;
            }
            case ImageRank::_2D: {
                if (image->numLayers() > 1) {
                    desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                    desc.Texture2DArray.FirstArraySlice = d3dImageView->firstLayer();
                    desc.Texture2DArray.ArraySize = d3dImageView->numLayers();
                    desc.Texture2DArray.MostDetailedMip = d3dImageView->firstMipLevel();
                    desc.Texture2DArray.MipLevels = d3dImageView->numMipLevels();
                }
                else {
                    desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                    desc.Texture2D.MostDetailedMip = d3dImageView->firstMipLevel();
                    desc.Texture2D.MipLevels = d3dImageView->numMipLevels();
                }
                break;
            }
            default:
                throw core::LogicError("D3d11: unknown image rank");
                break;
            }
        }
        device_->CreateShaderResourceView(d3dImageView->d3dViewedResource(), &desc, d3dImageView->srv_.releaseAndGetAddressOf());
    }
    if (!!(d3dImageView->bindFlags() & ImageBindFlags::RenderTarget)) {
        D3D11_RENDER_TARGET_VIEW_DESC desc = {};
        desc.Format = d3dImageView->dxgiFormat();
        if (view->isBuffer()) {
            BufferPtr buffer = view->viewedBuffer();
            desc.ViewDimension = D3D11_RTV_DIMENSION_BUFFER;
            desc.Buffer.FirstElement = 0;
            desc.Buffer.NumElements = d3dImageView->numBufferElements();
        }
        else {
            ImagePtr image = view->viewedImage();
            switch (image->rank()) {
            case ImageRank::_1D: {
                if (image->numLayers() > 1) {
                    desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1DARRAY;
                    desc.Texture1DArray.FirstArraySlice = d3dImageView->firstLayer();
                    desc.Texture1DArray.ArraySize = d3dImageView->numLayers();
                    desc.Texture1DArray.MipSlice = d3dImageView->firstMipLevel();
                }
                else {
                    desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1D;
                    desc.Texture1D.MipSlice = d3dImageView->firstMipLevel();
                }
                break;
            }
            case ImageRank::_2D: {
                if (image->numLayers() > 1) {
                    desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                    desc.Texture2DArray.FirstArraySlice = d3dImageView->firstLayer();
                    desc.Texture2DArray.ArraySize = d3dImageView->numLayers();
                    desc.Texture2DArray.MipSlice = d3dImageView->firstMipLevel();
                }
                else {
                    desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                    desc.Texture2D.MipSlice = d3dImageView->firstMipLevel();
                }
                break;
            }
            default:
                throw core::LogicError("D3d11: unknown image rank");
                break;
            }
        }
        device_->CreateRenderTargetView(d3dImageView->d3dViewedResource(), &desc, d3dImageView->rtv_.releaseAndGetAddressOf());
    }
    if (!!(d3dImageView->bindFlags() & ImageBindFlags::DepthStencil)) {
        D3D11_DEPTH_STENCIL_VIEW_DESC desc = {};
        desc.Format = d3dImageView->dxgiFormat();
        if (view->isBuffer()) {
            throw core::LogicError("D3d11: buffer cannot be bound as Depth Stencil");
        }
        else {
            ImagePtr image = view->viewedImage();
            switch (image->rank()) {
            case ImageRank::_1D: {
                if (image->numLayers() > 1) {
                    desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1DARRAY;
                    desc.Texture1DArray.FirstArraySlice = d3dImageView->firstLayer();
                    desc.Texture1DArray.ArraySize = d3dImageView->numLayers();
                    desc.Texture1DArray.MipSlice = d3dImageView->firstMipLevel();
                }
                else {
                    desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1D;
                    desc.Texture1D.MipSlice = d3dImageView->firstMipLevel();
                }
                break;
            }
            case ImageRank::_2D: {
                if (image->numLayers() > 1) {
                    desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                    desc.Texture2DArray.FirstArraySlice = d3dImageView->firstLayer();
                    desc.Texture2DArray.ArraySize = d3dImageView->numLayers();
                    desc.Texture2DArray.MipSlice = d3dImageView->firstMipLevel();
                }
                else {
                    desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                    desc.Texture2D.MipSlice = d3dImageView->firstMipLevel();
                }
                break;
            }
            default:
                throw core::LogicError("D3d11: unknown image rank");
                break;
            }
        }
        device_->CreateDepthStencilView(d3dImageView->d3dViewedResource(), &desc, d3dImageView->dsv_.releaseAndGetAddressOf());
    }
}

void D3d11Engine::initSamplerState_(SamplerState* state)
{
    D3d11SamplerState* d3dSamplerState = static_cast<D3d11SamplerState*>(state);
    D3D11_SAMPLER_DESC desc = {};
    UINT filter = 0;

    if (d3dSamplerState->magFilter() == FilterMode::Undefined) {
        throw core::LogicError("D3d11: undefined mag filter");
    }
    if (d3dSamplerState->minFilter() == FilterMode::Undefined) {
        throw core::LogicError("D3d11: undefined min filter");
    }
    if (d3dSamplerState->mipFilter() == FilterMode::Undefined) {
        throw core::LogicError("D3d11: undefined mip filter");
    }

    if (d3dSamplerState->maxAnisotropy() >= 1) {
        filter = D3D11_FILTER_ANISOTROPIC;
    }
    else {
        if (d3dSamplerState->magFilter() == FilterMode::Linear) {
            filter |= D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
        }
        if (d3dSamplerState->minFilter() == FilterMode::Linear) {
            filter |= D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
        }
        if (d3dSamplerState->mipFilter() == FilterMode::Linear) {
            filter |= D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
        }
    }
    if (d3dSamplerState->comparisonFunction() != ComparisonFunction::Disabled) {
        filter |= D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
    }
    desc.Filter = static_cast<D3D11_FILTER>(filter);
    desc.AddressU = imageWrapModeToD3DTextureAddressMode(d3dSamplerState->wrapModeU());
    desc.AddressV = imageWrapModeToD3DTextureAddressMode(d3dSamplerState->wrapModeV());
    desc.AddressW = imageWrapModeToD3DTextureAddressMode(d3dSamplerState->wrapModeW());
    desc.MipLODBias = d3dSamplerState->mipLODBias();
    desc.MaxAnisotropy = d3dSamplerState->maxAnisotropy();
    desc.ComparisonFunc = comparisonFunctionToD3DComparisonFunc(d3dSamplerState->comparisonFunction());
    // XXX add data() in vec4f
    memcpy(desc.BorderColor, &d3dSamplerState->wrapColor(), 4 * sizeof(float));
    desc.MinLOD = d3dSamplerState->minLOD();
    desc.MaxLOD = d3dSamplerState->maxLOD();
    device_->CreateSamplerState(&desc, d3dSamplerState->object_.releaseAndGetAddressOf());
}

void D3d11Engine::initGeometryView_(GeometryView* /*view*/)
{
    //D3d11GeometryView* d3dGeometryView = static_cast<D3d11GeometryView*>(view);
    // no-op ?
}

void D3d11Engine::initBlendState_(BlendState* state)
{
    D3d11BlendState* d3dBlendState = static_cast<D3d11BlendState*>(state);
    D3D11_BLEND_DESC desc = {};
    desc.AlphaToCoverageEnable = state->isAlphaToCoverageEnabled();
    desc.IndependentBlendEnable = state->isIndependentBlendEnabled();
    for (Int i = 0; i < 8; ++i) {
        const TargetBlendState& subState = state->targetBlendState(i);
        D3D11_RENDER_TARGET_BLEND_DESC& subDesc = desc.RenderTarget[i];
        subDesc.BlendEnable = subState.isEnabled();
        subDesc.SrcBlend = blendFactorToD3DBlend(subState.equationRGB().sourceFactor());
        subDesc.DestBlend = blendFactorToD3DBlend(subState.equationRGB().targetFactor());
        subDesc.BlendOp = blendOpToD3DBlendOp(subState.equationRGB().operation());
        subDesc.SrcBlendAlpha = blendFactorToD3DBlend(subState.equationAlpha().sourceFactor());
        subDesc.DestBlendAlpha = blendFactorToD3DBlend(subState.equationAlpha().targetFactor());
        subDesc.BlendOpAlpha = blendOpToD3DBlendOp(subState.equationAlpha().operation());
        subDesc.RenderTargetWriteMask = 0;
        if (!!(subState.writeMask() & BlendWriteMask::R)) {
            subDesc.RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_RED;
        }
        if (!!(subState.writeMask() & BlendWriteMask::G)) {
            subDesc.RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_GREEN;
        }
        if (!!(subState.writeMask() & BlendWriteMask::B)) {
            subDesc.RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_BLUE;
        }
        if (!!(subState.writeMask() & BlendWriteMask::A)) {
            subDesc.RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_ALPHA;
        }
    }
    device_->CreateBlendState(&desc, d3dBlendState->object_.releaseAndGetAddressOf());
}

void D3d11Engine::initRasterizerState_(RasterizerState* state)
{
    D3d11RasterizerState* d3dRasterizerState = static_cast<D3d11RasterizerState*>(state);
    D3D11_RASTERIZER_DESC desc = {};
    desc.FillMode = fillModeToD3DFillMode(state->fillMode());
    desc.CullMode = cullModeToD3DCullMode(state->cullMode());
    desc.FrontCounterClockwise = state->isFrontCounterClockwise();
    //desc.DepthBias;
    //desc.DepthBiasClamp;
    //desc.SlopeScaledDepthBias;
    desc.DepthClipEnable = state->isDepthClippingEnabled();
    desc.ScissorEnable = state->isScissoringEnabled();
    desc.MultisampleEnable = state->isMultisamplingEnabled();
    desc.AntialiasedLineEnable = state->isLineAntialiasingEnabled();
    device_->CreateRasterizerState(&desc, d3dRasterizerState->object_.releaseAndGetAddressOf());
}

void D3d11Engine::setSwapChain_(const SwapChainPtr& swapChain)
{
    D3d11SwapChain* d3dSwapChain = swapChain.get_static_cast<D3d11SwapChain>();
    setFramebuffer_(d3dSwapChain->defaultFrameBuffer());
    const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
    deviceCtx_->OMSetDepthStencilState(depthStencilState_.get(), 0);
    deviceCtx_->HSSetShader(NULL, NULL, 0);
    deviceCtx_->DSSetShader(NULL, NULL, 0);
    deviceCtx_->CSSetShader(NULL, NULL, 0);
}

void D3d11Engine::setFramebuffer_(const FramebufferPtr& framebuffer)
{
    if (!framebuffer) {
        deviceCtx_->OMSetRenderTargets(0, NULL, NULL);
        boundFramebuffer_.reset();
        return;
    }
    D3d11Framebuffer* d3dFramebuffer = framebuffer.get_static_cast<D3d11Framebuffer>();
    ID3D11RenderTargetView* rtvArray[1] = { d3dFramebuffer->rtvObject() };
    deviceCtx_->OMSetRenderTargets(1, rtvArray, d3dFramebuffer->dsvObject());
    boundFramebuffer_ = framebuffer;
}

void D3d11Engine::setViewport_(Int x, Int y, Int width, Int height)
{
    D3D11_VIEWPORT vp = {};
    vp.TopLeftX = static_cast<float>(x);
    vp.TopLeftY = static_cast<float>(y);
    vp.Width = static_cast<float>(width);
    vp.Height = static_cast<float>(height);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    deviceCtx_->RSSetViewports(1, &vp);
}

void D3d11Engine::setProgram_(const ProgramPtr& program)
{
    D3d11Program* d3dProgram = program.get_static_cast<D3d11Program>();
    deviceCtx_->VSSetShader(d3dProgram->vertexShader_.get(), NULL, 0);
    deviceCtx_->PSSetShader(d3dProgram->pixelShader_.get(), NULL, 0);
    deviceCtx_->GSSetShader(d3dProgram->geometryShader_.get(), NULL, 0);
    builtinLayouts_ = d3dProgram->builtinLayouts_;
}

void D3d11Engine::setBlendState_(const BlendStatePtr& state, const geometry::Vec4f& blendFactor)
{
    D3d11BlendState* d3dBlendState = state.get_static_cast<D3d11BlendState>();
    deviceCtx_->OMSetBlendState(d3dBlendState->object(), blendFactor.data(), 0xFFFFFFFF);
}

void D3d11Engine::setRasterizerState_(const RasterizerStatePtr& state)
{
    D3d11RasterizerState* d3dRasterizerState = state.get_static_cast<D3d11RasterizerState>();
    deviceCtx_->RSSetState(d3dRasterizerState->object());
}

void D3d11Engine::setStageConstantBuffers_(BufferPtr const* buffers, Int startIndex, Int count, ShaderStage shaderStage)
{
    const Int stageIdx = core::toUnderlying(shaderStage);
    StageConstantBufferArray& boundConstantBufferArray = boundConstantBufferArrays_[core::toUnderlying(shaderStage)];
    for (BufferPtr& buffer : boundConstantBufferArray) {
        D3d11Buffer* d3dBuffer = buffer.get_static_cast<D3d11Buffer>();
        if (d3dBuffer) {
            d3dBuffer->isBoundToD3dStage_[stageIdx] = false;
        }
    }

    std::array<ID3D11Buffer*, maxConstantBuffersPerStage> d3d11Buffers = {};
    for (Int i = 0; i < count; ++i) {
        boundConstantBufferArray[startIndex + i] = buffers[i];
        D3d11Buffer* d3dBuffer = buffers[i].get_static_cast<D3d11Buffer>();
        d3d11Buffers[i] = d3dBuffer ? d3dBuffer->object() : nullptr;
    }

    for (BufferPtr& buffer : boundConstantBufferArray) {
        D3d11Buffer* d3dBuffer = buffer.get_static_cast<D3d11Buffer>();
        if (d3dBuffer) {
            d3dBuffer->isBoundToD3dStage_[stageIdx] = true;
        }
    }

    size_t stageIndex = toIndex_(shaderStage);
    void (ID3D11DeviceContext::* setConstantBuffers)(UINT, UINT, ID3D11Buffer* const*) = std::array{
        &ID3D11DeviceContext::VSSetConstantBuffers,
        &ID3D11DeviceContext::GSSetConstantBuffers,
        &ID3D11DeviceContext::PSSetConstantBuffers,
    }[stageIndex];

    (deviceCtx_.get()->*setConstantBuffers)(
        static_cast<UINT>(startIndex), static_cast<UINT>(count), d3d11Buffers.data());
}

void D3d11Engine::setStageImageViews_(ImageViewPtr const* views, Int startIndex, Int count, ShaderStage shaderStage)
{
    const Int stageIdx = core::toUnderlying(shaderStage);
    StageImageViewArray& boundImageViewArray = boundImageViewArrays_[core::toUnderlying(shaderStage)];
    for (ImageViewPtr& view : boundImageViewArray) {
        D3d11ImageView* d3dView = view.get_static_cast<D3d11ImageView>();
        if (d3dView) {
            d3dView->isBoundToD3dStage_[stageIdx] = false;
        }
    }

    std::array<ID3D11ShaderResourceView*, maxImageViewsPerStage> d3d11SRVs = {};
    for (Int i = 0; i < count; ++i) {
        boundImageViewArray[startIndex + i] = views[i];
        D3d11ImageView* d3dView = views[i].get_static_cast<D3d11ImageView>();
        d3d11SRVs[i] = d3dView ? d3dView->srvObject() : nullptr;
    }

    for (ImageViewPtr& view : boundImageViewArray) {
        D3d11ImageView* d3dView = view.get_static_cast<D3d11ImageView>();
        if (d3dView) {
            d3dView->isBoundToD3dStage_[stageIdx] = true;
        }
    }

    size_t stageIndex = toIndex_(shaderStage);
    void (ID3D11DeviceContext::* setShaderResources)(UINT, UINT, ID3D11ShaderResourceView* const*) = std::array{
        &ID3D11DeviceContext::VSSetShaderResources,
        &ID3D11DeviceContext::GSSetShaderResources,
        &ID3D11DeviceContext::PSSetShaderResources,
    }[stageIndex];

    (deviceCtx_.get()->*setShaderResources)(
        static_cast<UINT>(startIndex), static_cast<UINT>(count), d3d11SRVs.data());
}

void D3d11Engine::setStageSamplers_(SamplerStatePtr const* states, Int startIndex, Int count, ShaderStage shaderStage)
{
    std::array<ID3D11SamplerState*, maxSamplersPerStage> d3d11SamplerStates = {};
    for (Int i = 0; i < count; ++i) {
        SamplerState* state = states[i].get();
        d3d11SamplerStates[i] = state ? static_cast<const D3d11SamplerState*>(state)->object() : nullptr;
    }

    size_t stageIndex = toIndex_(shaderStage);
    void (ID3D11DeviceContext::* setSamplers)(UINT, UINT, ID3D11SamplerState* const*) = std::array{
        &ID3D11DeviceContext::VSSetSamplers,
        &ID3D11DeviceContext::GSSetSamplers,
        &ID3D11DeviceContext::PSSetSamplers,
    }[stageIndex];

    (deviceCtx_.get()->*setSamplers)(
        static_cast<UINT>(startIndex), static_cast<UINT>(count), d3d11SamplerStates.data());
}

void D3d11Engine::updateBufferData_(Buffer* buffer, const void* data, Int lengthInBytes)
{
    D3d11Buffer* d3dBuffer = static_cast<D3d11Buffer*>(buffer);
    loadBuffer_(d3dBuffer, data, lengthInBytes);
    d3dBuffer->gpuLengthInBytes_ = lengthInBytes;
}

void D3d11Engine::draw_(GeometryView* view, UInt numIndices, UInt numInstances)
{
    UINT nIdx = core::int_cast<UINT>(numIndices);
    UINT nInst = core::int_cast<UINT>(numInstances);

    if (nIdx == 0) {
        return;
    }

    //PrimitiveType view->primitiveType()
    D3d11GeometryView* d3dGeometryView = static_cast<D3d11GeometryView*>(view);

    ID3D11InputLayout* d3d11Layout = builtinLayouts_[core::toUnderlying(view->builtinGeometryLayout())].get();
    if (d3d11Layout != layout_) {
        deviceCtx_->IASetInputLayout(d3d11Layout);
        layout_ = d3d11Layout;
    }

    std::array<ID3D11Buffer*, maxAttachedVertexBuffers> d3d11VertexBuffers = {};
    for (Int i = 0; i < maxAttachedVertexBuffers; ++i) {
        Buffer* vb = view->vertexBuffer(i).get();
        d3d11VertexBuffers[i] = vb ? static_cast<D3d11Buffer*>(vb)->object() : nullptr;
    }

    deviceCtx_->IASetVertexBuffers(
        0, maxAttachedVertexBuffers,
        d3d11VertexBuffers.data(),
        view->strides().data(),
        view->offsets().data());

    D3D11_PRIMITIVE_TOPOLOGY topology = d3dGeometryView->topology();
    if (topology != topology_) {
        topology_ = topology;
        deviceCtx_->IASetPrimitiveTopology(topology);
    }

    D3d11Buffer* indexBuffer = view->indexBuffer().get_static_cast<D3d11Buffer>();

    if (numInstances == 0) {
        if (indexBuffer) {
            deviceCtx_->DrawIndexed(nIdx, 0, 0);
        }
        else {
            deviceCtx_->Draw(nIdx, 0);
        }
    }
    else {
        if (indexBuffer) {
            deviceCtx_->DrawIndexedInstanced(nIdx, nInst, 0, 0, 0);
        }
        else {
            deviceCtx_->DrawInstanced(nIdx, nInst, 0, 0);
        }
    }
}

void D3d11Engine::clear_(const core::Color& color)
{
    ComPtr<ID3D11RenderTargetView> rtv;
    deviceCtx_->OMGetRenderTargets(1, rtv.releaseAndGetAddressOf(), NULL);
    std::array<float, 4> c = {
        static_cast<float>(color.r()),
        static_cast<float>(color.g()),
        static_cast<float>(color.b()),
        static_cast<float>(color.a())};
    deviceCtx_->ClearRenderTargetView(rtv.get(), c.data());
}

UInt64 D3d11Engine::present_(SwapChain* swapChain, UInt32 syncInterval, PresentFlags /*flags*/)
{
    D3d11SwapChain* d3dSwapChain = static_cast<D3d11SwapChain*>(swapChain);
    d3dSwapChain->dxgiSwapChain()->Present(syncInterval, 0);
    return std::chrono::nanoseconds(std::chrono::steady_clock::now() - engineStartTime()).count();
}


//void D3d11Engine::setProjectionMatrix_(const geometry::Mat4f& m)
//{
//    paintVertexShaderConstantBuffer_.projMatrix = m;
//    writeBufferReserved_(
//        vertexConstantBuffer_.get(), &paintVertexShaderConstantBuffer_,
//        sizeof(PaintVertexShaderConstantBuffer));
//}
//
//void D3d11Engine::setViewMatrix_(const geometry::Mat4f& m)
//{
//    paintVertexShaderConstantBuffer_.viewMatrix = m;
//    writeBufferReserved_(
//        vertexConstantBuffer_.get(), &paintVertexShaderConstantBuffer_,
//        sizeof(PaintVertexShaderConstantBuffer));
//}




//void D3d11Engine::setupVertexBufferForPaintShader_(Buffer* /*buffer*/)
//{
//    // no-op
//}

//void D3d11Engine::drawPrimitives_(Buffer* buffer, PrimitiveType type)
//{
//    D3d11Buffer* d3dBuffer = static_cast<D3d11Buffer*>(buffer);
//    ID3D11Buffer* object = d3dBuffer->object();
//    if (!object) return;
//
//    D3D_PRIMITIVE_TOPOLOGY d3dTopology = {};
//    switch (type) {
//    case PrimitiveType::Point:
//        d3dTopology = D3D_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_POINTLIST; break;
//    case PrimitiveType::LineList:
//        d3dTopology = D3D_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_LINELIST; break;
//    case PrimitiveType::LineStrip:
//        d3dTopology = D3D_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP; break;
//    case PrimitiveType::TriangleList:
//        d3dTopology = D3D_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST; break;
//    case PrimitiveType::TriangleStrip:
//        d3dTopology = D3D_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP; break;
//    default:
//        throw core::LogicError("D3d11Buffer: unsupported primitive type");
//    }
//
//    Int numVertices = d3dBuffer->lengthInBytes_ / sizeof(XYRGBVertex);
//    unsigned int stride = sizeof(XYRGBVertex);
//    unsigned int offset = 0;
//    deviceCtx_->IASetVertexBuffers(0, 1, &object, &stride, &offset);
//    deviceCtx_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//    deviceCtx_->Draw(core::int_cast<UINT>(numVertices), 0);
//}



//void D3d11Engine::releasePaintShader_()
//{
//    deviceCtx_->VSSetShader(NULL, NULL, 0);
//    deviceCtx_->VSSetConstantBuffers(0, 0, NULL);
//    deviceCtx_->PSSetShader(NULL, NULL, 0);
//}

// Private methods

void D3d11Engine::initBuiltinShaders_()
{
    // no-op atm, everything was done on create
}

bool D3d11Engine::loadBuffer_(D3d11Buffer* buffer, const void* data, Int dataSize)
{
    ComPtr<ID3D11Buffer>& object = buffer->object_;
    D3D11_BUFFER_DESC& desc = buffer->desc_;
    if (dataSize == 0) {
        return false;
    }

    if ((dataSize > desc.ByteWidth) || (dataSize * 4 < desc.ByteWidth) || !object) {
        UINT dataWidth = core::int_cast<UINT>(dataSize);
        desc.ByteWidth = dataWidth;
        if (desc.BindFlags & D3D11_BIND_CONSTANT_BUFFER) {
            desc.ByteWidth = (desc.ByteWidth + 0xFu) & ~UINT(0xFu);
        }

        if (data && desc.ByteWidth == dataWidth) {
            D3D11_SUBRESOURCE_DATA srData = {};
            srData.pSysMem = data;
            if (device_->CreateBuffer(&desc, &srData, object.releaseAndGetAddressOf()) < 0) {
                desc.ByteWidth = 0;
                return false;
            }
            onBufferRecreated_(buffer);
            return true;
        }
        else if (device_->CreateBuffer(&desc, NULL, object.releaseAndGetAddressOf()) < 0) {
            desc.ByteWidth = 0;
            return false;
        }
        onBufferRecreated_(buffer);
    }
    if (data) {
        return writeBufferReserved_(object.get(), data, dataSize);
    }
    return true;
}

void D3d11Engine::onBufferRecreated_(D3d11Buffer* buffer)
{
    // do rebinds
    for (Int i = 0; i < numShaderStages; ++i) {
        if (buffer->isBoundToD3dStage_[i]) {
            StageConstantBufferArray& arr = boundConstantBufferArrays_[i];
            setStageConstantBuffers_(arr.data(), 0, arr.size(), ShaderStage(i));
        }
    }
    bool shouldRebindFramebuffer_ = false;
    std::array<bool, numShaderStages> isStageD3dImageViewArrayDirty_ = {};
    for (D3d11ImageView* view : buffer->dependentD3dImageViews_) {
        // rebuild
        initImageView_(view);
        // check if needs rebind
        for (Int i = 0; i < numShaderStages; ++i) {
            if (view->isBoundToD3dStage_[i]) {
                isStageD3dImageViewArrayDirty_[i] = true;
            }
        }
        for (D3d11Framebuffer* framebuffer : view->dependentD3dFramebuffers_) {
            // rebuild
            initFramebuffer_(framebuffer);
            // check if needs rebind
            if (framebuffer == boundFramebuffer_.get()) {
                shouldRebindFramebuffer_ = true;
            }
        }
    }
    // rebind image views if needed
    for (Int i = 0; i < numShaderStages; ++i) {
        if (isStageD3dImageViewArrayDirty_[i]) {
            StageImageViewArray& arr = boundImageViewArrays_[i];
            setStageImageViews_(arr.data(), 0, arr.size(), ShaderStage(i));
        }
    }
    // rebind framebuffer if needed
    if (shouldRebindFramebuffer_) {
        setFramebuffer_(boundFramebuffer_);
    }
}

bool D3d11Engine::writeBufferReserved_(ID3D11Buffer* object, const void* data, Int dataSize)
{
    D3D11_MAPPED_SUBRESOURCE mappedResource = {};
    if (deviceCtx_->Map(object, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource) < 0) {
        return false;
    }
    memcpy(static_cast<char*>(mappedResource.pData), data, dataSize);
    deviceCtx_->Unmap(object, 0);
    return true;
}

//void D3d11Engine::updatePaintVertexShaderConstantBuffer_()
//{
//    PaintVertexShaderConstantBuffer buffer = {};
//    memcpy(buffer.projMatrix.data(), projMatrix_.data(), 16 * sizeof(float));
//    memcpy(buffer.viewMatrix.data(), viewMatrix_.data(), 16 * sizeof(float));
//    writeBufferReserved_(
//        vertexConstantBuffer_.get(), &buffer,
//        sizeof(PaintVertexShaderConstantBuffer));
//}





} // namespace vgc::graphics

#endif // VGC_CORE_COMPILER_MSVC
