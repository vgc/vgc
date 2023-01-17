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

#include <vgc/core/arithmetic.h>
#include <vgc/core/array.h>
#include <vgc/dom/element.h>
#include <vgc/graphics/engine.h>
#include <vgc/topology/vac.h>
#include <vgc/workspace/api.h>
#include <vgc/workspace/element.h>
#include <vgc/workspace/vertex.h>

namespace vgc::workspace {

// Component (drawn and selectable..)

struct CurveGraphics {

    void clear() {
        controlPoints_.reset();
        thickPolyline_.reset();
        numPoints_ = 0;
    }

    graphics::BufferPtr controlPoints_;
    graphics::BufferPtr thickPolyline_;
    Int numPoints_ = 0;
};

struct StrokeGraphics {

    void clear() {
        meshVertices_.reset();
        meshUVSTs_.reset();
        meshIndices_.reset();
    }

    graphics::BufferPtr meshVertices_;
    graphics::BufferPtr meshUVSTs_;
    graphics::BufferPtr meshIndices_;
};

struct EdgeGraphics {

    void clear() {
        curveGraphics_.clear();
        pointsGeometry_.reset();
        centerlineGeometry_.reset();
        strokeGraphics_.clear();
        strokeGeometry_.reset();
        selectionGeometry_.reset();
    }

    // Center line & Control points
    CurveGraphics curveGraphics_;
    graphics::GeometryViewPtr pointsGeometry_;
    graphics::GeometryViewPtr centerlineGeometry_;

    // Stroke
    StrokeGraphics strokeGraphics_;
    graphics::GeometryViewPtr strokeGeometry_;
    graphics::GeometryViewPtr selectionGeometry_;
};

struct EdgeGeometryComputationCache {

    void clear() {
        samples_.clear();
        triangulation_.clear();
        cps_.clear();
        startSampleOverride_ = 0;
        endSampleOverride_ = 0;
        edgeTesselationMode_ = -1;
    }

    geometry::CurveSampleArray samples_;
    geometry::Vec2dArray triangulation_; // to remove
    geometry::Vec2fArray cps_;
    Int startSampleOverride_ = 0;
    Int endSampleOverride_ = 0;
    int edgeTesselationMode_ = -1;
};

class VGC_WORKSPACE_API Edge : public VacElement {
private:
    friend class Workspace;

protected:
    Edge(Workspace* workspace, dom::Element* domElement)
        : VacElement(workspace, domElement) {
    }

public:
    topology::EdgeCell* vacEdge() const {
        topology::VacNode* n = vacNode();
        return n ? n->toCellUnchecked()->toEdgeCellUnchecked() : nullptr;
    }

    virtual void updateGeometry(core::AnimTime t);
};

class VGC_WORKSPACE_API KeyEdge : public Edge {
private:
    friend class Workspace;
    friend class KeyVertex; // for joins and caps

public:
    ~KeyEdge() override = default;

    KeyEdge(Workspace* workspace, dom::Element* domElement)
        : Edge(workspace, domElement) {
    }

    topology::KeyEdge* vacKeyEdge() const {
        topology::VacNode* n = vacNode();
        return n ? n->toCellUnchecked()->toKeyEdgeUnchecked() : nullptr;
    }

    void setTesselationMode(int mode) {
        edgeTesselationModeRequested_ = core::clamp(mode, 0, 2);
    }

    void updateGeometry();
    void updateGeometry(core::AnimTime t) override;

    geometry::Rect2d boundingBox(core::AnimTime t) const override;

protected:
    ElementError updateFromDom_(Workspace* workspace) override;

    void preparePaint_(core::AnimTime t, PaintOptions flags) override;

    void paint_(
        graphics::Engine* engine,
        core::AnimTime t,
        PaintOptions flags = PaintOption::None) const override;

private:
    KeyVertex* v0_ = nullptr;
    KeyVertex* v1_ = nullptr;

    // need an invalidation mechanism
    mutable EdgeGraphics cachedGraphics_;
    int edgeTesselationModeRequested_ = 2;

    // local data to build graphics resource, kept as copy
    // should this be in vac ?
    EdgeGeometryComputationCache geometry_;

    //void onUpdateError_();
    void updateGeometry_();
};

} // namespace vgc::workspace

#endif // VGC_WORKSPACE_EDGE_H
