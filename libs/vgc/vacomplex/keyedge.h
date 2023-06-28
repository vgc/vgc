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

#ifndef VGC_VACOMPLEX_KEYEDGE_H
#define VGC_VACOMPLEX_KEYEDGE_H

#include <memory>

#include <vgc/core/arithmetic.h>
#include <vgc/geometry/curve.h>
#include <vgc/geometry/rect2d.h>
#include <vgc/geometry/vec2d.h>
#include <vgc/vacomplex/api.h>
#include <vgc/vacomplex/cell.h>
#include <vgc/vacomplex/edgegeometry.h>
#include <vgc/vacomplex/keyvertex.h>

namespace vgc::vacomplex {

class VGC_VACOMPLEX_API KeyEdge final : public SpatioTemporalCell<EdgeCell, KeyCell> {
private:
    friend detail::Operations;
    friend KeyEdgeGeometry;

    explicit KeyEdge(core::Id id, core::AnimTime t) noexcept
        : SpatioTemporalCell(id, t)
        , samplingQuality_(geometry::CurveSamplingQuality::AdaptiveLow) {
    }

public:
    VGC_VACOMPLEX_DEFINE_SPATIOTEMPORAL_CELL_CAST_METHODS(Key, Edge)

    virtual ~KeyEdge();

    KeyVertex* startVertex() const {
        return startVertex_;
    }

    KeyVertex* endVertex() const {
        return endVertex_;
    }

    KeyEdgeGeometry* geometry() const {
        return geometry_.get();
    }

    geometry::CurveSamplingQuality samplingQuality() const {
        return samplingQuality_;
    }

    void snapGeometry();

    std::shared_ptr<const EdgeSampling> samplingShared() const;
    const EdgeSampling& sampling() const;
    const geometry::Rect2d& centerlineBoundingBox() const;

    /// Computes and returns a new array of samples for this edge according to the
    /// given `parameters`.
    ///
    /// Unlike `sampling()`, this function does not cache the result.
    ///
    EdgeSampling computeSampling(geometry::CurveSamplingQuality quality) const;

    bool isStartVertex(VertexCell* v) const override {
        return v == startVertex_;
    }

    bool isEndVertex(VertexCell* v) const override {
        return v == endVertex_;
    }

    bool isClosed() const override {
        return !startVertex_;
    }

    /// Returns the angle, in radians and in the interval (-π, π],
    /// between the X axis and the start tangent.
    ///
    double startAngle() const;

    /// Returns the angle, in radians and in the interval (-π, π],
    /// between the X axis and the reversed end tangent.
    ///
    double endAngle() const;

private:
    KeyVertex* startVertex_ = nullptr;
    KeyVertex* endVertex_ = nullptr;

    // position and orientation when not bound to vertices ?
    //detail::Transform2d transform_;

    std::shared_ptr<KeyEdgeGeometry> geometry_ = {};
    //bool isClosed_ = false;

    geometry::CurveSamplingQuality samplingQuality_ = {};
    mutable std::shared_ptr<const EdgeSampling> sampling_ = {};

    EdgeSampling computeSampling_(geometry::CurveSamplingQuality quality) const;
    void updateSampling_() const;

    void dirtyMesh_() override;
    void onBoundaryMeshChanged_() override;

    geometry::StrokeSample2dArray
    computeInputSamples_(const geometry::CurveSamplingParameters& parameters) const;
};

} // namespace vgc::vacomplex

#endif // VGC_VACOMPLEX_KEYEDGE_H
