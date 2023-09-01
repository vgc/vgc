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
#include <memory>

#include <vgc/core/arithmetic.h>
#include <vgc/core/array.h>
#include <vgc/core/color.h>
#include <vgc/dom/element.h>
#include <vgc/geometry/stroke.h>
#include <vgc/geometry/rect2d.h>
#include <vgc/geometry/vec2f.h>
#include <vgc/geometry/vec3f.h>
#include <vgc/geometry/vec4f.h>
#include <vgc/graphics/engine.h>
#include <vgc/graphics/geometryview.h>
#include <vgc/vacomplex/complex.h>
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
    // TODO: triangle iterator
public:
    static constexpr Int primitiveRestartIndex = -1;

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
    const geometry::Vec4fArray& stuvVertices() const {
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

    Int appendVertex(const geometry::Vec2d& position, const geometry::Vec4f& stuv) {
        Int idx = xyVertices_.length();
        xyVertices_.emplaceLast(position);
        stuvVertices_.emplaceLast(stuv);
        return idx;
    }

    Int numIndices() const {
        return indices_.length();
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
        indices_.append(primitiveRestartIndex);
        hasPrimRestart_ = true;
    }

private:
    geometry::Vec2dArray xyVertices_;
    geometry::Vec4fArray stuvVertices_;
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

    Int appendVertex(const geometry::Vec2d& position, const geometry::Vec4f& stuv) {
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
        clearCenterlineGeometry();
        clearOffsetLinesGeometry();
        clearStrokeGeometry();
        clearJoinGeometry();
        clearSelectionGeometry();
        isStyleDirty_ = true;
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

    // ┌─── x
    // │  ↑ side 1
    // │ ─segment─→
    // y  ↓ side 0
    //
    const graphics::GeometryViewPtr& offsetLineGeometry(Int side) const {
        return offsetLinesGeometry_[side];
    }

    // ┌─── x
    // │  ↑ side 1
    // │ ─segment─→
    // y  ↓ side 0
    //
    void
    setOffsetLineGeometry(Int side, const graphics::GeometryViewPtr& offsetLineGeometry) {
        offsetLinesGeometry_[side] = offsetLineGeometry;
    }

    void clearOffsetLinesGeometry() {
        offsetLinesGeometry_[0].reset();
        offsetLinesGeometry_[1].reset();
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

    bool hasStyle() const {
        return !isStyleDirty_;
    }

    // currently not an independent graphics object
    void setStyle() {
        isStyleDirty_ = false;
    }

    void clearStyle() {
        isStyleDirty_ = true;
    }

private:
    // Centerline
    graphics::GeometryViewPtr centerlineGeometry_;

    // OffsetLines
    std::array<graphics::GeometryViewPtr, 2> offsetLinesGeometry_;

    // Stroke
    graphics::GeometryViewPtr strokeGeometry_;
    graphics::GeometryViewPtr joinGeometry_;

    // Selection (requires centerline)
    graphics::GeometryViewPtr selectionGeometry_;

    // Style
    bool isStyleDirty_ = true;
};

namespace detail {

struct EdgeJoinPatchSample {
    geometry::Vec2d sidePoint;
    geometry::Vec3f sideSTV;
    Int centerPointSampleIndex = -1;
    geometry::Vec2d centerPoint;
    geometry::Vec3f centerSTV;
};

constexpr size_t aaaa = sizeof(EdgeJoinPatchSample);

struct EdgeJoinPatchMergeLocation {
    Int halfedgeNextSampleIndex = 0;
    double t = 1.0;
    geometry::StrokeSample2d sample;
};

struct EdgeJoinPatch {
    // Constraints:
    // - the samples at join origin must have a `s` of 0 for both sides.
    // - the samples at merge location must have the same `s`.
    // - the samples must be ordered from extension to merge location.
    std::array<core::Array<EdgeJoinPatchSample>, 2> sideSamples;
    float extensionS = 0.f;
    float overrideS = 0.f;
    EdgeJoinPatchMergeLocation mergeLocation = {};
    bool isCap = false;

    void clear() {
        sideSamples[0].clear();
        sideSamples[1].clear();
        mergeLocation = {};
        isCap = false;
        extensionS = 0.f;
        overrideS = 0.f;
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
    friend class VacKeyVertex;

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

    const core::Color& color() const {
        return color_;
    }

    const geometry::Rect2d& boundingBox() const {
        return bbox_;
    }

    const geometry::StrokeSample2dArray& preJoinSamples() const {
        return sampling_ ? sampling_->samples() : defaultSamples_;
    }

    const EdgeGraphics& graphics() const {
        return graphics_;
    }

    bool isSelectableAt(
        const geometry::Vec2d& pos,
        bool outlineOnly,
        double tol,
        double* outDistance) const;

    bool isSelectableInRect(const geometry::Rect2d& rect) const;

private:
    core::AnimTime time_;

    geometry::Rect2d bbox_ = {};
    geometry::Rect2d strokeBbox_ = {};

    // KeyFace::isSelectableInRect() calls its cycle edges' isSelectableInRect()
    // thus it becomes useful to cache the result when testing a set of cells.
    mutable geometry::Rect2d selectionTestCacheRect_ = geometry::Rect2d::empty;
    mutable bool selectionTestCacheResult_ = false;

    // stage PreJoinGeometry
    std::shared_ptr<const geometry::StrokeSampling2d> sampling_;
    geometry::StrokeSample2dArray defaultSamples_;

    // stage PostJoinGeometry
    // [0]: start patch
    // [1]: end patch
    std::array<detail::EdgeJoinPatch, 2> patches_;

    // stage StrokeMesh
    StuvMesh2d stroke_;

    // style (independent stage)
    core::Color color_;
    bool isStyleDirty_ = true;

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

    /// Returns whether this edge is a closed edge.
    ///
    bool isClosed() const {
        vacomplex::EdgeCell* edgeCell = vacEdgeCellNode();
        return edgeCell ? edgeCell->isClosed() : false;
    }

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

    void setTesselationMode(geometry::CurveSamplingQuality mode);

    std::optional<core::StringId> domTagName() const override;

    geometry::Rect2d boundingBox(core::AnimTime t) const override;

    bool isSelectableAt(
        const geometry::Vec2d& pos,
        bool outlineOnly,
        double tol,
        double* outDistance = nullptr,
        core::AnimTime t = {}) const override;

    bool isSelectableInRect(const geometry::Rect2d& rect, core::AnimTime t = {})
        const override;

    const VacKeyEdgeFrameData* computeFrameData(VacEdgeComputationStage stage);

    const VacEdgeCellFrameData*
    computeFrameDataAt(core::AnimTime t, VacEdgeComputationStage stage) override;

    VacKeyVertex* startVertex() const {
        return verticesInfo_[0].element;
    }

    VacKeyVertex* endVertex() const {
        return verticesInfo_[1].element;
    }

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
    mutable bool isLastDrawOfControlPointsSelected_ = false;

    geometry::CurveSamplingQuality edgeTesselationMode_ =
        geometry::CurveSamplingQuality::AdaptiveHigh;

    ElementStatus onDependencyChanged_(Element* dependency, ChangeFlags changes) override;
    ElementStatus onDependencyRemoved_(Element* dependency) override;

    // returns whether stroke changed
    static bool updateStrokeFromDom_(vacomplex::KeyEdgeData* data, const dom::Element* domElement);
    static void writeStrokeToDom_(dom::Element* domElement, const vacomplex::KeyEdgeData* data);
    static void clearStrokeFromDom_(dom::Element* domElement);

    // returns whether style changed
    static bool updatePropertiesFromDom_(vacomplex::KeyEdgeData* data, const dom::Element* domElement);
    static void writePropertiesToDom_(dom::Element* domElement, const vacomplex::KeyEdgeData* data, core::ConstSpan<core::StringId> propNames);
    static void writeAllPropertiesToDom_(dom::Element* domElement, const vacomplex::KeyEdgeData* data);

    ElementStatus updateFromDom_(Workspace* workspace) override;

    void updateFromVac_(vacomplex::NodeModificationFlags flags) override;

    void updateVertices_(const std::array<VacKeyVertex*, 2>& newVertices);

    ChangeFlags alreadyNotifiedChanges_ = {};
    ChangeFlags pendingNotifyChanges_ = {};

    void notifyChanges_(ChangeFlags changes, bool immediately = true);

    bool computeStrokeStyle_();

    void dirtyStrokeStyle_(bool notifyDependentsImmediately = true);

    bool computePreJoinGeometry_();
    bool computePostJoinGeometry_();
    bool computeStrokeMesh_();

    void dirtyPreJoinGeometry_(bool notifyDependentsImmediately = true);
    void dirtyPostJoinGeometry_(bool notifyDependentsImmediately = true);
    void dirtyStrokeMesh_(bool notifyDependentsImmediately = true);

    void dirtyJoinDataAtVertex_(const VacVertexCell* vertexCell) override;

    void onUpdateError_();
};

} // namespace vgc::workspace

#endif // VGC_WORKSPACE_EDGE_H
