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

#include <vgc/ui/detail/qopenglengine.h>

#include <chrono>
#include <limits>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#    include <QOpenGLVersionFunctionsFactory>
#endif
#include <QWindow>

#include <vgc/core/exceptions.h>
#include <vgc/core/paths.h>
#include <vgc/ui/logcategories.h>
#include <vgc/ui/qtutil.h>

namespace vgc::ui::detail::qopengl {

namespace {

// Returns the file path of a shader file as a QString
QString shaderPath_(const std::string& name) {
    std::string path = core::resourcePath("graphics/shaders/opengl/" + name);
    return toQt(path);
}

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

struct Vertex_XYRotWRGBA {
    float x, y, rot, w, r, g, b, a;
};

struct Vertex_XYUVRGBA {
    float x, y, u, v, r, g, b, a;
};

struct Vertex_RGBA {
    float r, g, b, a;
};

} // namespace

inline constexpr GLuint nullGLuint = 0;
inline constexpr GLuint badGLuint = (std::numeric_limits<GLuint>::max)();
inline constexpr GLenum badGLenum = (std::numeric_limits<GLenum>::max)();

struct GlFormat {
    GLenum internalFormat = 0;
    GLenum pixelType = 0;
    GLenum pixelFormat = 0;
};

class QglImageView;
class QglFramebuffer;

class QglBuffer : public Buffer {
protected:
    friend QglEngine;
    using Buffer::Buffer;

public:
    GLuint object() const {
        return object_;
    }

    GLenum usage() const {
        return usage_;
    }

protected:
    void release_(Engine* engine) override {
        Buffer::release_(engine);
        static_cast<QglEngine*>(engine)->api()->glDeleteBuffers(1, &object_);
    }

private:
    GLuint object_ = badGLuint;
    GLenum usage_ = badGLenum;
    Int allocatedSize_ = 0;
};
using QglBufferPtr = ResourcePtr<QglBuffer>;

class QglSamplerState : public SamplerState {
protected:
    friend QglEngine;
    using SamplerState::SamplerState;

public:
    GLuint object() const {
        return object_;
    }

    bool operator==(const QglSamplerState& other) const {
        if (this == &other) {
            return true;
        }

        if ((maxAnisotropyGL_ <= 1.f) != (other.maxAnisotropyGL_ <= 1.f)) {
            return false;
        }
        else if (maxAnisotropyGL_ != other.maxAnisotropyGL_) {
            return false;
        }
        else {
            if (magFilterGL_ != other.magFilterGL_) {
                return false;
            }
            if (minFilterGL_ != other.minFilterGL_) {
                return false;
            }
        }
        if (wrapS_ != other.wrapS_) {
            return false;
        }
        if (wrapT_ != other.wrapT_) {
            return false;
        }
        if (wrapR_ != other.wrapR_) {
            return false;
        }
        if (comparisonFunctionGL_ != other.comparisonFunctionGL_) {
            return false;
        }
        return true;
    }

    bool operator!=(const QglSamplerState& other) const {
        return !operator==(other);
    }

private:
    GLuint object_ = badGLuint;
    // cached values
    float maxAnisotropyGL_ = 1.0f;
    GLenum magFilterGL_ = badGLenum;
    GLenum minFilterGL_ = badGLenum;
    GLenum wrapS_ = badGLenum;
    GLenum wrapT_ = badGLenum;
    GLenum wrapR_ = badGLenum;
    GLenum comparisonFunctionGL_ = badGLenum;
};
using QglSamplerStatePtr = ResourcePtr<QglSamplerState>;

class QglImage : public Image {
protected:
    friend QglEngine;
    using Image::Image;

public:
    GLuint object() const {
        return object_;
    }

    GlFormat glFormat() const {
        return glFormat_;
    }

    GLenum target() const {
        return target_;
    }

protected:
    void releaseSubResources_() override {
        Image::releaseSubResources_();
        samplerState_.reset();
    }

    void release_(Engine* engine) override {
        Image::release_(engine);
        static_cast<QglEngine*>(engine)->api()->glDeleteTextures(1, &object_);
    }

private:
    GLuint object_ = badGLuint;
    GlFormat glFormat_ = {};
    GLenum target_ = badGLenum;

    friend QglImageView;
    QglSamplerStatePtr samplerState_;
};
using QglImagePtr = ResourcePtr<QglImage>;

class QglImageView : public ImageView {
protected:
    friend QglEngine;

    QglImageView(
        ResourceRegistry* registry,
        const ImageViewCreateInfo& createInfo,
        const ImagePtr& image)

        : ImageView(registry, createInfo, image) {

        samplerStatePtrAddress_ = &image.get_static_cast<QglImage>()->samplerState_;
    }

    QglImageView(
        ResourceRegistry* registry,
        const ImageViewCreateInfo& createInfo,
        const BufferPtr& buffer,
        PixelFormat format,
        UInt32 numBufferElements)

        : ImageView(registry, createInfo, buffer, format, numBufferElements) {

        samplerStatePtrAddress_ = &viewSamplerState_;
    }

public:
    GLuint object() const {
        QglImage* image = viewedImage().get_static_cast<QglImage>();
        return image ? image->object() : bufferTextureObject_;
    }

    GlFormat glFormat() const {
        return glFormat_;
    }

    GLenum target() const {
        return target_;
    }

protected:
    void releaseSubResources_() override {
        ImageView::releaseSubResources_();
        viewSamplerState_.reset();
        samplerStatePtrAddress_ = nullptr;
    }

    void release_(Engine* engine) override {
        ImageView::release_(engine);
        if (bufferTextureObject_ != badGLuint) {
            static_cast<QglEngine*>(engine)->api()->glDeleteTextures(
                1, &bufferTextureObject_);
        }
    }

    QglSamplerState* samplerState() const {
        return samplerStatePtrAddress_->get();
    }

    void setSamplerState(QglSamplerState* samplerState) const {
        return samplerStatePtrAddress_->reset(samplerState);
    }

private:
    GLuint bufferTextureObject_ = badGLuint;
    GlFormat glFormat_ = {};
    GLenum target_ = badGLenum;
    QglSamplerStatePtr viewSamplerState_;
    QglSamplerStatePtr* samplerStatePtrAddress_ = nullptr;
};
using QglImageViewPtr = ResourcePtr<QglImageView>;

class QglGeometryView : public GeometryView {
protected:
    friend QglEngine;

    QglGeometryView(ResourceRegistry* registry, const GeometryViewCreateInfo& info)
        : GeometryView(registry, info) {

        for (auto& vaos : builtinProgramVaos_) {
            vaos.fill(nullGLuint);
        }
    }

public:
    GLenum drawMode() const {
        return drawMode_;
    }

protected:
    void release_(Engine* engine) override {
        GeometryView::release_(engine);
        for (const auto& vaos : builtinProgramVaos_) {
            static_cast<QglEngine*>(engine)->api()->glDeleteVertexArrays(
                core::int_cast<GLsizei>(vaos.size()), vaos.data());
        }
    }

private:
    using BuiltinProgramVao_ = std::array<GLuint, numBuiltinGeometryLayouts>;

    GLenum drawMode_ = badGLenum;
    std::array<BuiltinProgramVao_, numBuiltinPrograms> builtinProgramVaos_;
};
using QglGeometryViewPtr = ResourcePtr<QglGeometryView>;

struct GlAttribPointerDesc {
    const char* name;
    GLuint index;
    GLint numElements;
    GLenum elementType;
    GLboolean normalized;
    GLsizei stride;
    uintptr_t offset;
    uintptr_t bufferIndex;
    bool isPerInstance;
};

class QglProgram : public Program {
protected:
    friend QglEngine;
    using Program::Program;

protected:
    void release_(Engine* engine) override {
        Program::release_(engine);
        prog_->release();
        prog_.reset();
    }

private:
    std::unique_ptr<QOpenGLShaderProgram> prog_;
    std::array<
        core::Array<GlAttribPointerDesc>,
        core::toUnderlying(BuiltinGeometryLayout::Max_) + 1>
        builtinLayouts_;
};
using QglProgramPtr = ResourcePtr<QglProgram>;

struct GlBlendEquation {
    GLenum operation = badGLenum;
    GLenum sourceFactor = badGLenum;
    GLenum targetFactor = badGLenum;

    constexpr bool operator==(const GlBlendEquation& other) const noexcept {
        return (
            operation == other.operation && sourceFactor == other.sourceFactor
            && targetFactor == other.targetFactor);
    }

    constexpr bool operator!=(const GlBlendEquation& other) const noexcept {
        return !operator==(other);
    }
};

class QglBlendState : public BlendState {
protected:
    friend QglEngine;
    using BlendState::BlendState;

protected:
    void release_(Engine* engine) override {
        BlendState::release_(engine);
    }

private:
    GlBlendEquation equationRGB_ = {};
    GlBlendEquation equationAlpha_ = {};
};
using QglBlendStatePtr = ResourcePtr<QglBlendState>;

class QglRasterizerState : public RasterizerState {
protected:
    friend QglEngine;
    using RasterizerState::RasterizerState;

protected:
    void release_(Engine* engine) override {
        RasterizerState::release_(engine);
    }

private:
    GLenum fillModeGL_ = badGLenum;
    GLenum cullModeGL_ = badGLenum;
};
using QglRasterizerStatePtr = ResourcePtr<QglRasterizerState>;

// no equivalent in D3D11, see OMSetRenderTargets
class QglFramebuffer : public Framebuffer {
protected:
    friend QglEngine;

    using Framebuffer::Framebuffer;

public:
    bool isDefault() const {
        return isDefault_;
    }

    GLuint object() const {
        return object_;
    }

protected:
    void releaseSubResources_() override {
        colorView_.reset();
        depthStencilView_.reset();
    }

    void release_(Engine* engine) override {
        Framebuffer::release_(engine);
        static_cast<QglEngine*>(engine)->api()->glDeleteFramebuffers(1, &object_);
    }

private:
    GLuint object_ = badGLuint;
    QglImageViewPtr colorView_;
    QglImageViewPtr depthStencilView_;

    bool isDefault_ = false;
};
using QglFramebufferPtr = ResourcePtr<QglFramebuffer>;

class QglSwapChain : public SwapChain {
protected:
    friend QglEngine;

    using SwapChain::SwapChain;

protected:
    void release_(Engine* engine) override {
        SwapChain::release_(engine);
    }

private:
    bool isExternal_ = false;
    QSurface* surface_ = nullptr;
    QWindow* window_ = nullptr;
    GLsizei height_ = 0;
};

// ENUM CONVERSIONS

GlFormat pixelFormatToGlFormat(PixelFormat format) {

    constexpr size_t numPixelFormats = VGC_ENUM_COUNT(PixelFormat);
    static constexpr std::array<GlFormat, numPixelFormats> map = {
#define VGC_PIXEL_FORMAT_MACRO_(                                                         \
    Enumerator,                                                                          \
    ElemSizeInBytes,                                                                     \
    DXGIFormat,                                                                          \
    OpenGLInternalFormat,                                                                \
    OpenGLPixelType,                                                                     \
    OpenGLPixelFormat)                                                                   \
    GlFormat{OpenGLInternalFormat, OpenGLPixelType, OpenGLPixelFormat},
#include <vgc/graphics/detail/pixelformats.h>
    };

    const UInt index = core::toUnderlying(format);
    if (index == 0 || index >= numPixelFormats) {
        throw core::LogicError("QglEngine: invalid PixelFormat enum value.");
    }

    return map[index];
}

GLenum primitiveTypeToGLenum(PrimitiveType type) {

    constexpr size_t numPrimitiveTypes = VGC_ENUM_COUNT(PrimitiveType);
    static_assert(numPrimitiveTypes == 6);
    static constexpr std::array<GLenum, numPrimitiveTypes> map = {
        badGLenum,         // Undefined,
        GL_POINTS,         // Point,
        GL_LINES,          // LineList,
        GL_LINE_STRIP,     // LineStrip,
        GL_TRIANGLES,      // TriangleList,
        GL_TRIANGLE_STRIP, // TriangleStrip,
    };

    const UInt index = core::toUnderlying(type);
    if (index == 0 || index >= numPrimitiveTypes) {
        throw core::LogicError("QglEngine: invalid PrimitiveType enum value.");
    }

    return map[index];
}

GLenum usageToGLenum(Usage usage, CpuAccessFlags cpuAccessFlags) {
    switch (usage) {
    case Usage::Default:
        return GL_DYNAMIC_DRAW;
    case Usage::Immutable:
        return GL_STATIC_DRAW;
    case Usage::Dynamic:
        return GL_STREAM_DRAW;
    case Usage::Staging: {
        if (cpuAccessFlags & CpuAccessFlag::Read) {
            if (cpuAccessFlags & CpuAccessFlag::Write) {
                throw core::LogicError(
                    "Qgl: staging buffer cannot habe both read and write cpu access.");
            }
            return GL_STATIC_READ;
        }
        else if (cpuAccessFlags & CpuAccessFlag::Write) {
            return GL_STATIC_COPY;
        }
        throw core::LogicError(
            "Qgl: staging buffer needs either read and write cpu access.");
    }
    default:
        break;
    }
    throw core::LogicError("QglEngine: unsupported usage.");
}

void processResourceMiscFlags(ResourceMiscFlags resourceMiscFlags) {
    if (resourceMiscFlags & ResourceMiscFlag::Shared) {
        throw core::LogicError(
            "QglEngine: ResourceMiscFlag::Shared is not supported at the moment.");
    }
    //if (resourceMiscFlags & ResourceMiscFlag::TextureCube) {
    //    throw core::LogicError("QglEngine: ResourceMiscFlag::TextureCube is not supported at the moment.");
    //}
    //if (resourceMiscFlags & ResourceMiscFlag::ResourceClamp) {
    //    throw core::LogicError("QglEngine: ResourceMiscFlag::ResourceClamp is not supported at the moment.");
    //}
    return;
}

GLenum imageWrapModeToGLenum(ImageWrapMode mode) {

    constexpr size_t numImageWrapModes = VGC_ENUM_COUNT(ImageWrapMode);
    static_assert(numImageWrapModes == 5);
    static constexpr std::array<GLenum, numImageWrapModes> map = {
        badGLenum,          // Undefined
        GL_REPEAT,          // Repeat
        GL_MIRRORED_REPEAT, // MirrorRepeat
        GL_CLAMP_TO_EDGE,   // Clamp
        GL_CLAMP_TO_BORDER, // ClampConstantColor
    };

    const UInt index = core::toUnderlying(mode);
    if (index == 0 || index >= numImageWrapModes) {
        throw core::LogicError("QglEngine: invalid ImageWrapMode enum value.");
    }

    return map[index];
}

GLenum comparisonFunctionToGLenum(ComparisonFunction func) {

    constexpr size_t numComparisonFunctions = VGC_ENUM_COUNT(ComparisonFunction);
    static_assert(numComparisonFunctions == 10);
    static constexpr std::array<GLenum, numComparisonFunctions> map = {
        badGLenum,   // Undefined
        GL_ALWAYS,   // Disabled
        GL_ALWAYS,   // Always
        GL_NEVER,    // Never
        GL_EQUAL,    // Equal
        GL_NOTEQUAL, // NotEqual
        GL_LESS,     // Less
        GL_LEQUAL,   // LessEqual
        GL_GREATER,  // Greater
        GL_GEQUAL,   // GreaterEqual
    };

    const UInt index = core::toUnderlying(func);
    if (index == 0 || index >= numComparisonFunctions) {
        throw core::LogicError("QglEngine: invalid ComparisonFunction enum value.");
    }

    return map[index];
}

GLenum blendFactorToGLenum(BlendFactor factor) {

    constexpr size_t numBlendFactors = VGC_ENUM_COUNT(BlendFactor);
    static_assert(numBlendFactors == 18);
    static constexpr std::array<GLenum, numBlendFactors> map = {
        badGLenum,                   // Undefined
        GL_ONE,                      // One
        GL_ZERO,                     // Zero
        GL_SRC_COLOR,                // SourceColor
        GL_ONE_MINUS_SRC_COLOR,      // OneMinusSourceColor
        GL_SRC_ALPHA,                // SourceAlpha
        GL_ONE_MINUS_SRC_ALPHA,      // OneMinusSourceAlpha
        GL_DST_COLOR,                // TargetColor
        GL_ONE_MINUS_DST_COLOR,      // OneMinusTargetColor
        GL_DST_ALPHA,                // TargetAlpha
        GL_ONE_MINUS_DST_ALPHA,      // OneMinusTargetAlpha
        GL_SRC_ALPHA_SATURATE,       // SourceAlphaSaturated
        GL_CONSTANT_COLOR,           // Constant
        GL_ONE_MINUS_CONSTANT_COLOR, // OneMinusConstant
        GL_SRC1_COLOR,               // SecondSourceColor
        GL_ONE_MINUS_SRC1_COLOR,     // OneMinusSecondSourceColor
        GL_SRC1_ALPHA,               // SecondSourceAlpha
        GL_ONE_MINUS_SRC1_ALPHA,     // OneMinusSecondSourceAlpha
    };

    const UInt index = core::toUnderlying(factor);
    if (index == 0 || index >= numBlendFactors) {
        throw core::LogicError("QglEngine: invalid BlendFactor enum value.");
    }

    return map[index];
}

GLenum blendOpToGLenum(BlendOp op) {

    constexpr size_t numBlendOps = VGC_ENUM_COUNT(BlendOp);
    static_assert(numBlendOps == 6);
    static constexpr std::array<GLenum, numBlendOps> map = {
        badGLenum,                // Undefined
        GL_FUNC_ADD,              // Add
        GL_FUNC_SUBTRACT,         // SourceMinusTarget
        GL_FUNC_REVERSE_SUBTRACT, // TargetMinusSource
        GL_MIN,                   // Min
        GL_MAX,                   // Max
    };

    const UInt index = core::toUnderlying(op);
    if (index == 0 || index >= numBlendOps) {
        throw core::LogicError("QglEngine: invalid BlendOp enum value.");
    }

    return map[index];
}

GLenum fillModeToGLenum(FillMode mode) {

    constexpr size_t numFillModes = VGC_ENUM_COUNT(FillMode);
    static_assert(numFillModes == 3);
    static constexpr std::array<GLenum, numFillModes> map = {
        badGLenum, // Undefined
        GL_FILL,   // Solid
        GL_LINE,   // Wireframe
    };

    const UInt index = core::toUnderlying(mode);
    if (index == 0 || index >= numFillModes) {
        throw core::LogicError("QglEngine: invalid FillMode enum value.");
    }

    return map[index];
}

GLenum cullModeToGLenum(CullMode mode) {

    constexpr size_t numCullModes = VGC_ENUM_COUNT(CullMode);
    static_assert(numCullModes == 4);
    static constexpr std::array<GLenum, numCullModes> map = {
        badGLenum, // Undefined
        0,         // None -> must disable culling
        GL_FRONT,  // Front
        GL_BACK,   // Back
    };

    const UInt index = core::toUnderlying(mode);
    if (index == 0 || index >= numCullModes) {
        throw core::LogicError("QglEngine: invalid CullMode enum value.");
    }

    return map[index];
}

GLenum magFilterModeToGLenum(FilterMode mode) {

    constexpr size_t numFilterModes = VGC_ENUM_COUNT(FilterMode);
    static_assert(numFilterModes == 3);
    static constexpr std::array<GLenum, numFilterModes> map = {
        badGLenum,  // Undefined
        GL_NEAREST, // Point
        GL_LINEAR,  // Linear
    };

    const UInt index = core::toUnderlying(mode);
    if (index == 0 || index >= numFilterModes) {
        throw core::LogicError(
            "QglEngine: invalid FilterMode enum value for the Mag filter.");
    }

    return map[index];
}

GLenum minMipFilterModesToGLenum(FilterMode minMode, FilterMode mipMode) {

    constexpr size_t numFilterModes = VGC_ENUM_COUNT(FilterMode);
    static_assert(numFilterModes == 3);

    const UInt minModeIndex = core::toUnderlying(minMode);
    if (minModeIndex == 0 || minModeIndex >= numFilterModes) {
        throw core::LogicError(
            "QglEngine: invalid FilterMode enum value for the Min filter.");
    }

    const UInt mipModeIndex = core::toUnderlying(mipMode);
    if (mipModeIndex == 0 || mipModeIndex >= numFilterModes) {
        throw core::LogicError(
            "QglEngine: invalid FilterMode enum value for the Mip filter.");
    }

    static constexpr std::array<GLenum, 4> map = {
        GL_NEAREST_MIPMAP_NEAREST, // min point,  mip point
        GL_NEAREST_MIPMAP_LINEAR,  // min point,  mip linear
        GL_LINEAR_MIPMAP_NEAREST,  // min linear, mip point
        GL_LINEAR_MIPMAP_LINEAR,   // min linear, mip linear
    };
    const UInt combinedIndex = 2 * (minModeIndex - 1) + (mipModeIndex - 1);

    return map[combinedIndex];
}

// ENGINE FUNCTIONS

QglEngine::QglEngine(const EngineCreateInfo& createInfo, QOpenGLContext* ctx)
    : Engine(createInfo)
    , ctx_(ctx)
    , isExternalCtx_(ctx ? true : false) {

    if (isExternalCtx_) {
        format_ = ctx_->format();
    }
    else {
        format_.setProfile(QSurfaceFormat::CoreProfile);
        format_.setVersion(requiredOpenGLVersionMajor, requiredOpenGLVersionMinor);
        //format_.setOption(QSurfaceFormat::DebugContext);

        // XXX only allow D24_S8 for now..
        format_.setDepthBufferSize(24);
        format_.setStencilBufferSize(8);
        format_.setSamples(createInfo.windowSwapChainFormat().numSamples());
        format_.setSwapInterval(0);
        PixelFormat pixelFormat = createInfo.windowSwapChainFormat().pixelFormat();
        if (pixelFormat == PixelFormat::RGBA_8_UNORM_SRGB) {
            format_.setColorSpace(QSurfaceFormat::sRGBColorSpace);
        }
        else {
            format_.setColorSpace(QSurfaceFormat::DefaultColorSpace);
        }

        // XXX use buffer count
        format_.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    }
    QSurfaceFormat::setDefaultFormat(format_);

    currentImageViews_.fill(nullptr);
    currentSamplerStates_.fill(nullptr);

    //createBuiltinResources_();
}

void QglEngine::onDestroyed() {
    Engine::onDestroyed();
    if (!isExternalCtx_) {
        // XXX that's wrong; solution is to add
        // Engine::destroyContext_ and call it on thread stop.
        ctx_->deleteLater();
    }
    ctx_ = nullptr;
    if (offscreenSurface_) {
        // XXX that's wrong; see comment above.
        offscreenSurface_->deleteLater();
        offscreenSurface_ = nullptr;
    }
    surface_ = nullptr;
}

/* static */
QglEnginePtr QglEngine::create(const EngineCreateInfo& createInfo) {
    QglEnginePtr engine(new QglEngine(createInfo, nullptr));
    engine->init_();
    return engine;
}

/* static */
QglEnginePtr
QglEngine::create(const EngineCreateInfo& createInfo, QOpenGLContext* externalCtx) {
    // Multithreading not supported atm, Qt does thread affinity
    VGC_CORE_ASSERT(!createInfo.isMultithreadingEnabled());
    QglEnginePtr engine(new QglEngine(createInfo, externalCtx));
    engine->init_();
    return engine;
}

SwapChainPtr QglEngine::createSwapChainFromSurface(QSurface* surface) {

    SwapChainCreateInfo createInfo = {};
    // XXX fill from surface format

    auto swapChain = makeUnique<QglSwapChain>(resourceRegistry_, createInfo);
    swapChain->window_ = nullptr;
    swapChain->surface_ = surface;
    swapChain->isExternal_ = true;

    return SwapChainPtr(swapChain.release());
}

// -- USER THREAD implementation functions --

void QglEngine::createBuiltinShaders_() {
    QglProgramPtr simpleProgram(
        new QglProgram(resourceRegistry_, BuiltinProgram::Simple));
    simpleProgram_ = simpleProgram;

    QglProgramPtr simpleTexturedProgram(
        new QglProgram(resourceRegistry_, BuiltinProgram::SimpleTextured));
    simpleTexturedProgram_ = simpleTexturedProgram;

    QglProgramPtr simpleTexturedDebugProgram(
        new QglProgram(resourceRegistry_, BuiltinProgram::SimpleTexturedDebug));
    simpleTexturedDebugProgram_ = simpleTexturedDebugProgram;

    QglProgramPtr sreenSpaceDisplacementProgram(
        new QglProgram(resourceRegistry_, BuiltinProgram::SreenSpaceDisplacement));
    sreenSpaceDisplacementProgram_ = sreenSpaceDisplacementProgram;
}

SwapChainPtr QglEngine::constructSwapChain_(const SwapChainCreateInfo& createInfo) {

    if (createInfo.windowNativeHandleType() != WindowNativeHandleType::QOpenGLWindow) {
        throw core::LogicError("QglEngine: unsupported WindowNativeHandleType value.");
    }

    //if (ctx_ == nullptr) {
    //    throw core::LogicError("ctx_ is null.");
    //}
    // XXX can it be an external context ??

    QWindow* wnd = static_cast<QWindow*>(createInfo.windowNativeHandle());
    wnd->setSurfaceType(QSurface::SurfaceType::OpenGLSurface);
    wnd->setFormat(format_);
    wnd->create();

    auto swapChain = makeUnique<QglSwapChain>(resourceRegistry_, createInfo);
    swapChain->window_ = wnd;
    swapChain->surface_ = wnd;
    swapChain->isExternal_ = false;
    swapChain->height_ = core::int_cast<GLsizei>(createInfo.height());

    return SwapChainPtr(swapChain.release());
}

FramebufferPtr QglEngine::constructFramebuffer_(const ImageViewPtr& colorImageView) {
    auto framebuffer = makeUnique<QglFramebuffer>(resourceRegistry_);
    framebuffer->colorView_ = static_pointer_cast<QglImageView>(colorImageView);
    return FramebufferPtr(framebuffer.release());
}

BufferPtr QglEngine::constructBuffer_(const BufferCreateInfo& createInfo) {
    auto buffer = makeUnique<QglBuffer>(resourceRegistry_, createInfo);
    buffer->usage_ = usageToGLenum(createInfo.usage(), createInfo.cpuAccessFlags());
    return BufferPtr(buffer.release());
}

ImagePtr QglEngine::constructImage_(const ImageCreateInfo& createInfo) {
    auto image = makeUnique<QglImage>(resourceRegistry_, createInfo);
    image->glFormat_ = pixelFormatToGlFormat(createInfo.pixelFormat());
    return ImagePtr(image.release());
}

ImageViewPtr QglEngine::constructImageView_(
    const ImageViewCreateInfo& createInfo,
    const ImagePtr& image) {

    auto view = makeUnique<QglImageView>(resourceRegistry_, createInfo, image);
    view->glFormat_ = image.get_static_cast<QglImage>()->glFormat();
    return ImageViewPtr(view.release());
}

ImageViewPtr QglEngine::constructImageView_(
    const ImageViewCreateInfo& createInfo,
    const BufferPtr& buffer,
    PixelFormat format,
    UInt32 numElements) {

    auto view = makeUnique<QglImageView>(
        resourceRegistry_, createInfo, buffer, format, numElements);
    view->glFormat_ = pixelFormatToGlFormat(format);
    return ImageViewPtr(view.release());
}

SamplerStatePtr
QglEngine::constructSamplerState_(const SamplerStateCreateInfo& createInfo) {
    auto state = makeUnique<QglSamplerState>(resourceRegistry_, createInfo);

    bool aniso = false;
    if (createInfo.maxAnisotropy() > 1) {
        if (hasAnisotropicFilteringSupport_) {
            // Value has already been sanitized by Engine.
            state->maxAnisotropyGL_ = static_cast<float>(createInfo.maxAnisotropy());
            if (state->maxAnisotropyGL_ > maxTextureMaxAnisotropy) {
                state->maxAnisotropyGL_ = maxTextureMaxAnisotropy;
            }
            aniso = true;
        }
        else {
            VGC_WARNING(LogVgcUi, "Anisotropic filtering is not supported.");
        }
    }
    if (aniso) {
        state->magFilterGL_ = GL_LINEAR;
        state->minFilterGL_ = GL_LINEAR_MIPMAP_LINEAR;
    }
    else {
        state->magFilterGL_ = magFilterModeToGLenum(createInfo.magFilter());
        state->minFilterGL_ =
            minMipFilterModesToGLenum(createInfo.minFilter(), createInfo.mipFilter());
    }

    state->wrapS_ = imageWrapModeToGLenum(createInfo.wrapModeU());
    state->wrapT_ = imageWrapModeToGLenum(createInfo.wrapModeV());
    state->wrapR_ = imageWrapModeToGLenum(createInfo.wrapModeW());
    state->comparisonFunctionGL_ =
        comparisonFunctionToGLenum(createInfo.comparisonFunction());
    return SamplerStatePtr(state.release());
}

GeometryViewPtr
QglEngine::constructGeometryView_(const GeometryViewCreateInfo& createInfo) {
    auto view = makeUnique<QglGeometryView>(resourceRegistry_, createInfo);
    view->drawMode_ = primitiveTypeToGLenum(createInfo.primitiveType());
    return GeometryViewPtr(view.release());
}

BlendStatePtr QglEngine::constructBlendState_(const BlendStateCreateInfo& createInfo) {
    auto state = makeUnique<QglBlendState>(resourceRegistry_, createInfo);
    if (state->isEnabled()) {
        const BlendEquation& equationRGB = state->equationRGB();
        GlBlendEquation& glEquationRGB = state->equationRGB_;
        glEquationRGB.operation = blendOpToGLenum(equationRGB.operation());
        glEquationRGB.sourceFactor = blendFactorToGLenum(equationRGB.sourceFactor());
        glEquationRGB.targetFactor = blendFactorToGLenum(equationRGB.targetFactor());
        const BlendEquation& equationAlpha = state->equationAlpha();
        GlBlendEquation& glEquationAlpha = state->equationAlpha_;
        glEquationAlpha.operation = blendOpToGLenum(equationAlpha.operation());
        glEquationAlpha.sourceFactor = blendFactorToGLenum(equationAlpha.sourceFactor());
        glEquationAlpha.targetFactor = blendFactorToGLenum(equationAlpha.targetFactor());
    }
    return BlendStatePtr(state.release());
}

RasterizerStatePtr
QglEngine::constructRasterizerState_(const RasterizerStateCreateInfo& createInfo) {
    auto state = makeUnique<QglRasterizerState>(resourceRegistry_, createInfo);
    state->fillModeGL_ = fillModeToGLenum(createInfo.fillMode());
    state->cullModeGL_ = cullModeToGLenum(createInfo.cullMode());
    return RasterizerStatePtr(state.release());
}

void QglEngine::onWindowResize_(SwapChain* aSwapChain, UInt32 /*width*/, UInt32 height) {

    // Store the height in order to be able to convert from OpenGL coordinates
    // (origin is bottom-left) to Engine coordinate (origin is top-left).
    //
    QglSwapChain* swapChain = static_cast<QglSwapChain*>(aSwapChain);
    swapChain->height_ = static_cast<GLsizei>(height);
}

//--  RENDER THREAD implementation functions --

void QglEngine::initContext_() {

    //format.setSamples(8); // mandatory, Qt ignores the QWindow format...

    VGC_CORE_ASSERT(format_.version() >= requiredOpenGLVersionQPair);

    if (!isExternalCtx_) {
        ctx_ = new QOpenGLContext();
        ctx_->setFormat(format_);
        [[maybe_unused]] bool ok = ctx_->create();
        VGC_CORE_ASSERT(ok);
    }
    VGC_CORE_ASSERT(ctx_->isValid());
    VGC_CORE_ASSERT(ctx_->format().version() >= requiredOpenGLVersionQPair);

    // must be create here since the QWindow constructor is not thread-safe
    // (can't construct windows is parallel threads)
    offscreenSurface_ = new QOffscreenSurface();
    offscreenSurface_->setFormat(format_);
    offscreenSurface_->create();
    VGC_CORE_ASSERT(offscreenSurface_->isValid());
    VGC_CORE_ASSERT(offscreenSurface_->format().version() >= requiredOpenGLVersionQPair);

    QSurface* surface = ctx_->surface();
    surface = surface ? surface : offscreenSurface_;

    ctx_->makeCurrent(offscreenSurface_);

    VGC_CORE_ASSERT(ctx_->isValid());

    // move to constructor with dummy context...
    hasAnisotropicFilteringSupport_ =
        ctx_->hasExtension("EXT_texture_filter_anisotropic");

    // Get API
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    api_ = ctx_->versionFunctions<OpenGLFunctions>();
#else
    api_ = QOpenGLVersionFunctionsFactory::get<OpenGLFunctions>(ctx_);
#endif
    //
    VGC_CORE_ASSERT(api_ != nullptr);
    [[maybe_unused]] bool ok = api_->initializeOpenGLFunctions();
    VGC_CORE_ASSERT(ok);

    if (hasAnisotropicFilteringSupport_) {
        api_->glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxTextureMaxAnisotropy);
    }
}

void QglEngine::initBuiltinResources_() {

    // Builtin layouts
    struct VertexAttribDesc {
        const char* name;
        GLint numElements;
        GLenum elementType;
        uintptr_t bufferIndex;
        uintptr_t offset;
        bool isPerInstance;
        GLboolean normalized;
        GLsizei stride;
    };

#define VGC_QGL_CREATE_BUILTIN_INPUT_LAYOUT(layout_tag)                                  \
    do {                                                                                 \
        constexpr Int8 layoutIndex =                                                     \
            core::toUnderlying(BuiltinGeometryLayout::##layout_tag);                     \
        core::Array<GlAttribPointerDesc>& layout =                                       \
            program->builtinLayouts_[layoutIndex];                                       \
        for (const auto& desc : layout_##layout_tag) {                                   \
            GlAttribPointerDesc& desc2 = layout.emplaceLast();                           \
            desc2.index = prog->attributeLocation(desc.name);                            \
            desc2.numElements = desc.numElements;                                        \
            desc2.elementType = desc.elementType;                                        \
            desc2.normalized = desc.normalized;                                          \
            desc2.stride = desc.stride;                                                  \
            desc2.offset = desc.offset;                                                  \
            desc2.bufferIndex = desc.bufferIndex;                                        \
            desc2.isPerInstance = desc.isPerInstance;                                    \
        }                                                                                \
    } while (0)

    // clang-format off
    GlAttribPointerDesc layout_XYRGB[] = {
        {"pos",    2, GL_FLOAT, 0, 0,                                false, 0},
        {"col",    3, GL_FLOAT, 0, offsetof(Vertex_XYRGB, r),        false, 0},
    };
    GlAttribPointerDesc layout_XYRGBA[] = {
        {"pos",    2, GL_FLOAT, 0, 0,                                false, 0},
        {"col",    4, GL_FLOAT, 0, offsetof(Vertex_XYRGBA, r),       false, 0},
    };                                                               
    GlAttribPointerDesc layout_XY_iRGBA[] = {                        
        {"pos",    2, GL_FLOAT, 0, 0,                                false, 0},
        {"col",    4, GL_FLOAT, 1, 0,                                true,  0},
    };                                                               
    GlAttribPointerDesc layout_XYUVRGBA[] = {                        
        {"pos",    2, GL_FLOAT, 0, 0,                                false, 0},
        {"uv",     2, GL_FLOAT, 0, offsetof(Vertex_XYUVRGBA, u),     false, 0},
        {"col",    4, GL_FLOAT, 0, offsetof(Vertex_XYUVRGBA, r),     false, 0},
    };                                                               
    GlAttribPointerDesc layout_XYUV_iRGBA[] = {                      
        {"pos",    2, GL_FLOAT, 0, 0,                                false, 0},
        {"uv",     2, GL_FLOAT, 0, offsetof(Vertex_XYUV, u),         false, 0},
        {"col",    4, GL_FLOAT, 1, 0,                                true,  0},
    };
    GlAttribPointerDesc layout_XYDxDy_iXYRotWRGBA[] = {
        {"pos",    2, GL_FLOAT, 0, 0,                                false, 0, },
        {"disp",   2, GL_FLOAT, 0, offsetof(Vertex_XYDxDy, dx),      false, 0, },
        {"ipos",   2, GL_FLOAT, 1, 0,                                true,  1, sizeof(Vertex_XYRotWRGBA)},
        {"rotate", 1, GL_FLOAT, 1, offsetof(Vertex_XYRotWRGBA, rot), true,  1, sizeof(Vertex_XYRotWRGBA)},
        {"offset", 1, GL_FLOAT, 1, offsetof(Vertex_XYRotWRGBA, w),   true,  1, sizeof(Vertex_XYRotWRGBA)},
        {"col",    4, GL_FLOAT, 1, offsetof(Vertex_XYRotWRGBA, r),   true,  1, sizeof(Vertex_XYRotWRGBA)},
    };
    // clang-format on

    // Initialize the simple shader
    {
        QglProgram* program = simpleProgram_.get_static_cast<QglProgram>();
        program->prog_.reset(new QOpenGLShaderProgram());
        QOpenGLShaderProgram* prog = program->prog_.get();
        prog->addShaderFromSourceFile(
            QOpenGLShader::Vertex, shaderPath_("simple.v.glsl"));
        prog->addShaderFromSourceFile(
            QOpenGLShader::Fragment, shaderPath_("simple.f.glsl"));
        prog->link();
        prog->bind();
        api_->glUniformBlockBinding(prog->programId(), 0, 0);
        prog->release();

        VGC_QGL_CREATE_BUILTIN_INPUT_LAYOUT(XYRGB);
        VGC_QGL_CREATE_BUILTIN_INPUT_LAYOUT(XYRGBA);
        VGC_QGL_CREATE_BUILTIN_INPUT_LAYOUT(XY_iRGBA);
        VGC_QGL_CREATE_BUILTIN_INPUT_LAYOUT(XYUVRGBA);
        VGC_QGL_CREATE_BUILTIN_INPUT_LAYOUT(XYUV_iRGBA);
    }

    // Initialize the simple textured shader
    {
        QglProgram* program = simpleTexturedProgram_.get_static_cast<QglProgram>();
        program->prog_.reset(new QOpenGLShaderProgram());
        QOpenGLShaderProgram* prog = program->prog_.get();
        prog->addShaderFromSourceFile(
            QOpenGLShader::Vertex, shaderPath_("simple_textured.v.glsl"));
        prog->addShaderFromSourceFile(
            QOpenGLShader::Fragment, shaderPath_("simple_textured.f.glsl"));
        prog->link();
        prog->bind();
        api_->glUniformBlockBinding(prog->programId(), 0, 0);

        // XXX temporary
        int tex0Loc_ = prog->uniformLocation("tex0f");
        VGC_ASSERT(tex0Loc_ >= 0);
        constexpr Int stageIndex = toIndex_(ShaderStage::Pixel);
        constexpr Int begIndex = maxSamplersPerStage * stageIndex;
        api_->glUniform1i(tex0Loc_, begIndex);

        prog->release();

        VGC_QGL_CREATE_BUILTIN_INPUT_LAYOUT(XYUVRGBA);
        VGC_QGL_CREATE_BUILTIN_INPUT_LAYOUT(XYUV_iRGBA);
    }

    // Initialize the simple textured debug shader
    {
        QglProgram* program = simpleTexturedDebugProgram_.get_static_cast<QglProgram>();
        program->prog_.reset(new QOpenGLShaderProgram());
        QOpenGLShaderProgram* prog = program->prog_.get();
        prog->addShaderFromSourceFile(
            QOpenGLShader::Vertex, shaderPath_("simple_textured.v.glsl"));
        prog->addShaderFromSourceFile(
            QOpenGLShader::Fragment, shaderPath_("simple_textured_debug.f.glsl"));
        prog->link();
        prog->bind();
        api_->glUniformBlockBinding(prog->programId(), 0, 0);
        prog->release();

        VGC_QGL_CREATE_BUILTIN_INPUT_LAYOUT(XYUVRGBA);
        VGC_QGL_CREATE_BUILTIN_INPUT_LAYOUT(XYUV_iRGBA);
    }

    // Initialize the sreen-space displacement shader
    {
        QglProgram* program =
            sreenSpaceDisplacementProgram_.get_static_cast<QglProgram>();
        program->prog_.reset(new QOpenGLShaderProgram());
        QOpenGLShaderProgram* prog = program->prog_.get();
        prog->addShaderFromSourceFile(
            QOpenGLShader::Vertex, shaderPath_("screen_space_displacement.v.glsl"));
        prog->addShaderFromSourceFile(
            QOpenGLShader::Fragment, shaderPath_("simple.f.glsl"));
        prog->link();
        prog->bind();
        api_->glUniformBlockBinding(prog->programId(), 0, 0);
        prog->release();

        VGC_QGL_CREATE_BUILTIN_INPUT_LAYOUT(XYDxDy_iXYRotWRGBA);
    }

#undef VGC_QGL_CREATE_BUILTIN_INPUT_LAYOUT
}

void QglEngine::initFramebuffer_(Framebuffer* aFramebuffer) {
    QglFramebuffer* framebuffer = static_cast<QglFramebuffer*>(aFramebuffer);
    api_->glGenFramebuffers(1, &framebuffer->object_);
    api_->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer->object_);
    // XXX handle the different textargets + ranks + layer !!
    api_->glFramebufferTexture2D(
        GL_DRAW_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        framebuffer->colorView_->object(),
        0);
    api_->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, boundFramebuffer_);
}

void QglEngine::initBuffer_(Buffer* aBuffer, const char* data, Int lengthInBytes) {
    QglBuffer* buffer = static_cast<QglBuffer*>(aBuffer);
    GLuint object = 0;
    api_->glGenBuffers(1, &object);
    buffer->object_ = object;
    loadBuffer_(buffer, data, lengthInBytes);
}

void QglEngine::initImage_(
    Image* aImage,
    const core::Span<const char>* mipLevelDataSpans,
    Int count) {

    QglImage* image = static_cast<QglImage*>(aImage);

    if (count > 0) {
        VGC_CORE_ASSERT(mipLevelDataSpans);
    }

    GLint numLayers = image->numLayers();
    GLint numMipLevels = image->numMipLevels();
    GLint maxMipLevel = numMipLevels - 1;
    [[maybe_unused]] bool isImmutable = image->usage() == Usage::Immutable;
    [[maybe_unused]] bool isMultisampled = image->numSamples() > 1;
    [[maybe_unused]] bool isMipmapGenEnabled = image->isMipGenerationEnabled();
    [[maybe_unused]] bool isArray = numLayers > 1;

    if (count > 0) {
        VGC_CORE_ASSERT(mipLevelDataSpans);
        // XXX let's consider for now that we are provided full mips or base level only.
        VGC_CORE_ASSERT(count == 1 || count == numMipLevels);
    }
    // Engine does assign full-set level count if it is 0 in createInfo.
    VGC_CORE_ASSERT(numMipLevels > 0);

    GLuint object = 0;
    api_->glGenTextures(1, &object);
    image->object_ = object;

    GLenum target = badGLenum;

    GlFormat glFormat = image->glFormat();

    api_->glActiveTexture(GL_TEXTURE0);

    if (image->rank() == ImageRank::_1D) {
        VGC_CORE_ASSERT(!isMultisampled);

        if (isArray) {
            target = GL_TEXTURE_1D_ARRAY;
            api_->glBindTexture(target, object);
            api_->glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
            api_->glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, maxMipLevel);
            for (Int mipLevel = 0; mipLevel < numMipLevels; ++mipLevel) {
                api_->glTexImage2D(
                    GL_TEXTURE_1D_ARRAY,
                    mipLevel,
                    glFormat.internalFormat,
                    image->width(),
                    numLayers,
                    0,
                    glFormat.pixelFormat,
                    glFormat.pixelType,
                    (mipLevel < count) ? mipLevelDataSpans[mipLevel].data()
                                       : nullptr); // XXX check size
            }
        }
        else {
            target = GL_TEXTURE_1D;
            api_->glBindTexture(target, object);
            api_->glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
            api_->glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, maxMipLevel);
            for (Int mipLevel = 0; mipLevel < numMipLevels; ++mipLevel) {
                api_->glTexImage1D(
                    GL_TEXTURE_1D,
                    mipLevel,
                    glFormat.internalFormat,
                    image->width(),
                    0,
                    glFormat.pixelFormat,
                    glFormat.pixelType,
                    (mipLevel < count) ? mipLevelDataSpans[mipLevel].data()
                                       : nullptr); // XXX check size
            }
        }
    }
    else {
        VGC_CORE_ASSERT(image->rank() == ImageRank::_2D);
        VGC_CORE_ASSERT(!isMultisampled || !mipLevelDataSpans);

        if (isArray) {
            if (isMultisampled) {
                target = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
                api_->glBindTexture(target, object);
                api_->glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
                api_->glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, maxMipLevel);
                api_->glTexImage3DMultisample(
                    GL_TEXTURE_2D_MULTISAMPLE,
                    image->numSamples(),
                    glFormat.internalFormat,
                    image->width(),
                    image->height(),
                    image->numLayers(),
                    GL_TRUE);
            }
            else {
                target = GL_TEXTURE_2D_ARRAY;
                api_->glBindTexture(target, object);
                api_->glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
                api_->glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, maxMipLevel);
                for (Int mipLevel = 0; mipLevel < numMipLevels; ++mipLevel) {
                    api_->glTexImage3D(
                        GL_TEXTURE_2D_ARRAY,
                        mipLevel,
                        glFormat.internalFormat,
                        image->width(),
                        image->height(),
                        image->numLayers(),
                        0,
                        glFormat.pixelFormat,
                        glFormat.pixelType,
                        (mipLevel < count) ? mipLevelDataSpans[mipLevel].data()
                                           : nullptr); // XXX check size
                }
            }
        }
        else {
            if (isMultisampled) {
                target = GL_TEXTURE_2D_MULTISAMPLE;
                api_->glBindTexture(target, object);
                api_->glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
                api_->glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, maxMipLevel);
                api_->glTexImage2DMultisample(
                    GL_TEXTURE_2D_MULTISAMPLE,
                    image->numSamples(),
                    glFormat.internalFormat,
                    image->width(),
                    image->height(),
                    GL_TRUE);
            }
            else {
                target = GL_TEXTURE_2D;
                api_->glBindTexture(target, object);
                api_->glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
                api_->glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, maxMipLevel);
                for (Int mipLevel = 0; mipLevel < numMipLevels; ++mipLevel) {
                    api_->glTexImage2D(
                        GL_TEXTURE_2D,
                        mipLevel,
                        glFormat.internalFormat,
                        image->width(),
                        image->height(),
                        0,
                        glFormat.pixelFormat,
                        glFormat.pixelType,
                        (mipLevel < count) ? mipLevelDataSpans[mipLevel].data()
                                           : nullptr); // XXX check size
                }
            }
        }
    }

    image->target_ = target;

    // Restore previous binding for target.
    QglImageView* currentView = static_cast<QglImageView*>(currentImageViews_[0].get());
    GLuint oldObject =
        (currentView && currentView->target() == target) ? currentView->object() : 0;
    api_->glBindTexture(target, oldObject);
}

void QglEngine::initImageView_(ImageView* aView) {
    QglImageView* view = static_cast<QglImageView*>(aView);
    QglBuffer* buffer = view->viewedBuffer().get_static_cast<QglBuffer>();
    if (buffer) {
        GLuint object = 0;
        api_->glGenTextures(1, &object);
        view->bufferTextureObject_ = object;
        api_->glBindBuffer(GL_TEXTURE_BUFFER, object);
        api_->glTexBuffer(
            GL_TEXTURE_BUFFER, view->glFormat().internalFormat, buffer->object_);
        api_->glBindBuffer(GL_TEXTURE_BUFFER, 0);
        view->target_ = GL_TEXTURE_BUFFER;
    }
    else {
        QglImage* image = view->viewedImage().get_static_cast<QglImage>();
        view->target_ = image->target();
    }
}

void QglEngine::initSamplerState_(SamplerState* aState) {
    QglSamplerState* state = static_cast<QglSamplerState*>(aState);
    GLuint object = 0;
    api_->glGenSamplers(1, &object);
    state->object_ = object;
    api_->glSamplerParameteri(object, GL_TEXTURE_MAG_FILTER, state->magFilterGL_);
    api_->glSamplerParameteri(object, GL_TEXTURE_MIN_FILTER, state->minFilterGL_);
    api_->glSamplerParameteri(object, GL_TEXTURE_WRAP_S, state->wrapS_);
    api_->glSamplerParameteri(object, GL_TEXTURE_WRAP_T, state->wrapT_);
    api_->glSamplerParameteri(object, GL_TEXTURE_WRAP_R, state->wrapR_);
    api_->glSamplerParameteri(
        object, GL_TEXTURE_COMPARE_FUNC, state->comparisonFunctionGL_);
    api_->glSamplerParameterf(
        object, GL_TEXTURE_MAX_ANISOTROPY_EXT, state->maxAnisotropyGL_);
    api_->glSamplerParameterf(object, GL_TEXTURE_LOD_BIAS, state->mipLODBias());
    api_->glSamplerParameterf(object, GL_TEXTURE_MIN_LOD, state->minLOD());
    api_->glSamplerParameterf(object, GL_TEXTURE_MAX_LOD, state->maxLOD());
}

void QglEngine::initGeometryView_(GeometryView* /*view*/) {
    // no-op, vaos are built per program
}

void QglEngine::initBlendState_(BlendState* /*state*/) {
    // no-op
}

void QglEngine::initRasterizerState_(RasterizerState* /*state*/) {
    // no-op
}

void QglEngine::setSwapChain_(const SwapChainPtr& aSwapChain) {
    if (aSwapChain) {
        boundSwapChain_ = aSwapChain;
        QglSwapChain* swapChain = aSwapChain.get_static_cast<QglSwapChain>();
        surface_ = swapChain->surface_;
        // this may have changed
        scHeight_ = swapChain->height_;
        updateViewportAndScissorRect_(scHeight_);
    }
    else {
        surface_ = offscreenSurface_;
        QSize size = offscreenSurface_->size();
        scHeight_ = static_cast<GLsizei>(size.height());
        updateViewportAndScissorRect_(scHeight_);
    }
    ctx_->makeCurrent(surface_);
    api_->glEnable(GL_SCISSOR_TEST); // always enabled in graphics::Engine

    // XXX temporary: see comment in preBeginFrame_
    api_->glDisable(GL_DEPTH_TEST);
    api_->glDisable(GL_STENCIL_TEST);

    api_->glDisable(GL_PRIMITIVE_RESTART);
    isPrimitiveRestartEnabled_ = false;
    lastIndexFormat_ = 0;
}

void QglEngine::setFramebuffer_(const FramebufferPtr& aFramebuffer) {
    QglFramebuffer* framebuffer = aFramebuffer.get_static_cast<QglFramebuffer>();
    GLuint object = framebuffer ? framebuffer->object() : 0;
    if (!object || boundFramebuffer_ != object) {
        api_->glBindFramebuffer(GL_FRAMEBUFFER, object);
        boundFramebuffer_ = object;
        updateViewportAndScissorRect_(
            framebuffer
                // XXX crash for buffer texture targets
                ? static_cast<GLsizei>(framebuffer->colorView_->viewedImage()->height())
                : scHeight_);
    }
}

void QglEngine::setViewport_(Int x, Int y, Int width, Int height) {
    viewportRect_ = {};
    viewportRect_.x = static_cast<GLint>(x);
    viewportRect_.y = static_cast<GLint>(y);
    viewportRect_.w = static_cast<GLsizei>(width);
    viewportRect_.h = static_cast<GLsizei>(height);
    api_->glViewport(
        viewportRect_.x,
        rtHeight_ - (viewportRect_.y + viewportRect_.h),
        viewportRect_.w,
        viewportRect_.h);
}

void QglEngine::setProgram_(const ProgramPtr& aProgram) {
    QglProgram* program = aProgram.get_static_cast<QglProgram>();
    if (program) {
        program->prog_->bind();
    }
    //api_->glUseProgram(object);
    boundProgram_ = aProgram;
}

void QglEngine::setBlendState_(
    const BlendStatePtr& aState,
    const geometry::Vec4f& constantFactors) {

    if (boundBlendState_ != aState) {
        QglBlendState* oldState = boundBlendState_.get_static_cast<QglBlendState>();
        QglBlendState* newState = aState.get_static_cast<QglBlendState>();
        // OpenGL ES doesn't not support glEnablei and glBlendFunci.

        const bool isAlphaToCoverageEnabled = newState->isAlphaToCoverageEnabled();
        if (!oldState
            || isAlphaToCoverageEnabled != oldState->isAlphaToCoverageEnabled()) {
            setEnabled_(GL_SAMPLE_ALPHA_TO_COVERAGE, isAlphaToCoverageEnabled);
        }

        //GL_SAMPLE_ALPHA_TO_COVERAGE

        if (newState->isEnabled()) {
            const GlBlendEquation& equationRGB = newState->equationRGB_;
            const GlBlendEquation& equationAlpha = newState->equationAlpha_;
            const BlendWriteMask writeMask = newState->writeMask();

            if (!oldState || equationRGB != oldState->equationRGB_
                || equationAlpha != oldState->equationAlpha_) {
                api_->glBlendEquationSeparate(
                    equationRGB.operation, equationAlpha.operation);
                api_->glBlendFuncSeparate(
                    equationRGB.sourceFactor,
                    equationRGB.targetFactor,
                    equationAlpha.sourceFactor,
                    equationAlpha.targetFactor);
            }

            if (!oldState || writeMask != oldState->writeMask()) {
                api_->glColorMask(
                    writeMask.has(BlendWriteMaskBit::R),
                    writeMask.has(BlendWriteMaskBit::G),
                    writeMask.has(BlendWriteMaskBit::B),
                    writeMask.has(BlendWriteMaskBit::A));
            }

            if (!oldState || !oldState->isEnabled()) {
                api_->glEnable(GL_BLEND);
            }
        }
        else if (!oldState || oldState->isEnabled()) {
            api_->glDisable(GL_BLEND);
        }

        boundBlendState_ = aState;
    }
    if (!currentBlendConstantFactors_.has_value()
        || currentBlendConstantFactors_.value() != constantFactors) {
        api_->glBlendColor(
            constantFactors.x(),
            constantFactors.y(),
            constantFactors.z(),
            constantFactors.w());

        currentBlendConstantFactors_ = constantFactors;
    }
}

void QglEngine::setRasterizerState_(const RasterizerStatePtr& aState) {

    if (boundRasterizerState_ != aState) {
        QglRasterizerState* oldState =
            boundRasterizerState_.get_static_cast<QglRasterizerState>();
        QglRasterizerState* newState = aState.get_static_cast<QglRasterizerState>();

        GLenum fillModeGL = newState->fillModeGL_;
        GLenum cullModeGL = newState->cullModeGL_;
        bool isFrontCounterClockwise = newState->isFrontCounterClockwise();
        bool isDepthClippingEnabled = newState->isDepthClippingEnabled();
        bool isMultisamplingEnabled = newState->isMultisamplingEnabled();
        bool isLineAntialiasingEnabled = newState->isLineAntialiasingEnabled();

        if (!oldState || fillModeGL != oldState->fillModeGL_) {
            api_->glPolygonMode(GL_FRONT_AND_BACK, fillModeGL);
        }

        if (cullModeGL == 0) {
            if (!oldState || oldState->cullModeGL_ != 0) {
                api_->glDisable(GL_CULL_FACE);
            }
        }
        else {
            if (!oldState || oldState->cullModeGL_ == 0) {
                api_->glCullFace(cullModeGL);
                api_->glEnable(GL_CULL_FACE);
            }
            else if (cullModeGL != oldState->cullModeGL_) {
                api_->glCullFace(cullModeGL);
            }
        }

        if (!oldState || isFrontCounterClockwise != oldState->isFrontCounterClockwise()) {
            api_->glFrontFace(isFrontCounterClockwise ? GL_CCW : GL_CW);
        }

        if (!oldState || isDepthClippingEnabled != oldState->isDepthClippingEnabled()) {
            setEnabled_(GL_DEPTH_CLAMP, isDepthClippingEnabled);
        }

        if (!oldState || isMultisamplingEnabled != oldState->isMultisamplingEnabled()) {
            setEnabled_(GL_MULTISAMPLE, isMultisamplingEnabled);
        }

        if (!oldState
            || isLineAntialiasingEnabled != oldState->isLineAntialiasingEnabled()) {
            setEnabled_(GL_LINE_SMOOTH, isLineAntialiasingEnabled);
        }

        boundRasterizerState_ = aState;
    }
}

void QglEngine::setScissorRect_(const geometry::Rect2f& rect) {
    scissorRect_ = {};
    scissorRect_.x = static_cast<GLint>(std::round(rect.xMin()));
    scissorRect_.y = static_cast<GLint>(std::round(rect.yMin()));
    GLint x2 = static_cast<GLint>(std::round(rect.xMax()));
    GLint y2 = static_cast<GLint>(std::round(rect.yMax()));
    scissorRect_.w = x2 - scissorRect_.x;
    scissorRect_.h = y2 - scissorRect_.y;
    api_->glScissor(
        scissorRect_.x,
        rtHeight_ - (scissorRect_.y + scissorRect_.h),
        scissorRect_.w,
        scissorRect_.h);
}

void QglEngine::setStageConstantBuffers_(
    const BufferPtr* aBuffers,
    Int startIndex,
    Int count,
    ShaderStage shaderStage) {

    GLuint stageBaseIndex = core::toUnderlying(shaderStage) * maxConstantBuffersPerStage;
    for (Int i = 0; i < count; ++i) {
        const QglBuffer* buffer = aBuffers[i].get_static_cast<QglBuffer>();
        api_->glBindBufferBase(
            GL_UNIFORM_BUFFER,
            stageBaseIndex + startIndex + i,
            buffer ? buffer->object() : 0);
    }
}

void QglEngine::setStageImageViews_(
    const ImageViewPtr* views,
    Int startIndex,
    Int count,
    ShaderStage shaderStage) {

    const Int stageIndex = toIndex_(shaderStage);
    const Int begIndex = maxSamplersPerStage * stageIndex + startIndex;
    const Int endIndex = begIndex + count;
    for (Int i = begIndex, j = 0; i != endIndex; ++i, ++j) {
        if (currentImageViews_[i] != views[j]) {
            api_->glActiveTexture(GL_TEXTURE0 + i);
            QglImageView* newView = views[j].get_static_cast<QglImageView>();
            GLenum newTarget = 0;
            if (newView) {
                newTarget = newView->target();
                api_->glBindTexture(newTarget, newView->object());
            }
            QglImageView* oldView = currentImageViews_[i].get_static_cast<QglImageView>();
            if (oldView) {
                GLenum oldTarget = oldView->target();
                if (oldTarget != newTarget) {
                    api_->glBindTexture(oldTarget, 0);
                }
            }
            currentImageViews_[i] = views[j];
        }
    }
}

void QglEngine::setStageSamplers_(
    const SamplerStatePtr* states,
    Int startIndex,
    Int count,
    ShaderStage shaderStage) {

    const Int stageIndex = toIndex_(shaderStage);
    const Int begIndex = maxSamplersPerStage * stageIndex + startIndex;
    const Int endIndex = begIndex + count;
    for (Int i = begIndex, j = 0; i != endIndex; ++i, ++j) {
        if (currentSamplerStates_[i] != states[j]) {
            currentSamplerStates_[i] = states[j];
            QglSamplerState* state = static_cast<QglSamplerState*>(states[j].get());
            api_->glBindSampler(i, state ? state->object() : 0);
        }
    }
}

void QglEngine::updateBufferData_(Buffer* aBuffer, const void* data, Int lengthInBytes) {
    QglBuffer* buffer = static_cast<QglBuffer*>(aBuffer);
    loadBuffer_(buffer, data, lengthInBytes);
}

void QglEngine::generateMips_(const ImageViewPtr& aImageView) {
    QglImageView* imageView = aImageView.get_static_cast<QglImageView>();
    api_->glActiveTexture(GL_TEXTURE0);
    GLenum target = imageView->target();
    api_->glBindTexture(target, imageView->object());
    api_->glGenerateMipmap(target);
    // Restore previous binding for target.
    QglImageView* currentView = static_cast<QglImageView*>(currentImageViews_[0].get());
    GLuint oldObject =
        (currentView && currentView->target() == target) ? currentView->object() : 0;
    api_->glBindTexture(target, oldObject);
}

// should do init at beginFrame if needed..

void QglEngine::draw_(
    GeometryView* aView,
    UInt numIndices,
    UInt numInstances,
    UInt startIndex,
    Int baseVertex) {

    GLsizei nIdx = core::int_cast<GLsizei>(numIndices);
    GLsizei nInst = core::int_cast<GLsizei>(numInstances);
    GLint baseVtx = core::int_cast<GLint>(baseVertex);

    if (nIdx == 0) {
        return;
    }

    QglGeometryView* view = static_cast<QglGeometryView*>(aView);
    QglProgram* program = boundProgram_.get_static_cast<QglProgram>();
    if (!program) {
        VGC_WARNING(LogVgcUi, "cannot draw without a bound program");
        return;
    }

    VGC_CORE_ASSERT(program->builtinId() != BuiltinProgram::NotBuiltin);
    Int progIdx = core::toUnderlying(program->builtinId());
    VGC_CORE_ASSERT(view->builtinGeometryLayout() != BuiltinGeometryLayout::NotBuiltin);
    Int layoutIdx = core::toUnderlying(view->builtinGeometryLayout());

    GLuint& cachedVao = view->builtinProgramVaos_[progIdx][layoutIdx];
    if (cachedVao == nullGLuint) {
        GLuint vao = {};
        api_->glGenVertexArrays(1, &vao);
        cachedVao = vao;

        api_->glBindVertexArray(vao);

        for (const GlAttribPointerDesc& attribDesc :
             program->builtinLayouts_[layoutIdx]) {
            // maybe we could sort the attribs by buffer index..
            QglBuffer* vbuf =
                view->vertexBuffer(attribDesc.bufferIndex).get_static_cast<QglBuffer>();
            api_->glBindBuffer(GL_ARRAY_BUFFER, vbuf->object());
            api_->glVertexAttribPointer(
                attribDesc.index,
                attribDesc.numElements,
                attribDesc.elementType,
                attribDesc.normalized,
                attribDesc.stride,
                reinterpret_cast<const GLvoid*>(attribDesc.offset));
            api_->glEnableVertexAttribArray(attribDesc.index);
            if (attribDesc.isPerInstance) {
                api_->glVertexAttribDivisor(attribDesc.index, 1);
            }
        }
        api_->glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    else {
        api_->glBindVertexArray(cachedVao);
    }

    QglBuffer* indexBuffer = view->indexBuffer().get_static_cast<QglBuffer>();

    if (indexBuffer) {
        GLenum indexFormat = GL_UNSIGNED_INT;
        UInt indexSize = 4;
        if (view->indexFormat() == IndexFormat::UInt16) {
            indexFormat = GL_UNSIGNED_SHORT;
            indexSize = 2;
        }

        if (indexFormat != lastIndexFormat_) {
            switch (indexFormat) {
            case GL_UNSIGNED_SHORT:
                api_->glPrimitiveRestartIndex(0xFFFFu);
                break;
            case GL_UNSIGNED_INT:
            default:
                api_->glPrimitiveRestartIndex(0xFFFFFFFFu);
                break;
            }
            lastIndexFormat_ = indexFormat;
        }

        if (!isPrimitiveRestartEnabled_) {
            api_->glEnable(GL_PRIMITIVE_RESTART);
            isPrimitiveRestartEnabled_ = true;
        }

        const GLvoid* indicesOffset = reinterpret_cast<GLvoid*>(startIndex * indexSize);
        api_->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer->object());

        if (numInstances == 0) {
            api_->glDrawElementsBaseVertex(
                view->drawMode_, nIdx, indexFormat, indicesOffset, baseVtx);
        }
        else {
            api_->glDrawElementsInstancedBaseVertex(
                view->drawMode_, nIdx, indexFormat, indicesOffset, nInst, baseVtx);
        }
    }
    else {
        GLint first = core::int_cast<GLint>(startIndex) + baseVtx;

        if (isPrimitiveRestartEnabled_) {
            api_->glDisable(GL_PRIMITIVE_RESTART);
            isPrimitiveRestartEnabled_ = false;
        }

        if (numInstances == 0) {
            api_->glDrawArrays(view->drawMode_, first, nIdx);
        }
        else {
            api_->glDrawArraysInstanced(view->drawMode_, first, nIdx, nInst);
        }
    }
}

void QglEngine::clear_(const core::Color& color) {
    api_->glClearColor(
        static_cast<float>(color.r()),
        static_cast<float>(color.g()),
        static_cast<float>(color.b()),
        static_cast<float>(color.a()));
    api_->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

UInt64 QglEngine::present_(
    SwapChain* aSwapChain,
    UInt32 /*syncInterval*/,
    PresentFlags /*flags*/) {

    QglSwapChain* swapChain = static_cast<QglSwapChain*>(aSwapChain);
    if (surface_ != swapChain->surface_) {
        ctx_->makeCurrent(swapChain->surface_);
    }
    ctx_->swapBuffers(swapChain->surface_);
    ctx_->makeCurrent(surface_);
    return 0;
}

void QglEngine::preBeginFrame_(SwapChain* aSwapChain, FrameKind kind) {

    if (kind != graphics::FrameKind::QWidget || !aSwapChain) {
        return;
    }
    QglSwapChain* swapChain = static_cast<QglSwapChain*>(aSwapChain);

    // Reset various states
    //
    boundFramebuffer_ = badGLuint;
    boundBlendState_.reset();
    currentBlendConstantFactors_.reset();
    boundRasterizerState_.reset();
    currentImageViews_.fill(nullptr);
    currentSamplerStates_.fill(nullptr);

    // Update viewport rect from values already set in OpenGL.
    //
    // We need this when using QglEngine in combination with QOpenGLWidget,
    // because in this situation Qt calls glViewport for us, so client code
    // (e.g., in UiWidget) never explicitly calls engine->setViewport().
    //
    // This ensures that we have correct values in viewportRect_, which is
    // important since functions like setScissorRect_ rely on it.
    //
    // Note: in theory, we're not supposed to be able to call glGetIntegerv()
    // here because preBeginFrame_() is called in the user thread, not the
    // render thread. In practice, it's ok since we have `kind == QWidget`, in
    // which case there is no multithreading.
    //
    GLint rect[4];
    api_->glGetIntegerv(GL_VIEWPORT, rect);
    viewportRect_.x = rect[0];
    viewportRect_.y = swapChain->height_ - (rect[1] + rect[3]);
    viewportRect_.w = rect[2];
    viewportRect_.h = rect[3];

    // XXX depth and stencil are currently disabled in setSwapChain_
    // TODO: set proper depth/stencil states (not yet implemented in Engine)
}

void QglEngine::updateViewportAndScissorRect_(GLsizei scHeight) {

    if (scHeight != rtHeight_) {
        rtHeight_ = scHeight;
        api_->glViewport(
            viewportRect_.x,
            rtHeight_ - (viewportRect_.y + viewportRect_.h),
            viewportRect_.w,
            viewportRect_.h);

        api_->glScissor(
            scissorRect_.x,
            rtHeight_ - (scissorRect_.y + scissorRect_.h),
            scissorRect_.w,
            scissorRect_.h);
    }
}

// Private methods

void QglEngine::makeCurrent_() {
    ctx_->makeCurrent(ctx_->surface());
}

bool QglEngine::loadBuffer_(class QglBuffer* buffer, const void* data, Int dataSize) {

    if (dataSize == 0) {
        return false;
    }

    const GLuint object = buffer->object();
    api_->glBindBuffer(GL_COPY_WRITE_BUFFER, object);

    Int allocSize = buffer->allocatedSize_;
    bool skipCopy = false;
    if ((dataSize > allocSize) || (dataSize * 4 < allocSize)) {
        GLsizeiptr dataWidth = core::int_cast<GLsizeiptr>(dataSize);
        allocSize = dataWidth;
        if (buffer->bindFlags() & BindFlag::ConstantBuffer) {
            allocSize = (allocSize + 0xFu) & ~GLsizeiptr(0xFu);
        }
        if (data && allocSize == dataWidth) {
            api_->glBufferData(GL_COPY_WRITE_BUFFER, allocSize, data, buffer->usage_);
            skipCopy = true;
        }
        else {
            api_->glBufferData(GL_COPY_WRITE_BUFFER, allocSize, nullptr, buffer->usage_);
        }
        buffer->allocatedSize_ = allocSize;
    }

    if (data && !skipCopy) {
        void* mapped = api_->glMapBuffer(GL_COPY_WRITE_BUFFER, GL_WRITE_ONLY);
        if (mapped) {
            std::memcpy(mapped, data, dataSize);
            api_->glUnmapBuffer(GL_COPY_WRITE_BUFFER);
        }
        else {
            VGC_ERROR(LogVgcUi, "Couldn't map buffer.");
        }
    }

    api_->glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
    return true;
}

} // namespace vgc::ui::detail::qopengl
