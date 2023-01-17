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

#ifndef VGC_GRAPHICS_D3D11_D3D11ENGINE_H
#define VGC_GRAPHICS_D3D11_D3D11ENGINE_H

#include <vgc/core/os.h>
#ifdef VGC_CORE_OS_WINDOWS

//#define USE_DXGI_VERSION_1_2

// clang-format off

#include <d3d11.h>
#include <dxgi1_2.h>

#include <array>
#include <chrono>
#include <memory>

#include <vgc/core/color.h>
#include <vgc/core/paths.h>
#include <vgc/geometry/mat4d.h>
#include <vgc/geometry/mat4f.h>
#include <vgc/geometry/vec2d.h>
#include <vgc/geometry/vec2f.h>
#include <vgc/graphics/api.h>
#include <vgc/graphics/engine.h>

#include <vgc/graphics/detail/comptr.h>

// clang-format on

namespace vgc::graphics {

VGC_DECLARE_OBJECT(D3d11Engine);

/// \class vgc::widget::D3d11Engine
/// \brief The D3D11-based graphics::Engine.
///
/// This class is an implementation of Engine using Direct3D 11.0.
///
class VGC_GRAPHICS_API D3d11Engine final : public Engine {
private:
    VGC_OBJECT(D3d11Engine, Engine)

protected:
    explicit D3d11Engine(const EngineCreateInfo& createInfo);

    void onDestroyed() override;

public:
    /// Creates a new D3d11Engine.
    ///
    static D3d11EnginePtr create(const EngineCreateInfo& createInfo);

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

    //--  RENDER THREAD implementation functions --

    void initContext_() override;
    void initBuiltinResources_() override;

    void initFramebuffer_(Framebuffer* framebuffer) override;
    void initBuffer_(Buffer* buffer, const char* data, Int lengthInBytes) override;

    void initImage_( //
        Image* image,
        const Span<const char>* mipLevelDataSpans,
        Int count) override;

    void initImageView_(ImageView* view) override;
    void initSamplerState_(SamplerState* state) override;
    void initGeometryView_(GeometryView* view) override;
    void initBlendState_(BlendState* state) override;
    void initRasterizerState_(RasterizerState* state) override;

    void setSwapChain_(const SwapChainPtr& swapChain) override;
    void setFramebuffer_(const FramebufferPtr& framebuffer) override;
    void setViewport_(Int x, Int y, Int width, Int height) override;
    void setProgram_(const ProgramPtr& program) override;

    void setBlendState_( //
        const BlendStatePtr& state,
        const geometry::Vec4f& constantFactors) override;

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

    void draw_(GeometryView* view, UInt numIndices, UInt numInstances) override;
    void clear_(const core::Color& color) override;

    UInt64
    present_(SwapChain* swapChain, UInt32 syncInterval, PresentFlags flags) override;

private:
#    ifdef USE_DXGI_VERSION_1_2
    ComPtr<IDXGIFactory2> factory_;
#    else
    ComPtr<IDXGIFactory> factory_;
#    endif
    ComPtr<ID3D11Device> device_;
    ComPtr<ID3D11DeviceContext> deviceCtx_;
    ComPtr<ID3D11DepthStencilState> depthStencilState_;
    std::array<ComPtr<ID3D11InputLayout>, numBuiltinGeometryLayouts> builtinLayouts_;
    ID3D11InputLayout* layout_ = nullptr;
    D3D11_PRIMITIVE_TOPOLOGY topology_ = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

    template<typename T, typename... Args>
    [[nodiscard]] std::unique_ptr<T> makeUnique(Args&&... args) {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }

    bool loadBuffer_(class D3d11Buffer* buffer, const void* data, Int dataSize);
    void onBufferRecreated_(class D3d11Buffer* buffer);
    bool writeBufferReserved_(ID3D11Buffer* object, const void* data, Int dataSize);

    // to support resizing buffers
    std::array<StageConstantBufferArray, numShaderStages> boundConstantBufferArrays_;
    std::array<StageImageViewArray, numShaderStages> boundImageViewArrays_;
    SwapChainPtr currentSwapchain_;
    FramebufferPtr boundFramebuffer_;
};

} // namespace vgc::graphics

#endif // VGC_CORE_OS_WINDOWS
#endif // VGC_GRAPHICS_D3D11_D3D11ENGINE_H
