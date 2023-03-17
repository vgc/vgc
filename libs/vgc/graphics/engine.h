// Copyright 2021 The VGC Developers
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

#ifndef VGC_GRAPHICS_ENGINE_H
#define VGC_GRAPHICS_ENGINE_H

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

#include <vgc/core/assert.h>
#include <vgc/core/color.h>
#include <vgc/core/flags.h>
#include <vgc/core/innercore.h>
#include <vgc/core/span.h>
#include <vgc/core/templateutil.h>
#include <vgc/geometry/mat4f.h>
#include <vgc/geometry/rect2f.h>
#include <vgc/geometry/vec4f.h>
#include <vgc/graphics/api.h>
#include <vgc/graphics/batch.h>
#include <vgc/graphics/blendstate.h>
#include <vgc/graphics/buffer.h>
#include <vgc/graphics/constants.h>
#include <vgc/graphics/detail/command.h>
#include <vgc/graphics/detail/pipelinestate.h>
#include <vgc/graphics/enums.h>
#include <vgc/graphics/font.h>
#include <vgc/graphics/framebuffer.h>
#include <vgc/graphics/geometryview.h>
#include <vgc/graphics/image.h>
#include <vgc/graphics/imageview.h>
#include <vgc/graphics/logcategories.h>
#include <vgc/graphics/program.h>
#include <vgc/graphics/rasterizerstate.h>
#include <vgc/graphics/resource.h>
#include <vgc/graphics/samplerstate.h>
#include <vgc/graphics/swapchain.h>

namespace vgc::graphics {

VGC_DECLARE_OBJECT(Engine);

namespace detail {

struct BuiltinConstants {
    geometry::Mat4f projMatrix;
    geometry::Mat4f viewMatrix;
    geometry::Vec4f viewport;
    UInt32 frameStartTimeInMs = 0;
};

} // namespace detail

class VGC_GRAPHICS_API WindowSwapChainFormat {
public:
    using FlagsType = UInt64;

    PixelFormat pixelFormat() const {
        return pixelFormat_;
    }

    void setPixelFormat(WindowPixelFormat pixelFormat) {
        pixelFormat_ = windowPixelFormatToPixelFormat(pixelFormat);
    }

    Int numSamples() const {
        return numSamples_;
    }

    void setNumSamples(Int numSamples) {
        numSamples_ = numSamples;
    }

    Int numBuffers() const {
        return numBuffers_;
    }

    void setNumBuffers(Int numBuffers) {
        numBuffers_ = numBuffers;
    }

    FlagsType flags() const {
        return flags_;
    }

    void setFlags(FlagsType flags) {
        flags_ = flags;
    }

private:
    PixelFormat pixelFormat_ = PixelFormat::RGBA_8_UNORM_SRGB;
    Int numSamples_ = 1;
    Int numBuffers_ = 2;
    FlagsType flags_ = 0;
};

class VGC_GRAPHICS_API EngineCreateInfo {
public:
    const WindowSwapChainFormat& windowSwapChainFormat() const {
        return windowSwapChainFormat_;
    }

    WindowSwapChainFormat& windowSwapChainFormat() {
        return windowSwapChainFormat_;
    }

    bool isMultithreadingEnabled() const {
        return isMultithreadingEnabled_;
    }

    void setMultithreadingEnabled(bool enabled) {
        isMultithreadingEnabled_ = enabled;
    }

private:
    WindowSwapChainFormat windowSwapChainFormat_ = {};
    bool isMultithreadingEnabled_ = false;
};

inline Int calculateMaxMipLevels(Int width, Int height) {
    return static_cast<Int>(std::floor(std::log2((std::max)(width, height)))) + 1;
}

// XXX add something to limit the number of pending frames for each swapchain..

/// \class vgc::graphics::Engine
/// \brief Abstract interface for graphics rendering.
///
/// This class is an abstract base class defining a common shared API for
/// graphics rendering. Implementations of this abstraction may provide
/// backends such as OpenGL, Vulkan, Direct3D, Metal, or software rendering.
///
/// The graphics engine is responsible for managing two matrix stacks: the
/// projection matrix stack, and the view matrix stack. When the engine is
/// constructed, each of these stacks is initialized with the identity matrix
/// as the only matrix in the stack. It is undefined behavior for clients to
/// call "pop" more times than "push": some implementations may emit an
/// exception (instantly or later), others may cause a crash (instantly or
/// later), or the drawing operations may fail.
///
class VGC_GRAPHICS_API Engine : public core::Object {
protected:
    VGC_OBJECT(Engine, core::Object)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

    using Command = detail::Command;
    using CommandUPtr = std::unique_ptr<Command>;

protected:
    /// Constructs an Engine. This constructor is an implementation detail only
    /// available to derived classes.
    ///
    Engine(const EngineCreateInfo& createInfo);

    void onDestroyed() override;

public:
    const WindowSwapChainFormat& windowSwapChainFormat() const {
        return createInfo_.windowSwapChainFormat();
    }

    bool isMultithreadingEnabled() const {
        return createInfo_.isMultithreadingEnabled();
    }

    // !! public methods should be called on user thread !!

    /// Creates a swap chain for the given window.
    ///
    SwapChainPtr createSwapChain(const SwapChainCreateInfo& desc);

    FramebufferPtr createFramebuffer(const ImageViewPtr& colorImageView);

    BufferPtr createBuffer(const BufferCreateInfo& createInfo, Int initialLengthInBytes);

    template<typename T>
    BufferPtr
    createBuffer(const BufferCreateInfo& createInfo, core::Array<T> initialData);

    BufferPtr createVertexBuffer(Int initialLengthInBytes);

    template<typename T>
    BufferPtr createVertexBuffer(core::Array<T> initialData, bool isDynamic);

    BufferPtr createIndexBuffer(IndexFormat indexFormat, Int initialIndexCount);

    template<IndexFormat Format>
    BufferPtr
    createIndexBuffer(core::Array<IndexFormatType<Format>> initialData, bool isDynamic);

    GeometryViewPtr createDynamicGeometryView(
        PrimitiveType primitiveType,
        BuiltinGeometryLayout vertexLayout,
        IndexFormat indexFormat = IndexFormat::None);

    GeometryViewPtr createDynamicTriangleListView(
        BuiltinGeometryLayout vertexLayout,
        IndexFormat indexFormat = IndexFormat::None);
    GeometryViewPtr createDynamicTriangleStripView(
        BuiltinGeometryLayout vertexLayout,
        IndexFormat indexFormat = IndexFormat::None);

    ImagePtr createImage(const ImageCreateInfo& createInfo);

    ImagePtr
    createImage(const ImageCreateInfo& createInfo, core::Array<char> initialData);

    ImageViewPtr
    createImageView(const ImageViewCreateInfo& createInfo, const ImagePtr& image);

    ImageViewPtr createImageView(
        const ImageViewCreateInfo& createInfo,
        const BufferPtr& buffer,
        PixelFormat format,
        Int numElements);

    void generateMips(const ImageViewPtr& imageView);

    SamplerStatePtr createSamplerState(const SamplerStateCreateInfo& createInfo);

    GeometryViewPtr createGeometryView(const GeometryViewCreateInfo& createInfo);

    BlendStatePtr createBlendState(const BlendStateCreateInfo& createInfo);

    RasterizerStatePtr createRasterizerState(const RasterizerStateCreateInfo& createInfo);

    /// Sets the framebuffer to be drawn to.
    ///
    void setFramebuffer(const FramebufferPtr& framebuffer = nullptr);
    void pushFramebuffer(const FramebufferPtr& framebuffer = nullptr);
    void popFramebuffer();

    void setViewport(Int x, Int y, Int width, Int height);
    void setViewport(const Viewport& viewport);
    void pushViewport(const Viewport& viewport);
    void popViewport();

    /// This sets the current program.
    ///
    void setProgram(BuiltinProgram builtinProgram);

    /// This pushes and sets the current program.
    ///
    void pushProgram(BuiltinProgram builtinProgram);

    void setProgram(const ProgramPtr& program);
    void pushProgram(const ProgramPtr& program);
    void popProgram();

    void setBlendState(
        const BlendStatePtr& state, //
        const geometry::Vec4f& constantFactors);
    void pushBlendState(
        const BlendStatePtr& state, //
        const geometry::Vec4f& constantFactors);
    void popBlendState();

    void setRasterizerState(const RasterizerStatePtr& state);
    void pushRasterizerState(const RasterizerStatePtr& state);
    void popRasterizerState();

    const geometry::Rect2f& scissorRect() const {
        return scissorRectStack_.top();
    }

    void setScissorRect(const geometry::Rect2f& rect);
    void pushScissorRect(const geometry::Rect2f& rect);
    void popScissorRect();

    void setStageConstantBuffers(
        const BufferPtr* buffers,
        Int startIndex,
        Int count,
        ShaderStage shaderStage);

    void setStageImageViews(
        const ImageViewPtr* views,
        Int startIndex,
        Int count,
        ShaderStage shaderStage);

    void setStageSamplers(
        const SamplerStatePtr* states,
        Int startIndex,
        Int count,
        ShaderStage shaderStage);

    /// Returns the current projection matrix (top-most on the stack).
    ///
    const geometry::Mat4f& projectionMatrix() const;

    /// Assigns `m` to the top-most matrix of the projection matrix stack.
    /// `m` becomes the current projection matrix.
    ///
    void setProjectionMatrix(const geometry::Mat4f& projectionMatrix);

    /// Duplicates the top-most matrix on the projection matrix stack.
    ///
    void pushProjectionMatrix();

    /// Appends `m` as the top-most matrix of the projection matrix stack.
    /// `m` becomes the current projection matrix.
    ///
    void pushProjectionMatrix(const geometry::Mat4f& projectionMatrix);

    /// Removes the top-most matrix of the projection matrix stack.
    /// The new top-most matrix becomes the current projection matrix.
    ///
    /// The behavior is undefined if there is only one matrix in the stack
    /// before calling this function.
    ///
    void popProjectionMatrix();

    /// Returns the current view matrix (top-most on the stack).
    ///
    const geometry::Mat4f& viewMatrix() const;

    /// Assigns `m` to the top-most matrix of the view matrix stack.
    /// `m` becomes the current view matrix.
    ///
    void setViewMatrix(const geometry::Mat4f& viewMatrix);

    /// Duplicates the top-most matrix on the view matrix stack.
    ///
    void pushViewMatrix();

    /// Appends `m` as the top-most matrix of the view matrix stack.
    /// `m` becomes the current view matrix.
    ///
    void pushViewMatrix(const geometry::Mat4f& viewMatrix);

    /// Removes the top-most matrix of the view matrix stack.
    /// The new top-most matrix becomes the current view matrix.
    ///
    /// The behavior is undefined if there is only one matrix in the stack
    /// before calling this function.
    ///
    void popViewMatrix();

    void pushPipelineParameters(PipelineParameters parameters);

    void popPipelineParameters(PipelineParameters parameters);

    /// Returns the default framebuffer.
    /// If a swapchain is set, it is a framebuffer that uses its backbuffer as color target.
    ///
    FramebufferPtr defaultFramebuffer();

    void setDefaultFramebuffer();

    /// Returns the current swapchain.
    ///
    SwapChainPtr swapChain();

    void setPresentCallback(std::function<void(UInt64 /*timestamp*/)>&& presentCallback);

    bool beginFrame(const SwapChainPtr& swapChain, FrameKind kind = FrameKind::Window);

    /// presentedCallback is called from an unspecified thread.
    ///
    void endFrame(Int syncInterval = 0, PresentFlags flags = PresentFlag::None);

    void onWindowResize(const SwapChainPtr& swapChain, Int width, Int height);

    template<typename T>
    void updateBufferData(const BufferPtr& buffer, core::Array<T> data);

    template<typename T>
    void updateVertexBufferData(const GeometryViewPtr& geometry, core::Array<T> data);

    void draw(
        const GeometryViewPtr& geometry,
        Int numIndices = -1,
        Int startIndex = 0,
        Int baseVertex = 0);

    void drawInstanced(
        const GeometryViewPtr& geometry,
        Int numIndices = -1,
        Int numInstances = -1,
        Int startIndex = 0,
        Int baseVertex = 0);

    //void createTextAtlasResource();

    //void drawText(const TextAtlasResource& gaText);

    /// Clears the whole render area with the given color.
    ///
    void clear(const core::Color& color);

    /// Submits the current command list for execution by the render thread.
    /// Returns the index assigned to the submitted command list.
    ///
    /// If the current command list is empty, `flush()` does nothing and
    /// returns the index of the previous list.
    ///
    Int flush();

    /// Submits the current command list if present then waits for all submitted
    /// command lists to finish being translated to GPU commands.
    ///
    void flushWait();

    std::chrono::steady_clock::time_point engineStartTime() const {
        return engineStartTime_;
    }

protected:
    using StageConstantBufferArray = std::array<BufferPtr, maxConstantBuffersPerStage>;
    using StageImageViewArray = std::array<ImageViewPtr, maxImageViewsPerStage>;
    using StageSamplerStateArray = std::array<SamplerStatePtr, maxSamplersPerStage>;

    // -- USER THREAD implementation functions --

    virtual void createBuiltinShaders_() = 0;

    virtual SwapChainPtr constructSwapChain_(const SwapChainCreateInfo& createInfo) = 0;
    virtual FramebufferPtr constructFramebuffer_(const ImageViewPtr& colorImageView) = 0;
    virtual BufferPtr constructBuffer_(const BufferCreateInfo& createInfo) = 0;
    virtual ImagePtr constructImage_(const ImageCreateInfo& createInfo) = 0;
    virtual ImageViewPtr
    constructImageView_(const ImageViewCreateInfo& createInfo, const ImagePtr& image) = 0;
    virtual ImageViewPtr constructImageView_(
        const ImageViewCreateInfo& createInfo,
        const BufferPtr& buffer,
        PixelFormat format,
        UInt32 numElements) = 0;
    virtual SamplerStatePtr
    constructSamplerState_(const SamplerStateCreateInfo& createInfo) = 0;
    virtual GeometryViewPtr
    constructGeometryView_(const GeometryViewCreateInfo& createInfo) = 0;
    virtual BlendStatePtr
    constructBlendState_(const BlendStateCreateInfo& createInfo) = 0;
    virtual RasterizerStatePtr
    constructRasterizerState_(const RasterizerStateCreateInfo& createInfo) = 0;

    virtual void onWindowResize_(SwapChain* swapChain, UInt32 width, UInt32 height) = 0;

    virtual void preBeginFrame_(SwapChain* swapChain, FrameKind kind);

    virtual bool shouldPresentWaitFromSyncedUserThread_() {
        return false;
    }

    // -- RENDER THREAD implementation functions --

    virtual void initContext_() = 0;
    virtual void initBuiltinResources_() = 0;

    virtual void initFramebuffer_(Framebuffer* framebuffer) = 0;
    virtual void initBuffer_(Buffer* buffer, const char* data, Int lengthInBytes) = 0;
    virtual void initImage_(
        Image* image,
        const core::Span<const char>* mipLevelDataSpans,
        Int count) = 0;
    virtual void initImageView_(ImageView* view) = 0;
    virtual void initSamplerState_(SamplerState* state) = 0;
    virtual void initGeometryView_(GeometryView* view) = 0;
    virtual void initBlendState_(BlendState* state) = 0;
    virtual void initRasterizerState_(RasterizerState* state) = 0;

    virtual void setSwapChain_(const SwapChainPtr& swapChain) = 0;
    virtual void setFramebuffer_(const FramebufferPtr& framebuffer) = 0;
    virtual void setViewport_(Int x, Int y, Int width, Int height) = 0;
    virtual void setProgram_(const ProgramPtr& program) = 0;
    virtual void setBlendState_(
        const BlendStatePtr& state,
        const geometry::Vec4f& constantFactors) = 0;
    virtual void setRasterizerState_(const RasterizerStatePtr& state) = 0;
    virtual void setScissorRect_(const geometry::Rect2f& rect) = 0;
    virtual void setStageConstantBuffers_(
        const BufferPtr* buffers,
        Int startIndex,
        Int count,
        ShaderStage shaderStage) = 0;
    virtual void setStageImageViews_(
        const ImageViewPtr* views,
        Int startIndex,
        Int count,
        ShaderStage shaderStage) = 0;
    virtual void setStageSamplers_(
        const SamplerStatePtr* states,
        Int startIndex,
        Int count,
        ShaderStage shaderStage) = 0;

    // XXX virtual void setSwapChainDefaultFramebuffer_(const SwapChainPtr& swapChain, const FramebufferPtr& framebuffer) = 0;

    virtual void
    updateBufferData_(Buffer* buffer, const void* data, Int lengthInBytes) = 0;

    virtual void generateMips_(const ImageViewPtr& imageView) = 0;

    virtual void draw_(
        GeometryView* view,
        UInt numIndices,
        UInt numInstances,
        UInt startIndex,
        Int baseVertex) = 0;

    virtual void clear_(const core::Color& color) = 0;

    virtual UInt64
    present_(SwapChain* swapChain, UInt32 syncInterval, PresentFlags flags) = 0;

protected:
    detail::ResourceRegistry* resourceRegistry_ = nullptr;

    void init_();

    // -- builtins --

    // The builtin programs are created by api-specific engine implementations.
    ProgramPtr simpleProgram_;
    ProgramPtr simpleTexturedProgram_;
    ProgramPtr simpleTexturedDebugProgram_;
    ProgramPtr sreenSpaceDisplacementProgram_;

    // -- builtin batching early impl --

    BufferPtr colorGradientsBuffer_; // 1D buffer
    ImageViewPtr colorGradientsBufferImageView_;

    ProgramPtr glyphAtlasProgram_;
    BufferPtr glyphAtlasBuffer_; // 1D layered
    ImageViewPtr glyphAtlasBufferImageView_;
    BufferPtr textBatch_;
    struct GlyphAtlasGlyphInfo {
        unsigned int texelIdx;
        unsigned int width;
        unsigned int height;
    };
    std::unordered_map<SizedGlyph*, GlyphAtlasGlyphInfo> allocatedGlyphs_;

    ProgramPtr iconAtlasProgram_;
    ImagePtr iconAtlasImage_; // 2D
    ImageViewPtr iconAtlasImageView_;
    ProgramPtr roundedRectangleProgram_;

    // -- QUEUING --

    // XXX future possible optimization: don't allocate commands but serialize them in a single storage.

    // cannot be flushed in out-of-order chunks unless userGarbagedResources is only sent with the last
    std::list<CommandUPtr> pendingCommands_;

    template<typename TCommand, typename... Args>
    void queueCommand_(Args&&... args) {
        if (!isMultithreadingEnabled()) {
            TCommand(std::forward<Args>(args)...).execute(this);
            return;
        }
        pendingCommands_.emplace_back(new TCommand(std::forward<Args>(args)...));
    }

    template<typename Lambda>
    void queueLambdaCommand_(std::string_view name, Lambda&& lambda) {
        if (!isMultithreadingEnabled()) {
            lambda(this);
            return;
        }
        pendingCommands_.emplace_back(
            // our deduction guide doesn't work on gcc 7.5.0
            // new detail::LambdaCommand(name, std::forward<Lambda>(lambda)));
            new detail::LambdaCommand<std::decay_t<Lambda>>(
                name, std::forward<Lambda>(lambda)));
    }

    template<typename Data, typename Lambda, typename... Args>
    void queueLambdaCommandWithParameters_(
        std::string_view name,
        Lambda&& lambda,
        Args&&... args) {
        if (!isMultithreadingEnabled()) {
            lambda(this, Data{std::forward<Args>(args)...});
            return;
        }
        pendingCommands_.emplace_back(
            new detail::LambdaCommandWithParameters<Data, std::decay_t<Lambda>>(
                name, std::forward<Lambda>(lambda), std::forward<Args>(args)...));
    }

    static constexpr size_t toIndex_(ShaderStage stage) {
        return static_cast<size_t>(core::toUnderlying(stage));
    }

    static size_t toIndexSafe_(ShaderStage stage) {
        Int i = core::toUnderlying(stage);
        if (i < 0 || i >= numShaderStages) {
            throw core::LogicError("Engine: invalid ShaderStage enum value.");
        }
        return static_cast<size_t>(i);
    }

    static constexpr PipelineParameter stageConstantBuffersParameter_(ShaderStage stage) {
        return std::array{
            PipelineParameter::VertexShaderConstantBuffers,
            PipelineParameter::GeometryShaderConstantBuffers,
            PipelineParameter::PixelShaderConstantBuffers}[toIndex_(stage)];
    }

    static constexpr PipelineParameter stageImageViewsParameter_(ShaderStage stage) {
        return std::array{
            PipelineParameter::VertexShaderImageViews,
            PipelineParameter::GeometryShaderImageViews,
            PipelineParameter::PixelShaderImageViews}[toIndex_(stage)];
    }

    static constexpr PipelineParameter stageSamplersParameter_(ShaderStage stage) {
        return std::array{
            PipelineParameter::VertexShaderSamplers,
            PipelineParameter::GeometryShaderSamplers,
            PipelineParameter::PixelShaderSamplers}[toIndex_(stage)];
    }

private:
    void createBuiltinResources_();

    ProgramPtr builtinProgram(BuiltinProgram builtinProgram);

    EngineCreateInfo createInfo_;

    // -- pipeline state on the user thread --

    template<typename T>
    class Stack : public core::Array<T> {
    private:
        using Container = core::Array<T>;

    public:
        using Container::Container;

        void pushTop() {
            // XXX add specific warning if empty
            Container::emplaceLast(Container::last());
        }

        void push(const T& value) {
            Container::emplaceLast(value);
        }

        void push(T&& value) {
            Container::emplaceLast(std::move(value));
        }

        void pop() {
            // XXX add specific warning if empty
            Container::removeLast();
        }

        const T& top() const {
            // XXX add specific warning if empty
            return Container::last();
        }

        T& top() {
            // XXX add specific warning if empty
            return Container::last();
        }
    };

    SwapChainPtr swapChain_;

    struct BlendStateAndConstant {
        BlendStateAndConstant() = default;
        BlendStateAndConstant(
            const BlendStatePtr& statePtr,
            const geometry::Vec4f& constantFactors)
            : statePtr(statePtr)
            , constantFactors(constantFactors) {
        }

        BlendStatePtr statePtr = {};
        geometry::Vec4f constantFactors = {};
    };

    Stack<FramebufferPtr> framebufferStack_;
    Stack<Viewport> viewportStack_;
    Stack<ProgramPtr> programStack_;
    Stack<BlendStateAndConstant> blendStateStack_;
    Stack<RasterizerStatePtr> rasterizerStateStack_;
    Stack<geometry::Rect2f> scissorRectStack_;

    // clang-format on

    using StageConstantBufferArrayStack = Stack<StageConstantBufferArray>;
    std::array<StageConstantBufferArrayStack, numShaderStages> constantBufferArrayStacks_;
    constexpr StageConstantBufferArrayStack&
    stageConstantBufferArrayStack_(ShaderStage shaderStage) {
        return constantBufferArrayStacks_[toIndex_(shaderStage)];
    }

    using StageImageViewArrayStack = Stack<StageImageViewArray>;
    std::array<StageImageViewArrayStack, numShaderStages> imageViewArrayStacks_;
    constexpr StageImageViewArrayStack&
    stageImageViewArrayStack_(ShaderStage shaderStage) {
        return imageViewArrayStacks_[toIndex_(shaderStage)];
    }

    using StageSamplerStateArrayStack = Stack<StageSamplerStateArray>;
    std::array<StageSamplerStateArrayStack, numShaderStages> samplerStateArrayStacks_;
    constexpr StageSamplerStateArrayStack&
    stageSamplerStateArrayStack_(ShaderStage shaderStage) {
        return samplerStateArrayStacks_[toIndex_(shaderStage)];
    }

    PipelineParameters dirtyPipelineParameters_ = PipelineParameter::None;

    // called in user thread
    void syncState_();
    void syncStageConstantBuffers_(ShaderStage shaderStage);
    void syncStageImageViews_(ShaderStage shaderStage);
    void syncStageSamplers_(ShaderStage shaderStage);

    // -- builtin constants + dirty bool --

    std::chrono::steady_clock::time_point engineStartTime_;
    std::chrono::steady_clock::time_point frameStartTime_;
    BufferPtr builtinConstantsBuffer_;
    Stack<geometry::Mat4f> projectionMatrixStack_;
    Stack<geometry::Mat4f> viewMatrixStack_;
    bool dirtyBuiltinConstantBuffer_ = false;

    // -- builtin batching early impl --

    //void flushBuiltinBatches_();
    //void prependBuiltinBatchesResourceUpdates_();

    // -- render thread + sync --

    std::thread renderThread_;
    std::mutex mutex_;
    std::condition_variable wakeRenderThreadConditionVariable_;
    std::condition_variable renderThreadEventConditionVariable_;
    Int lastExecutedCommandListId_ = 0;
    Int lastSubmittedCommandListId_ = 0;
    bool isThreadRunning_ = false;
    bool stopRequested_ = false;
    std::function<void(UInt64 /*timestamp*/)> presentCallback_;

    struct CommandList {
        CommandList(std::list<CommandUPtr>&& commands)
            : commands(std::move(commands)) {
        }

        std::list<CommandUPtr> commands;
    };
    core::Array<CommandList> commandQueue_;

    void renderThreadProc_();
    void startRenderThread_();
    void stopRenderThread_(); // blocking

    // -- threads sync --

    UInt submitPendingCommandList_();

    // returns false if translation was cancelled by a stop request.
    void waitCommandListTranslationFinished_(Int commandListId = 0);

    bool hasSubmittedCommandListPendingForTranslation_();

    // -- helpers --

    void sanitize_(SwapChainCreateInfo& createInfo);
    void sanitize_(BufferCreateInfo& createInfo);
    void sanitize_(ImageCreateInfo& createInfo);
    void sanitize_(ImageViewCreateInfo& createInfo);
    void sanitize_(SamplerStateCreateInfo& createInfo);
    void sanitize_(GeometryViewCreateInfo& createInfo);
    void sanitize_(BlendStateCreateInfo& createInfo);
    void sanitize_(RasterizerStateCreateInfo& createInfo);

    // -- checks --

    bool checkResourceIsValid_(Resource* resource) {
        if (!resource) {
            VGC_ERROR(LogVgcGraphics, "Unexpected null resource");
            return false;
        }
        if (!resource->registry_) {
            VGC_ERROR(LogVgcGraphics, "Trying to use a resource from a stopped engine");
            return false;
        }
        if (resource->registry_ != resourceRegistry_) {
            VGC_ERROR(LogVgcGraphics, "Trying to use a resource from an other engine");
            return false;
        }
        return true;
    }

    template<typename U>
    bool checkResourceIsValid_(const ResourcePtr<U>& resource) {
        return checkResourceIsValid_(resource.get());
    }
};

inline const geometry::Mat4f& Engine::projectionMatrix() const {
    return projectionMatrixStack_.top();
}

inline void Engine::setProjectionMatrix(const geometry::Mat4f& projectionMatrix) {
    projectionMatrixStack_.top() = projectionMatrix;
    dirtyBuiltinConstantBuffer_ = true;
}

inline void Engine::pushProjectionMatrix() {
    projectionMatrixStack_.pushTop();
}

inline void Engine::pushProjectionMatrix(const geometry::Mat4f& projectionMatrix) {
    projectionMatrixStack_.push(projectionMatrix);
    dirtyBuiltinConstantBuffer_ = true;
}

inline void Engine::popProjectionMatrix() {
    projectionMatrixStack_.pop();
    dirtyBuiltinConstantBuffer_ = true;
}

inline const geometry::Mat4f& Engine::viewMatrix() const {
    return viewMatrixStack_.top();
}

inline void Engine::setViewMatrix(const geometry::Mat4f& viewMatrix) {
    viewMatrixStack_.top() = viewMatrix;
    dirtyBuiltinConstantBuffer_ = true;
}

inline void Engine::pushViewMatrix() {
    viewMatrixStack_.pushTop();
}

inline void Engine::pushViewMatrix(const geometry::Mat4f& viewMatrix) {
    viewMatrixStack_.push(viewMatrix);
    dirtyBuiltinConstantBuffer_ = true;
}

inline void Engine::popViewMatrix() {
    viewMatrixStack_.pop();
    dirtyBuiltinConstantBuffer_ = true;
}

//inline FramebufferPtr Engine::defaultFramebuffer() {
//    SwapChain* swapChain = swapChain_.get();
//    return swapChain ? swapChain->framebuffer() : FramebufferPtr();
//}

inline void Engine::setDefaultFramebuffer() {
    //setFramebuffer(defaultFramebuffer());
    setFramebuffer(nullptr);
}

inline SwapChainPtr Engine::swapChain() {
    return swapChain_;
}

inline void
Engine::setPresentCallback(std::function<void(UInt64 /*timestamp*/)>&& presentCallback) {
    std::lock_guard<std::mutex> lock(mutex_);
    presentCallback_ = presentCallback;
}

template<typename T>
inline BufferPtr
Engine::createBuffer(const BufferCreateInfo& createInfo, core::Array<T> initialData) {
    BufferPtr buffer(constructBuffer_(createInfo));
    buffer->lengthInBytes_ = initialData.length() * sizeof(T);

    struct CommandParameters {
        BufferPtr buffer;
        core::Array<T> initialData;
    };
    queueLambdaCommandWithParameters_<CommandParameters>(
        "initBuffer",
        [](Engine* engine, const CommandParameters& p) {
            engine->initBuffer_(
                p.buffer.get(), p.initialData.data(), p.initialData.length() * sizeof(T));
        },
        buffer,
        std::move(initialData));
    return buffer;
}

template<typename T>
inline BufferPtr Engine::createVertexBuffer(core::Array<T> initialData, bool isDynamic) {
    BufferCreateInfo createInfo = BufferCreateInfo(BindFlag::VertexBuffer, isDynamic);
    return createBuffer(createInfo, std::move(initialData));
}

template<IndexFormat Format>
inline BufferPtr Engine::createIndexBuffer(
    core::Array<IndexFormatType<Format>> initialData,
    bool isDynamic) {

    BufferCreateInfo createInfo = BufferCreateInfo(BindFlag::IndexBuffer, isDynamic);
    return createBuffer(createInfo, std::move(initialData));
}

template<typename T>
inline void Engine::updateBufferData(const BufferPtr& buffer, core::Array<T> data) {
    Buffer* buf = buffer.get();
    if (!checkResourceIsValid_(buf)) {
        return;
    }

    if (!(buf->cpuAccessFlags() & CpuAccessFlag::Write)) {
        VGC_ERROR(LogVgcGraphics, "Cpu does not have write access on buffer.");
    }

    buf->lengthInBytes_ = data.length() * sizeof(T);

    struct CommandParameters {
        BufferPtr buffer;
        core::Array<T> data;
    };
    queueLambdaCommandWithParameters_<CommandParameters>(
        "updateBufferData",
        [](Engine* engine, const CommandParameters& p) {
            engine->updateBufferData_(
                p.buffer.get(), p.data.data(), p.data.length() * sizeof(T));
        },
        buffer,
        std::move(data));
}

template<typename T>
inline void
Engine::updateVertexBufferData(const GeometryViewPtr& geometry, core::Array<T> data) {
    GeometryView* geom = geometry.get();
    if (!checkResourceIsValid_(geom)) {
        return;
    }
    updateBufferData(geom->vertexBuffer(0), std::move(data));
}

inline Int Engine::flush() {
    if (isMultithreadingEnabled()) {
        return static_cast<Int>(submitPendingCommandList_());
    }
    return 0;
}

inline void Engine::flushWait() {
    if (isMultithreadingEnabled()) {
        UInt id = submitPendingCommandList_();
        waitCommandListTranslationFinished_(id);
    }
}

} // namespace vgc::graphics

#endif // VGC_GRAPHICS_ENGINE_H
