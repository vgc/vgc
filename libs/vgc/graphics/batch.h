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

#ifndef VGC_GRAPHICS_BATCH_H
#define VGC_GRAPHICS_BATCH_H

#include <vgc/core/arithmetic.h>
#include <vgc/core/array.h>
#include <vgc/geometry/vec2f.h>
#include <vgc/geometry/rect2f.h>
#include <vgc/graphics/api.h>
#include <vgc/graphics/buffer.h>
#include <vgc/graphics/enums.h>
#include <vgc/graphics/font.h>
#include <vgc/graphics/geometryview.h>
#include <vgc/graphics/resource.h>

namespace vgc::graphics {

VGC_DECLARE_OBJECT(Engine);

/// \class vgc::graphics::GeometryBatch
/// \brief Batch of geometry data.
///
class VGC_GRAPHICS_API GeometryBatch : public Resource {
protected:
    using Resource::Resource;

    void releaseSubResources_() override {
        vertexBuffer_.reset();
        indexBuffer_.reset();
        view_.reset();
    }

private:
    BufferPtr vertexBuffer_;
    BufferPtr indexBuffer_;
    GeometryViewPtr view_;
};
using GeometryBatchPtr = ResourcePtr<GeometryBatch>;

namespace detail {

struct VGC_GRAPHICS_API TextAtlasVertex {
    geometry::Vec2f pos_;
    geometry::Vec2f size_;
    geometry::Rect2f preClip_;
    unsigned int glyphIndex_;
    unsigned int colorIndex_;
    //unsigned int clipIndex_;
};

} // namespace detail

class VGC_GRAPHICS_API TextAtlasResource : public Resource {
protected:
    friend Engine;

    using Resource::Resource;

    core::Array<detail::TextAtlasVertex> data_;
};

} // namespace vgc::graphics

#endif // VGC_GRAPHICS_BATCH_H
