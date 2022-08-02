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

#ifndef VGC_GRAPHICS_FRAMEBUFFER_H
#define VGC_GRAPHICS_FRAMEBUFFER_H

#include <vgc/core/arithmetic.h>
#include <vgc/core/object.h>
#include <vgc/graphics/api.h>
#include <vgc/graphics/enums.h>
#include <vgc/graphics/resource.h>

namespace vgc::graphics {

VGC_DECLARE_OBJECT(Engine);

class VGC_GRAPHICS_API Viewport {
public:
    Viewport(Int x, Int y, Int width, Int height)
        : x_(x) , y_(y) , width_(width) , height_(height)
    {
    }

    Int x() const
    {
        return x_;
    }

    Int y() const
    {
        return y_;
    }

    Int width() const
    {
        return width_;
    }

    Int height() const
    {
        return height_;
    }

private:
    Int x_;
    Int y_;
    Int width_;
    Int height_;
};

/// \class vgc::graphics::Framebuffer
/// \brief Abstract framebuffer, a collection of graphics pipeline output attachments.
///
// Since a swap chain's render target view represents different buffers over
// time, a Vulkan implementation should probably cache a vkFramebuffer for each
// back buffer.
//
class VGC_GRAPHICS_API Framebuffer : public Resource {
protected:
    Framebuffer(ResourceRegistry* registry)
        : Resource(registry) {}

    using Resource::Resource;

    // can save batch here !
};
using FramebufferPtr = ResourcePtr<Framebuffer>;

} // namespace vgc::graphics

#endif // VGC_GRAPHICS_FRAMEBUFFER_H
