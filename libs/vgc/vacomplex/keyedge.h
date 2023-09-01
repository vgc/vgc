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
#include <utility>

#include <vgc/core/arithmetic.h>
#include <vgc/geometry/rect2d.h>
#include <vgc/geometry/stroke.h>
#include <vgc/vacomplex/api.h>
#include <vgc/vacomplex/cell.h>
#include <vgc/vacomplex/keyedgedata.h>
#include <vgc/vacomplex/keyvertex.h>

namespace vgc::vacomplex {

class KeyHalfedge;

class VGC_VACOMPLEX_API KeyEdge final : public SpatioTemporalCell<EdgeCell, KeyCell> {
private:
    friend detail::Operations;
    friend KeyEdgeData;

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

    KeyEdgeData* data() const {
        return data_.get();
    }

    geometry::CurveSamplingQuality samplingQuality() const {
        return samplingQuality_;
    }

    bool snapGeometry();

    std::shared_ptr<const geometry::StrokeSampling2d> strokeSamplingShared() const;
    const geometry::StrokeSampling2d& strokeSampling() const;
    const geometry::Rect2d& centerlineBoundingBox() const;

    /// Computes and returns a new array of samples for this edge according to the
    /// given `parameters`.
    ///
    /// Unlike `sampling()`, this function does not cache the result.
    ///
    geometry::StrokeSampling2d
    computeStrokeSampling(geometry::CurveSamplingQuality quality) const;

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

    std::unique_ptr<KeyEdgeData> data_ = {};
    //bool isClosed_ = false;

    std::unique_ptr<KeyEdgeData> stealData_() {
        detail::CellPropertiesPrivateInterface::setOwningCell(
            &data_->properties(), nullptr);
        return std::move(data_);
    }

    void setData_(std::unique_ptr<KeyEdgeData>&& data) {
        data_ = std::move(data);
        detail::CellPropertiesPrivateInterface::setOwningCell(&data_->properties(), this);
    }

    geometry::CurveSamplingQuality samplingQuality_ = {};
    mutable std::shared_ptr<const geometry::StrokeSampling2d> sampling_ = {};

    geometry::StrokeSampling2d
    computeStrokeSampling_(geometry::CurveSamplingQuality quality) const;
    void updateStrokeSampling_() const;

    void dirtyMesh_() override;
    bool updateGeometryFromBoundary_() override;

    void substituteKeyVertex_(KeyVertex* oldVertex, KeyVertex* newVertex) override;

    void substituteKeyHalfedge_(
        const KeyHalfedge& oldHalfedge,
        const KeyHalfedge& newHalfedge) override;

    void debugPrint_(core::StringWriter& out) override;

    geometry::StrokeSample2dArray
    computeInputSamples_(const geometry::CurveSamplingParameters& parameters) const;
};

} // namespace vgc::vacomplex

#endif // VGC_VACOMPLEX_KEYEDGE_H
