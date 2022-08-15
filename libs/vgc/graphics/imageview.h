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

#ifndef VGC_GRAPHICS_IMAGEVIEW_H
#define VGC_GRAPHICS_IMAGEVIEW_H

#include <vgc/core/arithmetic.h>
#include <vgc/graphics/api.h>
#include <vgc/graphics/buffer.h>
#include <vgc/graphics/enums.h>
#include <vgc/graphics/image.h>
#include <vgc/graphics/resource.h>

namespace vgc::graphics {

VGC_DECLARE_OBJECT(Engine);

/// \class vgc::graphics::ImageViewCreateInfo
/// \brief Parameters for image view creation.
///
class VGC_GRAPHICS_API ImageViewCreateInfo {
public:
    constexpr ImageViewCreateInfo() noexcept = default;

    Int firstLayer() const {
        return firstLayer_;
    }

    // texture views are not supported by Opengl until core 4.3
    //
    //void setFirstLayer(UInt8 firstLayer)
    //{
    //    firstLayer_ = firstLayer;
    //}

    Int numLayers() const {
        return numLayers_;
    }

    // texture views are not supported by Opengl until core 4.3
    //
    //void setNumLayers(UInt8 numLayers)
    //{
    //    numLayers_ = numLayers;
    //}

    Int lastLayer() const {
        // XXX test overflow
        return firstLayer_ + numLayers_ - 1;
    }

    Int firstMipLevel() const {
        return firstMipLevel_;
    }

    // texture views are not supported by Opengl until core 4.3
    //void setFirstMipLevel(UInt8 firstMipLevel)
    //{
    //    firstMipLevel_ = firstMipLevel;
    //}

    Int numMipLevels() const {
        return numMipLevels_;
    }

    // texture views are not supported by Opengl until core 4.3
    // Only effective when binding as a shader resource.
    //
    //void setNumMipLevels(UInt8 numMipLevels)
    //{
    //    numMipLevels_ = numMipLevels;
    //}

    Int lastMipLevel() const {
        // XXX test overflow
        return firstMipLevel_ + numMipLevels_ - 1;
    }

    ImageBindFlags bindFlags() const {
        return bindFlags_;
    }

    void setBindFlags(ImageBindFlags bindFlags) {
        bindFlags_ = bindFlags;
    }

private:
    Int firstLayer_ = 0;
    Int numLayers_ = 1;
    Int firstMipLevel_ = 0;
    Int numMipLevels_ = 1;

    ImageBindFlags bindFlags_ = ImageBindFlag::None;
};

/// \class vgc::graphics::ImageView
/// \brief Abstract view of an image buffer attachable to some stage of the graphics pipeline.
///
// Since a swap chain's render target view represents different buffers over
// time, a Vulkan implementation should probably cache a view for each
// back buffer.
//
// Concept mapping:
//  D3D11  -> Shader Resource View (SRV), Render Target View (RTV), Depth Stencil View (DSV)
//  OpenGL -> Texture
//  Vulkan -> Image View
// Looks like all three support buffers as image.
//
class VGC_GRAPHICS_API ImageView : public Resource {
protected:
    friend Engine;

    ImageView(
        ResourceRegistry* registry,
        const ImageViewCreateInfo& createInfo,
        const ImagePtr& image)

        : Resource(registry)
        , info_(createInfo)
        , viewedResource_(image)
        , pixelFormat_(image->pixelFormat()) {
    }

    ImageView(
        ResourceRegistry* registry,
        const ImageViewCreateInfo& createInfo,
        const BufferPtr& buffer,
        PixelFormat pixelFormat,
        Int numBufferElements)

        : Resource(registry)
        , info_(createInfo)
        , viewedResource_(buffer)
        , pixelFormat_(pixelFormat)
        , numBufferElements_(core::int_cast<UInt32>(numBufferElements)) {
    }

public:
    Int firstLayer() const {
        return info_.firstLayer();
    }

    Int numLayers() const {
        return info_.numLayers();
    }

    Int lastLayer() const {
        return info_.lastLayer();
    }

    Int firstMipLevel() const {
        return info_.firstMipLevel();
    }

    Int numMipLevels() const {
        return info_.numMipLevels();
    }

    Int lastMipLevel() const {
        return info_.lastMipLevel();
    }

    ImageBindFlags bindFlags() const {
        return info_.bindFlags();
    }

    PixelFormat pixelFormat() const {
        return pixelFormat_;
    }

    Int numBufferElements() const {
        return static_cast<Int>(numBufferElements_);
    }

    bool isBuffer() const {
        return numBufferElements_ > 0;
    }

    ResourcePtr<Buffer> viewedBuffer() const {
        return isBuffer() ? static_pointer_cast<Buffer>(viewedResource_) : nullptr;
    }

    ResourcePtr<Image> viewedImage() const {
        return isBuffer() ? nullptr : static_pointer_cast<Image>(viewedResource_);
    }

protected:
    void releaseSubResources_() override {
        viewedResource_.reset();
    }

private:
    ImageViewCreateInfo info_;
    ResourcePtr<Resource> viewedResource_;
    PixelFormat pixelFormat_;
    UInt32 numBufferElements_ = 0;
};
using ImageViewPtr = ResourcePtr<ImageView>;

} // namespace vgc::graphics

#endif // VGC_GRAPHICS_IMAGEVIEW_H
