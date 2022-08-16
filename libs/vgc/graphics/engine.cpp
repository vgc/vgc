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
    blendConstantFactorStack_.emplaceLast();
    rasterizerStateStack_.emplaceLast();

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
    blendConstantFactorStack_.clear();
    rasterizerStateStack_.clear();

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
            "Negative initialLengthInBytes ({}) provided to Engine::createBuffer()",
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
            "Negative initialLengthInBytes ({}) provided to Engine::createVertexBuffer()",
            initialLengthInBytes));
    }

    BufferCreateInfo createInfo = {};
    createInfo.setUsage(Usage::Dynamic);
    createInfo.setBindFlags(BindFlag::VertexBuffer);
    createInfo.setCpuAccessFlags(CpuAccessFlag::Write);
    createInfo.setResourceMiscFlags(ResourceMiscFlag::None);
    return createBuffer(createInfo, initialLengthInBytes);
}

GeometryViewPtr
Engine::createDynamicTriangleListView(BuiltinGeometryLayout vertexLayout) {
    BufferPtr vertexBuffer = createVertexBuffer(0);
    GeometryViewCreateInfo createInfo = {};
    createInfo.setBuiltinGeometryLayout(vertexLayout);
    createInfo.setPrimitiveType(PrimitiveType::TriangleList);
    createInfo.setVertexBuffer(0, vertexBuffer);
    return createGeometryView(createInfo);
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

    if (sanitizedCreateInfo.numMipLevels() == 0) {
        VGC_ERROR(
            LogVgcGraphics,
            "Cannot create an image with initial data if the number of mip levels is 0 "
            "(automatic).");
        return nullptr;
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
    if (framebufferStack_.last() != framebuffer) {
        framebufferStack_.last() = framebuffer;
        dirtyPipelineParameters_ |= PipelineParameter::Framebuffer;
    }
}

void Engine::setViewport(Int x, Int y, Int width, Int height) {
    viewportStack_.last() = Viewport(x, y, width, height);
    dirtyPipelineParameters_ |= PipelineParameter::Viewport;
}

void Engine::setProgram(BuiltinProgram builtinProgram) {

    ProgramPtr program = {};
    switch (builtinProgram) {
    case BuiltinProgram::Simple: {
        program = simpleProgram_;
        break;
    }
    default:
        break;
    }
    if (programStack_.last() != program) {
        programStack_.last() = program;
        if (true /*program->usesBuiltinConstants()*/) {
            BufferPtr& constantBufferRef =
                constantBufferArrayStacks_[toIndex_(ShaderStage::Vertex)].last()[0];
            if (constantBufferRef != builtinConstantsBuffer_) {
                constantBufferRef = builtinConstantsBuffer_;
                dirtyPipelineParameters_ |=
                    PipelineParameter::VertexShaderConstantBuffers;
            }
        }
        dirtyPipelineParameters_ |= PipelineParameter::Program;
    }
}

void Engine::setBlendState(
    const BlendStatePtr& state,
    const geometry::Vec4f& blendConstantFactor) {

    if (blendStateStack_.last() != state) {
        blendStateStack_.last() = state;
        dirtyPipelineParameters_ |= PipelineParameter::BlendState;
    }
    if (blendConstantFactorStack_.last() != blendConstantFactor) {
        blendConstantFactorStack_.last() = blendConstantFactor;
        dirtyPipelineParameters_ |= PipelineParameter::BlendState;
    }
}

void Engine::setRasterizerState(const RasterizerStatePtr& state) {
    if (rasterizerStateStack_.last() != state) {
        rasterizerStateStack_.last() = state;
        dirtyPipelineParameters_ |= PipelineParameter::RasterizerState;
    }
}

void Engine::setStageConstantBuffers(
    const BufferPtr* buffers,
    Int startIndex,
    Int count,
    ShaderStage shaderStage) {

    size_t stageIndex = toIndex_(shaderStage);
    StageConstantBufferArray& constantBufferArray =
        constantBufferArrayStacks_[stageIndex].emplaceLast();
    for (Int i = 0; i < count; ++i) {
        constantBufferArray[startIndex + i] = buffers[i];
    }
    dirtyPipelineParameters_ |= std::array{
        PipelineParameter::VertexShaderConstantBuffers,
        PipelineParameter::GeometryShaderConstantBuffers,
        PipelineParameter::PixelShaderConstantBuffers}[stageIndex];
}

void Engine::setStageImageViews(
    const ImageViewPtr* views,
    Int startIndex,
    Int count,
    ShaderStage shaderStage) {

    size_t stageIndex = toIndex_(shaderStage);
    StageImageViewArray& imageViewArray = imageViewArrayStacks_[stageIndex].emplaceLast();
    for (Int i = 0; i < count; ++i) {
        imageViewArray[startIndex + i] = views[i];
    }
    dirtyPipelineParameters_ |= std::array{
        PipelineParameter::VertexShaderImageViews,
        PipelineParameter::GeometryShaderImageViews,
        PipelineParameter::PixelShaderImageViews}[stageIndex];
}

void Engine::setStageSamplers(
    const SamplerStatePtr* states,
    Int startIndex,
    Int count,
    ShaderStage shaderStage) {

    size_t stageIndex = toIndex_(shaderStage);
    StageSamplerStateArray& samplerStateArray =
        samplerStateArrayStacks_[stageIndex].emplaceLast();
    for (Int i = 0; i < count; ++i) {
        samplerStateArray[startIndex + i] = states[i];
    }
    dirtyPipelineParameters_ |= std::array{
        PipelineParameter::VertexShaderSamplers,
        PipelineParameter::GeometryShaderSamplers,
        PipelineParameter::PixelShaderSamplers}[stageIndex];
}

void Engine::pushPipelineParameters(PipelineParameters parameters) {

    if (parameters & PipelineParameter::Framebuffer) {
        framebufferStack_.emplaceLast(framebufferStack_.last());
    }
    if (parameters & PipelineParameter::Viewport) {
        viewportStack_.emplaceLast(viewportStack_.last());
    }
    if (parameters & PipelineParameter::Program) {
        programStack_.emplaceLast(programStack_.last());
    }
    if (parameters & PipelineParameter::BlendState) {
        blendStateStack_.emplaceLast(blendStateStack_.last());
    }
    if (parameters & PipelineParameter::DepthStencilState) {
        // todo
    }
    if (parameters & PipelineParameter::RasterizerState) {
        rasterizerStateStack_.emplaceLast(rasterizerStateStack_.last());
    }
    if (parameters & PipelineParameter::AllShadersResources) {
        if (parameters & PipelineParameter::VertexShaderConstantBuffers) {
            StageConstantBufferArrayStack& constantBufferArrayStack =
                constantBufferArrayStacks_[toIndex_(ShaderStage::Vertex)];
            constantBufferArrayStack.emplaceLast(constantBufferArrayStack.last());
        }
        if (parameters & PipelineParameter::VertexShaderImageViews) {
            StageImageViewArrayStack& imageViewArrayStack =
                imageViewArrayStacks_[toIndex_(ShaderStage::Vertex)];
            imageViewArrayStack.emplaceLast(imageViewArrayStack.last());
        }
        if (parameters & PipelineParameter::VertexShaderSamplers) {
            StageSamplerStateArrayStack& samplerStateArrayStack =
                samplerStateArrayStacks_[toIndex_(ShaderStage::Vertex)];
            samplerStateArrayStack.emplaceLast(samplerStateArrayStack.last());
        }
        if (parameters & PipelineParameter::GeometryShaderConstantBuffers) {
            StageConstantBufferArrayStack& constantBufferArrayStack =
                constantBufferArrayStacks_[toIndex_(ShaderStage::Geometry)];
            constantBufferArrayStack.emplaceLast(constantBufferArrayStack.last());
        }
        if (parameters & PipelineParameter::GeometryShaderImageViews) {
            StageImageViewArrayStack& imageViewArrayStack =
                imageViewArrayStacks_[toIndex_(ShaderStage::Geometry)];
            imageViewArrayStack.emplaceLast(imageViewArrayStack.last());
        }
        if (parameters & PipelineParameter::GeometryShaderSamplers) {
            StageSamplerStateArrayStack& samplerStateArrayStack =
                samplerStateArrayStacks_[toIndex_(ShaderStage::Geometry)];
            samplerStateArrayStack.emplaceLast(samplerStateArrayStack.last());
        }
        if (parameters & PipelineParameter::PixelShaderConstantBuffers) {
            StageConstantBufferArrayStack& constantBufferArrayStack =
                constantBufferArrayStacks_[toIndex_(ShaderStage::Pixel)];
            constantBufferArrayStack.emplaceLast(constantBufferArrayStack.last());
        }
        if (parameters & PipelineParameter::PixelShaderImageViews) {
            StageImageViewArrayStack& imageViewArrayStack =
                imageViewArrayStacks_[toIndex_(ShaderStage::Pixel)];
            imageViewArrayStack.emplaceLast(imageViewArrayStack.last());
        }
        if (parameters & PipelineParameter::PixelShaderSamplers) {
            StageSamplerStateArrayStack& samplerStateArrayStack =
                samplerStateArrayStacks_[toIndex_(ShaderStage::Pixel)];
            samplerStateArrayStack.emplaceLast(samplerStateArrayStack.last());
        }
    }
}

void Engine::popPipelineParameters(PipelineParameters parameters) {

    if (parameters == PipelineParameter::None) {
        return;
    }

    if (parameters & PipelineParameter::Framebuffer) {
        framebufferStack_.removeLast();
        dirtyPipelineParameters_ |= PipelineParameter::Framebuffer;
    }
    if (parameters & PipelineParameter::Viewport) {
        viewportStack_.removeLast();
        dirtyPipelineParameters_ |= PipelineParameter::Viewport;
    }
    if (parameters & PipelineParameter::Program) {
        programStack_.removeLast();
        dirtyPipelineParameters_ |= PipelineParameter::Program;
    }
    if (parameters & PipelineParameter::BlendState) {
        blendStateStack_.removeLast();
        dirtyPipelineParameters_ |= PipelineParameter::BlendState;
    }
    if (parameters & PipelineParameter::DepthStencilState) {
        //dirtyPipelineParameters_ |= PipelineParameter::DepthStencilState;
    }
    if (parameters & PipelineParameter::RasterizerState) {
        rasterizerStateStack_.removeLast();
        dirtyPipelineParameters_ |= PipelineParameter::RasterizerState;
    }
    if (parameters & PipelineParameter::AllShadersResources) {
        if (parameters & PipelineParameter::VertexShaderConstantBuffers) {
            StageConstantBufferArrayStack& constantBufferArrayStack =
                constantBufferArrayStacks_[toIndex_(ShaderStage::Vertex)];
            constantBufferArrayStack.removeLast();
            dirtyPipelineParameters_ |= PipelineParameter::VertexShaderConstantBuffers;
        }
        if (parameters & PipelineParameter::VertexShaderImageViews) {
            StageImageViewArrayStack& imageViewArrayStack =
                imageViewArrayStacks_[toIndex_(ShaderStage::Vertex)];
            imageViewArrayStack.removeLast();
            dirtyPipelineParameters_ |= PipelineParameter::VertexShaderImageViews;
        }
        if (parameters & PipelineParameter::VertexShaderSamplers) {
            StageSamplerStateArrayStack& samplerStateArrayStack =
                samplerStateArrayStacks_[toIndex_(ShaderStage::Vertex)];
            samplerStateArrayStack.removeLast();
            dirtyPipelineParameters_ |= PipelineParameter::VertexShaderSamplers;
        }
        if (parameters & PipelineParameter::GeometryShaderConstantBuffers) {
            StageConstantBufferArrayStack& constantBufferArrayStack =
                constantBufferArrayStacks_[toIndex_(ShaderStage::Geometry)];
            constantBufferArrayStack.removeLast();
            dirtyPipelineParameters_ |= PipelineParameter::GeometryShaderConstantBuffers;
        }
        if (parameters & PipelineParameter::GeometryShaderImageViews) {
            StageImageViewArrayStack& imageViewArrayStack =
                imageViewArrayStacks_[toIndex_(ShaderStage::Geometry)];
            imageViewArrayStack.removeLast();
            dirtyPipelineParameters_ |= PipelineParameter::GeometryShaderImageViews;
        }
        if (parameters & PipelineParameter::GeometryShaderSamplers) {
            StageSamplerStateArrayStack& samplerStateArrayStack =
                samplerStateArrayStacks_[toIndex_(ShaderStage::Geometry)];
            samplerStateArrayStack.removeLast();
            dirtyPipelineParameters_ |= PipelineParameter::GeometryShaderSamplers;
        }
        if (parameters & PipelineParameter::PixelShaderConstantBuffers) {
            StageConstantBufferArrayStack& constantBufferArrayStack =
                constantBufferArrayStacks_[toIndex_(ShaderStage::Pixel)];
            constantBufferArrayStack.removeLast();
            dirtyPipelineParameters_ |= PipelineParameter::PixelShaderConstantBuffers;
        }
        if (parameters & PipelineParameter::PixelShaderImageViews) {
            StageImageViewArrayStack& imageViewArrayStack =
                imageViewArrayStacks_[toIndex_(ShaderStage::Pixel)];
            imageViewArrayStack.removeLast();
            dirtyPipelineParameters_ |= PipelineParameter::PixelShaderImageViews;
        }
        if (parameters & PipelineParameter::PixelShaderSamplers) {
            StageSamplerStateArrayStack& samplerStateArrayStack =
                samplerStateArrayStacks_[toIndex_(ShaderStage::Pixel)];
            samplerStateArrayStack.removeLast();
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

void Engine::syncState_() {

    if (dirtyBuiltinConstantBuffer_) {
        detail::BuiltinConstants constants = {};
        constants.projMatrix = projectionMatrixStack_.last();
        constants.viewMatrix = viewMatrixStack_.last();
        constants.frameStartTimeInMs = toMilliseconds(frameStartTime_ - engineStartTime_);
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

    const PipelineParameters parameters = dirtyPipelineParameters_;
    if (parameters == PipelineParameter::None) {
        return;
    }

    if (parameters & PipelineParameter::Framebuffer) {
        FramebufferPtr framebuffer = framebufferStack_.last();
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
            viewportStack_.last());
    }
    if (parameters & PipelineParameter::Program) {
        queueLambdaCommandWithParameters_<ProgramPtr>(
            "setProgram",
            [](Engine* engine, const ProgramPtr& p) { engine->setProgram_(p); },
            programStack_.last());
    }
    if (parameters & PipelineParameter::BlendState) {
        struct CommandParameters {
            BlendStatePtr blendState;
            geometry::Vec4f blendConstantFactor;
        };
        queueLambdaCommandWithParameters_<CommandParameters>(
            "setBlendState",
            [](Engine* engine, const CommandParameters& p) {
                engine->setBlendState_(p.blendState, p.blendConstantFactor);
            },
            blendStateStack_.last(),
            blendConstantFactorStack_.last());
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
            rasterizerStateStack_.last());
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
    queueLambdaCommandWithParameters_<CommandParameters>(
        "setStageConstantBuffers",
        [](Engine* engine, const CommandParameters& p) {
            engine->setStageConstantBuffers_(
                p.buffers.data(), 0, p.buffers.size(), p.shaderStage);
        },
        constantBufferArrayStacks_[toIndex_(shaderStage)].last(),
        shaderStage);
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

bool Engine::beginFrame(const SwapChainPtr& swapChain, FrameKind kind) {

    if (swapChain && !checkResourceIsValid_(swapChain)) {
        return false;
    }

    frameStartTime_ = std::chrono::steady_clock::now();
    dirtyBuiltinConstantBuffer_ = true;
    if (kind == FrameKind::QWidget) {
        dirtyPipelineParameters_ |= PipelineParameter::All;
        dirtyPipelineParameters_.unset(PipelineParameter::Framebuffer);
        dirtyPipelineParameters_.unset(PipelineParameter::Viewport);
        setStateDirty_();
    }

    // XXX check every stack has size one !

    if (swapChain_ != swapChain) {
        swapChain_ = swapChain;
        queueLambdaCommandWithParameters_<SwapChainPtr>(
            "setSwapChain",
            [](Engine* engine, const SwapChainPtr& swapChain) {
                engine->setSwapChain_(swapChain);
            },
            swapChain);
    }

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

void Engine::resizeSwapChain(const SwapChainPtr& swapChain, Int width, Int height) {
    if (!checkResourceIsValid_(swapChain)) {
        return;
    }
    flushWait();
    resizeSwapChain_(
        swapChain.get(), core::int_cast<UInt32>(width), core::int_cast<UInt32>(height));
}

void Engine::draw(const GeometryViewPtr& geometryView, Int numIndices, Int numInstances) {
    if (!checkResourceIsValid_(geometryView)) {
        return;
    }
    if (numIndices == 0) {
        return;
    }
    if (numInstances < 0) {
        VGC_WARNING(
            LogVgcGraphics, "Negative numInstances ({}), skipping draw.", numInstances);
        return;
    }
    syncState_();
    Int n = (numIndices >= 0) ? numIndices : geometryView->numVertices();
    UInt un = core::int_cast<UInt>(n);
    queueLambdaCommandWithParameters_<GeometryView*>(
        "draw",
        [=](Engine* engine, GeometryView* gv) { engine->draw_(gv, un, numInstances); },
        geometryView.get());
}

void Engine::clear(const core::Color& color) {
    syncState_();
    queueLambdaCommandWithParameters_<core::Color>(
        "clear", [](Engine* engine, const core::Color& c) { engine->clear_(c); }, color);
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

void Engine::sanitize_(BufferCreateInfo& /*createInfo*/) {
    // XXX
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
    if (!createInfo.isMipGenerationEnabled() && createInfo.numMipLevels() == 0) {
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

    //const float numMipLevels = createInfo.numMipLevels();
    //if (numMipLevels <= 0 || numMipLevels > maxImageLayers) {
    //    VGC_ERROR(LogVgcGraphics, "Requested image layers ({}) should be in the range [1, {}]", numLayers, maxImageLayers);
    //}

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

void Engine::sanitize_(SamplerStateCreateInfo& /*createInfo*/) {
    // XXX
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
