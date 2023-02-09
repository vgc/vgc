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

#ifndef VGC_WORKSPACE_VERTEX_H
#define VGC_WORKSPACE_VERTEX_H

#include <vgc/core/arithmetic.h>
#include <vgc/core/array.h>
#include <vgc/core/span.h>
#include <vgc/dom/element.h>
#include <vgc/topology/vac.h>
#include <vgc/workspace/api.h>
#include <vgc/workspace/element.h>

namespace vgc::workspace {

class VacVertexCell;
class VacEdgeCell;
class VacEdgeCellFrameData;

namespace detail {

// references an incident halfedge in a join
class VGC_WORKSPACE_API VacJoinHalfedge {
public:
    friend VacVertexCell;

    VacJoinHalfedge(VacEdgeCell* edgeCell, bool isReverse, Int32 group)
        : edgeCell_(edgeCell)
        , group_(group)
        , isReverse_(isReverse) {
    }

    constexpr VacEdgeCell* edgeCell() const {
        return edgeCell_;
    }

    constexpr bool isReverse() const {
        return isReverse_;
    }

    struct GrouplessEqualTo {
        constexpr bool
        operator()(const VacJoinHalfedge& lhs, const VacJoinHalfedge& rhs) const {
            return lhs.edgeCell_ == rhs.edgeCell_ && lhs.isReverse_ == rhs.isReverse_;
        }
    };

private:
    VacEdgeCell* edgeCell_;
    Int32 group_;
    bool isReverse_;
};

class VGC_WORKSPACE_API VacJoinHalfedgeFrameData {
public:
    friend VacVertexCell;

    VacJoinHalfedgeFrameData(const VacJoinHalfedge& halfedge)
        : halfedge_(halfedge) {
    }

    const VacJoinHalfedge& halfedge() const {
        return halfedge_;
    }

    constexpr double angle() const {
        return angle_;
    }

    constexpr double angleToNext() const {
        return angleToNext_;
    }

private:
    VacJoinHalfedge halfedge_;
    VacEdgeCellFrameData* edgeData_ = nullptr;
    geometry::Vec2d outgoingTangent_ = {};
    double angle_ = 0.0;
    double angleToNext_ = 0.0;
};

class VGC_WORKSPACE_API VacVertexCellFrameData {
public:
    friend VacVertexCell;

    VacVertexCellFrameData(const core::AnimTime& t)
        : time_(t) {
    }

    const core::AnimTime& time() const {
        return time_;
    }

    bool hasPosData() const {
        return isJoinComputed_;
    }

    const geometry::Vec2d& pos() const {
        return pos_;
    }

    bool hasJoinData() const {
        return isJoinComputed_;
    }

    void clearJoinData() {
        debugLinesRenderGeometry_.reset();
        debugQuadRenderGeometry_.reset();
        halfedgesData_.clear();
        isComputing_ = false;
        isJoinComputed_ = false;
    }

    void debugPaint(graphics::Engine* engine);

private:
    core::AnimTime time_;
    geometry::Vec2d pos_ = core::noInit;
    mutable graphics::GeometryViewPtr debugLinesRenderGeometry_;
    mutable graphics::GeometryViewPtr debugQuadRenderGeometry_;
    // join data
    core::Array<detail::VacJoinHalfedgeFrameData> halfedgesData_;
    bool isComputing_ = false;
    bool isJoinComputed_ = false;
    bool isPosComputed_ = false;
};

} // namespace detail

class VGC_WORKSPACE_API VacVertexCell : public VacElement {
private:
    friend class Workspace;
    friend class VacEdgeCell;
    friend class VacKeyEdge;
    friend class VacInbetweenVEdge;

protected:
    VacVertexCell(Workspace* workspace, dom::Element* domElement)
        : VacElement(workspace, domElement) {
    }

public:
    vacomplex::VertexCell* vacVertexCellNode() const {
        return vacCellUnchecked()->toVertexCellUnchecked();
    }

    void computeJoin(core::AnimTime t);

protected:
    void paint_(
        graphics::Engine* engine,
        core::AnimTime t,
        PaintOptions flags = PaintOption::None) const override;

    detail::VacVertexCellFrameData* frameData(core::AnimTime t) const;
    void computePos(detail::VacVertexCellFrameData& data);
    void computeJoin(detail::VacVertexCellFrameData& data);

    void rebuildJoinHalfedgesArray() const;
    void clearJoinHalfedgesJoinData() const;

    void addJoinHalfedge_(const detail::VacJoinHalfedge& joinHalfedge);
    void removeJoinHalfedge_(const detail::VacJoinHalfedge& joinHalfedge);

    void onInputGeometryChanged();
    void onJoinEdgeGeometryChanged_(VacEdgeCell* edge);

private:
    mutable core::Array<detail::VacJoinHalfedge> joinHalfedges_;
    mutable core::Array<detail::VacVertexCellFrameData> frameDataEntries_;
};

class VGC_WORKSPACE_API VacKeyVertex : public VacVertexCell {
private:
    friend class Workspace;
    friend class VacEdgeCell;
    friend class VacKeyEdge;
    friend class VacInbetweenVEdge;

public:
    ~VacKeyVertex() override = default;

    VacKeyVertex(Workspace* workspace, dom::Element* domElement)
        : VacVertexCell(workspace, domElement) {
    }

    topology::KeyVertex* vacKeyVertexNode() const {
        return vacCellUnchecked()->toKeyVertexUnchecked();
    }

    geometry::Rect2d boundingBox(core::AnimTime t) const override;

protected:
    ElementStatus updateFromDom_(Workspace* workspace) override;
};

class VGC_WORKSPACE_API VacInbetweenVertex : public VacVertexCell {
private:
    friend class Workspace;

public:
    ~VacInbetweenVertex() override = default;

    VacInbetweenVertex(Workspace* workspace, dom::Element* domElement)
        : VacVertexCell(workspace, domElement) {
    }

    vacomplex::InbetweenVertex* vacInbetweenVertexNode() const {
        return vacCellUnchecked()->toInbetweenVertexUnchecked();
    }

    geometry::Rect2d boundingBox(core::AnimTime t) const override;

protected:
    ElementStatus updateFromDom_(Workspace* workspace) override;
    void preparePaint_(core::AnimTime t, PaintOptions flags) override;
};

} // namespace vgc::workspace

#endif // VGC_WORKSPACE_VERTEX_H
