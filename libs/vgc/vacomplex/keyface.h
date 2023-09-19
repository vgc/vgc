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

#ifndef VGC_VACOMPLEX_KEYFACE_H
#define VGC_VACOMPLEX_KEYFACE_H

#include <initializer_list>

#include <vgc/geometry/curves2d.h>
#include <vgc/geometry/stroke.h>
#include <vgc/vacomplex/api.h>
#include <vgc/vacomplex/cell.h>
#include <vgc/vacomplex/keycycle.h>
#include <vgc/vacomplex/keyfacedata.h>
#include <vgc/vacomplex/keyvertex.h>

namespace vgc::vacomplex {

namespace detail {

VGC_VACOMPLEX_API
core::Array<KeyCycle> computeKeyFaceCandidateAt(
    geometry::Vec2d position,
    Group* group,
    core::FloatArray& trianglesBuffer,
    geometry::WindingRule windingRule = geometry::WindingRule::Odd,
    core::AnimTime t = {});

VGC_VACOMPLEX_API
bool computeKeyFaceFillTriangles(
    const core::Array<KeyCycle>& cycles,
    core::FloatArray& trianglesBuffer,
    geometry::WindingRule windingRule = geometry::WindingRule::Odd);

VGC_VACOMPLEX_API
bool computeKeyFaceFillTriangles(
    const core::Array<KeyCycle>& cycles,
    core::FloatArray& trianglesBuffer,
    geometry::CurveSamplingQuality quality,
    geometry::WindingRule windingRule = geometry::WindingRule::Odd);

} // namespace detail

/// \class vgc::geometry::KeyFaceVertexUsageIndex
/// \brief Index of a vertex usage in a key face.
///
class VGC_VACOMPLEX_API KeyFaceVertexUsageIndex {
public:
    constexpr KeyFaceVertexUsageIndex() noexcept
        : cycleIndex_(-1)
        , componentIndex_(-1) {
    }

    VGC_WARNING_PUSH
    VGC_WARNING_MSVC_DISABLE(26495) // member variable uninitialized
    KeyFaceVertexUsageIndex(core::NoInit) noexcept {
    }
    VGC_WARNING_POP

    KeyFaceVertexUsageIndex(Int cycleIndex, Int componentIndex) noexcept
        : cycleIndex_(cycleIndex)
        , componentIndex_(componentIndex) {
    }

    constexpr Int cycleIndex() const {
        return cycleIndex_;
    }

    void setCycleIndex(Int cycleIndex) {
        cycleIndex_ = cycleIndex;
    }

    constexpr Int componentIndex() const {
        return componentIndex_;
    }

    void setComponentIndex(Int componentIndex) {
        componentIndex_ = componentIndex;
    }

    friend bool
    operator==(const KeyFaceVertexUsageIndex& lhs, const KeyFaceVertexUsageIndex& rhs) {
        return lhs.cycleIndex_ == rhs.cycleIndex_
               && lhs.componentIndex_ == rhs.componentIndex_;
    }

    friend bool
    operator!=(const KeyFaceVertexUsageIndex& lhs, const KeyFaceVertexUsageIndex& rhs) {
        return !(lhs == rhs);
    }

    friend bool
    operator<(const KeyFaceVertexUsageIndex& lhs, const KeyFaceVertexUsageIndex& rhs) {
        if (lhs.cycleIndex_ != rhs.cycleIndex_) {
            return lhs.cycleIndex_ < rhs.cycleIndex_;
        }
        return lhs.componentIndex_ < rhs.componentIndex_;
    }

private:
    Int cycleIndex_;
    Int componentIndex_;
};

/// \class vgc::vacomplex::KeyFace
/// \brief A Key-Face Cell.
///
class VGC_VACOMPLEX_API KeyFace final : public SpatioTemporalCell<FaceCell, KeyCell> {
private:
    friend detail::Operations;

    explicit KeyFace(core::Id id, core::AnimTime t) noexcept
        : SpatioTemporalCell(id, t)
        , data_(detail::KeyFacePrivateKey{}, this) {
    }

public:
    VGC_VACOMPLEX_DEFINE_SPATIOTEMPORAL_CELL_CAST_METHODS(Key, Face)

    const core::Array<KeyCycle>& cycles() const {
        return cycles_;
    }

    KeyVertex* vertex(const KeyFaceVertexUsageIndex& usageIndex) const;

    KeyVertex* vertexIfValid(const KeyFaceVertexUsageIndex& usageIndex) const;

    geometry::Rect2d boundingBox() const override;

    geometry::Rect2d boundingBoxAt(core::AnimTime t) const override {
        if (existsAt(t)) {
            return boundingBox();
        }
        return geometry::Rect2d::empty;
    }

    KeyFaceData& data() {
        return data_;
    }

    const KeyFaceData& data() const {
        return data_;
    }

private:
    core::Array<KeyCycle> cycles_;
    KeyFaceData data_;

    void substituteKeyVertex_(KeyVertex* oldVertex, KeyVertex* newVertex) override;

    void substituteKeyEdge_(
        const class KeyHalfedge& oldHalfedge,
        const class KeyHalfedge& newHalfedge) override;

    void debugPrint_(core::StringWriter& out) override;
};

} // namespace vgc::vacomplex

#endif // VGC_VACOMPLEX_KEYFACE_H
