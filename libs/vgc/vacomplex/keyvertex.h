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

#ifndef VGC_VACOMPLEX_KEYVERTEX_H
#define VGC_VACOMPLEX_KEYVERTEX_H

#include <vgc/geometry/vec2d.h>
#include <vgc/vacomplex/api.h>
#include <vgc/vacomplex/cell.h>
#include <vgc/vacomplex/keyhalfedge.h>

namespace vgc::vacomplex {

class VGC_VACOMPLEX_API RingKeyHalfedge {
public:
    RingKeyHalfedge(KeyEdge* ke, bool direction) noexcept
        : halfedge_(ke, direction) {

        angle_ = halfedge_.startAngle();
    }

    operator const KeyHalfedge&() const {
        return halfedge_;
    }

    const KeyHalfedge& halfedge() const {
        return halfedge_;
    }

    KeyEdge* edge() const {
        return halfedge_.edge();
    }

    bool direction() const {
        return halfedge_.direction();
    }

    KeyVertex* endVertex() const {
        return halfedge_.endVertex();
    }

    /// Returns the angle between the x-axis and the start tangent.
    double angle() const {
        return angle_;
    }

    friend bool operator<(const RingKeyHalfedge& lhs, const RingKeyHalfedge& rhs) {
        if (lhs.angle_ != rhs.angle_) {
            return lhs.angle_ < rhs.angle_;
        }
        Int id1 = lhs.halfedge_.edge()->id();
        Int id2 = rhs.halfedge_.edge()->id();
        if (id1 != id2) {
            return id1 < id2;
        }
        // assumes `lhs.khe.direction() != rhs.khe.direction()`
        return lhs.halfedge_.direction();
    }

    friend bool operator==(const RingKeyHalfedge& rh1, const RingKeyHalfedge& rh2) {
        return rh1.halfedge_ == rh2.halfedge_;
    }

    friend bool operator!=(const RingKeyHalfedge& rh1, const RingKeyHalfedge& rh2) {
        return !(rh1 == rh2);
    }

private:
    KeyHalfedge halfedge_;
    double angle_;
};

// dev note: position could be a variant<vec2d, func, provider>
//           provider could have a dirty flag to not update data, especially important for
//           big value types like curve geometry in edges.

class VGC_VACOMPLEX_API KeyVertex final : public SpatioTemporalCell<VertexCell, KeyCell> {
private:
    friend detail::Operations;

    explicit KeyVertex(core::Id id, core::AnimTime t) noexcept
        : SpatioTemporalCell(id, t) {
    }

public:
    VGC_VACOMPLEX_DEFINE_SPATIOTEMPORAL_CELL_CAST_METHODS(Key, Vertex)

    geometry::Vec2d position() const {
        onMeshQueried();
        return position_;
    }

    geometry::Vec2d position(core::AnimTime /*t*/) const override {
        onMeshQueried();
        return position_;
    }

    geometry::Rect2d boundingBox() const override;

    geometry::Rect2d boundingBoxAt(core::AnimTime t) const override {
        if (existsAt(t)) {
            return boundingBox();
        }
        return geometry::Rect2d::empty;
    }

    /// Computes the ring of outgoing halfedges sorted so that iterating
    /// over it is equivalent to doing `halfedge = halfedge.previous().opposite()`.
    ///
    core::Array<RingKeyHalfedge> computeRingHalfedges() const;

private:
    geometry::Vec2d position_;

    void substituteKeyVertex_(KeyVertex* oldVertex, KeyVertex* newVertex) override;

    void substituteKeyEdge_(
        const KeyHalfedge& oldHalfedge,
        const KeyHalfedge& newHalfedge) override;

    void debugPrint_(core::StringWriter& out) override;
};

} // namespace vgc::vacomplex

#endif // VGC_VACOMPLEX_KEYVERTEX_H
