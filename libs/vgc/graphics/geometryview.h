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

#ifndef VGC_GRAPHICS_GEOMETRYVIEW_H
#define VGC_GRAPHICS_GEOMETRYVIEW_H

#include <array>

#include <vgc/core/arithmetic.h>
#include <vgc/graphics/api.h>
#include <vgc/graphics/enums.h>
#include <vgc/graphics/resource.h>
#include <vgc/graphics/buffer.h>

namespace vgc::graphics {

class GeometryView;

inline constexpr size_t maxAttachedVertexBuffers = 4;

using VertexBufferArray = std::array<BufferPtr, maxAttachedVertexBuffers>;
using VertexBufferStridesArray = std::array<UInt32, maxAttachedVertexBuffers>;
using VertexBufferOffsetsArray = std::array<UInt32, maxAttachedVertexBuffers>;

/// \class vgc::graphics::GeometryViewCreateInfo
/// \brief Parameters for geometry view creation.
///
class VGC_GRAPHICS_API GeometryViewCreateInfo {
public:
    GeometryViewCreateInfo() noexcept = default;

    PrimitiveType primitiveType() const
    {
        return primitiveType_;
    }

    void setPrimitiveType(PrimitiveType primitiveType)
    {
        primitiveType_ = primitiveType;
    }

    BuiltinGeometryLayout builtinGeometryLayout() const
    {
        return builtinGeometryLayout_;
    }

    void setBuiltinGeometryLayout(BuiltinGeometryLayout builtinGeometryLayout)
    {
        builtinGeometryLayout_ = builtinGeometryLayout;
    }

    const BufferPtr& indexBuffer() const
    {
        return indexBuffer_;
    }

    void setIndexBuffer(const BufferPtr& indexBuffer)
    {
        indexBuffer_ = indexBuffer;
    }

    const VertexBufferArray& vertexBuffers() const
    {
        return vertexBuffers_;
    }

    const BufferPtr& vertexBuffer(Int i) const
    {
        return vertexBuffers_[i];
    }

    void setVertexBuffer(Int i, const BufferPtr& vertexBuffer)
    {
        size_t idx = core::int_cast<size_t>(i);
        if (idx >= vertexBuffers_.size()) {
            throw core::IndexError(core::format(
                "Vertex buffer index {} is out of range [0, {}]", i, vertexBuffers_.size() - 1));
        }
        vertexBuffers_[idx] = vertexBuffer;
    }

    const VertexBufferStridesArray& strides() const
    {
        return strides_;
    }

    void setStride(Int i, UInt32 stride)
    {
        size_t idx = core::int_cast<size_t>(i);
        if (idx >= strides_.size()) {
            throw core::IndexError(core::format(
                "Stride index {} is out of range [0, {}]", i, strides_.size() - 1));
        }
        strides_[idx] = stride;
    }

    const VertexBufferOffsetsArray& offsets() const
    {
        return offsets_;
    }

    void setOffset(Int i, UInt32 offset)
    {
        size_t idx = core::int_cast<size_t>(i);
        if (idx >= offsets_.size()) {
            throw core::IndexError(core::format(
                "Offset index {} is out of range [0, {}]", i, offsets_.size() - 1));
        }
        offsets_[idx] = offset;
    }

private:
    friend GeometryView; // to reset resource pointers

    PrimitiveType primitiveType_ = PrimitiveType::Point;
    BuiltinGeometryLayout builtinGeometryLayout_ = BuiltinGeometryLayout::None;
    BufferPtr indexBuffer_ = {};
    VertexBufferArray vertexBuffers_ = {};
    VertexBufferStridesArray strides_ = {};
    VertexBufferOffsetsArray offsets_ = {};
};

/// \class vgc::graphics::GeometryView
/// \brief View on a sequence of primitives.
///
/// View on a sequence of primitives of type point, line, or triangle.
/// Vertices can have different components layed out in arrays.
/// These arrays can be interleaved and stored in a one or multiple buffers.
/// The layout must be described by a builtin enum or in the future by a generic
/// descriptor structure.
///
class VGC_GRAPHICS_API GeometryView : public Resource {
protected:
    friend Engine;

    GeometryView(ResourceRegistry* registry, const GeometryViewCreateInfo& info)
        : Resource(registry), info_(info)
    {
        // XXX check buffers against layout (slots, alignment, ..)

        for (const BufferPtr& vb : info_.vertexBuffers()) {
            if (vb && !(vb->bindFlags() & BindFlag::VertexBuffer)) {
                throw core::LogicError("Buffer needs BindFlags::VertexBuffer flag to be used as a vertex buffer");
            }
        }
        const BufferPtr& ib = info_.indexBuffer();
        if (ib && !(ib->bindFlags() & BindFlag::IndexBuffer)) {
            throw core::LogicError("Buffer needs BindFlags::IndexBuffer flag to be used as an index buffer");
        }

        BuiltinGeometryLayout builtinLayout = info.builtinGeometryLayout();
        if (builtinLayout != BuiltinGeometryLayout::None) {
            Int layoutIndex = core::toUnderlying(builtinLayout);
            if (info_.strides_[0] == 0) {
                info_.strides_[0] = std::array{
                    2 * 4, // XY
                    5 * 4, // XYRGB
                    3 * 4, // XYZ
                }[layoutIndex];
            }
        }
    }

public:
    PrimitiveType primitiveType() const
    {
        return info_.primitiveType();
    }

    BuiltinGeometryLayout builtinGeometryLayout() const
    {
        return info_.builtinGeometryLayout();
    }

    const BufferPtr& indexBuffer() const
    {
        return info_.indexBuffer();
    }

    const VertexBufferArray& vertexBuffers() const
    {
        return info_.vertexBuffers();
    }

    const BufferPtr& vertexBuffer(Int i) const
    {
        return info_.vertexBuffer(i);
    }

    const VertexBufferStridesArray& strides() const
    {
        return info_.strides();
    }

    const VertexBufferOffsetsArray& offsets() const
    {
        return info_.offsets();
    }

    const Int vertexSizeInBuffer(Int i) const
    {
        BuiltinGeometryLayout builtinLayout = info_.builtinGeometryLayout();
        if (builtinLayout != BuiltinGeometryLayout::None) {
            Int layoutIndex = core::toUnderlying(builtinLayout);
            if (i == 0) {
                return std::array{
                    2 * 4, // XY
                    5 * 4, // XYRGB
                    3 * 4, // XYZ
                }[layoutIndex];
            }
            return 0;
        }
        return -1;
    }

    const Int numVertices() const
    {
        Int elementSize = vertexSizeInBuffer(0);
        return info_.vertexBuffers()[0]->lengthInBytes() / elementSize;
    }

protected:
    void releaseSubResources_() override
    {
        for (BufferPtr& vb : info_.vertexBuffers_) {
            vb.reset();
        }
        info_.indexBuffer_.reset();
    }

private:
    GeometryViewCreateInfo info_;
};
using GeometryViewPtr = ResourcePtr<GeometryView>;

} // namespace vgc::graphics

#endif // VGC_GRAPHICS_GEOMETRYVIEW_H
