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

#ifndef VGC_GRAPHICS_SWAPCHAIN_H
#define VGC_GRAPHICS_SWAPCHAIN_H

#include <atomic>

#include <vgc/core/arithmetic.h>
#include <vgc/graphics/api.h>
#include <vgc/graphics/enums.h>
#include <vgc/graphics/framebuffer.h>
#include <vgc/graphics/resource.h>

namespace vgc::graphics {

VGC_DECLARE_OBJECT(Engine);

enum class WindowNativeHandleType : uint8_t {
    None = 0,
    Win32,
    QOpenGLWindow,
};

class VGC_GRAPHICS_API SwapChainCreateInfo {
public:
    UInt32 width() const
    {
        return width_;
    }

    void setWidth(UInt32 width)
    {
        width_ = width;
    }

    UInt32 height() const
    {
        return height_;
    }

    void setHeight(UInt32 height)
    {
        height_ = height;
    }

    SwapChainTargetFormat format() const
    {
        return format_;
    }

    void setFormat(SwapChainTargetFormat format)
    {
        format_ = format;
    }

    void* windowNativeHandle() const
    {
        return windowNativeHandle_;
    }

    WindowNativeHandleType windowNativeHandleType() const
    {
        return windowNativeHandleType_;
    }

    void setWindowNativeHandle(
        void* windowNativeHandle, WindowNativeHandleType windowNativeHandleType)
    {
        windowNativeHandle_ = windowNativeHandle;
        windowNativeHandleType_ = windowNativeHandleType;
    }

    UInt32 windowed() const
    {
        return windowed_;
    }

    void setWindowed(UInt32 windowed)
    {
        windowed_ = windowed;
    }

    UInt8 numSamples() const
    {
        return numSamples_;
    }

    void setNumSamples(UInt8 numSamples)
    {
        numSamples_ = numSamples;
    }

    UInt8 numBuffers() const
    {
        return numBuffers_;
    }

    void setNumBuffers(UInt8 numBuffers)
    {
        numBuffers_ = numBuffers;
    }

    UInt flags() const
    {
        return flags_;
    }

    void setFlags(UInt flags)
    {
        flags_ = flags;
    }

private:
    UInt32 width_ = 100;
    UInt32 height_ = 100;
    SwapChainTargetFormat format_ = SwapChainTargetFormat::RGBA_8_UNORM;
    // XXX add support for sample quality ?
    void* windowNativeHandle_ = nullptr;
    WindowNativeHandleType windowNativeHandleType_ = WindowNativeHandleType::None;
    bool windowed_ = true;
    UInt8 numSamples_ = 1;
    UInt8 numBuffers_ = 2;
    UInt flags_ = 0;
};

/// \class vgc::graphics::SwapChain
/// \brief Abstract window swap buffers chain.
///
class VGC_GRAPHICS_API SwapChain : public Resource {
protected:
    friend Engine;
    using Resource::Resource;

    SwapChain(ResourceRegistry* registry, const SwapChainCreateInfo& createInfo)
        : Resource(registry), info_(createInfo)
    {
    }

public:
    const SwapChainCreateInfo& desc() const
    {
        return info_;
    }

    ImageFormat backBufferFormat() const
    {
        return swapChainTargetFormatToImageFormat(info_.format());
    }

    UInt32 numPendingPresents() const
    {
        return numPendingPresents_.load();
    }

    const FramebufferPtr& defaultFrameBuffer() const
    {
        return defaultFrameBuffer_;
    }

protected:
    void releaseSubResources_() override
    {
        defaultFrameBuffer_.reset();
    }

    FramebufferPtr defaultFrameBuffer_;

private:
    SwapChainCreateInfo info_;
    std::atomic_uint32_t numPendingPresents_ = 0; // to limit queuing in the Engine.
};
using SwapChainPtr = ResourcePtr<SwapChain>;

} // namespace vgc::graphics

#endif // VGC_GRAPHICS_SWAPCHAIN_H
