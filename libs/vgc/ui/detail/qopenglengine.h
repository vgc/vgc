//// Copyright 2022 The VGC Developers
//// See the COPYRIGHT file at the top-level directory of this distribution
//// and at https://github.com/vgc/vgc/blob/master/COPYRIGHT
////
//// Licensed under the Apache License, Version 2.0 (the "License");
//// you may not use this file except in compliance with the License.
//// You may obtain a copy of the License at
////
////     http://www.apache.org/licenses/LICENSE-2.0
////
//// Unless required by applicable law or agreed to in writing, software
//// distributed under the License is distributed on an "AS IS" BASIS,
//// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef VGC_UI_QOPENGLENGINE_H
#define VGC_UI_QOPENGLENGINE_H

#include <chrono>
#include <memory>
#include <optional>

#include <QOffscreenSurface>
#include <QOpenGLBuffer>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLFunctions_3_2_Core>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QPointF>
#include <QString>

#include <vgc/core/color.h>
#include <vgc/core/paths.h>
#include <vgc/geometry/mat4d.h>
#include <vgc/geometry/mat4f.h>
#include <vgc/geometry/vec2d.h>
#include <vgc/geometry/vec2f.h>
#include <vgc/graphics/engine.h>
#include <vgc/ui/api.h>

namespace vgc::ui::detail {

inline geometry::Mat4f toMat4f(const geometry::Mat4d& m) {
    // TODO: implement Mat4d to Mat4f conversion directly in Mat4x classes
    return geometry::Mat4f(
        (float)m(0, 0),
        (float)m(0, 1),
        (float)m(0, 2),
        (float)m(0, 3),
        (float)m(1, 0),
        (float)m(1, 1),
        (float)m(1, 2),
        (float)m(1, 3),
        (float)m(2, 0),
        (float)m(2, 1),
        (float)m(2, 2),
        (float)m(2, 3),
        (float)m(3, 0),
        (float)m(3, 1),
        (float)m(3, 2),
        (float)m(3, 3));
}

namespace qopengl {

VGC_DECLARE_OBJECT(QglEngine);

using namespace ::vgc::graphics;

inline constexpr Int requiredOpenGLVersionMajor = 3;
inline constexpr Int requiredOpenGLVersionMinor = 3;
using OpenGLFunctions = QOpenGLFunctions_3_3_Core;
inline constexpr QPair<int, int>
    requiredOpenGLVersionQPair(requiredOpenGLVersionMajor, requiredOpenGLVersionMinor);

/// \class vgc::widget::QglEngine
/// \brief The QtOpenGL-based graphics::Engine.
///
/// This class is an implementation of graphics::Engine using QOpenGLContext and
/// OpenGL calls.
///
class VGC_UI_API QglEngine final : public Engine {
private:
    VGC_OBJECT(QglEngine, Engine)

protected:
    QglEngine(const EngineCreateInfo& createInfo, QOpenGLContext* ctx);

    void onDestroyed() override;

public:
    /// Creates a new OpenglEngine.
    ///
    static QglEnginePtr create(const EngineCreateInfo& createInfo);
    static QglEnginePtr
    create(const EngineCreateInfo& createInfo, QOpenGLContext* externalCtx);

    // not part of the common interface

    SwapChainPtr createSwapChainFromSurface(QSurface* surface);

    OpenGLFunctions* api() const {
        return api_;
    }

protected:
    // Implementation of Engine API

    // -- USER THREAD implementation functions --

    void createBuiltinShaders_() override;

    SwapChainPtr constructSwapChain_(const SwapChainCreateInfo& createInfo) override;
    FramebufferPtr constructFramebuffer_(const ImageViewPtr& colorImageView) override;
    BufferPtr constructBuffer_(const BufferCreateInfo& createInfo) override;
    ImagePtr constructImage_(const ImageCreateInfo& createInfo) override;
    ImageViewPtr constructImageView_(
        const ImageViewCreateInfo& createInfo,
        const ImagePtr& image) override;
    ImageViewPtr constructImageView_(
        const ImageViewCreateInfo& createInfo,
        const BufferPtr& buffer,
        PixelFormat format,
        UInt32 numElements) override;
    SamplerStatePtr
    constructSamplerState_(const SamplerStateCreateInfo& createInfo) override;
    GeometryViewPtr
    constructGeometryView_(const GeometryViewCreateInfo& createInfo) override;
    BlendStatePtr constructBlendState_(const BlendStateCreateInfo& createInfo) override;
    RasterizerStatePtr
    constructRasterizerState_(const RasterizerStateCreateInfo& createInfo) override;

    void onWindowResize_(SwapChain* swapChain, UInt32 width, UInt32 height) override;

    void preBeginFrame_(SwapChain* swapChain, FrameKind kind) override;

    //--  RENDER THREAD implementation functions --

    void initContext_() override;
    void initBuiltinResources_() override;

    void initFramebuffer_(Framebuffer* framebuffer) override;
    void initBuffer_(Buffer* buffer, const char* data, Int lengthInBytes) override;
    void initImage_(
        Image* image,
        const core::Span<const char>* mipLevelDataSpans,
        Int numMipLevels) override;
    void initImageView_(ImageView* view) override;
    void initSamplerState_(SamplerState* state) override;
    void initGeometryView_(GeometryView* view) override;
    void initBlendState_(BlendState* state) override;
    void initRasterizerState_(RasterizerState* state) override;

    void setSwapChain_(const SwapChainPtr& swapChain) override;
    void setFramebuffer_(const FramebufferPtr& framebuffer) override;
    void setViewport_(Int x, Int y, Int width, Int height) override;
    void setProgram_(const ProgramPtr& program) override;
    void setBlendState_(const BlendStatePtr& state, const geometry::Vec4f& blendFactor)
        override;
    void setRasterizerState_(const RasterizerStatePtr& state) override;
    void setScissorRect_(const geometry::Rect2f& rect) override;
    void setStageConstantBuffers_(
        const BufferPtr* buffers,
        Int startIndex,
        Int count,
        ShaderStage shaderStage) override;
    void setStageImageViews_(
        const ImageViewPtr* views,
        Int startIndex,
        Int count,
        ShaderStage shaderStage) override;
    void setStageSamplers_(
        const SamplerStatePtr* states,
        Int startIndex,
        Int count,
        ShaderStage shaderStage) override;

    void updateBufferData_(Buffer* buffer, const void* data, Int lengthInBytes) override;

    void generateMips_(const ImageViewPtr& imageView) override;

    void draw_(
        GeometryView* view,
        UInt numIndices,
        UInt numInstances,
        UInt startIndex,
        Int baseVertex) override;
    void clear_(const core::Color& color) override;

    UInt64
    present_(SwapChain* swapChain, UInt32 syncInterval, PresentFlags flags) override;

    void updateViewportAndScissorRect_(GLsizei rtHeight);

private:
    // XXX keep only format of first chain and compare against new windows ?
    QSurfaceFormat format_;
    QOpenGLContext* ctx_ = nullptr;
    QOffscreenSurface* offscreenSurface_;
    bool isExternalCtx_ = false;
    OpenGLFunctions* api_ = nullptr;
    QSurface* surface_ = nullptr;

    // state tracking
    SwapChainPtr boundSwapChain_;
    GLsizei rtHeight_ = 0;
    GLsizei scHeight_ = 0;
    GLuint boundFramebuffer_ = 0;
    struct GLRect {
        GLint x;
        GLint y;
        GLsizei w;
        GLsizei h;
    };
    GLRect viewportRect_;
    GLRect scissorRect_;
    BlendStatePtr boundBlendState_;
    std::optional<geometry::Vec4f> currentBlendConstantFactors_;
    RasterizerStatePtr boundRasterizerState_;
    ProgramPtr boundProgram_;
    GLenum lastIndexFormat_ = GL_NONE;

    static constexpr UInt32 numTextureUnits = maxSamplersPerStage * numShaderStages;
    static_assert(maxSamplersPerStage == maxImageViewsPerStage);
    std::array<ImageViewPtr, numTextureUnits> currentImageViews_;
    std::array<SamplerStatePtr, numTextureUnits> currentSamplerStates_;

    // helpers

    template<typename T, typename... Args>
    [[nodiscard]] std::unique_ptr<T> makeUnique(Args&&... args) {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }

    void makeCurrent_();
    bool loadBuffer_(class QglBuffer* buffer, const void* data, Int dataSize);

    bool hasAnisotropicFilteringSupport_ = false;
    float maxTextureMaxAnisotropy = 0;

    void setEnabled_(GLenum capability, bool isEnabled) {
        if (isEnabled) {
            api_->glEnable(capability);
        }
        else {
            api_->glDisable(capability);
        }
    }
};

} // namespace qopengl

using QglEngine = qopengl::QglEngine;
using QglEnginePtr = qopengl::QglEnginePtr;

} // namespace vgc::ui::detail

#endif // VGC_UI_QOPENGLENGINE_H
