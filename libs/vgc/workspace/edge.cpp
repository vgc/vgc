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

#include <vgc/workspace/edge.h>
#include <vgc/workspace/workspace.h>

namespace vgc::workspace {

void Edge::updateGeometry(core::AnimTime /*t*/) {
}

void KeyEdge::updateGeometry() {
    updateGeometry_();
}

void KeyEdge::updateGeometry(core::AnimTime /*t*/) {
    updateGeometry_();
}

geometry::Rect2d KeyEdge::boundingBox(core::AnimTime /*t*/) const {
    return geometry::Rect2d::empty;
}

ElementError KeyEdge::updateFromDom_(Workspace* workspace) {
    namespace ds = dom::strings;
    dom::Element* const domElement = this->domElement();

    std::optional<Element*> ve0opt =
        workspace->getElementFromPathAttribute(domElement, ds::startvertex, ds::vertex);
    std::optional<Element*> ve1opt =
        workspace->getElementFromPathAttribute(domElement, ds::endvertex, ds::vertex);

    Element* parentElement = this->parent();
    Element* ve0 = ve0opt.value_or(nullptr);
    Element* ve1 = ve1opt.value_or(nullptr);

    replaceDependency(v0_, ve0);
    replaceDependency(v1_, ve1);
    // XXX impl custom safe cast
    v0_ = dynamic_cast<KeyVertex*>(ve0);
    v1_ = dynamic_cast<KeyVertex*>(ve1);

    // what's the cleanest way to report/notify that this edge has actually changed.
    // what are the different categories of changes that matter to dependents ?
    // for instance an edge wanna know if a vertex moves or has a new style (new join)

    if (ve0opt.has_value() != ve1opt.has_value()) {
        removeVacNode();
        return ElementError::InvalidAttribute;
    }

    if (ve0opt.has_value() && !ve0) {
        removeVacNode();
        return ElementError::UnresolvedDependency;
    }

    if (ve1opt.has_value() && !ve1) {
        removeVacNode();
        return ElementError::UnresolvedDependency;
    }

    topology::KeyVertex* kv0 = nullptr;
    topology::KeyVertex* kv1 = nullptr;
    topology::VacGroup* parentGroup = nullptr;

    // update dependencies (vertices)
    if (ve0) {
        workspace->updateElementFromDom(ve0);
        topology::VacNode* vn0 = ve0->vacNode();
        if (vn0) {
            kv0 = vn0->toCellUnchecked()->toKeyVertexUnchecked();
        }
        if (ve0->hasError() || !kv0) {
            removeVacNode();
            return ElementError::ErrorInDependency;
        }
    }
    if (ve1) {
        workspace->updateElementFromDom(ve1);
        topology::VacNode* vn1 = ve1->vacNode();
        if (vn1) {
            kv1 = vn1->toCellUnchecked()->toKeyVertexUnchecked();
        }
        if (ve1->hasError() || !kv0) {
            removeVacNode();
            return ElementError::ErrorInDependency;
        }
    }
    if (parentElement) {
        workspace->updateElementFromDom(parentElement);
        topology::VacNode* parentNode = parentElement->vacNode();
        if (parentNode) {
            // checked cast to group, could be something invalid
            parentGroup = parentNode->toGroup();
        }
    }

    if (!parentGroup) {
        removeVacNode();
        return ElementError::ErrorInParent;
    }

    topology::KeyEdge* ke = nullptr;

    // check if it needs to be rebuilt
    if (vacNode_) {
        ke = vacNode_->toCellUnchecked()->toKeyEdgeUnchecked();
        topology::KeyVertex* oldKv0 = ke->startVertex();
        topology::KeyVertex* oldKv1 = ke->endVertex();
        if (kv0 != oldKv0 || kv1 != oldKv1) {
            // dirty geometry
            geometry_.clear();
            // remove current node
            topology::ops::removeNode(vacNode_, false);
            vacNode_ = nullptr;
            ke = nullptr;
        }
    }
    else {
        if (kv0 && kv1) {
            ke = topology::ops::createKeyEdge(
                domElement->internalId(), parentGroup, kv0, kv1);
        }
        else {
            // XXX warning if kv0 || kv1 ?
            ke =
                topology::ops::createKeyClosedEdge(domElement->internalId(), parentGroup);
        }
        vacNode_ = ke;
    }
    // XXX warning on null parent group ?

    if (ke) {
        const auto& points = domElement->getAttribute(ds::positions).getVec2dArray();
        const auto& widths = domElement->getAttribute(ds::widths).getDoubleArray();

        if (&ke->points() != &points.get() || &ke->widths() != &widths.get()) {
            topology::ops::setKeyEdgeCurvePoints(ke, points);
            topology::ops::setKeyEdgeCurveWidths(ke, widths);
            // dirty geometry
            geometry_.clear();
        }

        // XXX should we snap here ?
        //     group view matrices may not be ready..
        //     maybe we could add two init functions to workspace::Element
        //     one for intrinsic data, one for dependent data.

        return ElementError::None;
    }

    return ElementError::InvalidAttribute;
}

void KeyEdge::preparePaint_(core::AnimTime /*t*/, PaintOptions /*flags*/) {
    // todo, use paint options to not compute everything or with lower quality
    updateGeometry_();
}

void KeyEdge::paint_(graphics::Engine* engine, core::AnimTime /*t*/, PaintOptions flags)
    const {

    // if not already done (should we leave preparePaint_ optional?)
    const_cast<KeyEdge*>(this)->updateGeometry_();

    using namespace graphics;
    namespace ds = dom::strings;

    const dom::Element* const domElement = this->domElement();
    // XXX "implicit" cells' domElement would be the composite ?

    constexpr PaintOptions strokeOptions = {PaintOption::Selected, PaintOption::Draft};

    // XXX todo: reuse geometry objects, create buffers separately (attributes waiting in EdgeGraphics).

    if (flags.hasAny(strokeOptions)
        || !flags.has(PaintOption::Outline) && !cachedGraphics_.strokeGeometry_) {
        cachedGraphics_.strokeGeometry_ =
            engine->createDynamicTriangleStripView(BuiltinGeometryLayout::XY_iRGBA);

        GeometryViewCreateInfo createInfo = {};
        createInfo.setBuiltinGeometryLayout(BuiltinGeometryLayout::XY_iRGBA);
        createInfo.setPrimitiveType(PrimitiveType::TriangleStrip);
        createInfo.setVertexBuffer(0, cachedGraphics_.strokeGeometry_->vertexBuffer(0));
        BufferPtr selectionInstanceBuffer = engine->createVertexBuffer(4 * 4);
        createInfo.setVertexBuffer(1, selectionInstanceBuffer);
        cachedGraphics_.selectionGeometry_ = engine->createGeometryView(createInfo);

        core::Color color = domElement->getAttribute(ds::color).getColor();

        geometry::Vec2fArray strokeVertices;
        if (edgeTesselationModeRequested_ <= 2) {
            for (const geometry::CurveSample& s : geometry_.samples_) {
                geometry::Vec2d p0 = s.leftPoint();
                strokeVertices.emplaceLast(geometry::Vec2f(p0));
                geometry::Vec2d p1 = s.rightPoint();
                strokeVertices.emplaceLast(geometry::Vec2f(p1));
            }
        }
        else {
            for (const geometry::Vec2d& p : geometry_.triangulation_) {
                strokeVertices.emplaceLast(geometry::Vec2f(p));
            }
        }
        engine->updateBufferData(
            cachedGraphics_.strokeGeometry_->vertexBuffer(0), //
            std::move(strokeVertices));

        engine->updateBufferData(
            cachedGraphics_.strokeGeometry_->vertexBuffer(1), //
            core::Array<float>({color.r(), color.g(), color.b(), color.a()}));
        engine->updateBufferData(
            selectionInstanceBuffer, //
            core::Array<float>(
                {1.0f - color.r(), 1.0f - color.g(), 1.0f - color.b(), 1.0f}));
    }

    constexpr PaintOptions centerlineOptions = {PaintOption::Outline};

    if (flags.hasAny(centerlineOptions) && !cachedGraphics_.centerlineGeometry_) {
        cachedGraphics_.centerlineGeometry_ = engine->createDynamicTriangleStripView(
            BuiltinGeometryLayout::XYDxDy_iXYRotRGBA);

        core::FloatArray lineInstData;
        lineInstData.extend({0.f, 0.f, 1.f, 0.02f, 0.64f, 1.0f, 1.f, 0.f /*padding*/});

        float lineHalfSize = 2.f;

        geometry::Vec4fArray lineVertices;
        for (const geometry::CurveSample& s : geometry_.samples_) {
            geometry::Vec2f p = geometry::Vec2f(s.position());
            geometry::Vec2f n = geometry::Vec2f(s.normal());
            // clang-format off
            lineVertices.emplaceLast(p.x(), p.y(), -lineHalfSize * n.x(), -lineHalfSize * n.y());
            lineVertices.emplaceLast(p.x(), p.y(),  lineHalfSize * n.x(),  lineHalfSize * n.y());
            // clang-format on
        }

        engine->updateBufferData(
            cachedGraphics_.centerlineGeometry_->vertexBuffer(0),
            std::move(lineVertices));
        engine->updateBufferData(
            cachedGraphics_.centerlineGeometry_->vertexBuffer(1),
            std::move(lineInstData));
    }

    constexpr PaintOptions pointsOptions = {PaintOption::Outline};

    if (flags.hasAny(pointsOptions) && !cachedGraphics_.pointsGeometry_) {
        cachedGraphics_.pointsGeometry_ = engine->createDynamicTriangleStripView(
            BuiltinGeometryLayout::XYDxDy_iXYRotRGBA);

        float pointHalfSize = 5.f;

        core::Array<geometry::Vec4f> pointVertices;
        // clang-format off
        pointVertices.extend({
            {0, 0, -pointHalfSize, -pointHalfSize},
            {0, 0, -pointHalfSize,  pointHalfSize},
            {0, 0,  pointHalfSize, -pointHalfSize},
            {0, 0,  pointHalfSize,  pointHalfSize} });
        // clang-format on

        core::FloatArray pointInstData;
        const Int numPoints = geometry_.cps_.length();
        const float dl = 1.f / numPoints;
        for (Int j = 0; j < numPoints; ++j) {
            geometry::Vec2f p = geometry_.cps_[j];
            float l = j * dl;
            pointInstData.extend(
                {p.x(),
                 p.y(),
                 0.f,
                 (l > 0.5f ? 2 * (1.f - l) : 1.f),
                 0.f,
                 (l < 0.5f ? 2 * l : 1.f),
                 1.f});
        }

        engine->updateBufferData(
            cachedGraphics_.pointsGeometry_->vertexBuffer(0), std::move(pointVertices));
        engine->updateBufferData(
            cachedGraphics_.pointsGeometry_->vertexBuffer(1), std::move(pointInstData));
    }

    if (flags.has(PaintOption::Selected)) {
        engine->setProgram(graphics::BuiltinProgram::Simple);
        engine->draw(cachedGraphics_.selectionGeometry_);
    }
    else if (!flags.has(PaintOption::Outline)) {
        engine->setProgram(graphics::BuiltinProgram::Simple);
        engine->draw(cachedGraphics_.strokeGeometry_);
    }

    if (flags.has(PaintOption::Outline)) {
        engine->setProgram(graphics::BuiltinProgram::SreenSpaceDisplacement);
        engine->draw(cachedGraphics_.centerlineGeometry_);
        engine->drawInstanced(cachedGraphics_.pointsGeometry_);
    }
}

void KeyEdge::updateGeometry_() {

    topology::KeyEdge* ke = vacKeyEdge();
    if (!ke) {
        // error ?
        return;
    }

    // check if we need an update
    if (edgeTesselationModeRequested_ == geometry_.edgeTesselationMode_) {
        return;
    }

    geometry_.clear();
    geometry_.edgeTesselationMode_ = edgeTesselationModeRequested_;

    geometry::Curve curve;
    curve.setPositionData(&ke->points());
    curve.setWidthData(&ke->widths());

    double maxAngle = 0.05;
    int minQuads = 1;
    int maxQuads = 64;
    if (edgeTesselationModeRequested_ <= 2) {
        if (edgeTesselationModeRequested_ == 0) {
            maxQuads = 1;
        }
        else if (edgeTesselationModeRequested_ == 1) {
            minQuads = 10;
            maxQuads = 10;
        }
        geometry::CurveSamplingParameters samplingParams = {};
        samplingParams.setMaxAngle(maxAngle * 0.5); // matches triangulate()
        samplingParams.setMinIntraSegmentSamples(minQuads - 1);
        samplingParams.setMaxIntraSegmentSamples(maxQuads - 1);
        curve.sampleRange(samplingParams, geometry_.samples_);
        geometry_.endSampleOverride_ = geometry_.samples_.length();
    }
    else {
        geometry_.triangulation_ = curve.triangulate(maxAngle, 1, 64);
    }

    const geometry::Vec2dArray& d = *curve.positionData();
    for (const geometry::Vec2d& p : d) {
        geometry_.cps_.emplaceLast(geometry::Vec2f(p));
    }

    // XXX shouldn't do it for draft -> add quality enum for current cached geometry
    if (v0_) {
        v0_->updateJoinsAndCaps();
    }
    if (v1_) {
        v1_->updateJoinsAndCaps();
    }

    cachedGraphics_.clear();
}

} // namespace vgc::workspace
