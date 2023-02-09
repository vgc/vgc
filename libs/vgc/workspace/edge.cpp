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

#include <vgc/core/span.h>
#include <vgc/workspace/edge.h>
#include <vgc/workspace/vertex.h>

#include <vgc/workspace/workspace.h>

namespace vgc::workspace {

const VacEdgeCellFrameData* VacEdgeCell::computeStandaloneGeometryAt(core::AnimTime t) {
    VacEdgeCellFrameData* data = frameData(t);
    if (data) {
        computeStandaloneGeometry(*data);
    }
    return data;
}

const VacEdgeCellFrameData* VacEdgeCell::computeGeometryAt(core::AnimTime t) {
    VacEdgeCellFrameData* data = frameData(t);
    if (data) {
        computeStandaloneGeometry(*data);
        computeGeometry(*data);
    }
    return data;
}

geometry::Rect2d VacKeyEdge::boundingBox(core::AnimTime /*t*/) const {
    return geometry::Rect2d::empty;
}

ElementStatus VacKeyEdge::updateFromDom_(Workspace* workspace) {
    namespace ds = dom::strings;
    dom::Element* const domElement = this->domElement();

    std::array<std::optional<Element*>, 2> verticesOpt = {
        workspace->getElementFromPathAttribute(domElement, ds::startvertex, ds::vertex),
        workspace->getElementFromPathAttribute(domElement, ds::endvertex, ds::vertex)};

    Element* parentElement = this->parent();

    std::array<VacKeyVertex*, 2> oldVertices = {};
    std::array<VacKeyVertex*, 2> newVertices = {};
    for (Int i = 0; i < 2; ++i) {
        VacKeyVertex* const oldVertex = verticesInfo_[i].element;
        Element* const newVertexElement = verticesOpt[i].value_or(nullptr);
        // XXX impl custom safe cast
        VacKeyVertex* const newVertex = dynamic_cast<VacKeyVertex*>(newVertexElement);
        oldVertices[i] = oldVertex;
        newVertices[i] = newVertex;
        if (replaceDependency(oldVertex, newVertex)) {
            detail::VacJoinHalfedge he(this, i == 1, 0);
            if (oldVertex) {
                oldVertex->removeJoinHalfedge_(he);
            }
            if (newVertex) {
                newVertex->addJoinHalfedge_(he);
            }
            verticesInfo_[i].element = newVertex;
        }
    }

    // What's the cleanest way to report/notify that this edge has actually changed ?
    // What are the different categories of changes that matter to dependents ?
    // For instance an edge wanna know if a vertex moves or has a new style (new join)

    if (verticesOpt[0].has_value() != verticesOpt[1].has_value()) {
        onUpdateError_();
        return ElementStatus::InvalidAttribute;
    }

    for (Int i = 0; i < 2; ++i) {
        if (verticesOpt[i].has_value() && !newVertices[i]) {
            onUpdateError_();
            return ElementStatus::UnresolvedDependency;
        }
    }

    // update vac to get vertex nodes
    std::array<vacomplex::KeyVertex*, 2> vacKvs = {};
    for (Int i = 0; i < 2; ++i) {
        VacKeyVertex* v = newVertices[i];
        if (v) {
            workspace->updateElementFromDom(v);
            vacKvs[i] = v->vacKeyVertexNode();
            if (v->hasError() || !vacKvs[i]) {
                onUpdateError_();
                return ElementStatus::ErrorInDependency;
            }
        }
    }

    // update group
    vacomplex::Group* parentGroup = nullptr;
    if (parentElement) {
        workspace->updateElementFromDom(parentElement);
        vacomplex::Node* parentNode = parentElement->vacNode();
        if (parentNode) {
            // checked cast to group, could be something invalid
            parentGroup = parentNode->toGroup();
        }
    }
    if (!parentGroup) {
        onUpdateError_();
        return ElementStatus::ErrorInParent;
    }

    vacomplex::KeyEdge* ke = vacKeyEdgeNode();

    // check if it needs to be rebuilt
    if (ke) {
        vacomplex::KeyVertex* oldKv0 = ke->startVertex();
        vacomplex::KeyVertex* oldKv1 = ke->endVertex();
        if (vacKvs[0] != oldKv0 || vacKvs[0] != oldKv1) {
            removeVacNode();
            ke = nullptr;
        }
    }

    if (!ke) {
        const core::Id id = domElement->internalId();
        if (vacKvs[0] && vacKvs[1]) {
            ke = topology::ops::createKeyOpenEdge(id, parentGroup, vacKvs[0], vacKvs[1]);
        }
        else {
            // XXX warning if kv0 || kv1 ?
            ke = topology::ops::createKeyClosedEdge(id, parentGroup);
        }
        setVacNode(ke);
    }

    // XXX warning on null parent group ?

    if (ke) {
        const auto& points = domElement->getAttribute(ds::positions).getVec2dArray();
        const auto& widths = domElement->getAttribute(ds::widths).getDoubleArray();

        if (&ke->points() != &points.get() || &ke->widths() != &widths.get()) {
            topology::ops::setKeyEdgeCurvePoints(ke, points);
            topology::ops::setKeyEdgeCurveWidths(ke, widths);
            onInputGeometryChanged();
        }

        // XXX should we snap here ?
        //     group view matrices may not be ready..
        //     maybe we could add two init functions to workspace::Element
        //     one for intrinsic data, one for dependent data.

        return ElementStatus::Ok;
    }

    onUpdateError_();
    return ElementStatus::InvalidAttribute;
}

void VacKeyEdge::onDependencyBeingDestroyed_(Element* dependency) {
    for (VertexInfo& vi : verticesInfo_) {
        if (vi.element == dependency) {
            vi.element = nullptr;
        }
    }
}

void VacKeyEdge::preparePaint_(core::AnimTime t, PaintOptions /*flags*/) {
    // todo, use paint options to not compute everything or with lower quality
    computeGeometryAt(t);
}

void VacKeyEdge::paint_(graphics::Engine* engine, core::AnimTime t, PaintOptions flags)
    const {

    topology::KeyEdge* ke = vacKeyEdgeNode();
    if (t != ke->time()) {
        return;
    }

    // if not already done (should we leave preparePaint_ optional?)
    const_cast<VacKeyEdge*>(this)->computeGeometry(frameData_);

    using namespace graphics;
    namespace ds = dom::strings;

    const dom::Element* const domElement = this->domElement();
    // XXX "implicit" cells' domElement would be the composite ?

    constexpr PaintOptions strokeOptions = {PaintOption::Selected, PaintOption::Draft};

    // XXX todo: reuse geometry objects, create buffers separately (attributes waiting in EdgeGraphics).

    EdgeGraphics& graphics = frameData_.graphics_;

    if (flags.hasAny(strokeOptions)
        || (!flags.has(PaintOption::Outline) && !graphics.strokeGeometry_)) {
        graphics.strokeGeometry_ =
            engine->createDynamicTriangleStripView(BuiltinGeometryLayout::XY_iRGBA);
        graphics.joinGeometry_ = engine->createDynamicTriangleStripView(
            BuiltinGeometryLayout::XY_iRGBA, IndexFormat::UInt32);

        GeometryViewCreateInfo createInfo = {};
        createInfo.setBuiltinGeometryLayout(BuiltinGeometryLayout::XY_iRGBA);
        createInfo.setPrimitiveType(PrimitiveType::TriangleStrip);
        createInfo.setVertexBuffer(0, graphics.strokeGeometry_->vertexBuffer(0));
        BufferPtr selectionInstanceBuffer = engine->createVertexBuffer(Int(4) * 4);
        createInfo.setVertexBuffer(1, selectionInstanceBuffer);
        graphics.selectionGeometry_ = engine->createGeometryView(createInfo);

        core::Color color = domElement->getAttribute(ds::color).getColor();

        geometry::Vec2fArray strokeVertices;
        geometry::Vec2fArray joinVertices;
        core::Array<UInt32> joinIndices;

        if (edgeTesselationModeRequested_ <= 2) {

            std::array<Int, 2> maxSampleOverrides = {
                std::max(
                    frameData_.patches_[0].sampleOverride_,
                    frameData_.patches_[1].sampleOverride_),
                std::max(
                    frameData_.patches_[2].sampleOverride_,
                    frameData_.patches_[3].sampleOverride_)};
            Int totalOverride = maxSampleOverrides[0] + maxSampleOverrides[1];

            auto samples = core::Span(frameData_.samples_);
            auto coreSamples =
                samples.subspan(maxSampleOverrides[0], samples.length() - totalOverride);

            for (const geometry::CurveSample& s : coreSamples) {
                geometry::Vec2d p0 = s.leftPoint();
                geometry::Vec2d p1 = s.rightPoint();
                strokeVertices.emplaceLast(geometry::Vec2f(p0));
                strokeVertices.emplaceLast(geometry::Vec2f(p1));
            }

            auto getSampleSidePoint = [](const geometry::CurveSample& s, bool isLeft) {
                return isLeft ? s.leftPoint() : s.rightPoint();
            };

            for (Int i = 0; i < 4; ++i) {
                const auto& patch = frameData_.patches_[i];
                const bool isStart = i < 2;
                const bool isLeft = i % 2 == 0;
                if (isStart) {
                    if (patch.sampleOverride_ < maxSampleOverrides[0]) {
                        auto fillSamples = samples.subspan(
                            patch.sampleOverride_,
                            maxSampleOverrides[0] - patch.sampleOverride_ + 1);
                        UInt32 index = core::int_cast<UInt32>(joinVertices.length());
                        if (joinIndices.length()) {
                            joinIndices.emplaceLast(-1);
                        }
                        for (const geometry::CurveSample& s : fillSamples) {
                            geometry::Vec2d cp = s.position();
                            geometry::Vec2d sp = getSampleSidePoint(s, isLeft);
                            joinVertices.emplaceLast(geometry::Vec2f(sp));
                            joinVertices.emplaceLast(geometry::Vec2f(cp));
                            joinIndices.emplaceLast(index);
                            joinIndices.emplaceLast(index + 1);
                            index += 2;
                        }
                    }
                }
                else {
                    if (patch.sampleOverride_ < maxSampleOverrides[1]) {
                        auto fillSamples = samples.subspan(
                            samples.length() - 1 - maxSampleOverrides[1],
                            maxSampleOverrides[1] - patch.sampleOverride_ + 1);
                        UInt32 index = core::int_cast<UInt32>(joinVertices.length());
                        if (joinIndices.length()) {
                            joinIndices.emplaceLast(-1);
                        }
                        for (const geometry::CurveSample& s : fillSamples) {
                            geometry::Vec2d cp = s.position();
                            geometry::Vec2d sp = getSampleSidePoint(s, isLeft);
                            joinVertices.emplaceLast(geometry::Vec2f(sp));
                            joinVertices.emplaceLast(geometry::Vec2f(cp));
                            joinIndices.emplaceLast(index);
                            joinIndices.emplaceLast(index + 1);
                            index += 2;
                        }
                    }
                }
            }
        }
        else {
            for (const geometry::Vec2d& p : frameData_.triangulation_) {
                strokeVertices.emplaceLast(geometry::Vec2f(p));
            }
        }
        engine->updateBufferData(
            graphics.strokeGeometry_->vertexBuffer(0), //
            std::move(strokeVertices));
        engine->updateBufferData(
            graphics.strokeGeometry_->vertexBuffer(1), //
            core::Array<float>({color.r(), color.g(), color.b(), color.a()}));

        engine->updateBufferData(
            graphics.joinGeometry_->vertexBuffer(0), //
            std::move(joinVertices));
        engine->updateBufferData(
            graphics.joinGeometry_->vertexBuffer(1), //
            core::Array<float>({color.g(), color.b(), color.r(), color.a()}));
        engine->updateBufferData(
            graphics.joinGeometry_->indexBuffer(), //
            std::move(joinIndices));

        engine->updateBufferData(
            selectionInstanceBuffer, //
            core::Array<float>(
                {1.0f - color.r(), 1.0f - color.g(), 1.0f - color.b(), 1.0f}));
    }

    constexpr PaintOptions centerlineOptions = {PaintOption::Outline};

    if (flags.hasAny(centerlineOptions) && !graphics.centerlineGeometry_) {
        graphics.centerlineGeometry_ = engine->createDynamicTriangleStripView(
            BuiltinGeometryLayout::XYDxDy_iXYRotRGBA);

        core::FloatArray lineInstData;
        lineInstData.extend({0.f, 0.f, 1.f, 0.02f, 0.64f, 1.0f, 1.f, 0.f /*padding*/});

        float lineHalfSize = 2.f;

        geometry::Vec4fArray lineVertices;
        for (const geometry::CurveSample& s : frameData_.samples_) {
            geometry::Vec2f p = geometry::Vec2f(s.position());
            geometry::Vec2f n = geometry::Vec2f(s.normal());
            // clang-format off
            lineVertices.emplaceLast(p.x(), p.y(), -lineHalfSize * n.x(), -lineHalfSize * n.y());
            lineVertices.emplaceLast(p.x(), p.y(),  lineHalfSize * n.x(),  lineHalfSize * n.y());
            // clang-format on
        }

        engine->updateBufferData(
            graphics.centerlineGeometry_->vertexBuffer(0), std::move(lineVertices));
        engine->updateBufferData(
            graphics.centerlineGeometry_->vertexBuffer(1), std::move(lineInstData));
    }

    constexpr PaintOptions pointsOptions = {PaintOption::Outline};

    if (flags.hasAny(pointsOptions) && !graphics.pointsGeometry_) {
        graphics.pointsGeometry_ = engine->createDynamicTriangleStripView(
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
        const Int numPoints = frameData_.controlPoints_.length();
        const float dl = 1.f / numPoints;
        for (Int j = 0; j < numPoints; ++j) {
            geometry::Vec2f p = frameData_.controlPoints_[j];
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
            graphics.pointsGeometry_->vertexBuffer(0), std::move(pointVertices));
        engine->updateBufferData(
            graphics.pointsGeometry_->vertexBuffer(1), std::move(pointInstData));
    }

    if (flags.has(PaintOption::Selected)) {
        engine->setProgram(graphics::BuiltinProgram::Simple);
        engine->draw(graphics.selectionGeometry_);
    }
    else if (!flags.has(PaintOption::Outline)) {
        engine->setProgram(graphics::BuiltinProgram::Simple);
        engine->draw(graphics.strokeGeometry_);
        engine->draw(graphics.joinGeometry_);
    }

    if (flags.has(PaintOption::Outline)) {
        engine->setProgram(graphics::BuiltinProgram::SreenSpaceDisplacement);
        engine->draw(graphics.centerlineGeometry_);
        engine->drawInstanced(graphics.pointsGeometry_);
    }
}

void VacKeyEdge::onVacNodeRemoved_() {
    onInputGeometryChanged();
}

VacEdgeCellFrameData* VacKeyEdge::frameData(core::AnimTime t) const {
    vacomplex::EdgeCell* cell = vacEdgeCellNode();
    if (!cell) {
        return nullptr;
    }
    if (frameData_.time() == t) {
        return &frameData_;
    }
    return nullptr;
}

void VacKeyEdge::computeStandaloneGeometry(VacEdgeCellFrameData& data) {

    if (edgeTesselationModeRequested_ == data.edgeTesselationMode_) {
        return;
    }
    if (data.isStandaloneGeometryComputed_ || data.isComputing_) {
        return;
    }
    topology::KeyEdge* ke = vacKeyEdgeNode();
    if (!ke) {
        return;
    }

    data.isComputing_ = true;
    VGC_DEBUG_TMP("VacKeyEdge({})->computeStandaloneGeometry_", (void*)this);

    // TODO: compute vertices pos and snap

    // check if we need an update

    data.edgeTesselationMode_ = edgeTesselationModeRequested_;

    geometry::Curve curve;
    curve.setPositions(ke->points());
    curve.setWidths(ke->widths());

    double maxAngle = 0.05;
    Int minQuads = 1;
    Int maxQuads = 64;
    if (edgeTesselationModeRequested_ <= 2) {
        if (edgeTesselationModeRequested_ == 0) {
            maxQuads = 1;
        }
        else if (edgeTesselationModeRequested_ == 1) {
            minQuads = 1;
            maxQuads = 8;
        }
        geometry::CurveSamplingParameters samplingParams = {};
        samplingParams.setMaxAngle(maxAngle * 0.5); // matches triangulate()
        samplingParams.setMinIntraSegmentSamples(minQuads - 1);
        samplingParams.setMaxIntraSegmentSamples(maxQuads - 1);
        curve.sampleRange(samplingParams, data.samples_);
    }
    else {
        data.triangulation_ = curve.triangulate(maxAngle, 1, 64);
    }
    data.samplingVersion_++;

    for (const geometry::Vec2d& p : curve.positions()) {
        data.controlPoints_.emplaceLast(geometry::Vec2f(p));
    }

    data.isStandaloneGeometryComputed_ = true;
    data.isComputing_ = false;

    data.graphics_.clear();
}

void VacKeyEdge::computeGeometry(VacEdgeCellFrameData& data) {

    if (data.isGeometryComputed_ || data.isComputing_) {
        return;
    }
    topology::KeyEdge* ke = vacKeyEdgeNode();
    if (!ke) {
        return;
    }

    computeStandaloneGeometry(data);

    data.isComputing_ = true;
    VGC_DEBUG_TMP("VacKeyEdge({})->computeGeometry_", (void*)this);

    // XXX shouldn't do it for draft -> add quality enum for current cached geometry
    VacKeyVertex* v0 = verticesInfo_[0].element;
    if (v0) {
        v0->computeJoin(ke->time());
    }
    VacKeyVertex* v1 = verticesInfo_[1].element;
    if (v1 && v1 != v0) {
        v1->computeJoin(ke->time());
    }

    data.isGeometryComputed_ = true;
    data.isComputing_ = false;

    // XXX clear less ?
    data.graphics_.clear();
}

void VacKeyEdge::onInputGeometryChanged() {
    frameData_.clear();
    for (VertexInfo& vi : verticesInfo_) {
        if (vi.element) {
            vi.element->onJoinEdgeGeometryChanged_(this);
        }
    }
}

void VacKeyEdge::clearStartJoinData() {
    frameData_.clearStartJoinData();
}

void VacKeyEdge::clearEndJoinData() {
    frameData_.clearEndJoinData();
}

void VacKeyEdge::clearJoinData() {
    frameData_.clearStartJoinData();
    frameData_.clearEndJoinData();
}

void VacKeyEdge::onUpdateError_() {
    removeVacNode();
}

} // namespace vgc::workspace
