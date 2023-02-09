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

#ifndef VGC_TOPOLOGY_KEYEDGE_H
#define VGC_TOPOLOGY_KEYEDGE_H

#include <vgc/core/arithmetic.h>
#include <vgc/geometry/curve.h>
#include <vgc/geometry/vec2d.h>
#include <vgc/topology/api.h>
#include <vgc/topology/cell.h>
#include <vgc/topology/edgegeometry.h>
#include <vgc/topology/keyvertex.h>

namespace vgc::topology {

class VGC_TOPOLOGY_API KeyEdge final : public SpatioTemporalCell<EdgeCell, KeyCell> {
private:
    friend detail::Operations;

    explicit KeyEdge(core::Id id, core::AnimTime t) noexcept
        : SpatioTemporalCell(id, t) {
    }

public:
    VGC_TOPOLOGY_DEFINE_SPATIOTEMPORAL_CELL_CAST_METHODS(Key, Edge)

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
        return v == startVertex_->toVertexCellUnchecked();
    }

    bool isEndVertex(VertexCell* v) const override {
        return v == endVertex_->toVertexCellUnchecked();
    }

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
};

} // namespace vgc::topology

#endif // VGC_TOPOLOGY_KEYEDGE_H
