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
#include <vgc/graphics/image.h>
#include <vgc/graphics/imageview.h>
#include <vgc/graphics/resource.h>

namespace vgc::graphics {

VGC_DECLARE_OBJECT(Engine);

enum class WindowNativeHandleType : uint8_t {
    None = 0,
    Win32,
    QOpenGLWindow,
};

// XXX do we let width/height configurable ?
//     shouldn't it match the window size anyway ?

class VGC_GRAPHICS_API SwapChainCreateInfo {
public:
    using FlagsType = UInt64;

    Int width() const {
        return width_;
    }

    void setWidth(Int width) {
        width_ = width;
    }

    Int height() const {
        return height_;
    }

    void setHeight(Int height) {
        height_ = height;
    }

    void* windowNativeHandle() const {
        return windowNativeHandle_;
    }

    WindowNativeHandleType windowNativeHandleType() const {
        return windowNativeHandleType_;
    }

    void setWindowNativeHandle(
        void* windowNativeHandle,
        WindowNativeHandleType windowNativeHandleType) {

        windowNativeHandle_ = windowNativeHandle;
        windowNativeHandleType_ = windowNativeHandleType;
    }

    bool isWindowed() const {
        return isWindowed_;
    }

    void setWindowed(bool isWindowed) {
        isWindowed_ = isWindowed;
    }

    FlagsType flags() const {
        return flags_;
    }

    void setFlags(FlagsType flags) {
        flags_ = flags;
    }

private:
    Int width_ = 100;
    Int height_ = 100;
    void* windowNativeHandle_ = nullptr;
    WindowNativeHandleType windowNativeHandleType_ = WindowNativeHandleType::None;
    bool isWindowed_ = true;
    FlagsType flags_ = 0;
};

/// \class vgc::graphics::SwapChain
/// \brief Abstract window swap buffers chain.
///
class VGC_GRAPHICS_API SwapChain : public Resource {
protected:
    friend Engine;
    using Resource::Resource;

    SwapChain(ResourceRegistry* registry, const SwapChainCreateInfo& createInfo)

        : Resource(registry)
        , info_(createInfo) {
    }

public:
    const SwapChainCreateInfo& createInfo() const {
        return info_;
    }

    Int numPendingPresents() const {
        // static_cast is safe because value is always small
        return static_cast<Int>(numPendingPresents_.load());
    }

private:
    SwapChainCreateInfo info_;

    std::atomic_uint32_t numPendingPresents_ = 0; // to limit queuing in the Engine.
};
using SwapChainPtr = ResourcePtr<SwapChain>;

} // namespace vgc::graphics

#endif // VGC_GRAPHICS_SWAPCHAIN_H
