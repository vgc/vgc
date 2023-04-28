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

class VGC_VACOMPLEX_API EdgeSampling {
public:
    EdgeSampling() noexcept = default;

    explicit EdgeSampling(const geometry::CurveSampleArray& samples)
        : samples_(samples) {
    }

    explicit EdgeSampling(geometry::CurveSampleArray&& samples)
        : samples_(std::move(samples)) {
    }

    EdgeSampling(const EdgeSampling&) = delete;
    EdgeSampling& operator=(const EdgeSampling&) = delete;

    const geometry::CurveSampleArray& samples() const {
        return samples_;
    }

private:
    geometry::CurveSampleArray samples_ = {};
};

class VGC_VACOMPLEX_API KeyEdge final : public SpatioTemporalCell<EdgeCell, KeyCell> {
private:
    friend detail::Operations;

    explicit KeyEdge(core::Id id, core::AnimTime t) noexcept
        : SpatioTemporalCell(id, t)
        , samplingParameters_(geometry::CurveSamplingQuality::AdaptiveLow) {
    }

public:
    VGC_VACOMPLEX_DEFINE_SPATIOTEMPORAL_CELL_CAST_METHODS(Key, Edge)

    using SharedConstPoints = std::shared_ptr<const geometry::Vec2dArray>;
    using SharedConstWidths = std::shared_ptr<const core::DoubleArray>;

    KeyVertex* startVertex() const {
        return startVertex_;
    }

    KeyVertex* endVertex() const {
        return endVertex_;
    }

    const KeyEdgeGeometry* geometry() const {
        return geometry_.get();
    }

    //void setGeometryParameters(const KeyEdgeGeometryParameters& parameters);

    //

    //EdgeSampling computeSamplingAt(core::AnimTime /*t*/) {
    //    // XXX todo
    //    return EdgeSampling(-1);
    //}

    //void
    //setCachedSamplingParameters(const geometry::CurveSamplingParameters& parameters) {
    //    cachedSamplingParameters_ = parameters;
    //}

    const geometry::CurveSamplingParameters& samplingParameters() const {
        return samplingParameters_;
    }

    std::shared_ptr<const EdgeSampling> samplingShared() const;
    const EdgeSampling& sampling() const;
    const geometry::Rect2d& samplingBoundingBox() const;

    /// Computes and returns a new array of samples for this edge according to the
    /// given `parameters`.
    ///
    /// Unlike `sampling()`, this function does not cache the result.
    ///
    geometry::CurveSampleArray
    computeSampling(const geometry::CurveSamplingParameters& parameters) const;

    // XXX temporary, we should use geometry_.
    const geometry::Vec2dArray& points() const {
        return points_ ? *points_ : fallbackPoints_;
    }

    // XXX temporary, we should use geometry_.
    const core::DoubleArray& widths() const {
        return widths_ ? *widths_ : fallbackWidths_;
    }

    // XXX temporary, we should use geometry_.
    Int64 dataVersion() const {
        return dataVersion_;
    }

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

    // XXX temporary, we should use "KeyEdgeGeometry geometry_".
    SharedConstPoints points_;
    SharedConstWidths widths_;
    geometry::Vec2dArray fallbackPoints_;
    core::DoubleArray fallbackWidths_;
    Int64 dataVersion_ = 0;

    std::unique_ptr<KeyEdgeGeometry> geometry_ = {};
    //bool isClosed_ = false;

    geometry::CurveSamplingParameters samplingParameters_ = {};
    mutable geometry::CurveSampleArray inputSamples_ = {};
    mutable std::shared_ptr<const EdgeSampling> snappedSampling_ = {};
    mutable geometry::Rect2d snappedSamplingBbox_ = {};

    void dirtyInputSampling_();

    geometry::CurveSampleArray
    computeInputSamples_(const geometry::CurveSamplingParameters& parameters) const;
};

} // namespace vgc::vacomplex

#endif // VGC_VACOMPLEX_KEYEDGE_H
