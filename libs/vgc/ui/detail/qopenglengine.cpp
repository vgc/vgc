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
    std::string path = core::resourcePath("graphics/opengl/" + name);
    return toQt(path);
}

struct XYRGBVertex {
    float x, y, r, g, b;
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

    bool isEquivalentTo(const QglSamplerState& other) const {
        if (maxAnisotropyGL_ > 1.f && other.maxAnisotropyGL_ > 1.f) {
            if (maxAnisotropyGL_ != other.maxAnisotropyGL_) {
                return false;
            }
        }
        else {
            if (magFilterGL_ != other.magFilterGL_) {
                return false;
            }
            if (minFilterGL_ != other.minFilterGL_) {
                return false;
            }
            if (mipFilterGL_ != other.mipFilterGL_) {
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

private:
    float maxAnisotropyGL_ = 0.f;
    GLenum magFilterGL_ = badGLenum;
    GLenum minFilterGL_ = badGLenum;
    GLenum mipFilterGL_ = badGLenum;
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
        return formatGL_;
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
    GlFormat formatGL_ = {};
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
    GlFormat glFormat() const {
        return formatGL_;
    }

    GLuint object() const {
        QglImage* image = viewedImage().get_static_cast<QglImage>();
        return image ? image->object() : bufferTextureObject_;
    }

protected:
    void releaseSubResources_() override {
        ImageView::releaseSubResources_();
        viewSamplerState_.reset();
        samplerStatePtrAddress_ = nullptr;
    }

    void release_(Engine* engine) override {
        ImageView::release_(engine);
        static_cast<QglEngine*>(engine)->api()->glDeleteTextures(
            1, &bufferTextureObject_);
    }

private:
    GLuint bufferTextureObject_ = badGLuint;
    GlFormat formatGL_ = {};
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
    GLuint index;
    GLint numElements;
    GLenum elementType;
    GLboolean normalized;
    GLsizei stride;
    uintptr_t offset;
    uintptr_t bufferIndex;
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
};

// ENUM CONVERSIONS

GlFormat pixelFormatToGlFormat(PixelFormat format) {

    using F = GlFormat;
    static_assert(numPixelFormats == 47);

    // clang-format off
    static constexpr std::array<GlFormat, numPixelFormats> map = {
        // InternalFormat,          PixelType,                          PixelType
        F{ 0,                       0,                                  0                   },  // Unknown
        // Depth
        F{ GL_DEPTH_COMPONENT16,    GL_UNSIGNED_SHORT,                  GL_DEPTH_COMPONENT  },  // D_16_UNORM,
        F{ GL_DEPTH_COMPONENT32F,   GL_FLOAT,                           GL_DEPTH_COMPONENT  },  // D_32_FLOAT,
        // Depth + Stencil
        F{ GL_DEPTH24_STENCIL8,     GL_UNSIGNED_INT_24_8,               GL_DEPTH_STENCIL    },  // DS_24_UNORM_8_UINT,
        F{ GL_DEPTH32F_STENCIL8,    GL_FLOAT_32_UNSIGNED_INT_24_8_REV,  GL_DEPTH_STENCIL    },  // DS_32_FLOAT_8_UINT_24_X,
        // Red
        F{ GL_R8,                   GL_UNSIGNED_BYTE,                   GL_RED              },  // R_8_UNORM
        F{ GL_R8_SNORM,             GL_BYTE,                            GL_RED              },  // R_8_SNORM
        F{ GL_R8UI,                 GL_UNSIGNED_BYTE,                   GL_RED_INTEGER      },  // R_8_UINT
        F{ GL_R8I,                  GL_BYTE,                            GL_RED_INTEGER      },  // R_8_SINT
        F{ GL_R16,                  GL_UNSIGNED_SHORT,                  GL_RED              },  // R_16_UNORM
        F{ GL_R16_SNORM,            GL_SHORT,                           GL_RED              },  // R_16_SNORM
        F{ GL_R16UI,                GL_UNSIGNED_SHORT,                  GL_RED_INTEGER      },  // R_16_UINT
        F{ GL_R16I,                 GL_SHORT,                           GL_RED_INTEGER      },  // R_16_SINT
        F{ GL_R16F,                 GL_HALF_FLOAT,                      GL_RED              },  // R_16_FLOAT
        F{ GL_R32UI,                GL_UNSIGNED_INT,                    GL_RED_INTEGER      },  // R_32_UINT
        F{ GL_R32I,                 GL_INT,                             GL_RED_INTEGER      },  // R_32_SINT
        F{ GL_R32F,                 GL_FLOAT,                           GL_RED              },  // R_32_FLOAT
        // RG
        F{ GL_RG8,                  GL_UNSIGNED_BYTE,                   GL_RG               },  // RG_8_UNORM
        F{ GL_RG8_SNORM,            GL_BYTE,                            GL_RG               },  // RG_8_SNORM
        F{ GL_RG8UI,                GL_UNSIGNED_BYTE,                   GL_RG_INTEGER       },  // RG_8_UINT
        F{ GL_RG8I,                 GL_BYTE,                            GL_RG_INTEGER       },  // RG_8_SINT
        F{ GL_RG16,                 GL_UNSIGNED_SHORT,                  GL_RG               },  // RG_16_UNORM
        F{ GL_RG16_SNORM,           GL_SHORT,                           GL_RG               },  // RG_16_SNORM
        F{ GL_RG16UI,               GL_UNSIGNED_SHORT,                  GL_RG_INTEGER       },  // RG_16_UINT
        F{ GL_RG16I,                GL_SHORT,                           GL_RG_INTEGER       },  // RG_16_SINT
        F{ GL_RG16F,                GL_HALF_FLOAT,                      GL_RG               },  // RG_16_FLOAT
        F{ GL_RG32UI,               GL_UNSIGNED_INT,                    GL_RG_INTEGER       },  // RG_32_UINT
        F{ GL_RG32I,                GL_INT,                             GL_RG_INTEGER       },  // RG_32_SINT
        F{ GL_RG32F,                GL_FLOAT,                           GL_RG               },  // RG_32_FLOAT
        // RGB
        F{ GL_R11F_G11F_B10F,       GL_UNSIGNED_INT_10F_11F_11F_REV,    GL_RGB              },  // RGB_11_11_10_FLOAT
        F{ GL_RGB32UI,              GL_UNSIGNED_INT,                    GL_RGB_INTEGER      },  // RGB_32_UINT
        F{ GL_RGB32I,               GL_INT,                             GL_RGB_INTEGER      },  // RGB_32_SINT
        F{ GL_RGB32F,               GL_FLOAT,                           GL_RGB              },  // RGB_32_FLOAT
        // RGBA
        F{ GL_RGBA8,                GL_UNSIGNED_BYTE,                   GL_RGBA             },  // RGBA_8_UNORM
        F{ GL_SRGB8_ALPHA8,         GL_UNSIGNED_BYTE,                   GL_RGBA             },  // RGBA_8_UNORM_SRGB
        F{ GL_RGBA8_SNORM,          GL_BYTE,                            GL_RGBA             },  // RGBA_8_SNORM
        F{ GL_RGBA8UI,              GL_UNSIGNED_BYTE,                   GL_RGBA_INTEGER     },  // RGBA_8_UINT
        F{ GL_RGBA8I,               GL_BYTE,                            GL_RGBA_INTEGER     },  // RGBA_8_SINT
        F{ GL_RGB10_A2,             GL_UNSIGNED_INT_10_10_10_2,         GL_RGBA             },  // RGBA_10_10_10_2_UNORM
        F{ GL_RGB10_A2UI,           GL_UNSIGNED_INT_10_10_10_2,         GL_RGBA_INTEGER     },  // RGBA_10_10_10_2_UINT
        F{ GL_RGBA16,               GL_UNSIGNED_SHORT,                  GL_RGBA             },  // RGBA_16_UNORM
        F{ GL_RGBA16UI,             GL_UNSIGNED_SHORT,                  GL_RGBA_INTEGER     },  // RGBA_16_UINT
        F{ GL_RGBA16I,              GL_SHORT,                           GL_RGBA_INTEGER     },  // RGBA_16_SINT
        F{ GL_RGBA16F,              GL_HALF_FLOAT,                      GL_RGBA             },  // RGBA_16_FLOAT
        F{ GL_RGBA32UI,             GL_UNSIGNED_INT,                    GL_RGBA_INTEGER     },  // RGBA_32_UINT
        F{ GL_RGBA32I,              GL_INT,                             GL_RGBA_INTEGER     },  // RGBA_32_SINT
        F{ GL_RGBA32F,              GL_FLOAT,                           GL_RGBA             },  // RGBA_32_FLOAT
    };
    // clang-format on

    const UInt index = core::toUnderlying(format);
    if (index == 0 || index >= numPixelFormats) {
        throw core::LogicError("QglEngine: invalid PrimitiveType enum value");
    }

    return map[index];
}

GLenum primitiveTypeToGLenum(PrimitiveType type) {

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
        throw core::LogicError("QglEngine: invalid PrimitiveType enum value");
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
            "Qgl: staging buffer needs either read and write cpu access");
    }
    default:
        break;
    }
    throw core::LogicError("QglEngine: unsupported usage");
}

void processResourceMiscFlags(ResourceMiscFlags resourceMiscFlags) {
    if (resourceMiscFlags & ResourceMiscFlag::Shared) {
        throw core::LogicError(
            "QglEngine: ResourceMiscFlag::Shared is not supported at the moment");
    }
    //if (resourceMiscFlags & ResourceMiscFlag::TextureCube) {
    //    throw core::LogicError("QglEngine: ResourceMiscFlag::TextureCube is not supported at the moment");
    //}
    //if (resourceMiscFlags & ResourceMiscFlag::ResourceClamp) {
    //    throw core::LogicError("QglEngine: ResourceMiscFlag::ResourceClamp is not supported at the moment");
    //}
    return;
}

GLenum imageWrapModeToGLenum(ImageWrapMode mode) {

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
        throw core::LogicError("QglEngine: invalid ImageWrapMode enum value");
    }

    return map[index];
}

GLenum comparisonFunctionToGLenum(ComparisonFunction func) {

    static_assert(numComparisonFunctions == 10);
    static constexpr std::array<GLenum, numComparisonFunctions> map = {
        badGLenum,   // Undefined
        GL_NEVER,    // Disabled
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
        throw core::LogicError("QglEngine: invalid ComparisonFunction enum value");
    }

    return map[index];
}

GLenum blendFactorToGLenum(BlendFactor factor) {

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
        throw core::LogicError("QglEngine: invalid BlendFactor enum value");
    }

    return map[index];
}

GLenum blendOpToGLenum(BlendOp op) {

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
        throw core::LogicError("QglEngine: invalid BlendOp enum value");
    }

    return map[index];
}

GLenum fillModeToGLenum(FillMode mode) {

    static_assert(numFillModes == 3);
    static constexpr std::array<GLenum, numFillModes> map = {
        badGLenum, // Undefined
        GL_FILL,   // Solid
        GL_LINE,   // Wireframe
    };

    const UInt index = core::toUnderlying(mode);
    if (index == 0 || index >= numFillModes) {
        throw core::LogicError("QglEngine: invalid FillMode enum value");
    }

    return map[index];
}

GLenum cullModeToGLenum(CullMode mode) {

    static_assert(numCullModes == 4);
    static constexpr std::array<GLenum, numCullModes> map = {
        badGLenum,         // Undefined
        GL_FRONT_AND_BACK, // None -> must disable culling
        GL_FRONT,          // Front
        GL_BACK,           // Back
    };

    const UInt index = core::toUnderlying(mode);
    if (index == 0 || index >= numCullModes) {
        throw core::LogicError("QglEngine: invalid CullMode enum value");
    }

    return map[index];
}

GLenum filterModeToGLenum(FilterMode mode) {

    static_assert(numFilterModes == 3);
    static constexpr std::array<GLenum, numFilterModes> map = {
        badGLenum,  // Undefined
        GL_NEAREST, // Point
        GL_LINEAR,  // Linear
    };

    const UInt index = core::toUnderlying(mode);
    if (index == 0 || index >= numFilterModes) {
        throw core::LogicError("QglEngine: invalid FilterMode enum value");
    }

    return map[index];
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

        // XXX use buffer count
        format_.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    }
    QSurfaceFormat::setDefaultFormat(format_);

    currentImageViews_.fill(nullptr);
    currentSamplerStates_.fill(nullptr);
    isTextureStateDirtyMap_.fill(true);
    isAnyTextureStateDirty_ = true;

    //createBuiltinResources_();
}

void QglEngine::onDestroyed() {
    Engine::onDestroyed();
    if (!isExternalCtx_) {
        delete ctx_;
    }
    ctx_ = nullptr;
    delete offscreenSurface_;
    offscreenSurface_ = nullptr;
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
    image->formatGL_ = pixelFormatToGlFormat(createInfo.pixelFormat());
    return ImagePtr(image.release());
}

ImageViewPtr QglEngine::constructImageView_(
    const ImageViewCreateInfo& createInfo,
    const ImagePtr& image) {

    auto view = makeUnique<QglImageView>(resourceRegistry_, createInfo, image);
    view->formatGL_ = image.get_static_cast<QglImage>()->glFormat();
    return ImageViewPtr(view.release());
}

ImageViewPtr QglEngine::constructImageView_(
    const ImageViewCreateInfo& createInfo,
    const BufferPtr& buffer,
    PixelFormat format,
    UInt32 numElements) {

    auto view = makeUnique<QglImageView>(
        resourceRegistry_, createInfo, buffer, format, numElements);
    view->formatGL_ = pixelFormatToGlFormat(format);
    return ImageViewPtr(view.release());
}

SamplerStatePtr
QglEngine::constructSamplerState_(const SamplerStateCreateInfo& createInfo) {
    auto state = makeUnique<QglSamplerState>(resourceRegistry_, createInfo);
    state->magFilterGL_ = filterModeToGLenum(createInfo.magFilter());
    state->minFilterGL_ = filterModeToGLenum(createInfo.minFilter());
    state->mipFilterGL_ = filterModeToGLenum(createInfo.mipFilter());
    if (createInfo.maxAnisotropy() >= 1) {
        if (hasAnisotropicFilteringSupport_) {
            state->maxAnisotropyGL_ = static_cast<float>(createInfo.maxAnisotropy());
        }
        else {
            VGC_WARNING(LogVgcUi, "Anisotropic filtering is not supported.");
        }
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

void QglEngine::resizeSwapChain_(
    SwapChain* /*swapChain*/,
    UInt32 /*width*/,
    UInt32 /*height*/) {
    // XXX anything to do ?
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
}

void QglEngine::initBuiltinResources_() {

    // Initialize shader program
    QglProgram* simpleProgram = simpleProgram_.get_static_cast<QglProgram>();
    simpleProgram->prog_.reset(new QOpenGLShaderProgram());
    QOpenGLShaderProgram* prog = simpleProgram->prog_.get();
    prog->addShaderFromSourceFile(
        QOpenGLShader::Vertex,
        shaderPath_("iv4pos_iv4col_um4proj_um4view_ov4fcol.v.glsl"));
    prog->addShaderFromSourceFile(QOpenGLShader::Fragment, shaderPath_("iv4fcol.f.glsl"));
    prog->link();
    prog->bind();
    int xyLoc_ = prog->attributeLocation("pos");
    int rgbLoc_ = prog->attributeLocation("col");
    api_->glUniformBlockBinding(prog->programId(), 0, 0);
    prog->release();

    core::Array<GlAttribPointerDesc>& layout =
        simpleProgram->builtinLayouts_[core::toUnderlying(BuiltinGeometryLayout::XYRGB)];

    GlAttribPointerDesc& xyDesc = layout.emplaceLast();
    xyDesc.index = xyLoc_;
    xyDesc.numElements = 2;
    xyDesc.elementType = GL_FLOAT;
    xyDesc.normalized = false;
    xyDesc.stride = sizeof(XYRGBVertex);
    xyDesc.offset = static_cast<uintptr_t>(offsetof(XYRGBVertex, x));
    xyDesc.bufferIndex = 0;

    GlAttribPointerDesc& rgbDesc = layout.emplaceLast();
    rgbDesc.index = rgbLoc_;
    rgbDesc.numElements = 3;
    rgbDesc.elementType = GL_FLOAT;
    rgbDesc.normalized = false;
    rgbDesc.stride = sizeof(XYRGBVertex);
    rgbDesc.offset = static_cast<uintptr_t>(offsetof(XYRGBVertex, r));
    rgbDesc.bufferIndex = 0;
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
    const Span<const char>* mipLevelDataSpans,
    Int count) {

    QglImage* image = static_cast<QglImage*>(aImage);

    if (count <= 0) {
        mipLevelDataSpans = nullptr;
    }
    else {
        VGC_CORE_ASSERT(mipLevelDataSpans);
    }

    GLint numLayers = image->numLayers();
    GLint numMipLevels = image->numMipLevels();
    [[maybe_unused]] bool isImmutable = image->usage() == Usage::Immutable;
    [[maybe_unused]] bool isMultisampled = image->numSamples() > 1;
    [[maybe_unused]] bool isMipmapGenEnabled = image->isMipGenerationEnabled();
    [[maybe_unused]] bool isArray = numLayers > 1;

    VGC_CORE_ASSERT(isMipmapGenEnabled || (numMipLevels > 0));

    GLuint object = 0;
    api_->glGenTextures(1, &object);
    image->object_ = object;

    GLenum target = badGLenum;

    if (mipLevelDataSpans) {
        // XXX let's consider for now that we are provided full mips or nothing
        VGC_CORE_ASSERT(numMipLevels == count);
        VGC_CORE_ASSERT(numMipLevels > 0);
    }
    else {
        VGC_CORE_ASSERT(!isImmutable);
    }

    GlFormat glFormat = image->glFormat();

    if (image->rank() == ImageRank::_1D) {
        VGC_CORE_ASSERT(!isMultisampled);

        if (isArray) {
            target = GL_TEXTURE_1D_ARRAY;
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
                    mipLevelDataSpans ? mipLevelDataSpans[mipLevel].data()
                                      : nullptr); // XXX check size
            }
        }
        else {
            target = GL_TEXTURE_1D;
            for (Int mipLevel = 0; mipLevel < numMipLevels; ++mipLevel) {
                api_->glTexImage1D(
                    GL_TEXTURE_1D,
                    mipLevel,
                    glFormat.internalFormat,
                    image->width(),
                    0,
                    glFormat.pixelFormat,
                    glFormat.pixelType,
                    mipLevelDataSpans ? mipLevelDataSpans[mipLevel].data()
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
                        mipLevelDataSpans ? mipLevelDataSpans[mipLevel].data()
                                          : nullptr); // XXX check size
                }
            }
        }
        else {
            if (isMultisampled) {
                target = GL_TEXTURE_2D_MULTISAMPLE;
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
                        mipLevelDataSpans ? mipLevelDataSpans[mipLevel].data()
                                          : nullptr); // XXX check size
                }
            }
        }
    }

    image->target_ = target;
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
            GL_TEXTURE_BUFFER, view->formatGL_.internalFormat, buffer->object_);
        api_->glBindBuffer(GL_TEXTURE_BUFFER, 0);
    }
}

void QglEngine::initSamplerState_(SamplerState* /*state*/) {
    // no-op
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

void QglEngine::setSwapChain_(const SwapChainPtr& swapChain) {
    if (swapChain) {
        surface_ = swapChain.get_static_cast<QglSwapChain>()->surface_;
    }
    else {
        surface_ = offscreenSurface_;
    }
    ctx_->makeCurrent(surface_);
}

void QglEngine::setFramebuffer_(const FramebufferPtr& aFramebuffer) {
    QglFramebuffer* framebuffer = aFramebuffer.get_static_cast<QglFramebuffer>();
    GLuint object = framebuffer ? framebuffer->object() : 0;
    api_->glBindFramebuffer(GL_FRAMEBUFFER, object);
    boundFramebuffer_ = object;
}

void QglEngine::setViewport_(Int x, Int y, Int width, Int height) {
    api_->glViewport(x, y, width, height);
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
    const geometry::Vec4f& blendFactor) {

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
    if (!currentBlendFactor_.has_value() || currentBlendFactor_.value() != blendFactor) {
        api_->glBlendColor(
            blendFactor.x(), blendFactor.y(), blendFactor.z(), blendFactor.w());

        currentBlendFactor_ = blendFactor;
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
        bool isScissoringEnabled = newState->isScissoringEnabled();
        bool isMultisamplingEnabled = newState->isMultisamplingEnabled();
        bool isLineAntialiasingEnabled = newState->isLineAntialiasingEnabled();

        if (!oldState || fillModeGL != oldState->fillModeGL_) {
            api_->glPolygonMode(GL_FRONT_AND_BACK, fillModeGL);
        }

        if (!oldState || cullModeGL != oldState->cullModeGL_) {
            api_->glCullFace(cullModeGL);
        }

        if (!oldState || isFrontCounterClockwise != oldState->isFrontCounterClockwise()) {
            api_->glFrontFace(isFrontCounterClockwise ? GL_CCW : GL_CW);
        }

        if (!oldState || isDepthClippingEnabled != oldState->isDepthClippingEnabled()) {
            setEnabled_(GL_DEPTH_CLAMP, isDepthClippingEnabled);
        }

        if (!oldState || isScissoringEnabled != oldState->isScissoringEnabled()) {
            setEnabled_(GL_SCISSOR_TEST, isScissoringEnabled);
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
    const ImageViewPtr* /*views*/,
    Int /*startIndex*/,
    Int /*count*/,
    ShaderStage /*shaderStage*/) {

    // todo, + defer coupling with sampler
}

void QglEngine::setStageSamplers_(
    const SamplerStatePtr* /*states*/,
    Int /*startIndex*/,
    Int /*count*/,
    ShaderStage /*shaderStage*/) {

    // todo, + defer coupling with view
}

void QglEngine::updateBufferData_(Buffer* aBuffer, const void* data, Int lengthInBytes) {
    QglBuffer* buffer = static_cast<QglBuffer*>(aBuffer);
    loadBuffer_(buffer, data, lengthInBytes);
}

// should do init at beginFrame if needed..

void QglEngine::draw_(GeometryView* aView, UInt numIndices, UInt numInstances) {

    syncTextureStates_();

    GLsizei nIdx = core::int_cast<GLsizei>(numIndices);
    GLsizei nInst = core::int_cast<GLsizei>(numInstances);

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
        }
        api_->glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    else {
        api_->glBindVertexArray(cachedVao);
    }

    QglBuffer* indexBuffer = view->indexBuffer().get_static_cast<QglBuffer>();
    GLenum indexFormat = (view->indexFormat() == IndexFormat::UInt16) ? GL_UNSIGNED_SHORT
                                                                      : GL_UNSIGNED_INT;

    if (numInstances == 0) {
        if (indexBuffer) {
            api_->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer->object());
            api_->glDrawElements(view->drawMode_, nIdx, indexFormat, 0);
        }
        else {
            api_->glDrawArrays(view->drawMode_, 0, nIdx);
        }
    }
    else {
        if (indexBuffer) {
            api_->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer->object());
            api_->glDrawElementsInstanced(view->drawMode_, nIdx, indexFormat, 0, nInst);
        }
        else {
            api_->glDrawArraysInstanced(view->drawMode_, 0, nIdx, nInst);
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

void QglEngine::setStateDirty_() {
    boundFramebuffer_ = badGLuint;
    boundBlendState_.reset();
    currentBlendFactor_.reset();
    boundRasterizerState_.reset();
    currentImageViews_.fill(nullptr);
    currentSamplerStates_.fill(nullptr);
    isTextureStateDirtyMap_.fill(true);
    // temporary
    api_->glDisable(GL_DEPTH_TEST);
    api_->glDisable(GL_STENCIL_TEST);
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

void QglEngine::syncTextureStates_() {
    // XXX todo
}

} // namespace vgc::ui::detail::qopengl
