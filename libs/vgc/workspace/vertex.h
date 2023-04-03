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
class VacKeyVertex;
class VacInbetweenVertex;
class VacEdgeCell;
class VacEdgeCellFrameData;
class VacVertexCellFrameData;

// references an incident halfedge in a join
class VGC_WORKSPACE_API VacJoinHalfedge {
public:
    friend VacVertexCell;

    VacJoinHalfedge(VacEdgeCell* edgeCell, bool isReverse, Int32 joinGroup)
        : edgeCell_(edgeCell)
        , joinGroup_(joinGroup)
        , isReverse_(isReverse) {
    }

    VacEdgeCell* edgeCell() const {
        return edgeCell_;
    }

    bool isReverse() const {
        return isReverse_;
    }

    Int joinGroup() const {
        return joinGroup_;
    }

    struct GrouplessEqualTo {
        constexpr bool
        operator()(const VacJoinHalfedge& lhs, const VacJoinHalfedge& rhs) const {
            return lhs.edgeCell_ == rhs.edgeCell_ && lhs.isReverse_ == rhs.isReverse_;
        }
    };

private:
    VacEdgeCell* edgeCell_;
    Int32 joinGroup_;
    bool isReverse_;
};

namespace detail {

class VacJoinFrameData;

// outgoing halfedge
class VGC_WORKSPACE_API VacJoinHalfedgeFrameData {
public:
    friend VacJoinFrameData;
    friend VacVertexCellFrameData;
    friend VacVertexCell;
    friend VacKeyVertex;
    friend VacInbetweenVertex;

    VacJoinHalfedgeFrameData(const VacJoinHalfedge& halfedge)
        : halfedge_(halfedge) {
    }

    const VacJoinHalfedge& halfedge() const {
        return halfedge_;
    }

    VacEdgeCell* edgeCell() const {
        return halfedge_.edgeCell();
    }

    bool isReverse() const {
        return halfedge_.isReverse();
    }

    Int joinGroup() const {
        return halfedge_.joinGroup();
    }

    double angle() const {
        return angle_;
    }

    double angleToNext() const {
        return angleToNext_;
    }

    void clearJoinData() {
        edgeData_ = nullptr;
        inputSamples_.clear();
        patchLength_ = 0;
        workingSamples_.clear();
        sidePatchData_[0].clear();
        sidePatchData_[1].clear();
    }

private:
    VacJoinHalfedge halfedge_;

    // precomputation cache
    VacEdgeCellFrameData* edgeData_ = nullptr;
    geometry::Vec2d outgoingTangent_ = {};
    geometry::Vec2d halfwidths_ = {};
    geometry::Vec2d patchCutLimits_ = {};
    core::Array<geometry::CurveSample> inputSamples_;
    double angle_ = 0.0;
    double angleToNext_ = 0.0;

    // join result
    double patchLength_ = 0;
    core::Array<geometry::CurveSample> workingSamples_;
    struct SidePatchData {
        // straight join model data
        double filletLength = 0;
        double joinHalfwidth = 0;
        bool isCutFillet = false;
        double extLength = 0;
        //Ray borderRay = {};

        void clear() {
            filletLength = 0;
            joinHalfwidth = 0;
            isCutFillet = false;
            extLength = 0;
        }
    };
    std::array<SidePatchData, 2> sidePatchData_ = {};
};

class VGC_WORKSPACE_API VacJoinFrameData {
private:
    friend VacVertexCellFrameData;
    friend VacVertexCell;
    friend VacKeyVertex;
    friend VacInbetweenVertex;

public:
    void clear() {
        halfedgesFrameData_.clear();
    }

private:
    core::Array<VacJoinHalfedgeFrameData> halfedgesFrameData_;
};

} // namespace detail

class VGC_WORKSPACE_API VacVertexCellFrameData {
private:
    friend VacVertexCell;
    friend VacKeyVertex;
    friend VacInbetweenVertex;

public:
    VacVertexCellFrameData(const core::AnimTime& t)
        : time_(t) {
    }

    const core::AnimTime& time() const {
        return time_;
    }

    bool hasPosition() const {
        return isPositionComputed_;
    }

    const geometry::Vec2d& position() const {
        return position_;
    }

private:
    core::AnimTime time_;
    geometry::Vec2d position_ = core::noInit;
    mutable graphics::GeometryViewPtr joinDebugLinesRenderGeometry_;
    mutable graphics::GeometryViewPtr joinDebugQuadRenderGeometry_;
    detail::VacJoinFrameData joinData_;
    bool isPositionComputed_ = false;
    bool isJoinComputed_ = false;
    bool isComputing_ = false;

    bool hasJoinData_() const {
        return isJoinComputed_;
    }

    void clearJoinData_() {
        joinData_.clear();
        isJoinComputed_ = false;
        joinDebugLinesRenderGeometry_.reset();
        joinDebugQuadRenderGeometry_.reset();
    }

    void debugPaint_(graphics::Engine* engine);
};

class VGC_WORKSPACE_API VacVertexCell : public VacElement {
private:
    friend class Workspace;
    friend class VacEdgeCell;
    friend class VacKeyEdge;
    friend class VacInbetweenVEdge;

protected:
    VacVertexCell(Workspace* workspace)
        : VacElement(workspace) {
    }

public:
    vacomplex::VertexCell* vacVertexCellNode() const {
        vacomplex::Cell* cell = vacCellUnchecked();
        return cell ? cell->toVertexCellUnchecked() : nullptr;
    }

    virtual const VacVertexCellFrameData* computeFrameDataAt(core::AnimTime t) = 0;

protected:
    void rebuildJoinHalfedgesArray() const;
    void dirtyJoinHalfedgesJoinData_(const VacEdgeCell* exluded) const;

    // incident join halfedges
    const core::Array<VacJoinHalfedge>& joinHalfedges() const {
        return joinHalfedges_;
    }

private:
    mutable core::Array<VacJoinHalfedge> joinHalfedges_;

    void addJoinHalfedge_(const VacJoinHalfedge& joinHalfedge);
    void removeJoinHalfedge_(const VacJoinHalfedge& joinHalfedge);

    virtual void onAddJoinHalfedge_(const VacJoinHalfedge& joinHalfedge) = 0;
    virtual void onRemoveJoinHalfedge_(const VacJoinHalfedge& joinHalfedge) = 0;
    virtual void onJoinEdgePreJoinGeometryChanged_(const VacEdgeCell* edge) = 0;
};

class VGC_WORKSPACE_API VacKeyVertex final : public VacVertexCell {
private:
    friend class Workspace;
    friend class VacEdgeCell;
    friend class VacKeyEdge;
    friend class VacInbetweenVEdge;

public:
    ~VacKeyVertex() override = default;

    VacKeyVertex(Workspace* workspace)
        : VacVertexCell(workspace)
        , frameData_({}) {
    }

    vacomplex::KeyVertex* vacKeyVertexNode() const {
        vacomplex::Cell* cell = vacCellUnchecked();
        return cell ? cell->toKeyVertexUnchecked() : nullptr;
    }

    std::optional<core::StringId> domTagName() const override;

    geometry::Rect2d boundingBox(core::AnimTime t) const override;

    bool isSelectableAt(
        const geometry::Vec2d& pos,
        bool outlineOnly,
        double tol,
        double* outDistance = nullptr,
        core::AnimTime t = {}) const override;

    const VacVertexCellFrameData* computeFrameDataAt(core::AnimTime t) override;

protected:
    void onPaintDraw(
        graphics::Engine* engine,
        core::AnimTime t,
        PaintOptions flags = PaintOption::None) const override;

private:
    mutable VacVertexCellFrameData frameData_;

    ElementStatus updateFromDom_(Workspace* workspace) override;
    void updateFromVac_() override;

    void onAddJoinHalfedge_(const VacJoinHalfedge& joinHalfedge) override;
    void onRemoveJoinHalfedge_(const VacJoinHalfedge& joinHalfedge) override;
    void onJoinEdgePreJoinGeometryChanged_(const VacEdgeCell* edge) override;

    void computePosition_();
    void computeJoin_();

    void dirtyPosition_();
    void dirtyJoin_();

    void onUpdateError_();
};

} // namespace vgc::workspace

#endif // VGC_WORKSPACE_VERTEX_H
