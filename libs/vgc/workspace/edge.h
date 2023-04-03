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

#ifndef VGC_WORKSPACE_EDGE_H
#define VGC_WORKSPACE_EDGE_H

#include <initializer_list>

#include <vgc/core/arithmetic.h>
#include <vgc/core/array.h>
#include <vgc/core/color.h>
#include <vgc/dom/element.h>
#include <vgc/geometry/vec2d.h>
#include <vgc/geometry/vec2f.h>
#include <vgc/geometry/vec4d.h>
#include <vgc/geometry/vec4f.h>
#include <vgc/graphics/engine.h>
#include <vgc/graphics/geometryview.h>
#include <vgc/topology/vac.h>
#include <vgc/workspace/api.h>
#include <vgc/workspace/element.h>
#include <vgc/workspace/vertex.h>

namespace vgc::workspace {

// Terminology:
// - Centerline: the sampled centerline from input geometry.
// - OffsetLines: the lines offseted by the varying halfwidths from the centerline,
//                and parameterized with corresponding centerline arclength.
// - JoinedLines: Centerline and OffsetLines with patches for joins/caps.
// - Stroke: Triangulated and parameterized "(s, t, u, v)" hull.
//           Optionally post-treated to remove overlaps. (Shader-ready)

// Component (drawn and selectable..)

class VGC_WORKSPACE_API StuvMesh2d {
public:
    StuvMesh2d() noexcept = default;

    void reset(bool isStrip) {
        isStrip_ = isStrip;
        reset();
    }

    void reset() {
        xyVertices_.clear();
        stuvVertices_.clear();
        indices_.clear();
        hasPrimRestart_ = false;
    }

    /// Returns the xy component of vertices.
    const geometry::Vec2dArray& xyVertices() const {
        return xyVertices_;
    }

    /// Returns the stuv component of vertices.
    const geometry::Vec4dArray& stuvVertices() const {
        return stuvVertices_;
    }

    /// Returns the vertex indices.
    const core::IntArray& indices() const {
        return indices_;
    }

    bool isTriangleStrip() const {
        return isStrip_;
    }

    bool isTriangleList() const {
        return !isStrip_;
    }

    bool isIndexed() const {
        return indices_.length() > 0;
    }

    bool hasPrimitiveRestart() const {
        return hasPrimRestart_;
    }

    Int numVertices() const {
        return xyVertices_.length();
    }

    Int appendVertex(const geometry::Vec2d& position, const geometry::Vec4d& stuv) {
        Int idx = xyVertices_.length();
        xyVertices_.emplaceLast(position);
        stuvVertices_.emplaceLast(stuv);
        return idx;
    }

    void appendIndex(Int i) {
        Int n = xyVertices_.length();
        if (i < 0 || i >= n) {
            throw core::IndexError(
                core::format("Vertex index {} is out of range [0, {}).", i, n));
        }
        indices_.append(i);
    }

    void appendIndexUnchecked(Int i) {
        indices_.append(i);
    }

    void appendIndices(std::initializer_list<Int> is) {
        Int n = xyVertices_.length();
        for (Int i : is) {
            if (i < 0 || i >= n) {
                throw core::IndexError(
                    core::format("Vertex index {} is out of range [0, {}).", i, n));
            }
        }
        indices_.extend(is);
    }

    void appendIndicesUnchecked(std::initializer_list<Int> is) {
        indices_.extend(is);
    }

    void appendPrimitiveRestartIndex() {
        indices_.append(-1);
        hasPrimRestart_ = true;
    }

private:
    geometry::Vec2dArray xyVertices_;
    geometry::Vec4dArray stuvVertices_;
    core::IntArray indices_;
    bool isStrip_ = false;
    bool hasPrimRestart_ = false;
};

class VGC_WORKSPACE_API StuvTriangleListMesh2dBuilder {
public:
    StuvTriangleListMesh2dBuilder(StuvMesh2d& mesh)
        : mesh_(mesh) {

        mesh_.reset(false);
    }

    Int appendVertex(const geometry::Vec2d& position, const geometry::Vec4d& stuv) {
        return mesh_.appendVertex(position, stuv);
    }

    void appendTriangle(Int i0, Int i1, Int i2) {
        mesh_.appendIndices({i0, i1, i2});
    }

private:
    StuvMesh2d& mesh_;
};

namespace detail {

graphics::GeometryViewPtr
loadMeshGraphics(graphics::Engine* engine, const StuvMesh2d& mesh);

} // namespace detail

class VGC_WORKSPACE_API EdgeGraphics {
public:
    void clear() {
        centerlineGeometry_.reset();
        strokeGeometry_.reset();
        joinGeometry_.reset();
        selectionGeometry_.reset();
    }

    const graphics::GeometryViewPtr& centerlineGeometry() const {
        return centerlineGeometry_;
    }

    void setCenterlineGeometry(const graphics::GeometryViewPtr& centerlineGeometry) {
        centerlineGeometry_ = centerlineGeometry;
    }

    void clearCenterlineGeometry() {
        centerlineGeometry_.reset();
    }

    const graphics::GeometryViewPtr& strokeGeometry() const {
        return strokeGeometry_;
    }

    void setStrokeGeometry(const graphics::GeometryViewPtr& strokeGeometry) {
        strokeGeometry_ = strokeGeometry;
    }

    void clearStrokeGeometry() {
        strokeGeometry_.reset();
    }

    const graphics::GeometryViewPtr& joinGeometry() const {
        return joinGeometry_;
    }

    void setJoinGeometry(const graphics::GeometryViewPtr& joinGeometry) {
        joinGeometry_ = joinGeometry;
    }

    void clearJoinGeometry() {
        joinGeometry_.reset();
    }

    const graphics::GeometryViewPtr& selectionGeometry() const {
        return selectionGeometry_;
    }

    void setSelectionGeometry(const graphics::GeometryViewPtr& selectionGeometry) {
        selectionGeometry_ = selectionGeometry;
    }

    void clearSelectionGeometry() {
        selectionGeometry_.reset();
    }

private:
    // Centerline
    graphics::GeometryViewPtr centerlineGeometry_;

    // Stroke
    graphics::GeometryViewPtr strokeGeometry_;
    graphics::GeometryViewPtr joinGeometry_;

    // Selection (requires centerline)
    graphics::GeometryViewPtr selectionGeometry_;
};

namespace detail {

struct EdgeJoinPatchSample {
    geometry::Vec2d centerPoint;
    geometry::Vec2d sidePoint;
    geometry::Vec4f sideSTUV;
    geometry::Vec2f centerSU;
    Int centerPointSampleIndex = -1;
};

struct EdgeJoinPatchMergeLocation {
    Int halfedgeNextSampleIndex = 0;
    double t = 1.0;
    geometry::CurveSample sample;
};

struct EdgeJoinPatch {
    std::array<core::Array<EdgeJoinPatchSample>, 2> sideSamples;
    EdgeJoinPatchMergeLocation mergeLocation = {};
    bool isCap = false;

    void clear() {
        sideSamples[0].clear();
        sideSamples[1].clear();
        mergeLocation = {};
        isCap = false;
    }
};

} // namespace detail

enum class VacEdgeComputationStage : UInt8 {
    /// No geometry computed.
    Clear,
    /// KeyEdge: Snapped Centerline + OffsetLines.
    /// InbetweenEdge: Interpolated and Snapped Centerline + OffsetLines.
    PreJoinGeometry,
    /// KeyEdge/InbetweenEdge: Join Patches.
    PostJoinGeometry,
    /// KeyEdge/InbetweenEdge: Final triangle mesh, with overlaps/cusps optionally removed.
    StrokeMesh,
    EnumMin = Clear,
    EnumMax = StrokeMesh
};

class VGC_WORKSPACE_API VacEdgeCellFrameData {
private:
    friend class VacEdgeCell;
    friend class VacKeyEdge;
    friend class VacInbetweenEdge;
    friend class VacVertexCell;

public:
    VacEdgeCellFrameData(const core::AnimTime& t) noexcept
        : time_(t) {
    }

    virtual ~VacEdgeCellFrameData() = default;

    virtual void clear() {
        resetToStage(VacEdgeComputationStage::Clear);
    }

    VacEdgeComputationStage stage() const {
        return stage_;
    }

    bool resetToStage(VacEdgeComputationStage stage);

    const core::AnimTime& time() const {
        return time_;
    }

    const geometry::CurveSampleArray& preJoinSamples() const {
        return samples_;
    }

    const EdgeGraphics& graphics() const {
        return graphics_;
    }

    bool isSelectableAt(
        const geometry::Vec2d& pos,
        bool outlineOnly,
        double tol,
        double* outDistance) const;

private:
    core::AnimTime time_;
    geometry::Rect2d bbox_ = {};

    // stage PreJoinGeometry
    geometry::CurveSampleArray samples_;

    // stage PostJoinGeometry
    // [0]: start patch
    // [1]: end patch
    std::array<detail::EdgeJoinPatch, 2> patches_;

    // stage StrokeMesh
    StuvMesh2d stroke_;
    core::Color color_;
    bool hasPendingColorChange_ = false;

    // Note: only valid for a single engine at the moment.
    EdgeGraphics graphics_;

    VacEdgeComputationStage stage_ = VacEdgeComputationStage::Clear;
    bool isComputing_ = false;
};

// wrapper to benefit from "final" specifier.
class VGC_WORKSPACE_API VacKeyEdgeFrameData final : public VacEdgeCellFrameData {
public:
    VacKeyEdgeFrameData(const core::AnimTime& t) noexcept
        : VacEdgeCellFrameData(t) {
    }
};

class VGC_WORKSPACE_API VacEdgeCell : public VacElement {
private:
    friend class Workspace;
    friend class VacVertexCell;

protected:
    VacEdgeCell(Workspace* workspace)
        : VacElement(workspace) {
    }

public:
    vacomplex::EdgeCell* vacEdgeCellNode() const {
        vacomplex::Cell* cell = vacCellUnchecked();
        return cell ? cell->toEdgeCellUnchecked() : nullptr;
    }

    virtual const VacEdgeCellFrameData*
    computeFrameDataAt(core::AnimTime t, VacEdgeComputationStage stage) = 0;

private:
    virtual void dirtyJoinDataAtVertex_(const VacVertexCell* vertexCell) = 0;
};

class VGC_WORKSPACE_API VacKeyEdge final : public VacEdgeCell {
private:
    friend class Workspace;
    // for joins and caps
    friend class VacVertexCell;
    friend class VacKeyVertex;

public:
    ~VacKeyEdge() override;

    VacKeyEdge(Workspace* workspace)
        : VacEdgeCell(workspace)
        , frameData_({}) {
    }

    vacomplex::KeyEdge* vacKeyEdgeNode() const {
        vacomplex::Cell* cell = vacCellUnchecked();
        return cell ? cell->toKeyEdgeUnchecked() : nullptr;
    }

    void setTesselationMode(int mode);

    std::optional<core::StringId> domTagName() const override;

    geometry::Rect2d boundingBox(core::AnimTime t) const override;

    bool isSelectableAt(
        const geometry::Vec2d& pos,
        bool outlineOnly,
        double tol,
        double* outDistance = nullptr,
        core::AnimTime t = {}) const override;

    const VacKeyEdgeFrameData* computeFrameData(VacEdgeComputationStage stage);

    const VacEdgeCellFrameData*
    computeFrameDataAt(core::AnimTime t, VacEdgeComputationStage stage) override;

protected:
    void onPaintPrepare(core::AnimTime t, PaintOptions flags) override;

    void onPaintDraw(
        graphics::Engine* engine,
        core::AnimTime t,
        PaintOptions flags = PaintOption::None) const override;

private:
    struct VertexInfo {
        VacKeyVertex* element = nullptr;
        Int joinGroup = 0;
    };
    std::array<VertexInfo, 2> verticesInfo_ = {};

    mutable VacKeyEdgeFrameData frameData_;
    geometry::Vec2dArray controlPoints_;
    mutable graphics::GeometryViewPtr controlPointsGeometry_;
    geometry::CurveSampleArray inputSamples_;
    Int samplingVersion_ = -1;
    int edgeTesselationMode_ = -1;
    int edgeTesselationModeRequested_ = 2;
    bool isInputSamplingDirty_ = true;

    void onDependencyChanged_(Element* dependency, ChangeFlags changes) override;
    void onDependencyRemoved_(Element* dependency) override;

    ElementStatus updateFromDom_(Workspace* workspace) override;
    void updateFromVac_() override;

    void updateVertices_(const std::array<VacKeyVertex*, 2>& newVertices);

    ChangeFlags alreadyNotifiedChanges_ = {};
    ChangeFlags pendingNotifyChanges_ = {};

    void notifyChanges_();

    bool computeInputSampling_();
    bool computePreJoinGeometry_();
    bool computePostJoinGeometry_();
    bool computeStrokeMesh_();

    void dirtyInputSampling_(bool notifyDependents = true);
    void dirtyPreJoinGeometry_(bool notifyDependents = true);
    void dirtyPostJoinGeometry_(bool notifyDependents = true);
    void dirtyStrokeMesh_(bool notifyDependents = true);

    void dirtyJoinDataAtVertex_(const VacVertexCell* vertexCell) override;

    void onUpdateError_();
};

} // namespace vgc::workspace

#endif // VGC_WORKSPACE_EDGE_H
