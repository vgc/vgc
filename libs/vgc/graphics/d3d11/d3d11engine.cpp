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

#include <vgc/graphics/d3d11/d3d11engine.h>

#ifdef VGC_CORE_OS_WINDOWS

// clang-format off

#include <array>
#include <chrono>
#include <filesystem>

#include <d3d11.h>
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler")
#pragma comment(lib, "d3d11")

#include <vgc/core/compiler.h>
#include <vgc/core/exceptions.h>

// clang-format on

namespace vgc::graphics {

namespace {

// Returns the file path of a shader file as a QString
std::filesystem::path shaderPath_(const std::string& name) {
    return std::filesystem::path(core::resourcePath("graphics/shaders/d3d11/" + name));
}

// core::resourcePath("graphics/d3d11/" + name);

struct Vertex_XY {
    float x, y;
};

struct Vertex_XYDxDy {
    float x, y, dx, dy;
};

struct Vertex_XYUV {
    float x, y, u, v;
};

struct Vertex_XYRGB {
    float x, y, r, g, b;
};

struct Vertex_XYRGBA {
    float x, y, r, g, b, a;
};

struct Vertex_XYRotRGBA {
    float x, y, rot, r, g, b, a;
};

struct Vertex_XYUVRGBA {
    float x, y, u, v, r, g, b, a;
};

struct Vertex_RGBA {
    float r, g, b, a;
};

} // namespace

class D3d11ImageView;
class D3d11Framebuffer;

class D3d11Buffer : public Buffer {
protected:
    friend D3d11Engine;
    using Buffer::Buffer;

public:
    ID3D11Buffer* object() const {
        return object_.get();
    }

    const D3D11_BUFFER_DESC& desc() const {
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
    ID3D11Resource* object() const {
        return object_.get();
    }

    DXGI_FORMAT dxgiFormat() const {
        return dxgiFormat_;
    }

protected:
    void release_(Engine* engine) override {
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

    D3d11ImageView(
        ResourceRegistry* registry,
        const ImageViewCreateInfo& createInfo,
        const ImagePtr& image)

        : ImageView(registry, createInfo, image) {
    }

    D3d11ImageView(
        ResourceRegistry* registry,
        const ImageViewCreateInfo& createInfo,
        const BufferPtr& buffer,
        PixelFormat format,
        UInt32 numBufferElements)

        : ImageView(registry, createInfo, buffer, format, numBufferElements) {

        d3dBuffer_ = buffer.get_static_cast<D3d11Buffer>();
        d3dBuffer_->dependentD3dImageViews_.append(this);
    }

public:
    ID3D11ShaderResourceView* srvObject() const {
        return srv_.get();
    }

    ID3D11RenderTargetView* rtvObject() const {
        return rtv_.get();
    }

    ID3D11DepthStencilView* dsvObject() const {
        return dsv_.get();
    }

    DXGI_FORMAT dxgiFormat() const {
        return dxgiFormat_;
    }

    ID3D11Resource* d3dViewedResource() const {
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
    void release_(Engine* engine) override {
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
    void release_(Engine* engine) override {
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
    void release_(Engine* engine) override {
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
    std::array<ComPtr<ID3D11InputLayout>, numBuiltinGeometryLayouts> builtinLayouts_;
};
using D3d11ProgramPtr = ResourcePtr<D3d11Program>;

class D3d11BlendState : public BlendState {
protected:
    friend D3d11Engine;
    using BlendState::BlendState;

public:
    ID3D11BlendState* object() const {
        return object_.get();
    }

protected:
    void release_(Engine* engine) override {
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
    ID3D11RasterizerState* object() const {
        return object_.get();
    }

protected:
    void release_(Engine* engine) override {
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
        const D3d11ImageViewPtr& depthStencilView)

        : Framebuffer(registry)
        , colorView_(colorView)
        , depthStencilView_(depthStencilView) {

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
    ID3D11RenderTargetView* rtvObject() const {
        return colorView_ ? static_cast<ID3D11RenderTargetView*>(colorView_->rtvObject())
                          : nullptr;
    }

    ID3D11DepthStencilView* dsvObject() const {
        return depthStencilView_
                   ? static_cast<ID3D11DepthStencilView*>(depthStencilView_->dsvObject())
                   : nullptr;
    }

protected:
    void releaseSubResources_() override {
        colorView_.reset();
        depthStencilView_.reset();
    }

    void release_(Engine* engine) override;

private:
    D3d11ImageViewPtr colorView_;
    D3d11ImageViewPtr depthStencilView_;

    bool isBoundToD3D_ = false;

    // used to clear backpointers at release time
    friend D3d11ImageView;
    D3d11ImageView* d3dColorView_ = nullptr;
    D3d11ImageView* d3dDepthStencilView_ = nullptr;
};
using D3d11FramebufferPtr = ResourcePtr<D3d11Framebuffer>;

void D3d11Buffer::release_(Engine* engine) {
    Buffer::release_(engine);
    object_.reset();
    for (D3d11ImageView* view : dependentD3dImageViews_) {
        view->d3dBuffer_ = nullptr;
    }
    dependentD3dImageViews_.clear();
}

void D3d11ImageView::release_(Engine* engine) {
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

void D3d11Framebuffer::release_(Engine* engine) {
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

#    ifdef USE_DXGI_VERSION_1_2
using IDXGISwapChainX = IDXGISwapChain1;
#    else
using IDXGISwapChainX = IDXGISwapChain;
#    endif

class D3d11SwapChain : public SwapChain {
protected:
    friend D3d11Engine;

    using SwapChain::SwapChain;

public:
    IDXGISwapChainX* dxgiSwapChain() const {
        return dxgiSwapChain_.get();
    }

    ID3D11RenderTargetView* rtvObject() {
        return rtv_.get();
    }

protected:
    void release_(Engine* engine) override {
        SwapChain::release_(engine);
        rtv_.reset();
        dxgiSwapChain_.reset();
    }

private:
    ComPtr<IDXGISwapChainX> dxgiSwapChain_;
    ComPtr<ID3D11RenderTargetView> rtv_;
};

// ENUM CONVERSIONS

DXGI_FORMAT pixelFormatToDxgiFormat(PixelFormat format) {

    constexpr size_t numPixelFormats = VGC_ENUM_COUNT(PixelFormat);
    static_assert(numPixelFormats == 47);
    static constexpr std::array<DXGI_FORMAT, numPixelFormats> map = {
#    define VGC_PIXEL_FORMAT_MACRO_(                                                     \
        Enumerator,                                                                      \
        ElemSizeInBytes,                                                                 \
        DXGIFormat,                                                                      \
        OpenGLInternalFormat,                                                            \
        OpenGLPixelType,                                                                 \
        OpenGLPixelFormat)                                                               \
        DXGIFormat,
#    include <vgc/graphics/detail/pixelformats.h>
    };

    const UInt index = core::toUnderlying(format);
    if (index == 0 || index >= numPixelFormats) {
        throw core::LogicError("D3d11Engine: invalid PixelFormat enum value.");
    }

    return map[index];
}

D3D_PRIMITIVE_TOPOLOGY primitiveTypeToD3DPrimitiveTopology(PrimitiveType type) {

    constexpr size_t numPrimitiveTypes = VGC_ENUM_COUNT(PrimitiveType);
    static_assert(numPrimitiveTypes == 6);
    static constexpr std::array<D3D_PRIMITIVE_TOPOLOGY, numPrimitiveTypes> map = {
        D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED,     // Undefined,
        D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,     // Point,
        D3D11_PRIMITIVE_TOPOLOGY_LINELIST,      // LineList,
        D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,     // LineStrip,
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,  // TriangleList,
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, // TriangleStrip,
    };

    const UInt index = core::toUnderlying(type);
    if (index == 0 || index >= numPrimitiveTypes) {
        throw core::LogicError("D3d11Engine: invalid PrimitiveType enum value.");
    }

    return map[index];
}

D3D11_USAGE usageToD3DUsage(Usage usage) {
    constexpr size_t numUsages = VGC_ENUM_COUNT(Usage);
    static_assert(numUsages == 4);
    static constexpr std::array<D3D11_USAGE, numUsages> map = {
        D3D11_USAGE_DEFAULT,   // Default
        D3D11_USAGE_IMMUTABLE, // Immutable
        D3D11_USAGE_DYNAMIC,   // Dynamic
        D3D11_USAGE_STAGING,   // Staging
    };

    const UInt index = core::toUnderlying(usage);
    if (index >= numUsages) {
        throw core::LogicError("D3d11Engine: invalid Usage enum value.");
    }

    return map[index];
}

UINT resourceMiscFlagsToD3DResourceMiscFlags(ResourceMiscFlags resourceMiscFlags) {
    UINT x = 0;
    if (resourceMiscFlags & ResourceMiscFlag::Shared) {
        x |= D3D11_RESOURCE_MISC_SHARED;
    }
    //if (resourceMiscFlags & ResourceMiscFlag::DrawIndirectArgs) {
    //    x |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
    //}
    //if (resourceMiscFlags & ResourceMiscFlag::BufferRaw) {
    //    x |= D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
    //}
    //if (resourceMiscFlags & ResourceMiscFlag::BufferStructured) {
    //    x |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    //}
    //if (resourceMiscFlags & ResourceMiscFlag::ResourceClamp) {
    //    x |= D3D11_RESOURCE_MISC_RESOURCE_CLAMP;
    //}
    //if (resourceMiscFlags & ResourceMiscFlag::SharedKeyedMutex) {
    //    x |= D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;
    //}
    return x;
}

D3D11_TEXTURE_ADDRESS_MODE imageWrapModeToD3DTextureAddressMode(ImageWrapMode mode) {

    constexpr size_t numImageWrapModes = VGC_ENUM_COUNT(ImageWrapMode);
    static_assert(numImageWrapModes == 5);
    static constexpr std::array<D3D11_TEXTURE_ADDRESS_MODE, numImageWrapModes> map = {
        D3D11_TEXTURE_ADDRESS_MODE(0), // Undefined
        D3D11_TEXTURE_ADDRESS_WRAP,    // Repeat
        D3D11_TEXTURE_ADDRESS_MIRROR,  // MirrorRepeat
        D3D11_TEXTURE_ADDRESS_CLAMP,   // Clamp
        D3D11_TEXTURE_ADDRESS_BORDER,  // ClampConstantColor
    };

    const UInt index = core::toUnderlying(mode);
    if (index == 0 || index >= numImageWrapModes) {
        throw core::LogicError("D3d11Engine: invalid ImageWrapMode enum value.");
    }

    return map[index];
}

D3D11_COMPARISON_FUNC comparisonFunctionToD3DComparisonFunc(ComparisonFunction func) {

    constexpr size_t numComparisonFunctions = VGC_ENUM_COUNT(ComparisonFunction);
    static_assert(numComparisonFunctions == 10);
    static constexpr std::array<D3D11_COMPARISON_FUNC, numComparisonFunctions> map = {
        D3D11_COMPARISON_FUNC(0),       // Undefined
        D3D11_COMPARISON_ALWAYS,        // Disabled
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
        throw core::LogicError("D3d11Engine: invalid ComparisonFunction enum value.");
    }

    return map[index];
}

D3D11_BLEND blendFactorToD3DBlend(BlendFactor factor) {

    constexpr size_t numBlendFactors = VGC_ENUM_COUNT(BlendFactor);
    static_assert(numBlendFactors == 18);
    static constexpr std::array<D3D11_BLEND, numBlendFactors> map = {
        D3D11_BLEND(0),               // Undefined
        D3D11_BLEND_ONE,              // One
        D3D11_BLEND_ZERO,             // Zero
        D3D11_BLEND_SRC_COLOR,        // SourceColor
        D3D11_BLEND_INV_SRC_COLOR,    // OneMinusSourceColor
        D3D11_BLEND_SRC_ALPHA,        // SourceAlpha
        D3D11_BLEND_INV_SRC_ALPHA,    // OneMinusSourceAlpha
        D3D11_BLEND_DEST_COLOR,       // TargetColor
        D3D11_BLEND_INV_DEST_COLOR,   // OneMinusTargetColor
        D3D11_BLEND_DEST_ALPHA,       // TargetAlpha
        D3D11_BLEND_INV_DEST_ALPHA,   // OneMinusTargetAlpha
        D3D11_BLEND_SRC_ALPHA_SAT,    // SourceAlphaSaturated
        D3D11_BLEND_BLEND_FACTOR,     // Constant
        D3D11_BLEND_INV_BLEND_FACTOR, // OneMinusConstant
        D3D11_BLEND_SRC1_COLOR,       // SecondSourceColor
        D3D11_BLEND_INV_SRC1_COLOR,   // OneMinusSecondSourceColor
        D3D11_BLEND_SRC1_ALPHA,       // SecondSourceAlpha
        D3D11_BLEND_INV_SRC1_ALPHA,   // OneMinusSecondSourceAlpha
    };

    const UInt index = core::toUnderlying(factor);
    if (index == 0 || index >= numBlendFactors) {
        throw core::LogicError("D3d11Engine: invalid BlendFactor enum value.");
    }

    return map[index];
}

D3D11_BLEND_OP blendOpToD3DBlendOp(BlendOp op) {

    constexpr size_t numBlendOps = VGC_ENUM_COUNT(BlendOp);
    static_assert(numBlendOps == 6);
    static constexpr std::array<D3D11_BLEND_OP, numBlendOps> map = {
        D3D11_BLEND_OP(0),           // Undefined
        D3D11_BLEND_OP_ADD,          // Add
        D3D11_BLEND_OP_SUBTRACT,     // SourceMinusTarget
        D3D11_BLEND_OP_REV_SUBTRACT, // TargetMinusSource
        D3D11_BLEND_OP_MIN,          // Min
        D3D11_BLEND_OP_MAX,          // Max
    };

    const UInt index = core::toUnderlying(op);
    if (index == 0 || index >= numBlendOps) {
        throw core::LogicError("D3d11Engine: invalid BlendOp enum value.");
    }

    return map[index];
}

D3D11_FILL_MODE fillModeToD3DFillMode(FillMode mode) {

    constexpr size_t numFillModes = VGC_ENUM_COUNT(FillMode);
    static_assert(numFillModes == 3);
    static constexpr std::array<D3D11_FILL_MODE, numFillModes> map = {
        D3D11_FILL_MODE(0),   // Undefined
        D3D11_FILL_SOLID,     // Solid
        D3D11_FILL_WIREFRAME, // Wireframe
    };

    const UInt index = core::toUnderlying(mode);
    if (index == 0 || index >= numFillModes) {
        throw core::LogicError("D3d11Engine: invalid FillMode enum value.");
    }

    return map[index];
}

D3D11_CULL_MODE cullModeToD3DCullMode(CullMode mode) {

    constexpr size_t numCullModes = VGC_ENUM_COUNT(CullMode);
    static_assert(numCullModes == 4);
    static constexpr std::array<D3D11_CULL_MODE, numCullModes> map = {
        D3D11_CULL_MODE(0), // Undefined
        D3D11_CULL_NONE,    // None
        D3D11_CULL_FRONT,   // Front
        D3D11_CULL_BACK,    // Back
    };

    const UInt index = core::toUnderlying(mode);
    if (index == 0 || index >= numCullModes) {
        throw core::LogicError("D3d11Engine: invalid CullMode enum value.");
    }

    return map[index];
}

// ENGINE FUNCTIONS

D3d11Engine::D3d11Engine(const EngineCreateInfo& createInfo)
    : Engine(createInfo) {

    // XXX add success checks (S_OK)

    // Setup creation flags.
    // https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_create_device_flag
    //
    // Note 1: To use D3D11_CREATE_DEVICE_DEBUG, end users must have
    // D3D11*SDKLayers.dll installed; otherwise, device creation fails.
    //
    // Note 2: We could use D3D11_CREATE_DEVICE_SINGLETHREADED
    // if we defer creation of buffers and swapchain.
    //
    UINT creationFlags = 0;
#    ifdef VGC_DEBUG_BUILD
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#    endif

    const D3D_FEATURE_LEVEL featureLevels[1] = {D3D_FEATURE_LEVEL_11_0};
    D3D11CreateDevice(
        NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        creationFlags,
        featureLevels,
        1,
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

    //createBuiltinResources_();
}

void D3d11Engine::onDestroyed() {
    Engine::onDestroyed();
}

/* static */
D3d11EnginePtr D3d11Engine::create(const EngineCreateInfo& createInfo) {
    D3d11EnginePtr engine(new D3d11Engine(createInfo));
    engine->init_();
    return engine;
}

// USER THREAD pimpl functions

// clang-format off

void D3d11Engine::createBuiltinShaders_() {

    D3d11ProgramPtr simpleProgram(
        new D3d11Program(resourceRegistry_, BuiltinProgram::Simple));
    simpleProgram_ = simpleProgram;

    // Create the simple shader (vertex)
    {
        D3d11Program* program = simpleProgram.get();
        ComPtr<ID3DBlob> errorBlob;
        ComPtr<ID3DBlob> vertexShaderBlob;
        HRESULT hres = D3DCompileFromFile(
            shaderPath_("simple.v.hlsl").wstring().c_str(),
            NULL, NULL, "main", "vs_4_0", 0, 0,
            vertexShaderBlob.releaseAndGetAddressOf(),
            errorBlob.releaseAndGetAddressOf());

        if (hres < 0) {
            std::string errString =
                (errorBlob ? std::string(
                     static_cast<const char*>(errorBlob->GetBufferPointer()))
                           : core::format("unknown D3DCompile error (0x{:X}).", hres));
            throw core::RuntimeError(errString);
        }
        errorBlob.reset();

        ComPtr<ID3D11VertexShader> vertexShader;
        device_->CreateVertexShader(
            vertexShaderBlob->GetBufferPointer(),
            vertexShaderBlob->GetBufferSize(),
            NULL,
            vertexShader.releaseAndGetAddressOf());
        program->vertexShader_ = vertexShader;

        // Create Input Layout for XYRGB
        {
            ComPtr<ID3D11InputLayout> inputLayout;
            UINT rOffset = static_cast<UINT>(offsetof(Vertex_XYRGB, r));
            D3D11_INPUT_ELEMENT_DESC layout[] = {
                {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 0,       D3D11_INPUT_PER_VERTEX_DATA, 0},
                {"COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, rOffset, D3D11_INPUT_PER_VERTEX_DATA, 0},
            };
            device_->CreateInputLayout(
                layout, 2,
                vertexShaderBlob->GetBufferPointer(),
                vertexShaderBlob->GetBufferSize(),
                inputLayout.releaseAndGetAddressOf());

            constexpr Int8 layoutIndex = core::toUnderlying(BuiltinGeometryLayout::XYRGB);
            program->builtinLayouts_[layoutIndex] = inputLayout;
        }

        // Create Input Layout for XYRGBA
        {
            ComPtr<ID3D11InputLayout> inputLayout;
            UINT rOffset = static_cast<UINT>(offsetof(Vertex_XYRGBA, r));
            D3D11_INPUT_ELEMENT_DESC layout[] = {
                {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 0,       D3D11_INPUT_PER_VERTEX_DATA, 0},
                {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, rOffset, D3D11_INPUT_PER_VERTEX_DATA, 0},
            };
            device_->CreateInputLayout(
                layout, 2,
                vertexShaderBlob->GetBufferPointer(),
                vertexShaderBlob->GetBufferSize(),
                inputLayout.releaseAndGetAddressOf());

            constexpr Int8 layoutIndex = core::toUnderlying(BuiltinGeometryLayout::XYRGBA);
            program->builtinLayouts_[layoutIndex] = inputLayout;
        }

        // Create Input Layout for XY_iRGBA
        {
            ComPtr<ID3D11InputLayout> inputLayout;
            D3D11_INPUT_ELEMENT_DESC layout[] = {
                {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 0, D3D11_INPUT_PER_VERTEX_DATA,   0},
                {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 0},
            };
            device_->CreateInputLayout(
                layout, 2,
                vertexShaderBlob->GetBufferPointer(),
                vertexShaderBlob->GetBufferSize(),
                inputLayout.releaseAndGetAddressOf());

            constexpr Int8 layoutIndex = core::toUnderlying(BuiltinGeometryLayout::XY_iRGBA);
            program->builtinLayouts_[layoutIndex] = inputLayout;
        }
    }

    // Create the simple shader (fragment)
    {
        D3d11Program* program = simpleProgram.get();
        ComPtr<ID3DBlob> errorBlob;
        ComPtr<ID3DBlob> pixelShaderBlob;

        HRESULT hres = D3DCompileFromFile(
            shaderPath_("simple.f.hlsl").wstring().c_str(),
            NULL, NULL, "main", "ps_4_0", 0, 0,
            pixelShaderBlob.releaseAndGetAddressOf(),
            errorBlob.releaseAndGetAddressOf());

        if (hres < 0) {
            std::string errString =
                (errorBlob ? std::string(
                     static_cast<const char*>(errorBlob->GetBufferPointer()))
                           : core::format("unknown D3DCompile error (0x{:X}).", hres));
            throw core::RuntimeError(errString);
        }
        errorBlob.reset();

        ComPtr<ID3D11PixelShader> pixelShader;
        device_->CreatePixelShader(
            pixelShaderBlob->GetBufferPointer(),
            pixelShaderBlob->GetBufferSize(),
            NULL,
            pixelShader.releaseAndGetAddressOf());
        program->pixelShader_ = pixelShader;
    }

    D3d11ProgramPtr simpleTexturedProgram(
        new D3d11Program(resourceRegistry_, BuiltinProgram::SimpleTextured));
    simpleTexturedProgram_ = simpleTexturedProgram;

    // Create the simple textured shader (vertex)
    {
        D3d11Program* program = simpleTexturedProgram.get();

        ComPtr<ID3DBlob> errorBlob;
        ComPtr<ID3DBlob> vertexShaderBlob;
        HRESULT hres = D3DCompileFromFile(
            shaderPath_("simple_textured.v.hlsl").wstring().c_str(),
            NULL, NULL, "main", "vs_4_0", 0, 0,
            vertexShaderBlob.releaseAndGetAddressOf(),
            errorBlob.releaseAndGetAddressOf());

        if (hres < 0) {
            std::string errString =
                (errorBlob ? std::string(
                    static_cast<const char*>(errorBlob->GetBufferPointer()))
                    : core::format("unknown D3DCompile error (0x{:X}).", hres));
            throw core::RuntimeError(errString);
        }
        errorBlob.reset();

        ComPtr<ID3D11VertexShader> vertexShader;
        device_->CreateVertexShader(
            vertexShaderBlob->GetBufferPointer(),
            vertexShaderBlob->GetBufferSize(),
            NULL,
            vertexShader.releaseAndGetAddressOf());
        program->vertexShader_ = vertexShader;

        // Create Input Layout for XYUVRGBA
        {
            ComPtr<ID3D11InputLayout> inputLayout;
            UINT uOffset = static_cast<UINT>(offsetof(Vertex_XYUVRGBA, u));
            UINT rOffset = static_cast<UINT>(offsetof(Vertex_XYUVRGBA, r));
            D3D11_INPUT_ELEMENT_DESC layout[] = {
                {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 0,       D3D11_INPUT_PER_VERTEX_DATA, 0},
                {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, uOffset, D3D11_INPUT_PER_VERTEX_DATA, 0},
                {"COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, rOffset, D3D11_INPUT_PER_VERTEX_DATA, 0},
            };
            device_->CreateInputLayout(
                layout, 3,
                vertexShaderBlob->GetBufferPointer(),
                vertexShaderBlob->GetBufferSize(),
                inputLayout.releaseAndGetAddressOf());

            constexpr Int8 layoutIndex = core::toUnderlying(BuiltinGeometryLayout::XYUVRGBA);
            program->builtinLayouts_[layoutIndex] = inputLayout;
        }

        // Create Input Layout for XYUV_iRGBA
        {
            ComPtr<ID3D11InputLayout> inputLayout;
            UINT uOffset = static_cast<UINT>(offsetof(Vertex_XYUV, u));
            D3D11_INPUT_ELEMENT_DESC layout[] = {
                {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 0,       D3D11_INPUT_PER_VERTEX_DATA,   0},
                {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, uOffset, D3D11_INPUT_PER_VERTEX_DATA,   0},
                {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0,       D3D11_INPUT_PER_INSTANCE_DATA, 0},
            };
            device_->CreateInputLayout(
                layout, 3,
                vertexShaderBlob->GetBufferPointer(),
                vertexShaderBlob->GetBufferSize(),
                inputLayout.releaseAndGetAddressOf());

            constexpr Int8 layoutIndex = core::toUnderlying(BuiltinGeometryLayout::XYUV_iRGBA);
            program->builtinLayouts_[layoutIndex] = inputLayout;
        }
    }

    // Create the simple textured shader (fragment)
    {
        D3d11Program* program = simpleTexturedProgram.get();
        ComPtr<ID3DBlob> errorBlob;
        ComPtr<ID3DBlob> pixelShaderBlob;

        HRESULT hres = D3DCompileFromFile(
            shaderPath_("simple_textured.f.hlsl").wstring().c_str(),
            NULL, NULL, "main", "ps_4_0", 0, 0,
            pixelShaderBlob.releaseAndGetAddressOf(),
            errorBlob.releaseAndGetAddressOf());

        if (hres < 0) {
            std::string errString =
                (errorBlob ? std::string(
                    static_cast<const char*>(errorBlob->GetBufferPointer()))
                    : core::format("unknown D3DCompile error (0x{:X}).", hres));
            throw core::RuntimeError(errString);
        }
        errorBlob.reset();

        ComPtr<ID3D11PixelShader> pixelShader;
        device_->CreatePixelShader(
            pixelShaderBlob->GetBufferPointer(),
            pixelShaderBlob->GetBufferSize(),
            NULL,
            pixelShader.releaseAndGetAddressOf());
        program->pixelShader_ = pixelShader;
    }

    D3d11ProgramPtr sreenSpaceDisplacementProgram(
        new D3d11Program(resourceRegistry_, BuiltinProgram::SimpleTextured));
    sreenSpaceDisplacementProgram_ = sreenSpaceDisplacementProgram;

    // Create the sreen-space displacement shader (vertex)
    {
        D3d11Program* program = sreenSpaceDisplacementProgram.get();

        ComPtr<ID3DBlob> errorBlob;
        ComPtr<ID3DBlob> vertexShaderBlob;
        HRESULT hres = D3DCompileFromFile(
            shaderPath_("screen_space_displacement.v.hlsl").wstring().c_str(),
            NULL, NULL, "main", "vs_4_0", 0, 0,
            vertexShaderBlob.releaseAndGetAddressOf(),
            errorBlob.releaseAndGetAddressOf());

        if (hres < 0) {
            std::string errString =
                (errorBlob ? std::string(
                    static_cast<const char*>(errorBlob->GetBufferPointer()))
                    : core::format("unknown D3DCompile error (0x{:X}).", hres));
            throw core::RuntimeError(errString);
        }
        errorBlob.reset();

        ComPtr<ID3D11VertexShader> vertexShader;
        device_->CreateVertexShader(
            vertexShaderBlob->GetBufferPointer(),
            vertexShaderBlob->GetBufferSize(),
            NULL,
            vertexShader.releaseAndGetAddressOf());
        program->vertexShader_ = vertexShader;

        // Create Input Layout for XYDxDy_iXYRotRGBA
        {
            ComPtr<ID3D11InputLayout> inputLayout;
            UINT dxOffset = static_cast<UINT>(offsetof(Vertex_XYDxDy, dx));
            UINT rOffset = static_cast<UINT>(offsetof(Vertex_XYRotRGBA, r));
            D3D11_INPUT_ELEMENT_DESC layout[] = {
                {"POSITION",     0, DXGI_FORMAT_R32G32_FLOAT,       0, 0,        D3D11_INPUT_PER_VERTEX_DATA,   0},
                {"DISPLACEMENT", 0, DXGI_FORMAT_R32G32_FLOAT,       0, dxOffset, D3D11_INPUT_PER_VERTEX_DATA,   0},
                {"POSITION",     1, DXGI_FORMAT_R32G32B32_FLOAT,    1, 0,        D3D11_INPUT_PER_INSTANCE_DATA, 1},
                {"COLOR",        0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, rOffset,  D3D11_INPUT_PER_INSTANCE_DATA, 1},
            };
            device_->CreateInputLayout(
                layout, 4,
                vertexShaderBlob->GetBufferPointer(),
                vertexShaderBlob->GetBufferSize(),
                inputLayout.releaseAndGetAddressOf());

            constexpr Int8 layoutIndex = core::toUnderlying(BuiltinGeometryLayout::XYDxDy_iXYRotRGBA);
            program->builtinLayouts_[layoutIndex] = inputLayout;
        }
    }

    // Create the simple instanced shader (fragment)
    {
        sreenSpaceDisplacementProgram->pixelShader_ = simpleProgram->pixelShader_;
    }

    // Create depth-stencil State
    {
        D3D11_DEPTH_STENCIL_DESC desc = {};
        desc.DepthEnable = false;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
        desc.StencilEnable = false;
        desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
        desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        desc.BackFace = desc.FrontFace;
        device_->CreateDepthStencilState(
            &desc, depthStencilState_.releaseAndGetAddressOf());
    }
}

// clang-format on

SwapChainPtr D3d11Engine::constructSwapChain_(const SwapChainCreateInfo& createInfo) {
    if (!device_) {
        throw core::LogicError("device_ is null.");
    }

    if (createInfo.windowNativeHandleType() != WindowNativeHandleType::Win32) {
        return nullptr;
    }

    const WindowSwapChainFormat& wscFormat = windowSwapChainFormat();
    UINT width = static_cast<UINT>(createInfo.width());
    UINT height = static_cast<UINT>(createInfo.height());
    UINT numSamples = static_cast<UINT>(wscFormat.numSamples());
    UINT numBuffers = static_cast<UINT>(wscFormat.numBuffers());
    PixelFormat wpFormat = wscFormat.pixelFormat();
    HWND hWnd = static_cast<HWND>(createInfo.windowNativeHandle());

    ComPtr<IDXGISwapChainX> dxgiSwapChain;

#    ifdef USE_DXGI_VERSION_1_2
    DXGI_SWAP_CHAIN_DESC1 sd = {};
    sd.Width = width;
    sd.Height = height;
    sd.Format = pixelFormatToDxgiFormat(wpFormat);
    sd.Stereo = FALSE;
    if (numSamples > 1) {
        VGC_WARNING(
            LogVgcGraphics, "Flip model swapchains do not support multisampling.");
    }
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    // do we need DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT ?
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = numBuffers;
    sd.Scaling = DXGI_SCALING_NONE; // not supported on windows 7
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    if (factory_->CreateSwapChainForHwnd(
            device_.get(), hWnd, &sd, NULL, NULL, dxgiSwapChain.releaseAndGetAddressOf())
        != S_OK) {
        throw core::LogicError("D3d11Engine: could not create DXGI_1.2 swap chain.");
    }
#    else
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = numBuffers;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = pixelFormatToDxgiFormat(wpFormat);
    sd.BufferDesc.RefreshRate.Numerator = 0;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    // do we need DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT ?
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = numSamples;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = true;
    sd.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;

    if (factory_->CreateSwapChain(
            device_.get(), &sd, dxgiSwapChain.releaseAndGetAddressOf())
        < 0) {
        throw core::LogicError("D3d11Engine: could not create DXGI_1.0 swap chain.");
    }
#    endif

    ComPtr<ID3D11Texture2D> backBuffer;
    dxgiSwapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.releaseAndGetAddressOf()));

    ComPtr<ID3D11RenderTargetView> backBufferView;
    device_->CreateRenderTargetView(
        backBuffer.get(), NULL, backBufferView.releaseAndGetAddressOf());

    auto swapChain = makeUnique<D3d11SwapChain>(resourceRegistry_, createInfo);
    swapChain->dxgiSwapChain_ = dxgiSwapChain;
    swapChain->rtv_ = backBufferView;

    return SwapChainPtr(swapChain.release());
}

FramebufferPtr D3d11Engine::constructFramebuffer_(const ImageViewPtr& colorImageView) {
    auto framebuffer = makeUnique<D3d11Framebuffer>(
        resourceRegistry_, static_pointer_cast<D3d11ImageView>(colorImageView), nullptr);
    return FramebufferPtr(framebuffer.release());
}

BufferPtr D3d11Engine::constructBuffer_(const BufferCreateInfo& createInfo) {

    auto buffer = makeUnique<D3d11Buffer>(resourceRegistry_, createInfo);
    D3D11_BUFFER_DESC& desc = buffer->desc_;

    desc.Usage = usageToD3DUsage(createInfo.usage());

    const BindFlags bindFlags = createInfo.bindFlags();
    if (bindFlags & BindFlag::ConstantBuffer) {
        desc.BindFlags |= D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER;
        if (bindFlags != BindFlag::ConstantBuffer) {
            throw core::LogicError("D3d11Buffer: BindFlag::UniformBuffer cannot be "
                                   "combined with any other bind flag.");
        }
    }
    else {
        if (bindFlags & BindFlag::VertexBuffer) {
            desc.BindFlags |= D3D11_BIND_VERTEX_BUFFER;
        }
        if (bindFlags & BindFlag::IndexBuffer) {
            desc.BindFlags |= D3D11_BIND_INDEX_BUFFER;
        }
        if (bindFlags & BindFlag::ConstantBuffer) {
            desc.BindFlags |= D3D11_BIND_CONSTANT_BUFFER;
        }
        if (bindFlags & BindFlag::ShaderResource) {
            desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
        }
        if (bindFlags & BindFlag::RenderTarget) {
            desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
        }
        if (bindFlags & BindFlag::DepthStencil) {
            desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
        }
        if (bindFlags & BindFlag::UnorderedAccess) {
            desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
        }
        if (bindFlags & BindFlag::StreamOutput) {
            desc.BindFlags |= D3D11_BIND_STREAM_OUTPUT;
        }
    }

    const ResourceMiscFlags resourceMiscFlags = createInfo.resourceMiscFlags();
    desc.MiscFlags = resourceMiscFlagsToD3DResourceMiscFlags(resourceMiscFlags);

    const CpuAccessFlags cpuAccessFlags = createInfo.cpuAccessFlags();
    if (cpuAccessFlags & CpuAccessFlag::Write) {
        desc.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;
    }
    if (cpuAccessFlags & CpuAccessFlag::Read) {
        desc.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;
    }

    return BufferPtr(buffer.release());
}

ImagePtr D3d11Engine::constructImage_(const ImageCreateInfo& createInfo) {

    DXGI_FORMAT dxgiFormat = pixelFormatToDxgiFormat(createInfo.pixelFormat());
    if (dxgiFormat == DXGI_FORMAT_UNKNOWN) {
        throw core::LogicError("D3d11: unknown image pixel format.");
    }

    auto image = makeUnique<D3d11Image>(resourceRegistry_, createInfo);
    image->dxgiFormat_ = dxgiFormat;
    return ImagePtr(image.release());
}

ImageViewPtr D3d11Engine::constructImageView_(
    const ImageViewCreateInfo& createInfo,
    const ImagePtr& image) {

    // XXX should check bind flags compatibility in abstract engine

    auto imageView = makeUnique<D3d11ImageView>(resourceRegistry_, createInfo, image);
    imageView->dxgiFormat_ = static_cast<D3d11Image*>(image.get())->dxgiFormat();
    return ImageViewPtr(imageView.release());
}

ImageViewPtr D3d11Engine::constructImageView_(
    const ImageViewCreateInfo& createInfo,
    const BufferPtr& buffer,
    PixelFormat format,
    UInt32 numElements) {

    // XXX should check bind flags compatibility in abstract engine

    DXGI_FORMAT dxgiFormat = pixelFormatToDxgiFormat(format);
    if (dxgiFormat == DXGI_FORMAT_UNKNOWN) {
        throw core::LogicError("D3d11: unknown image pixel format.");
    }

    auto view = makeUnique<D3d11ImageView>(
        resourceRegistry_, createInfo, buffer, format, numElements);
    view->dxgiFormat_ = dxgiFormat;
    return ImageViewPtr(view.release());
}

SamplerStatePtr
D3d11Engine::constructSamplerState_(const SamplerStateCreateInfo& createInfo) {
    auto state = makeUnique<D3d11SamplerState>(resourceRegistry_, createInfo);
    return SamplerStatePtr(state.release());
}

GeometryViewPtr
D3d11Engine::constructGeometryView_(const GeometryViewCreateInfo& createInfo) {

    D3D_PRIMITIVE_TOPOLOGY topology =
        primitiveTypeToD3DPrimitiveTopology(createInfo.primitiveType());
    if (topology == D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED) {
        throw core::LogicError("D3d11: unknown primitive type.");
    }

    auto view = makeUnique<D3d11GeometryView>(resourceRegistry_, createInfo);
    view->topology_ = topology;
    return GeometryViewPtr(view.release());
}

BlendStatePtr D3d11Engine::constructBlendState_(const BlendStateCreateInfo& createInfo) {
    auto state = makeUnique<D3d11BlendState>(resourceRegistry_, createInfo);
    return BlendStatePtr(state.release());
}

RasterizerStatePtr
D3d11Engine::constructRasterizerState_(const RasterizerStateCreateInfo& createInfo) {
    auto state = makeUnique<D3d11RasterizerState>(resourceRegistry_, createInfo);
    return RasterizerStatePtr(state.release());
}

void D3d11Engine::onWindowResize_(SwapChain* swapChain, UInt32 width, UInt32 height) {
    D3d11SwapChain* d3dSwapChain = static_cast<D3d11SwapChain*>(swapChain);
    IDXGISwapChainX* dxgiSwapChain = d3dSwapChain->dxgiSwapChain();

    d3dSwapChain->rtv_.reset();
    HRESULT hres = dxgiSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    if (hres < 0) {
        throw core::LogicError("D3d11Engine: could not resize swap chain buffers.");
    }

    ComPtr<ID3D11Texture2D> backBuffer;
    dxgiSwapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.releaseAndGetAddressOf()));

    ComPtr<ID3D11RenderTargetView> backBufferView;
    device_->CreateRenderTargetView(
        backBuffer.get(), NULL, backBufferView.releaseAndGetAddressOf());

    d3dSwapChain->rtv_ = backBufferView.get();

    if (d3dSwapChain == currentSwapchain_.get() && !boundFramebuffer_) {
        // rebind rtv
        setFramebuffer_(nullptr);
    }
}

// -- RENDER THREAD functions --

void D3d11Engine::initContext_() {
    // no-op
}

void D3d11Engine::initBuiltinResources_() {
    // no-op
}

void D3d11Engine::initFramebuffer_(Framebuffer* /*framebuffer*/) {
    // no-op
}

void D3d11Engine::initBuffer_(Buffer* buffer, const char* data, Int lengthInBytes) {
    D3d11Buffer* d3dBuffer = static_cast<D3d11Buffer*>(buffer);
    if (lengthInBytes) {
        loadBuffer_(d3dBuffer, data, lengthInBytes);
    }
}

void D3d11Engine::initImage_(
    Image* image_,
    const Span<const char>* mipLevelDataSpans,
    Int count) {

    D3d11Image* image = static_cast<D3d11Image*>(image_);

    const UINT width = static_cast<UINT>(image->width());
    const UINT height = static_cast<UINT>(image->height());
    const UINT numSamples = static_cast<UINT>(image->numSamples());
    const UINT numLayers = static_cast<UINT>(image->numLayers());
    const UINT numMipLevels = static_cast<UINT>(image->numMipLevels());

    if (count > 0) {
        VGC_CORE_ASSERT(mipLevelDataSpans);
        // XXX let's consider for now that we are provided full mips or base level only.
        VGC_CORE_ASSERT(count == 1 || count == numMipLevels);
    }
    // Engine does assign full-set level count if it is 0 in createInfo.
    VGC_CORE_ASSERT(numMipLevels > 0);

    [[maybe_unused]] bool isImmutable = image->usage() == Usage::Immutable;
    [[maybe_unused]] bool isMultisampled = numSamples > 1;
    bool isMipmapGenEnabled =
        image->resourceMiscFlags().has(ResourceMiscFlag::GenerateMips);

    D3D11_USAGE d3dUsage = usageToD3DUsage(image->usage());

    UINT d3dBindFlags = 0;
    if (image->bindFlags() & ImageBindFlag::ShaderResource) {
        d3dBindFlags |= D3D11_BIND_SHADER_RESOURCE;
    }
    if (image->bindFlags() & ImageBindFlag::RenderTarget) {
        d3dBindFlags |= D3D11_BIND_RENDER_TARGET;
    }
    if (image->bindFlags() & ImageBindFlag::DepthStencil) {
        d3dBindFlags |= D3D11_BIND_DEPTH_STENCIL;
    }

    const CpuAccessFlags cpuAccessFlags = image->cpuAccessFlags();
    UINT d3dCPUAccessFlags = 0;
    if (cpuAccessFlags & CpuAccessFlag::Write) {
        d3dCPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;
    }
    if (cpuAccessFlags & CpuAccessFlag::Read) {
        d3dCPUAccessFlags |= D3D11_CPU_ACCESS_READ;
    }

    UINT d3dMiscFlags =
        resourceMiscFlagsToD3DResourceMiscFlags(image->resourceMiscFlags());
    if (isMipmapGenEnabled) {
        d3dMiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
    }

    // XXX add size checks
    // see https://docs.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11device-createtexture1d
    // see https://docs.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11device-createtexture2d
    core::Array<D3D11_SUBRESOURCE_DATA> initData(numMipLevels * numLayers);
    if (count > 0) {
        VGC_CORE_ASSERT(numMipLevels > 0);
        initData.resize(numMipLevels * numLayers);
        UINT levelWidth = width;
        UINT levelHeight = height;
        UINT bpp = image->bytesPerPixel();
        for (Int mipLevel = 0; mipLevel < count; ++mipLevel) {
            const Span<const char>& mipLevelDataSpan = mipLevelDataSpans[mipLevel];
            // each span has all layers
            Int layerStride = mipLevelDataSpan.length() / numLayers;
            VGC_CORE_ASSERT(layerStride * numLayers == mipLevelDataSpan.length());
            for (Int layerIdx = 0; layerIdx < numLayers; ++layerIdx) {
                // layer0_mip0..layer0_mipN..layerN_mip0..layerN_mipN
                // equivalent to D3D11CalcSubresource:
                Int subresIndex = mipLevel + layerIdx * numMipLevels;
                D3D11_SUBRESOURCE_DATA& initialData = initData[subresIndex];
                initialData.pSysMem = mipLevelDataSpan.data() + layerStride * layerIdx;
                initialData.SysMemPitch = levelWidth * bpp;
                // XXX check span size !!!
            }
            // compute next level size
            levelWidth /= 2;
            levelHeight /= 2;
            if (levelWidth > 0) {
                if (levelHeight == 0) {
                    levelHeight = 1;
                }
            }
            else if (levelHeight > 0) {
                if (levelWidth == 0) {
                    levelWidth = 1;
                }
            }
        }
    }
    else {
        VGC_CORE_ASSERT(!isImmutable);
    }

    if (image->rank() == ImageRank::_1D) {
        VGC_CORE_ASSERT(!isMultisampled);

        D3D11_TEXTURE1D_DESC desc = {};
        desc.Width = width;
        desc.MipLevels = numMipLevels;
        desc.ArraySize = numLayers;
        desc.Format = image->dxgiFormat();
        desc.Usage = d3dUsage;
        desc.BindFlags = d3dBindFlags;
        desc.CPUAccessFlags = d3dCPUAccessFlags;
        desc.MiscFlags = d3dMiscFlags;

        ComPtr<ID3D11Texture1D> texture;
        device_->CreateTexture1D(
            &desc,
            (count == numMipLevels) ? initData.data() : nullptr,
            texture.releaseAndGetAddressOf());
        image->object_ = texture;
    }
    else {
        VGC_CORE_ASSERT(image->rank() == ImageRank::_2D);
        VGC_CORE_ASSERT(!isMultisampled || count == 0);

        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = numMipLevels;
        desc.ArraySize = numLayers;
        desc.Format = image->dxgiFormat();
        desc.SampleDesc.Count = numSamples;
        desc.Usage = d3dUsage;
        desc.BindFlags = d3dBindFlags;
        desc.CPUAccessFlags = d3dCPUAccessFlags;
        desc.MiscFlags = d3dMiscFlags;

        ComPtr<ID3D11Texture2D> texture;
        device_->CreateTexture2D(
            &desc,
            (count == numMipLevels) ? initData.data() : nullptr,
            texture.releaseAndGetAddressOf());
        image->object_ = texture;
    }

    if (count < numMipLevels) {
        for (Int mipLevel = 0; mipLevel < count; ++mipLevel) {
            for (Int layerIdx = 0; layerIdx < numLayers; ++layerIdx) {
                // equivalent to D3D11CalcSubresource:
                Int subresIndex = mipLevel + layerIdx * numMipLevels;
                const D3D11_SUBRESOURCE_DATA& initialData = initData[subresIndex];
                deviceCtx_->UpdateSubresource(
                    image->object_.get(),
                    // No need for int_cast, unlikely to overflow.
                    static_cast<UINT>(subresIndex),
                    0,
                    initialData.pSysMem,
                    initialData.SysMemPitch,
                    0);
            }
        }
    }
}

void D3d11Engine::initImageView_(ImageView* view) {

    D3d11ImageView* d3dImageView = static_cast<D3d11ImageView*>(view);

    UINT firstLayer = static_cast<UINT>(d3dImageView->firstLayer());
    UINT numLayers = static_cast<UINT>(d3dImageView->numLayers());
    UINT firstMipLevel = static_cast<UINT>(d3dImageView->firstMipLevel());
    UINT numMipLevels = static_cast<UINT>(d3dImageView->numMipLevels());
    UINT numBufferElements = static_cast<UINT>(d3dImageView->numBufferElements());

    if (numMipLevels == 0) {
        numMipLevels = static_cast<UINT>(-1);
    }

    if (d3dImageView->bindFlags() & ImageBindFlag::ShaderResource) {
        D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
        desc.Format = d3dImageView->dxgiFormat();
        if (view->isBuffer()) {
            BufferPtr buffer = view->viewedBuffer();
            desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
            desc.Buffer.FirstElement = 0;
            desc.Buffer.NumElements = numBufferElements;
        }
        else {
            ImagePtr image = view->viewedImage();
            switch (image->rank()) {
            case ImageRank::_1D: {
                if (numLayers > 1) {
                    desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1DARRAY;
                    desc.Texture1DArray.FirstArraySlice = firstLayer;
                    desc.Texture1DArray.ArraySize = numLayers;
                    desc.Texture1DArray.MostDetailedMip = firstMipLevel;
                    desc.Texture1DArray.MipLevels = numMipLevels;
                }
                else {
                    desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
                    desc.Texture1D.MostDetailedMip = firstMipLevel;
                    desc.Texture1D.MipLevels = numMipLevels;
                }
                break;
            }
            case ImageRank::_2D: {
                if (image->numLayers() > 1) {
                    desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                    desc.Texture2DArray.FirstArraySlice = firstLayer;
                    desc.Texture2DArray.ArraySize = numLayers;
                    desc.Texture2DArray.MostDetailedMip = firstMipLevel;
                    desc.Texture2DArray.MipLevels = numMipLevels;
                }
                else {
                    desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                    desc.Texture2D.MostDetailedMip = firstMipLevel;
                    desc.Texture2D.MipLevels = numMipLevels;
                }
                break;
            }
            default:
                throw core::LogicError("D3d11: unknown image rank.");
                break;
            }
        }
        device_->CreateShaderResourceView(
            d3dImageView->d3dViewedResource(),
            &desc,
            d3dImageView->srv_.releaseAndGetAddressOf());
    }
    if (d3dImageView->bindFlags() & ImageBindFlag::RenderTarget) {
        D3D11_RENDER_TARGET_VIEW_DESC desc = {};
        desc.Format = d3dImageView->dxgiFormat();
        if (view->isBuffer()) {
            BufferPtr buffer = view->viewedBuffer();
            desc.ViewDimension = D3D11_RTV_DIMENSION_BUFFER;
            desc.Buffer.FirstElement = 0;
            desc.Buffer.NumElements = numBufferElements;
        }
        else {
            ImagePtr image = view->viewedImage();
            switch (image->rank()) {
            case ImageRank::_1D: {
                if (image->numLayers() > 1) {
                    desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1DARRAY;
                    desc.Texture1DArray.FirstArraySlice = firstLayer;
                    desc.Texture1DArray.ArraySize = numLayers;
                    desc.Texture1DArray.MipSlice = firstMipLevel;
                }
                else {
                    desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1D;
                    desc.Texture1D.MipSlice = firstMipLevel;
                }
                break;
            }
            case ImageRank::_2D: {
                if (image->numLayers() > 1) {
                    desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                    desc.Texture2DArray.FirstArraySlice = firstLayer;
                    desc.Texture2DArray.ArraySize = numLayers;
                    desc.Texture2DArray.MipSlice = firstMipLevel;
                }
                else {
                    desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                    desc.Texture2D.MipSlice = firstMipLevel;
                }
                break;
            }
            default:
                throw core::LogicError("D3d11: unknown image rank.");
                break;
            }
        }
        device_->CreateRenderTargetView(
            d3dImageView->d3dViewedResource(),
            &desc,
            d3dImageView->rtv_.releaseAndGetAddressOf());
    }
    if (d3dImageView->bindFlags() & ImageBindFlag::DepthStencil) {
        D3D11_DEPTH_STENCIL_VIEW_DESC desc = {};
        desc.Format = d3dImageView->dxgiFormat();
        if (view->isBuffer()) {
            throw core::LogicError("D3d11: buffer cannot be bound as Depth Stencil.");
        }
        else {
            ImagePtr image = view->viewedImage();
            switch (image->rank()) {
            case ImageRank::_1D: {
                if (image->numLayers() > 1) {
                    desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1DARRAY;
                    desc.Texture1DArray.FirstArraySlice = firstLayer;
                    desc.Texture1DArray.ArraySize = numLayers;
                    desc.Texture1DArray.MipSlice = firstMipLevel;
                }
                else {
                    desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1D;
                    desc.Texture1D.MipSlice = firstMipLevel;
                }
                break;
            }
            case ImageRank::_2D: {
                if (image->numLayers() > 1) {
                    desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                    desc.Texture2DArray.FirstArraySlice = firstLayer;
                    desc.Texture2DArray.ArraySize = numLayers;
                    desc.Texture2DArray.MipSlice = firstMipLevel;
                }
                else {
                    desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                    desc.Texture2D.MipSlice = firstMipLevel;
                }
                break;
            }
            default:
                throw core::LogicError("D3d11: unknown image rank.");
                break;
            }
        }
        device_->CreateDepthStencilView(
            d3dImageView->d3dViewedResource(),
            &desc,
            d3dImageView->dsv_.releaseAndGetAddressOf());
    }
}

void D3d11Engine::initSamplerState_(SamplerState* state) {
    D3d11SamplerState* d3dSamplerState = static_cast<D3d11SamplerState*>(state);
    D3D11_SAMPLER_DESC desc = {};
    UINT filter = 0;

    if (d3dSamplerState->magFilter() == FilterMode::Undefined) {
        throw core::LogicError("D3d11: undefined mag filter.");
    }
    if (d3dSamplerState->minFilter() == FilterMode::Undefined) {
        throw core::LogicError("D3d11: undefined min filter.");
    }
    if (d3dSamplerState->mipFilter() == FilterMode::Undefined) {
        throw core::LogicError("D3d11: undefined mip filter.");
    }

    if (d3dSamplerState->maxAnisotropy() > 1) {
        // This enum value is equivalent to a "ANISOTROPIC" flag.
        filter = D3D11_FILTER_ANISOTROPIC;
    }
    else {
        if (d3dSamplerState->magFilter() == FilterMode::Linear) {
            // This enum value is equivalent to a "MAG_LINEAR" flag.
            filter |= D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
        }
        if (d3dSamplerState->minFilter() == FilterMode::Linear) {
            // This enum value is equivalent to a "MIN_LINEAR" flag.
            filter |= D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
        }
        if (d3dSamplerState->mipFilter() == FilterMode::Linear) {
            // This enum value is equivalent to a "MIP_LINEAR" flag.
            filter |= D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
        }
    }
    if (d3dSamplerState->comparisonFunction() != ComparisonFunction::Disabled) {
        // This enum value is equivalent to the "COMPARISON" flag.
        filter |= D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
    }
    desc.Filter = static_cast<D3D11_FILTER>(filter);
    desc.AddressU = imageWrapModeToD3DTextureAddressMode(d3dSamplerState->wrapModeU());
    desc.AddressV = imageWrapModeToD3DTextureAddressMode(d3dSamplerState->wrapModeV());
    desc.AddressW = imageWrapModeToD3DTextureAddressMode(d3dSamplerState->wrapModeW());
    desc.MaxAnisotropy = static_cast<UINT>(d3dSamplerState->maxAnisotropy());
    desc.ComparisonFunc =
        comparisonFunctionToD3DComparisonFunc(d3dSamplerState->comparisonFunction());
    memcpy(desc.BorderColor, d3dSamplerState->wrapColor().data(), 4 * sizeof(float));
    desc.MipLODBias = d3dSamplerState->mipLODBias();
    desc.MinLOD = d3dSamplerState->minLOD();
    desc.MaxLOD = d3dSamplerState->maxLOD();
    device_->CreateSamplerState(&desc, d3dSamplerState->object_.releaseAndGetAddressOf());
}

void D3d11Engine::initGeometryView_(GeometryView* /*view*/) {
    //D3d11GeometryView* d3dGeometryView = static_cast<D3d11GeometryView*>(view);
    // no-op ?
}

void D3d11Engine::initBlendState_(BlendState* state) {
    D3d11BlendState* d3dBlendState = static_cast<D3d11BlendState*>(state);
    D3D11_BLEND_DESC desc = {};
    desc.AlphaToCoverageEnable = state->isAlphaToCoverageEnabled();
    desc.IndependentBlendEnable = false;
    D3D11_RENDER_TARGET_BLEND_DESC& subDesc = desc.RenderTarget[0];
    subDesc.BlendEnable = state->isEnabled();
    subDesc.SrcBlend = blendFactorToD3DBlend(state->equationRGB().sourceFactor());
    subDesc.DestBlend = blendFactorToD3DBlend(state->equationRGB().targetFactor());
    subDesc.BlendOp = blendOpToD3DBlendOp(state->equationRGB().operation());
    subDesc.SrcBlendAlpha = blendFactorToD3DBlend(state->equationAlpha().sourceFactor());
    subDesc.DestBlendAlpha = blendFactorToD3DBlend(state->equationAlpha().targetFactor());
    subDesc.BlendOpAlpha = blendOpToD3DBlendOp(state->equationAlpha().operation());
    subDesc.RenderTargetWriteMask = 0;
    if (state->writeMask() & BlendWriteMaskBit::R) {
        subDesc.RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_RED;
    }
    if (state->writeMask() & BlendWriteMaskBit::G) {
        subDesc.RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_GREEN;
    }
    if (state->writeMask() & BlendWriteMaskBit::B) {
        subDesc.RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_BLUE;
    }
    if (state->writeMask() & BlendWriteMaskBit::A) {
        subDesc.RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_ALPHA;
    }
    device_->CreateBlendState(&desc, d3dBlendState->object_.releaseAndGetAddressOf());
}

void D3d11Engine::initRasterizerState_(RasterizerState* state) {
    D3d11RasterizerState* d3dRasterizerState = static_cast<D3d11RasterizerState*>(state);
    D3D11_RASTERIZER_DESC desc = {};
    desc.FillMode = fillModeToD3DFillMode(state->fillMode());
    desc.CullMode = cullModeToD3DCullMode(state->cullMode());
    desc.FrontCounterClockwise = state->isFrontCounterClockwise();
    //desc.DepthBias;
    //desc.DepthBiasClamp;
    //desc.SlopeScaledDepthBias;
    desc.DepthClipEnable = state->isDepthClippingEnabled();
    desc.ScissorEnable = true; // scissor test always enabled in graphics::Engine
    desc.MultisampleEnable = state->isMultisamplingEnabled();
    desc.AntialiasedLineEnable = state->isLineAntialiasingEnabled();
    device_->CreateRasterizerState(
        &desc, d3dRasterizerState->object_.releaseAndGetAddressOf());
}

void D3d11Engine::setSwapChain_(const SwapChainPtr& swapChain) {

    if (swapChain != currentSwapchain_ && !boundFramebuffer_) {
        currentSwapchain_ = swapChain;
        setFramebuffer_(nullptr);
    }
    deviceCtx_->OMSetDepthStencilState(depthStencilState_.get(), 0);
    deviceCtx_->HSSetShader(NULL, NULL, 0);
    deviceCtx_->DSSetShader(NULL, NULL, 0);
    deviceCtx_->CSSetShader(NULL, NULL, 0);
}

void D3d11Engine::setFramebuffer_(const FramebufferPtr& framebuffer) {
    if (!framebuffer) {
        D3d11SwapChain* d3dSwapChain =
            currentSwapchain_.get_static_cast<D3d11SwapChain>();
        ID3D11RenderTargetView* rtvArray[1] = {
            d3dSwapChain ? d3dSwapChain->rtvObject() : NULL};
        deviceCtx_->OMSetRenderTargets(1, rtvArray, NULL);
        boundFramebuffer_.reset();
        return;
    }
    D3d11Framebuffer* d3dFramebuffer = framebuffer.get_static_cast<D3d11Framebuffer>();
    ID3D11RenderTargetView* rtvArray[1] = {d3dFramebuffer->rtvObject()};
    deviceCtx_->OMSetRenderTargets(1, rtvArray, d3dFramebuffer->dsvObject());
    boundFramebuffer_ = framebuffer;
}

void D3d11Engine::setViewport_(Int x, Int y, Int width, Int height) {
    D3D11_VIEWPORT vp = {};
    vp.TopLeftX = static_cast<float>(x);
    vp.TopLeftY = static_cast<float>(y);
    vp.Width = static_cast<float>(width);
    vp.Height = static_cast<float>(height);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    deviceCtx_->RSSetViewports(1, &vp);
}

void D3d11Engine::setProgram_(const ProgramPtr& program) {
    D3d11Program* d3dProgram = program.get_static_cast<D3d11Program>();
    if (d3dProgram) {
        deviceCtx_->VSSetShader(d3dProgram->vertexShader_.get(), NULL, 0);
        deviceCtx_->PSSetShader(d3dProgram->pixelShader_.get(), NULL, 0);
        deviceCtx_->GSSetShader(d3dProgram->geometryShader_.get(), NULL, 0);
        builtinLayouts_ = d3dProgram->builtinLayouts_;
    }
    else {
        deviceCtx_->VSSetShader(NULL, NULL, 0);
        deviceCtx_->PSSetShader(NULL, NULL, 0);
        deviceCtx_->GSSetShader(NULL, NULL, 0);
        builtinLayouts_.fill(nullptr);
    }
}

void D3d11Engine::setBlendState_(
    const BlendStatePtr& state,
    const geometry::Vec4f& constantFactors) {

    D3d11BlendState* d3dBlendState = state.get_static_cast<D3d11BlendState>();
    deviceCtx_->OMSetBlendState(
        d3dBlendState->object(), constantFactors.data(), 0xFFFFFFFF);
}

void D3d11Engine::setRasterizerState_(const RasterizerStatePtr& state) {
    D3d11RasterizerState* d3dRasterizerState =
        state.get_static_cast<D3d11RasterizerState>();
    deviceCtx_->RSSetState(d3dRasterizerState->object());
}

void D3d11Engine::setScissorRect_(const geometry::Rect2f& rect) {
    // "By convention, the right and bottom edges of the rectangle are normally considered exclusive."
    // See https://learn.microsoft.com/en-us/windows/win32/direct3d11/d3d11-rect
    //     https://learn.microsoft.com/en-us/previous-versions//dd162897(v=vs.85)
    //
    D3D11_RECT r = {};
    r.left = static_cast<LONG>(std::round(rect.xMin()));
    r.top = static_cast<LONG>(std::round(rect.yMin()));
    r.right = static_cast<LONG>(std::round(rect.xMax()));
    r.bottom = static_cast<LONG>(std::round(rect.yMax()));
    deviceCtx_->RSSetScissorRects(1, &r);
}

void D3d11Engine::setStageConstantBuffers_(
    const BufferPtr* buffers,
    Int startIndex,
    Int count,
    ShaderStage shaderStage) {
    const Int stageIdx = core::toUnderlying(shaderStage);
    StageConstantBufferArray& boundConstantBufferArray =
        boundConstantBufferArrays_[core::toUnderlying(shaderStage)];
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
    void (ID3D11DeviceContext::*setConstantBuffers)(UINT, UINT, ID3D11Buffer* const*) =
        std::array{
            &ID3D11DeviceContext::VSSetConstantBuffers,
            &ID3D11DeviceContext::GSSetConstantBuffers,
            &ID3D11DeviceContext::PSSetConstantBuffers,
        }[stageIndex];

    (deviceCtx_.get()->*setConstantBuffers)(
        static_cast<UINT>(startIndex), static_cast<UINT>(count), d3d11Buffers.data());
}

void D3d11Engine::setStageImageViews_(
    const ImageViewPtr* views,
    Int startIndex,
    Int count,
    ShaderStage shaderStage) {

    const Int stageIdx = core::toUnderlying(shaderStage);
    StageImageViewArray& boundImageViewArray =
        boundImageViewArrays_[core::toUnderlying(shaderStage)];
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
    void (ID3D11DeviceContext::*setShaderResources)(
        UINT, UINT, ID3D11ShaderResourceView* const*) = std::array{
        &ID3D11DeviceContext::VSSetShaderResources,
        &ID3D11DeviceContext::GSSetShaderResources,
        &ID3D11DeviceContext::PSSetShaderResources,
    }[stageIndex];

    (deviceCtx_.get()->*setShaderResources)(
        static_cast<UINT>(startIndex), static_cast<UINT>(count), d3d11SRVs.data());
}

void D3d11Engine::setStageSamplers_(
    const SamplerStatePtr* states,
    Int startIndex,
    Int count,
    ShaderStage shaderStage) {

    std::array<ID3D11SamplerState*, maxSamplersPerStage> d3d11SamplerStates = {};
    for (Int i = 0; i < count; ++i) {
        SamplerState* state = states[i].get();
        d3d11SamplerStates[i] =
            state ? static_cast<const D3d11SamplerState*>(state)->object() : nullptr;
    }

    size_t stageIndex = toIndex_(shaderStage);
    void (ID3D11DeviceContext::*setSamplers)(UINT, UINT, ID3D11SamplerState* const*) =
        std::array{
            &ID3D11DeviceContext::VSSetSamplers,
            &ID3D11DeviceContext::GSSetSamplers,
            &ID3D11DeviceContext::PSSetSamplers,
        }[stageIndex];

    (deviceCtx_.get()->*setSamplers)(
        static_cast<UINT>(startIndex),
        static_cast<UINT>(count),
        d3d11SamplerStates.data());
}

void D3d11Engine::updateBufferData_(
    Buffer* aBuffer,
    const void* data,
    Int lengthInBytes) {

    D3d11Buffer* buffer = static_cast<D3d11Buffer*>(aBuffer);
    loadBuffer_(buffer, data, lengthInBytes);
}

void D3d11Engine::generateMips_(const ImageViewPtr& aImageView) {
    D3d11ImageView* d3dView = aImageView.get_static_cast<D3d11ImageView>();
    ID3D11ShaderResourceView* srv = d3dView->srvObject();
    if (srv) {
        deviceCtx_->GenerateMips(srv);
    }
    else {
        VGC_ERROR(LogVgcGraphics, "Null resource view.");
    }
}

void D3d11Engine::draw_(GeometryView* view, UInt numIndices, UInt numInstances) {

    UINT nIdx = core::int_cast<UINT>(numIndices);
    UINT nInst = core::int_cast<UINT>(numInstances);

    if (nIdx == 0) {
        return;
    }

    //PrimitiveType view->primitiveType()
    D3d11GeometryView* d3dGeometryView = static_cast<D3d11GeometryView*>(view);

    ID3D11InputLayout* d3d11Layout =
        builtinLayouts_[core::toUnderlying(view->builtinGeometryLayout())].get();
    if (d3d11Layout != layout_) {
        deviceCtx_->IASetInputLayout(d3d11Layout);
        layout_ = d3d11Layout;
    }

    std::array<ID3D11Buffer*, maxAttachedVertexBuffers> d3d11VertexBuffers = {};
    for (Int i = 0; i < maxAttachedVertexBuffers; ++i) {
        Buffer* vb = view->vertexBuffer(i).get();
        d3d11VertexBuffers[i] = vb ? static_cast<D3d11Buffer*>(vb)->object() : nullptr;
    }

    // convert strides to UINTs
    const VertexBufferStridesArray& intStrides = view->strides();
    std::array<UINT, maxAttachedVertexBuffers> strides = {};
    for (Int i = 0; i < maxAttachedVertexBuffers; ++i) {
        strides[i] = static_cast<UINT>(intStrides[i]);
    }

    // convert offsets to UINTs
    const VertexBufferOffsetsArray& intOffsets = view->offsets();
    std::array<UINT, maxAttachedVertexBuffers> offsets = {};
    for (Int i = 0; i < maxAttachedVertexBuffers; ++i) {
        offsets[i] = static_cast<UINT>(intOffsets[i]);
    }

    deviceCtx_->IASetVertexBuffers(
        0,
        maxAttachedVertexBuffers,
        d3d11VertexBuffers.data(),
        strides.data(),
        offsets.data());

    D3D11_PRIMITIVE_TOPOLOGY topology = d3dGeometryView->topology();
    if (topology != topology_) {
        topology_ = topology;
        deviceCtx_->IASetPrimitiveTopology(topology);
    }

    D3d11Buffer* indexBuffer = view->indexBuffer().get_static_cast<D3d11Buffer>();
    DXGI_FORMAT indexFormat = (view->indexFormat() == IndexFormat::UInt16)
                                  ? DXGI_FORMAT_R16_UINT
                                  : DXGI_FORMAT_R32_UINT;

    if (numInstances == 0) {
        if (indexBuffer) {
            deviceCtx_->IASetIndexBuffer(indexBuffer->object(), indexFormat, 0);
            deviceCtx_->DrawIndexed(nIdx, 0, 0);
        }
        else {
            deviceCtx_->Draw(nIdx, 0);
        }
    }
    else {
        if (indexBuffer) {
            deviceCtx_->IASetIndexBuffer(indexBuffer->object(), indexFormat, 0);
            deviceCtx_->DrawIndexedInstanced(nIdx, nInst, 0, 0, 0);
        }
        else {
            deviceCtx_->DrawInstanced(nIdx, nInst, 0, 0);
        }
    }
}

void D3d11Engine::clear_(const core::Color& color) {
    ComPtr<ID3D11RenderTargetView> rtv;
    deviceCtx_->OMGetRenderTargets(1, rtv.releaseAndGetAddressOf(), NULL);
    if (rtv) {
        std::array<float, 4> c = {
            static_cast<float>(color.r()),
            static_cast<float>(color.g()),
            static_cast<float>(color.b()),
            static_cast<float>(color.a())};
        deviceCtx_->ClearRenderTargetView(rtv.get(), c.data());
    }
    else {
        VGC_WARNING(
            LogVgcGraphics, "Engine::clear() called but no target is currently set.");
    }
}

UInt64
D3d11Engine::present_(SwapChain* swapChain, UInt32 syncInterval, PresentFlags /*flags*/) {
    D3d11SwapChain* d3dSwapChain = static_cast<D3d11SwapChain*>(swapChain);
    d3dSwapChain->dxgiSwapChain()->Present(syncInterval, 0);
    return std::chrono::nanoseconds(std::chrono::steady_clock::now() - engineStartTime())
        .count();
}

// Private methods

bool D3d11Engine::loadBuffer_(D3d11Buffer* buffer, const void* data, Int dataSize) {
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
            if (device_->CreateBuffer(&desc, &srData, object.releaseAndGetAddressOf())
                < 0) {

                desc.ByteWidth = 0;
                return false;
            }
            onBufferRecreated_(buffer);
            return true;
        }
        else if (
            device_->CreateBuffer(&desc, NULL, object.releaseAndGetAddressOf()) < 0) {

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

void D3d11Engine::onBufferRecreated_(D3d11Buffer* buffer) {

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

bool D3d11Engine::writeBufferReserved_(
    ID3D11Buffer* object,
    const void* data,
    Int dataSize) {

    D3D11_MAPPED_SUBRESOURCE mappedResource = {};
    if (deviceCtx_->Map(object, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource) < 0) {
        return false;
    }
    memcpy(static_cast<char*>(mappedResource.pData), data, dataSize);
    deviceCtx_->Unmap(object, 0);
    return true;
}

} // namespace vgc::graphics

#endif // VGC_CORE_OS_WINDOWS
