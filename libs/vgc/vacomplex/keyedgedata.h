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

#ifndef VGC_VACOMPLEX_EDGEGEOMETRY_H
#define VGC_VACOMPLEX_EDGEGEOMETRY_H

#include <array>
#include <memory>

#include <vgc/core/arithmetic.h>
#include <vgc/geometry/mat3d.h>
#include <vgc/geometry/stroke.h> // AbstractStroke2d
#include <vgc/vacomplex/api.h>
#include <vgc/vacomplex/celldata.h>

// how to share edge shape correctly ?
// an inbetween edge that doesn't change should have the same shape for all times
// we also need edge shape source/def, which can be different curve types
// -> EdgeParameters ?

namespace vgc::vacomplex {

class KeyEdge;
class KeyEdgeData;

namespace detail {

class Operations;

}

/// \class vgc::vacomplex::KeyEdgeData
/// \brief Authored model of the edge geometry.
///
/// It can be translated from dom or set manually.
///
// Dev Notes:
// Edge geometry is relative to end vertices position.
// We want to snap the source geometry in its own space when:
//    - releasing a dragged end vertex
//    - right before sculpting
//    - right before control point dragging
// We have to snap output geometry (sampling) when the source
// geometry is not already snapped (happens in many cases).
//
// In which space do we sample ?
// inbetweening -> common ancestor for best identification of interest points
//
class VGC_VACOMPLEX_API KeyEdgeData final : public CellData {
private:
    friend detail::Operations;
    friend KeyEdge;

public:
    KeyEdgeData(bool isClosed)
        : isClosed_(isClosed) {
    }

    KeyEdgeData(const KeyEdgeData& other);
    KeyEdgeData(KeyEdgeData&& other) noexcept;
    KeyEdgeData& operator=(const KeyEdgeData& other);
    KeyEdgeData& operator=(KeyEdgeData&& other) noexcept;

    ~KeyEdgeData() override;

    std::unique_ptr<KeyEdgeData> clone() const;

    KeyEdge* keyEdge() const;

    bool isClosed() const {
        return isClosed_;
    }

    const geometry::AbstractStroke2d* stroke() const;

    void setStroke(const geometry::AbstractStroke2d* stroke);
    void setStroke(std::unique_ptr<geometry::AbstractStroke2d>&& stroke);

    /// Expects positions in object space.
    ///
    void snap(
        const geometry::Vec2d& snapStartPosition,
        const geometry::Vec2d& snapEndPosition,
        geometry::CurveSnapTransformationMode mode =
            geometry::CurveSnapTransformationMode::LinearInArclength);

    /// Expects delta in object space.
    ///
    void translate(const geometry::Vec2d& delta);

    /// Expects transformation in object space.
    ///
    void transform(const geometry::Mat3d& transformation);

    static std::unique_ptr<KeyEdgeData> fromConcatStep(
        const KeyHalfedgeData& khd1,
        const KeyHalfedgeData& khd2,
        bool smoothJoin);

    void finalizeConcat();

    static std::unique_ptr<KeyEdgeData>
    fromGlueOpen(core::ConstSpan<KeyHalfedgeData> khds);

    static std::unique_ptr<KeyEdgeData> fromGlueClosed(
        core::ConstSpan<KeyHalfedgeData> khds,
        core::ConstSpan<double> uOffsets);

    static std::unique_ptr<KeyEdgeData> fromGlue(
        core::ConstSpan<KeyHalfedgeData> khds,
        std::unique_ptr<geometry::AbstractStroke2d>&& gluedStroke);

private:
    std::unique_ptr<geometry::AbstractStroke2d> stroke_;
    // In case stroke_ == nullptr.
    bool isClosed_;
};

//std::shared_ptr<const EdgeSampling> snappedSampling_;
//virtual EdgeSampling computeSampling() = 0;

// key edge
//   geometry as pointer or type, but if it's a type it could be integrated to key edge...
//   if pointer then poly or inner pointer again ? poly is more efficient..

} // namespace vgc::vacomplex

#endif // VGC_VACOMPLEX_EDGEGEOMETRY_H
