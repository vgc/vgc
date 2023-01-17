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

#include <vgc/graphics/engine.h>

#include <tuple> // std::tuple_size

namespace vgc::graphics {

Engine::Engine(const EngineCreateInfo& createInfo)
    : Object()
    , resourceRegistry_(new detail::ResourceRegistry())
    , createInfo_(createInfo) {

    framebufferStack_.emplaceLast();
    viewportStack_.emplaceLast(0, 0, 0, 0);
    programStack_.emplaceLast();
    blendStateStack_.emplaceLast();
    rasterizerStateStack_.emplaceLast();
    scissorRectStack_.emplaceLast();

    for (Int i = 0; i < numShaderStages; ++i) {
        constantBufferArrayStacks_[i].emplaceLast();
        imageViewArrayStacks_[i].emplaceLast();
        samplerStateArrayStacks_[i].emplaceLast();
    }

    projectionMatrixStack_.emplaceLast(geometry::Mat4f::identity);
    viewMatrixStack_.emplaceLast(geometry::Mat4f::identity);
}

void Engine::onDestroyed() {

    swapChain_.reset();

    framebufferStack_.clear();
    viewportStack_.clear();
    programStack_.clear();
    blendStateStack_.clear();
    rasterizerStateStack_.clear();
    scissorRectStack_.clear();

    for (Int i = 0; i < numShaderStages; ++i) {
        constantBufferArrayStacks_[i].clear();
        imageViewArrayStacks_[i].clear();
        samplerStateArrayStacks_[i].clear();
    }

    projectionMatrixStack_.clear();
    viewMatrixStack_.clear();

    colorGradientsBuffer_.reset(); // 1D buffer
    colorGradientsBufferImageView_.reset();

    glyphAtlasProgram_.reset();
    glyphAtlasBuffer_.reset();
    glyphAtlasBufferImageView_.reset();

    iconAtlasProgram_.reset();
    iconAtlasImage_.reset();
    iconAtlasImageView_.reset();

    roundedRectangleProgram_.reset();

    if (isMultithreadingEnabled()) {
        stopRenderThread_();
    }
    else {
        resourceRegistry_->releaseAllResources(this);
    }
}

SwapChainPtr Engine::createSwapChain(const SwapChainCreateInfo& createInfo) {

    // sanitize create info
    SwapChainCreateInfo sanitizedCreateInfo = createInfo;
    sanitize_(sanitizedCreateInfo);

    return constructSwapChain_(sanitizedCreateInfo);
}

FramebufferPtr Engine::createFramebuffer(const ImageViewPtr& colorImageView) {

    // XXX should check bind flags compatibility here

    FramebufferPtr framebuffer = constructFramebuffer_(colorImageView);
    queueLambdaCommandWithParameters_<Framebuffer*>(
        "initFramebuffer",
        [](Engine* engine, Framebuffer* p) { engine->initFramebuffer_(p); },
        framebuffer.get());
    return framebuffer;
}

BufferPtr
Engine::createBuffer(const BufferCreateInfo& createInfo, Int initialLengthInBytes) {

    if (initialLengthInBytes < 0) {
        throw core::NegativeIntegerError(core::format(
            "Negative initialLengthInBytes ({}) provided to Engine::createBuffer().",
            initialLengthInBytes));
    }

    // sanitize create info
    BufferCreateInfo sanitizedCreateInfo = createInfo;
    sanitize_(sanitizedCreateInfo);

    BufferPtr buffer = constructBuffer_(sanitizedCreateInfo);
    buffer->lengthInBytes_ = initialLengthInBytes;

    struct CommandParameters {
        Buffer* buffer;
        Int initialLengthInBytes;
    };
    queueLambdaCommandWithParameters_<CommandParameters>(
        "initBufferZeroed",
        [](Engine* engine, const CommandParameters& p) {
            engine->initBuffer_(p.buffer, nullptr, p.initialLengthInBytes);
        },
        buffer.get(),
        initialLengthInBytes);
    return buffer;
}

BufferPtr Engine::createVertexBuffer(Int initialLengthInBytes) {

    if (initialLengthInBytes < 0) {
        throw core::NegativeIntegerError(core::format(
            "Negative initialLengthInBytes ({}) provided to "
            "Engine::createVertexBuffer().",
            initialLengthInBytes));
    }

    BufferCreateInfo createInfo = BufferCreateInfo(BindFlag::VertexBuffer, true);
    return createBuffer(createInfo, initialLengthInBytes);
}

BufferPtr Engine::createIndexBuffer(IndexFormat indexFormat, Int initialIndexCount) {
    if (initialIndexCount < 0) {
        throw core::NegativeIntegerError(core::format(
            "Negative initialIndexCount ({}) provided to Engine::createIndexBuffer().",
            initialIndexCount));
    }

    size_t indexSize = 0;
    switch (indexFormat) {
    case IndexFormat::UInt16:
        indexSize = sizeof(uint16_t);
        break;
    case IndexFormat::UInt32:
        indexSize = sizeof(uint32_t);
        break;
    default:
        throw core::LogicError(
            "Engine::createIndexBuffer(): invalid IndexFormat enum value.");
    }

    BufferCreateInfo createInfo = BufferCreateInfo(BindFlag::IndexBuffer, true);
    return createBuffer(createInfo, initialIndexCount * indexSize);
}

GeometryViewPtr Engine::createDynamicGeometryView(
    PrimitiveType primitiveType,
    BuiltinGeometryLayout vertexLayout,
    IndexFormat indexFormat) {

    BufferPtr vertexBuffer = createVertexBuffer(0);
    GeometryViewCreateInfo createInfo = {};
    createInfo.setBuiltinGeometryLayout(vertexLayout);
    createInfo.setPrimitiveType(primitiveType);
    createInfo.setVertexBuffer(0, vertexBuffer);
    if (vertexLayout >= BuiltinGeometryLayout::XY_iRGBA) {
        BufferPtr instanceBuffer = createVertexBuffer(0);
        createInfo.setVertexBuffer(1, instanceBuffer);
    }
    if (indexFormat != IndexFormat::None) {
        BufferPtr indexBuffer = createIndexBuffer(indexFormat, 0);
        createInfo.setIndexBuffer(indexBuffer);
        createInfo.setIndexFormat(indexFormat);
    }
    return createGeometryView(createInfo);
}

GeometryViewPtr Engine::createDynamicTriangleListView(
    BuiltinGeometryLayout vertexLayout,
    IndexFormat indexFormat) {
    return createDynamicGeometryView(
        PrimitiveType::TriangleList, vertexLayout, indexFormat);
}

GeometryViewPtr Engine::createDynamicTriangleStripView(
    BuiltinGeometryLayout vertexLayout,
    IndexFormat indexFormat) {
    return createDynamicGeometryView(
        PrimitiveType::TriangleStrip, vertexLayout, indexFormat);
}

ImagePtr Engine::createImage(const ImageCreateInfo& createInfo) {

    // sanitize create info
    ImageCreateInfo sanitizedCreateInfo = createInfo;
    sanitize_(sanitizedCreateInfo);

    if (createInfo.usage() == Usage::Immutable) {
        VGC_ERROR(
            LogVgcGraphics, "Cannot create an immutable image without initial data.");
        return nullptr;
    }

    // construct
    ImagePtr image(constructImage_(sanitizedCreateInfo));

    // queue init
    struct CommandParameters {
        Image* image;
    };
    queueLambdaCommandWithParameters_<CommandParameters>(
        "initImage",
        [](Engine* engine, const CommandParameters& p) {
            engine->initImage_(p.image, nullptr, 0);
        },
        image.get());
    return image;
}

ImagePtr
Engine::createImage(const ImageCreateInfo& createInfo, core::Array<char> initialData) {

    // sanitize create info
    ImageCreateInfo sanitizedCreateInfo = createInfo;
    sanitize_(sanitizedCreateInfo);

    if (sanitizedCreateInfo.isMultisampled()) {
        VGC_ERROR(
            LogVgcGraphics,
            "Initial data ignored: multisampled image cannot be initialized with data on "
            "creation.");
        return createImage(sanitizedCreateInfo);
    }

    // construct
    ImagePtr image(constructImage_(sanitizedCreateInfo));

    // queue init
    struct CommandParameters {
        Image* image;
        core::Array<char> initialData;
    };
    queueLambdaCommandWithParameters_<CommandParameters>(
        "initImage",
        [](Engine* engine, const CommandParameters& p) {
            Span<const char> m0 = {p.initialData.data(), p.initialData.length()};
            engine->initImage_(p.image, &m0, 1);
        },
        image.get(),
        std::move(initialData));
    return image;
}

ImageViewPtr
Engine::createImageView(const ImageViewCreateInfo& createInfo, const ImagePtr& image) {

    // sanitize create info
    ImageViewCreateInfo sanitizedCreateInfo = createInfo;
    sanitize_(sanitizedCreateInfo);

    ImageViewPtr imageView = constructImageView_(sanitizedCreateInfo, image);
    queueLambdaCommandWithParameters_<ImageView*>(
        "initImageView",
        [](Engine* engine, ImageView* p) { engine->initImageView_(p); },
        imageView.get());
    return imageView;
}

ImageViewPtr Engine::createImageView(
    const ImageViewCreateInfo& createInfo,
    const BufferPtr& buffer,
    PixelFormat format,
    Int numElements) {

    ImageViewCreateInfo sanitizedCreateInfo = createInfo;
    sanitize_(sanitizedCreateInfo);

    ImageViewPtr imageView = constructImageView_(
        sanitizedCreateInfo, buffer, format, core::int_cast<UInt32>(numElements));
    queueLambdaCommandWithParameters_<ImageView*>(
        "initBufferImageView",
        [](Engine* engine, ImageView* p) { engine->initImageView_(p); },
        imageView.get());
    return imageView;
}

void Engine::generateMips(const ImageViewPtr& imageView) {
    if (!imageView) {
        return;
    }
    if (imageView->isBuffer()) {
        BufferPtr buffer = imageView->viewedBuffer();
        if (!buffer->resourceMiscFlags().has(ResourceMiscFlag::GenerateMips)) {
            VGC_WARNING(
                LogVgcGraphics,
                "MIP generation ignored: the given Buffer Resource was not created with "
                "ResourceMiscFlag::GenerateMips.");
            return;
        }
    }
    else {
        ImagePtr image = imageView->viewedImage();
        if (!image->resourceMiscFlags().has(ResourceMiscFlag::GenerateMips)) {
            VGC_WARNING(
                LogVgcGraphics,
                "MIP generation ignored: the given Image Resource was not created with "
                "ResourceMiscFlag::GenerateMips.");
            return;
        }
    }
    queueLambdaCommandWithParameters_<ImageViewPtr>(
        "generateMips",
        [](Engine* engine, const ImageViewPtr& p) { engine->generateMips_(p); },
        imageView);
}

SamplerStatePtr Engine::createSamplerState(const SamplerStateCreateInfo& createInfo) {

    SamplerStateCreateInfo sanitizedCreateInfo = createInfo;
    sanitize_(sanitizedCreateInfo);

    SamplerStatePtr samplerState = constructSamplerState_(sanitizedCreateInfo);
    queueLambdaCommandWithParameters_<SamplerState*>(
        "initSamplerState",
        [](Engine* engine, SamplerState* p) { engine->initSamplerState_(p); },
        samplerState.get());
    return samplerState;
}

GeometryViewPtr Engine::createGeometryView(const GeometryViewCreateInfo& createInfo) {

    GeometryViewCreateInfo sanitizedCreateInfo = createInfo;
    sanitize_(sanitizedCreateInfo);

    GeometryViewPtr geometryView = constructGeometryView_(sanitizedCreateInfo);
    queueLambdaCommandWithParameters_<GeometryView*>(
        "initGeometryView",
        [](Engine* engine, GeometryView* p) { engine->initGeometryView_(p); },
        geometryView.get());
    return geometryView;
}

BlendStatePtr Engine::createBlendState(const BlendStateCreateInfo& createInfo) {

    BlendStateCreateInfo sanitizedCreateInfo = createInfo;
    sanitize_(sanitizedCreateInfo);

    BlendStatePtr blendState = constructBlendState_(sanitizedCreateInfo);
    queueLambdaCommandWithParameters_<BlendState*>(
        "initBlendState",
        [](Engine* engine, BlendState* p) { engine->initBlendState_(p); },
        blendState.get());
    return blendState;
}

RasterizerStatePtr
Engine::createRasterizerState(const RasterizerStateCreateInfo& createInfo) {

    RasterizerStateCreateInfo sanitizedCreateInfo = createInfo;
    sanitize_(sanitizedCreateInfo);

    RasterizerStatePtr rasterizerState = constructRasterizerState_(sanitizedCreateInfo);
    queueLambdaCommandWithParameters_<RasterizerState*>(
        "initRasterizerState",
        [](Engine* engine, RasterizerState* p) { engine->initRasterizerState_(p); },
        rasterizerState.get());
    return rasterizerState;
}

void Engine::setFramebuffer(const FramebufferPtr& framebuffer) {
    if (framebuffer && !checkResourceIsValid_(framebuffer)) {
        return;
    }
    if (framebufferStack_.top() != framebuffer) {
        framebufferStack_.top() = framebuffer;
        dirtyPipelineParameters_ |= PipelineParameter::Framebuffer;
    }
}

void Engine::pushFramebuffer(const FramebufferPtr& framebuffer) {
    if (framebuffer && !checkResourceIsValid_(framebuffer)) {
        return;
    }
    if (framebufferStack_.top() != framebuffer) {
        dirtyPipelineParameters_ |= PipelineParameter::Framebuffer;
    }
    framebufferStack_.push(framebuffer);
}

void Engine::popFramebuffer() {
    FramebufferPtr oldTop = framebufferStack_.last();
    framebufferStack_.pop();
    if (framebufferStack_.top() != oldTop) {
        dirtyPipelineParameters_ |= PipelineParameter::Framebuffer;
    }
}

void Engine::setViewport(Int x, Int y, Int width, Int height) {
    setViewport(Viewport(x, y, width, height));
}

void Engine::setViewport(const Viewport& viewport) {
    viewportStack_.top() = viewport;
    dirtyPipelineParameters_ |= PipelineParameter::Viewport;
}

void Engine::pushViewport(const Viewport& viewport) {
    viewportStack_.push(viewport);
    dirtyPipelineParameters_ |= PipelineParameter::Viewport;
}

void Engine::popViewport() {
    viewportStack_.pop();
    dirtyPipelineParameters_ |= PipelineParameter::Viewport;
}

void Engine::setProgram(BuiltinProgram builtinProgram_) {
    ProgramPtr program = builtinProgram(builtinProgram_);
    setProgram(program);
}

void Engine::pushProgram(BuiltinProgram builtinProgram_) {
    ProgramPtr program = builtinProgram(builtinProgram_);
    pushProgram(program);
}

void Engine::setProgram(const ProgramPtr& program) {
    ProgramPtr oldTop = programStack_.top();
    if (program != oldTop) {
        if (program && (!oldTop || !program->isBuiltin() != oldTop->isBuiltin())) {
            dirtyPipelineParameters_ |= PipelineParameter::VertexShaderConstantBuffers;
        }
        programStack_.top() = program;
        dirtyPipelineParameters_ |= PipelineParameter::Program;
    }
}

void Engine::pushProgram(const ProgramPtr& program) {
    ProgramPtr oldTop = programStack_.top();
    if (program != oldTop) {
        if (program && (!oldTop || !program->isBuiltin() != oldTop->isBuiltin())) {
            dirtyPipelineParameters_ |= PipelineParameter::VertexShaderConstantBuffers;
        }
        dirtyPipelineParameters_ |= PipelineParameter::Program;
    }
    programStack_.push(program);
}

void Engine::popProgram() {
    ProgramPtr oldTop = programStack_.top();
    programStack_.pop();
    ProgramPtr newTop = programStack_.top();
    if (newTop != oldTop) {
        if (newTop && (!oldTop || newTop->isBuiltin() != oldTop->isBuiltin())) {
            dirtyPipelineParameters_ |= PipelineParameter::VertexShaderConstantBuffers;
        }
        dirtyPipelineParameters_ |= PipelineParameter::Program;
    }
}

void Engine::setBlendState(
    const BlendStatePtr& state,
    const geometry::Vec4f& constantFactors) {

    BlendStateAndConstant& top = blendStateStack_.top();
    if (top.statePtr != state) {
        top.statePtr = state;
        dirtyPipelineParameters_ |= PipelineParameter::BlendState;
    }
    if (top.constantFactors != constantFactors) {
        top.constantFactors = constantFactors;
        dirtyPipelineParameters_ |= PipelineParameter::BlendState;
    }
}

void Engine::pushBlendState(
    const BlendStatePtr& state,
    const geometry::Vec4f& constantFactors) {

    const BlendStateAndConstant& oldTop = blendStateStack_.top();
    if ((oldTop.statePtr != state) || (oldTop.constantFactors != constantFactors)) {
        dirtyPipelineParameters_ |= PipelineParameter::BlendState;
    }
    blendStateStack_.emplaceLast(state, constantFactors);
}

void Engine::popBlendState() {
    const BlendStateAndConstant& oldTop = blendStateStack_.top();
    blendStateStack_.pop();
    const BlendStateAndConstant& top = blendStateStack_.top();
    if ((top.statePtr != oldTop.statePtr)
        || (top.constantFactors != oldTop.constantFactors)) {
        dirtyPipelineParameters_ |= PipelineParameter::BlendState;
    }
}

void Engine::setRasterizerState(const RasterizerStatePtr& state) {
    if (rasterizerStateStack_.top() != state) {
        rasterizerStateStack_.top() = state;
        dirtyPipelineParameters_ |= PipelineParameter::RasterizerState;
    }
}

void Engine::pushRasterizerState(const RasterizerStatePtr& state) {
    rasterizerStateStack_.push(state);
    if (rasterizerStateStack_.top() != state) {
        dirtyPipelineParameters_ |= PipelineParameter::RasterizerState;
    }
}

void Engine::popRasterizerState() {
    RasterizerStatePtr oldTop = rasterizerStateStack_.top();
    rasterizerStateStack_.pop();
    if (rasterizerStateStack_.top() != oldTop) {
        dirtyPipelineParameters_ |= PipelineParameter::RasterizerState;
    }
}

void Engine::setScissorRect(const geometry::Rect2f& rect) {
    scissorRectStack_.top() = rect;
    dirtyPipelineParameters_ |= PipelineParameter::ScissorRect;
}

void Engine::pushScissorRect(const geometry::Rect2f& rect) {
    scissorRectStack_.push(rect);
    dirtyPipelineParameters_ |= PipelineParameter::ScissorRect;
}

void Engine::popScissorRect() {
    scissorRectStack_.pop();
    dirtyPipelineParameters_ |= PipelineParameter::ScissorRect;
}

void Engine::setStageConstantBuffers(
    const BufferPtr* buffers,
    Int startIndex,
    Int count,
    ShaderStage shaderStage) {

    if (shaderStage == ShaderStage::None) {
        return;
    }

    size_t stageIndex = toIndexSafe_(shaderStage);
    StageConstantBufferArray& constantBufferArray =
        constantBufferArrayStacks_[stageIndex].emplaceLast();
    for (Int i = 0; i < count; ++i) {
        constantBufferArray[startIndex + i] = buffers[i];
    }
    dirtyPipelineParameters_ |= stageConstantBuffersParameter_(shaderStage);
}

void Engine::setStageImageViews(
    const ImageViewPtr* views,
    Int startIndex,
    Int count,
    ShaderStage shaderStage) {

    if (shaderStage == ShaderStage::None) {
        return;
    }

    for (Int i = 0; i < count; ++i) {
        if (!views[i]->bindFlags().has(ImageBindFlag::ShaderResource)) {
            VGC_ERROR(
                LogVgcGraphics,
                "All views given to setStageImageViews() should have the flag "
                "ImageBindFlag::ShaderResource set.");
            return;
        }
    }

    size_t stageIndex = toIndexSafe_(shaderStage);
    StageImageViewArray& imageViewArray = imageViewArrayStacks_[stageIndex].emplaceLast();
    for (Int i = 0; i < count; ++i) {
        imageViewArray[startIndex + i] = views[i];
    }
    dirtyPipelineParameters_ |= stageImageViewsParameter_(shaderStage);
}

void Engine::setStageSamplers(
    const SamplerStatePtr* states,
    Int startIndex,
    Int count,
    ShaderStage shaderStage) {

    if (shaderStage == ShaderStage::None) {
        return;
    }

    size_t stageIndex = toIndexSafe_(shaderStage);
    StageSamplerStateArray& samplerStateArray =
        samplerStateArrayStacks_[stageIndex].emplaceLast();
    for (Int i = 0; i < count; ++i) {
        samplerStateArray[startIndex + i] = states[i];
    }
    dirtyPipelineParameters_ |= stageSamplersParameter_(shaderStage);
}

void Engine::pushPipelineParameters(PipelineParameters parameters) {
    if (parameters == PipelineParameter::None) {
        return;
    }

    if (parameters & PipelineParameter::Framebuffer) {
        framebufferStack_.pushTop();
    }
    if (parameters & PipelineParameter::Viewport) {
        viewportStack_.pushTop();
    }
    if (parameters & PipelineParameter::Program) {
        programStack_.pushTop();
    }
    if (parameters & PipelineParameter::BlendState) {
        blendStateStack_.pushTop();
    }
    if (parameters & PipelineParameter::DepthStencilState) {
        // todo
    }
    if (parameters & PipelineParameter::RasterizerState) {
        rasterizerStateStack_.pushTop();
    }
    if (parameters & PipelineParameter::ScissorRect) {
        scissorRectStack_.pushTop();
    }
    if (parameters & PipelineParameter::AllShadersResources) {
        if (parameters & PipelineParameter::VertexShaderConstantBuffers) {
            stageConstantBufferArrayStack_(ShaderStage::Vertex).pushTop();
        }
        if (parameters & PipelineParameter::VertexShaderImageViews) {
            stageImageViewArrayStack_(ShaderStage::Vertex).pushTop();
        }
        if (parameters & PipelineParameter::VertexShaderSamplers) {
            stageSamplerStateArrayStack_(ShaderStage::Vertex).pushTop();
        }
        if (parameters & PipelineParameter::GeometryShaderConstantBuffers) {
            stageConstantBufferArrayStack_(ShaderStage::Geometry).pushTop();
        }
        if (parameters & PipelineParameter::GeometryShaderImageViews) {
            stageImageViewArrayStack_(ShaderStage::Geometry).pushTop();
        }
        if (parameters & PipelineParameter::GeometryShaderSamplers) {
            stageSamplerStateArrayStack_(ShaderStage::Geometry).pushTop();
        }
        if (parameters & PipelineParameter::PixelShaderConstantBuffers) {
            stageConstantBufferArrayStack_(ShaderStage::Pixel).pushTop();
        }
        if (parameters & PipelineParameter::PixelShaderImageViews) {
            stageImageViewArrayStack_(ShaderStage::Pixel).pushTop();
        }
        if (parameters & PipelineParameter::PixelShaderSamplers) {
            stageSamplerStateArrayStack_(ShaderStage::Pixel).pushTop();
        }
    }
}

void Engine::popPipelineParameters(PipelineParameters parameters) {
    if (parameters == PipelineParameter::None) {
        return;
    }
    if (parameters & PipelineParameter::Framebuffer) {
        framebufferStack_.pop();
        dirtyPipelineParameters_ |= PipelineParameter::Framebuffer;
    }
    if (parameters & PipelineParameter::Viewport) {
        viewportStack_.pop();
        dirtyPipelineParameters_ |= PipelineParameter::Viewport;
    }
    if (parameters & PipelineParameter::Program) {
        programStack_.pop();
        dirtyPipelineParameters_ |= PipelineParameter::Program;
    }
    if (parameters & PipelineParameter::BlendState) {
        blendStateStack_.pop();
        dirtyPipelineParameters_ |= PipelineParameter::BlendState;
    }
    if (parameters & PipelineParameter::DepthStencilState) {
        //dirtyPipelineParameters_ |= PipelineParameter::DepthStencilState;
    }
    if (parameters & PipelineParameter::RasterizerState) {
        rasterizerStateStack_.pop();
        dirtyPipelineParameters_ |= PipelineParameter::RasterizerState;
    }
    if (parameters & PipelineParameter::ScissorRect) {
        scissorRectStack_.pop();
        dirtyPipelineParameters_ |= PipelineParameter::ScissorRect;
    }
    if (parameters & PipelineParameter::AllShadersResources) {
        if (parameters & PipelineParameter::VertexShaderConstantBuffers) {
            stageConstantBufferArrayStack_(ShaderStage::Vertex).pop();
            dirtyPipelineParameters_ |= PipelineParameter::VertexShaderConstantBuffers;
        }
        if (parameters & PipelineParameter::VertexShaderImageViews) {
            stageImageViewArrayStack_(ShaderStage::Vertex).pop();
            dirtyPipelineParameters_ |= PipelineParameter::VertexShaderImageViews;
        }
        if (parameters & PipelineParameter::VertexShaderSamplers) {
            stageSamplerStateArrayStack_(ShaderStage::Vertex).pop();
            dirtyPipelineParameters_ |= PipelineParameter::VertexShaderSamplers;
        }
        if (parameters & PipelineParameter::GeometryShaderConstantBuffers) {
            stageConstantBufferArrayStack_(ShaderStage::Geometry).pop();
            dirtyPipelineParameters_ |= PipelineParameter::GeometryShaderConstantBuffers;
        }
        if (parameters & PipelineParameter::GeometryShaderImageViews) {
            stageImageViewArrayStack_(ShaderStage::Geometry).pop();
            dirtyPipelineParameters_ |= PipelineParameter::GeometryShaderImageViews;
        }
        if (parameters & PipelineParameter::GeometryShaderSamplers) {
            stageSamplerStateArrayStack_(ShaderStage::Geometry).pop();
            dirtyPipelineParameters_ |= PipelineParameter::GeometryShaderSamplers;
        }
        if (parameters & PipelineParameter::PixelShaderConstantBuffers) {
            stageConstantBufferArrayStack_(ShaderStage::Pixel).pop();
            dirtyPipelineParameters_ |= PipelineParameter::PixelShaderConstantBuffers;
        }
        if (parameters & PipelineParameter::PixelShaderImageViews) {
            stageImageViewArrayStack_(ShaderStage::Pixel).pop();
            dirtyPipelineParameters_ |= PipelineParameter::PixelShaderImageViews;
        }
        if (parameters & PipelineParameter::PixelShaderSamplers) {
            stageSamplerStateArrayStack_(ShaderStage::Pixel).pop();
            dirtyPipelineParameters_ |= PipelineParameter::PixelShaderSamplers;
        }
    }
}

namespace {

UInt32 toMilliseconds(const std::chrono::steady_clock::duration& d) {
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    return core::int_cast<UInt32>(duration_cast<milliseconds>(d).count());
}

} // namespace

bool Engine::beginFrame(const SwapChainPtr& swapChain, FrameKind kind) {

    if (swapChain && !checkResourceIsValid_(swapChain)) {
        return false;
    }

    frameStartTime_ = std::chrono::steady_clock::now();
    dirtyBuiltinConstantBuffer_ = true;
    if (kind == FrameKind::QWidget) {
        // XXX Maybe we'd want to move this to preBeginFrame_, but we do not
        // have access to dirtyPipelineParameters_ there.
        dirtyPipelineParameters_ |= PipelineParameter::All;
        dirtyPipelineParameters_.unset(PipelineParameter::Framebuffer);
        dirtyPipelineParameters_.unset(PipelineParameter::Viewport);
    }
    preBeginFrame_(swapChain.get(), kind);

    // XXX check every stack has size one !

    swapChain_ = swapChain;
    // do this unconditionally since the swapchain size may have changed.
    queueLambdaCommandWithParameters_<SwapChainPtr>(
        "setSwapChain",
        [](Engine* engine, const SwapChainPtr& swapChain) {
            engine->setSwapChain_(swapChain);
        },
        swapChain);

    if (kind != FrameKind::QWidget) {
        setDefaultFramebuffer();
        dirtyPipelineParameters_ |= PipelineParameter::Framebuffer;
    }

    return true;
}

void Engine::endFrame(Int syncInterval, PresentFlags flags) {

    // check syncInterval >= 0
    syncInterval = syncInterval > 0 ? syncInterval : 0;
    UInt32 uSyncInterval = core::int_cast<UInt32>(syncInterval);

    ++swapChain_->numPendingPresents_;
    bool shouldWait = syncInterval > 0;

    if (!isMultithreadingEnabled()) {
        UInt64 timestamp = present_(swapChain_.get(), uSyncInterval, flags);
        --swapChain_->numPendingPresents_;
        if (presentCallback_) {
            presentCallback_(timestamp);
        }
    }
    else if (shouldWait && shouldPresentWaitFromSyncedUserThread_()) {
        // Preventing dead-locks
        // See https://docs.microsoft.com/en-us/windows/win32/api/DXGI1_2/nf-dxgi1_2-idxgiswapchain1-present1#remarks
        flushWait();
        UInt64 timestamp =
            present_(swapChain_.get(), core::int_cast<UInt32>(syncInterval), flags);
        --swapChain_->numPendingPresents_;
        if (presentCallback_) {
            presentCallback_(timestamp);
        }
    }
    else {
        struct CommandParameters {
            SwapChain* swapChain;
            UInt32 syncInterval;
            PresentFlags flags;
        };
        queueLambdaCommandWithParameters_<CommandParameters>(
            "present",
            [](Engine* engine, const CommandParameters& p) {
                UInt64 timestamp = engine->present_(p.swapChain, p.syncInterval, p.flags);
                --p.swapChain->numPendingPresents_;

                if (engine->presentCallback_) {
                    engine->presentCallback_(timestamp);
                }
            },
            swapChain_.get(),
            core::int_cast<UInt32>(syncInterval),
            flags);

        if (shouldWait) {
            flushWait();
        }
        else {
            submitPendingCommandList_();
        }
    }

    submitPendingCommandList_();
    if (!isMultithreadingEnabled()) {
        resourceRegistry_->releaseAndDeleteGarbagedResources(this);
    }
}

void Engine::onWindowResize(const SwapChainPtr& swapChain, Int width, Int height) {
    if (!checkResourceIsValid_(swapChain)) {
        return;
    }
    flushWait();
    onWindowResize_(
        swapChain.get(), core::int_cast<UInt32>(width), core::int_cast<UInt32>(height));
}

void Engine::draw(const GeometryViewPtr& geometryView, Int numIndices) {
    if (!checkResourceIsValid_(geometryView)) {
        return;
    }
    if (numIndices == 0) {
        return;
    }
    syncState_();
    Int n = (numIndices >= 0) ? numIndices : geometryView->numVertices();
    UInt un = core::int_cast<UInt>(n);

    queueLambdaCommandWithParameters_<GeometryView*>(
        "draw",
        [=](Engine* engine, GeometryView* gv) { engine->draw_(gv, un, 0); },
        geometryView.get());
}

void Engine::drawInstanced(
    const GeometryViewPtr& geometryView,
    Int numIndices,
    Int numInstances) {

    if (!checkResourceIsValid_(geometryView)) {
        return;
    }
    if (numIndices == 0) {
        return;
    }
    syncState_();
    Int n = (numIndices >= 0) ? numIndices : geometryView->numVertices();
    UInt un = core::int_cast<UInt>(n);
    Int k = (numInstances >= 0) ? numInstances : geometryView->numInstances();
    UInt uk = core::int_cast<UInt>(k);

    queueLambdaCommandWithParameters_<GeometryView*>(
        "draw",
        [=](Engine* engine, GeometryView* gv) { engine->draw_(gv, un, uk); },
        geometryView.get());
}

void Engine::clear(const core::Color& color) {
    syncState_();
    queueLambdaCommandWithParameters_<core::Color>(
        "clear", [](Engine* engine, const core::Color& c) { engine->clear_(c); }, color);
}

void Engine::preBeginFrame_(SwapChain*, FrameKind) {
}

void Engine::init_() {

    engineStartTime_ = std::chrono::steady_clock::now();
    if (isMultithreadingEnabled()) {
        startRenderThread_();
    }
    else {
        initContext_();
    }
    createBuiltinResources_();
    queueLambdaCommand_(
        "initBuiltinResources", [](Engine* engine) { engine->initBuiltinResources_(); });

    // QglEngine::initContext_ is not thread-safe (static Qt create() functions
    // related to OpenGL access a global context pointer that is not
    // synchronized).
    if (isMultithreadingEnabled()) {
        flushWait();
    }
}

void Engine::createBuiltinResources_() {
    {
        BufferCreateInfo createInfo = {};
        createInfo.setUsage(Usage::Dynamic);
        createInfo.setBindFlags(BindFlag::ConstantBuffer);
        createInfo.setCpuAccessFlags(CpuAccessFlag::Write);
        builtinConstantsBuffer_ =
            createBuffer(createInfo, sizeof(detail::BuiltinConstants));
    }

    createBuiltinShaders_();
}

ProgramPtr Engine::builtinProgram(BuiltinProgram builtinProgram) {
    ProgramPtr program = {};
    switch (builtinProgram) {
    case BuiltinProgram::Simple:
        return simpleProgram_;
    case BuiltinProgram::SimpleTextured:
        return simpleTexturedProgram_;
    case BuiltinProgram::SreenSpaceDisplacement:
        return sreenSpaceDisplacementProgram_;
    default:
        break;
    }
    return ProgramPtr();
}

void Engine::syncState_() {

    const PipelineParameters parameters = dirtyPipelineParameters_;

    if (dirtyBuiltinConstantBuffer_ || (parameters & PipelineParameter::Viewport)) {
        detail::BuiltinConstants constants = {};
        constants.projMatrix = projectionMatrixStack_.top();
        constants.viewMatrix = viewMatrixStack_.top();
        constants.frameStartTimeInMs = toMilliseconds(frameStartTime_ - engineStartTime_);
        const Viewport& vp = viewportStack_.top();
        constants.viewport = geometry::Vec4f(
            static_cast<float>(vp.x()),
            static_cast<float>(vp.y()),
            static_cast<float>(vp.width()),
            static_cast<float>(vp.height()));
        struct CommandParameters {
            Buffer* buffer;
            detail::BuiltinConstants constants;
        };
        queueLambdaCommandWithParameters_<CommandParameters>(
            "updateBuiltinConstantBufferData",
            [](Engine* engine, const CommandParameters& p) {
                engine->updateBufferData_(
                    p.buffer, &p.constants, sizeof(detail::BuiltinConstants));
            },
            builtinConstantsBuffer_.get(),
            constants);
        dirtyBuiltinConstantBuffer_ = false;
    }

    if (parameters == PipelineParameter::None) {
        return;
    }

    if (parameters & PipelineParameter::Framebuffer) {
        FramebufferPtr framebuffer = framebufferStack_.top();
        queueLambdaCommandWithParameters_<FramebufferPtr>(
            "setFramebuffer",
            [](Engine* engine, const FramebufferPtr& p) { engine->setFramebuffer_(p); },
            framebuffer);
    }
    if (parameters & PipelineParameter::Viewport) {
        queueLambdaCommandWithParameters_<Viewport>(
            "setViewport",
            [](Engine* engine, const Viewport& vp) {
                engine->setViewport_(vp.x(), vp.y(), vp.width(), vp.height());
            },
            viewportStack_.top());
    }
    if (parameters & PipelineParameter::Program) {
        queueLambdaCommandWithParameters_<ProgramPtr>(
            "setProgram",
            [](Engine* engine, const ProgramPtr& p) { engine->setProgram_(p); },
            programStack_.top());
    }
    if (parameters & PipelineParameter::BlendState) {
        struct CommandParameters {
            BlendStatePtr blendState;
            geometry::Vec4f blendConstantFactors;
        };
        const BlendStateAndConstant& top = blendStateStack_.top();
        queueLambdaCommandWithParameters_<CommandParameters>(
            "setBlendState",
            [](Engine* engine, const CommandParameters& p) {
                engine->setBlendState_(p.blendState, p.blendConstantFactors);
            },
            top.statePtr,
            top.constantFactors);
    }
    if (parameters & PipelineParameter::DepthStencilState) {
        //dirtyPipelineParameters_ |= PipelineParameter::DepthStencilState;
    }
    if (parameters & PipelineParameter::RasterizerState) {
        queueLambdaCommandWithParameters_<RasterizerStatePtr>(
            "setRasterizerState",
            [](Engine* engine, const RasterizerStatePtr& p) {
                engine->setRasterizerState_(p);
            },
            rasterizerStateStack_.top());
    }
    if (parameters & PipelineParameter::ScissorRect) {
        queueLambdaCommandWithParameters_<geometry::Rect2f>(
            "setScissorRect",
            [](Engine* engine, const geometry::Rect2f& sr) {
                engine->setScissorRect_(sr);
            },
            scissorRectStack_.top());
    }
    if (parameters & PipelineParameter::AllShadersResources) {

        // XXX should prevent the use of the same image-view twice because
        // OpenGL couples the concepts of view and sampler under the concept
        // of texture.

        if (parameters & PipelineParameter::VertexShaderConstantBuffers) {
            syncStageConstantBuffers_(ShaderStage::Vertex);
        }
        if (parameters & PipelineParameter::VertexShaderImageViews) {
            syncStageImageViews_(ShaderStage::Vertex);
        }
        if (parameters & PipelineParameter::VertexShaderSamplers) {
            syncStageSamplers_(ShaderStage::Vertex);
        }
        if (parameters & PipelineParameter::GeometryShaderConstantBuffers) {
            syncStageConstantBuffers_(ShaderStage::Geometry);
        }
        if (parameters & PipelineParameter::GeometryShaderImageViews) {
            syncStageImageViews_(ShaderStage::Geometry);
        }
        if (parameters & PipelineParameter::GeometryShaderSamplers) {
            syncStageSamplers_(ShaderStage::Geometry);
        }
        if (parameters & PipelineParameter::PixelShaderConstantBuffers) {
            syncStageConstantBuffers_(ShaderStage::Pixel);
        }
        if (parameters & PipelineParameter::PixelShaderImageViews) {
            syncStageImageViews_(ShaderStage::Pixel);
        }
        if (parameters & PipelineParameter::PixelShaderSamplers) {
            syncStageSamplers_(ShaderStage::Pixel);
        }
    }

    dirtyPipelineParameters_ = PipelineParameter::None;
}

void Engine::syncStageConstantBuffers_(ShaderStage shaderStage) {
    struct CommandParameters {
        StageConstantBufferArray buffers;
        ShaderStage shaderStage;
    };
    CommandParameters params = {};
    params.buffers = constantBufferArrayStacks_[toIndex_(shaderStage)].top();
    params.shaderStage = shaderStage;
    Program* program = programStack_.top().get();
    if (program && program->isBuiltin()) {
        params.buffers[0] = builtinConstantsBuffer_;
    }
    queueLambdaCommandWithParameters_<CommandParameters>(
        "setStageConstantBuffers",
        [](Engine* engine, const CommandParameters& p) {
            engine->setStageConstantBuffers_(
                p.buffers.data(), 0, p.buffers.size(), p.shaderStage);
        },
        std::move(params));
}

void Engine::syncStageImageViews_(ShaderStage shaderStage) {
    struct CommandParameters {
        StageImageViewArray views;
        ShaderStage shaderStage;
    };
    queueLambdaCommandWithParameters_<CommandParameters>(
        "setStageImageViews",
        [](Engine* engine, const CommandParameters& p) {
            engine->setStageImageViews_(p.views.data(), 0, p.views.size(), p.shaderStage);
        },
        imageViewArrayStacks_[toIndex_(shaderStage)].last(),
        shaderStage);
}

void Engine::syncStageSamplers_(ShaderStage shaderStage) {
    struct CommandParameters {
        StageSamplerStateArray states;
        ShaderStage shaderStage;
    };
    queueLambdaCommandWithParameters_<CommandParameters>(
        "setStageSamplers",
        [](Engine* engine, const CommandParameters& p) {
            engine->setStageSamplers_(p.states.data(), 0, p.states.size(), p.shaderStage);
        },
        samplerStateArrayStacks_[toIndex_(shaderStage)].last(),
        shaderStage);
}

// -- render thread + sync --

// XXX add try/catch ?
void Engine::renderThreadProc_() {

    initContext_();
    std::unique_lock<std::mutex> lock(mutex_, std::defer_lock);
    while (1) {
        lock.lock();

        // wait for work or stop request
        wakeRenderThreadConditionVariable_.wait(
            lock, [&] { return !commandQueue_.isEmpty() || stopRequested_; });

        // userResourcesToSetAsInternalOnly_

        // if requested, stop
        if (stopRequested_) {
            // cancel submitted lists
            commandQueue_.clear();
            lastExecutedCommandListId_ = lastSubmittedCommandListId_;
            // release all resources..
            resourceRegistry_->releaseAllResources(this);
            // notify
            lock.unlock();
            renderThreadEventConditionVariable_.notify_all();
            return;
        }

        // else commandQueue_ is not empty, so prepare some work
        CommandList commandList = std::move(commandQueue_.first());
        commandQueue_.removeFirst();

        lock.unlock();

        // execute commands
        for (const CommandUPtr& command : commandList.commands) {
            command->execute(this);
        }

        lock.lock();
        ++lastExecutedCommandListId_;
        lock.unlock();

        renderThreadEventConditionVariable_.notify_all();

        // release garbaged resources (locking)
        resourceRegistry_->releaseAndDeleteGarbagedResources(this);
    }
}

void Engine::startRenderThread_() {
    if (stopRequested_) {
        throw core::LogicError("Engine: restarts are not supported.");
    }
    if (!isThreadRunning_) {
        renderThread_ = std::thread([=] { this->renderThreadProc_(); });
        isThreadRunning_ = true;
    }
}

void Engine::stopRenderThread_() {
    pendingCommands_.clear();
    swapChain_.reset();
    if (isThreadRunning_) {
        std::unique_lock<std::mutex> lock(mutex_);
        stopRequested_ = true;
        lock.unlock();
        wakeRenderThreadConditionVariable_.notify_all();
        VGC_CORE_ASSERT(renderThread_.joinable());
        renderThread_.join();
        isThreadRunning_ = false;
        return;
    }
}

UInt Engine::submitPendingCommandList_() {
    std::unique_lock<std::mutex> lock(mutex_);
    bool notifyRenderThread = false;
    UInt id = lastSubmittedCommandListId_;
    if (!pendingCommands_.empty()) {
        notifyRenderThread = commandQueue_.isEmpty();
        commandQueue_.emplaceLast(std::move(pendingCommands_));
        pendingCommands_.clear();
        id = ++lastSubmittedCommandListId_;
    }
    lock.unlock();
    if (notifyRenderThread) {
        wakeRenderThreadConditionVariable_.notify_all();
    }
    return id;
}

void Engine::waitCommandListTranslationFinished_(Int commandListId) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (commandListId == 0) {
        commandListId = lastSubmittedCommandListId_;
    }
    renderThreadEventConditionVariable_.wait(
        lock, [&] { return lastExecutedCommandListId_ == commandListId; });
    lock.unlock();
}

void Engine::sanitize_(SwapChainCreateInfo& /*createInfo*/) {
    // XXX
}

void Engine::sanitize_(BufferCreateInfo& createInfo) {

    Usage usage = createInfo.usage();
    if (usage == Usage::Immutable) {
        if (createInfo.isMipGenerationEnabled()) {
            VGC_WARNING(
                LogVgcGraphics,
                "ResourceMiscFlag::GenerateMips is set but usage is Usage::Immutable. "
                "The ResourceMiscFlag in question is being unset automatically.");
            ResourceMiscFlags resourceMiscFlags = createInfo.resourceMiscFlags();
            resourceMiscFlags.unset(ResourceMiscFlag::GenerateMips);
            createInfo.setResourceMiscFlags(resourceMiscFlags);
        }
    }

    BindFlags bindFlags = createInfo.bindFlags();
    if (createInfo.isMipGenerationEnabled()) {
        if (!bindFlags.has(BindFlag::RenderTarget)) {
            VGC_WARNING(
                LogVgcGraphics,
                "BindFlag::RenderTarget is not set but "
                "ResourceMiscFlag::GenerateMips is. "
                "The BindFlag in question is being set automatically.");
            bindFlags.set(BindFlag::RenderTarget);
        }
        if (!bindFlags.has(BindFlag::ShaderResource)) {
            VGC_WARNING(
                LogVgcGraphics,
                "BindFlag::ShaderResource is not set but "
                "ResourceMiscFlag::GenerateMips is. "
                "The BindFlag in question is being set automatically.");
            bindFlags.set(BindFlag::ShaderResource);
        }
        createInfo.setBindFlags(bindFlags);
    }
}

void Engine::sanitize_(ImageCreateInfo& createInfo) {

    bool isMultisampled = createInfo.numSamples() > 1;
    if (isMultisampled) {
        if (createInfo.rank() == ImageRank::_1D) {
            VGC_WARNING(
                LogVgcGraphics,
                "Number of samples ignored: multisampling is not available for 1D "
                "images.");
            createInfo.setNumSamples(1);
        }
        if (createInfo.numMipLevels() != 1) {
            VGC_WARNING(
                LogVgcGraphics,
                "Number of mip levels ignored: multisampled image can only have level "
                "0.");
            createInfo.setNumMipLevels(1);
        }
    }

    Usage usage = createInfo.usage();
    if (usage == Usage::Immutable) {
        if (createInfo.isMipGenerationEnabled()) {
            VGC_WARNING(
                LogVgcGraphics,
                "ResourceMiscFlag::GenerateMips is set but usage is Usage::Immutable. "
                "The ResourceMiscFlag in question is being unset automatically, and "
                "numMipLevels is set to 1 if it was 0.");
            ResourceMiscFlags resourceMiscFlags = createInfo.resourceMiscFlags();
            resourceMiscFlags.unset(ResourceMiscFlag::GenerateMips);
            createInfo.setResourceMiscFlags(resourceMiscFlags);
            if (createInfo.numMipLevels() == 0) {
                createInfo.setNumMipLevels(1);
            }
        }
    }

    ImageBindFlags bindFlags = createInfo.bindFlags();
    if (createInfo.isMipGenerationEnabled()) {
        if (!bindFlags.has(ImageBindFlag::RenderTarget)) {
            VGC_WARNING(
                LogVgcGraphics,
                "ImageBindFlag::RenderTarget is not set but "
                "ResourceMiscFlag::GenerateMips is. "
                "The ImageBindFlag in question is being set automatically.");
            bindFlags.set(ImageBindFlag::RenderTarget);
        }
        if (!bindFlags.has(ImageBindFlag::ShaderResource)) {
            VGC_WARNING(
                LogVgcGraphics,
                "ImageBindFlag::ShaderResource is not set but "
                "ResourceMiscFlag::GenerateMips is. "
                "The ImageBindFlag in question is being set automatically.");
            bindFlags.set(ImageBindFlag::ShaderResource);
        }
        createInfo.setBindFlags(bindFlags);
    }
    else if (createInfo.numMipLevels() == 0) {
        VGC_WARNING(
            LogVgcGraphics,
            "Automatic number of mip levels resolves to 1 since mip generation is not "
            "enabled.");
        createInfo.setNumMipLevels(1);
    }

    const Int width = createInfo.width();
    if (width <= 0 || width > maxImageWidth) {
        std::string err = core::format(
            "Requested image width ({}) should be in the range [1, {}].",
            width,
            maxImageWidth);
        if (width <= 0) {
            throw core::RangeError(err);
        }
        else {
            VGC_ERROR(LogVgcGraphics, err);
        }
    }

    const Int height = createInfo.height();
    if (height != 0
        && core::toUnderlying(createInfo.rank()) < core::toUnderlying(ImageRank::_2D)) {
        VGC_WARNING(LogVgcGraphics, "Height ignored: image rank must be at least 2D.");
        createInfo.setHeight(0);
    }
    else if (height <= 0 || height > maxImageHeight) {
        std::string err = core::format(
            "Requested image height ({}) should be in the range [1, {}].",
            height,
            maxImageHeight);
        if (height <= 0) {
            throw core::RangeError(err);
        }
        else {
            VGC_ERROR(LogVgcGraphics, err);
        }
    }

    const Int numLayers = createInfo.numLayers();
    if (numLayers <= 0 || numLayers > maxImageLayers) {
        VGC_ERROR(
            LogVgcGraphics,
            "Requested number of image layers ({}) should be in the range [1, {}].",
            numLayers,
            maxImageLayers);
    }

    const Int numMipLevels = createInfo.numMipLevels();
    const Int maxMipLevels = calculateMaxMipLevels(width, height);
    if (numMipLevels < 0 || numMipLevels > maxMipLevels) {
        VGC_ERROR(
            LogVgcGraphics,
            "Requested number of mip levels ({}) should be in the range [0, {}]",
            numLayers,
            maxMipLevels);
    }
    if (numMipLevels == 0) {
        createInfo.setNumMipLevels(maxMipLevels);
    }

    // XXX check is power of 2
    const Int numSamples = createInfo.numSamples();
    if (numSamples <= 0 || numSamples > maxNumSamples) {
        static_assert(maxNumSamples == 8); // hard-coded list
        VGC_ERROR(
            LogVgcGraphics,
            "Requested number of samples ({}) should be either 1, 2, 4, or 8.",
            numSamples);
    }

    //usage
    //bindFlags
    //cpuAccessFlags
    //resourceMiscFlags
}

void Engine::sanitize_(ImageViewCreateInfo& /*createInfo*/) {
    // XXX should check bind flags compatibility here
}

void Engine::sanitize_(SamplerStateCreateInfo& createInfo) {
    if (createInfo.maxAnisotropy() <= 1) {
        createInfo.setMaxAnisotropy(1);
    }
}

void Engine::sanitize_(GeometryViewCreateInfo& /*createInfo*/) {
    // XXX
}

void Engine::sanitize_(BlendStateCreateInfo& /*createInfo*/) {
    // XXX
}

void Engine::sanitize_(RasterizerStateCreateInfo& /*createInfo*/) {
    // XXX
}

} // namespace vgc::graphics
