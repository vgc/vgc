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

bool VacEdgeCellFrameData::resetToStage(VacEdgeComputationStage stage) {
    if (stage >= stage_) {
        return false;
    }
    switch (stage_) {
    case VacEdgeComputationStage::StrokeMesh:
        if (stage == VacEdgeComputationStage::StrokeMesh) {
            break;
        }
        else {
            stroke_.reset(false);
            graphics_.clearStrokeGeometry();
            graphics_.clearJoinGeometry();
            [[fallthrough]];
        }
    case VacEdgeComputationStage::PostJoinGeometry:
        if (stage == VacEdgeComputationStage::PostJoinGeometry) {
            break;
        }
        else {
            // There is no dedicated post-join geometry at the moment.
            // Stroke mesh is computed directly from join patches (managed by vertices)
            // and pre-join geometry.
            [[fallthrough]];
        }
    case VacEdgeComputationStage::PreJoinGeometry:
        if (stage == VacEdgeComputationStage::PreJoinGeometry) {
            break;
        }
        else {
            patches_[0].clear();
            patches_[1].clear();
            samples_.clear();
            graphics_.clearCenterlineGeometry();
            graphics_.clearSelectionGeometry();
            [[fallthrough]];
        }
    case VacEdgeComputationStage::Clear:
        break;
    }
    stage_ = stage;
    return true;
}

bool VacEdgeCellFrameData::isSelectableAt(
    const geometry::Vec2d& position,
    bool outlineOnly,
    double tol,
    double* outDistance) const {

    using Vec2d = geometry::Vec2d;

    if (bbox_.isEmpty()) {
        return false;
    }

    geometry::Rect2d inflatedBbox = bbox_;
    inflatedBbox.setPMin(inflatedBbox.pMin() - Vec2d(tol, tol));
    inflatedBbox.setPMax(inflatedBbox.pMax() + Vec2d(tol, tol));
    if (!inflatedBbox.contains(position)) {
        return false;
    }
    // use "binary search"-style tree/array of bboxes?

    if (samples_.isEmpty()) {
        return false;
    }

    double shortestDistance = core::DoubleInfinity;

    auto it1 = samples_.begin();
    // is p in sample outline-mode-selection disk?
    shortestDistance =
        (std::min)(shortestDistance, (it1->position() - position).length());

    for (auto it0 = it1++; it1 != samples_.end(); it0 = it1++) {
        // is p in sample outline-mode-selection disk?
        shortestDistance =
            (std::min)(shortestDistance, (it1->position() - position).length());

        // in segment outline-mode-selection box?
        const Vec2d p0 = it0->position();
        const Vec2d p1 = it1->position();
        const Vec2d seg = (p1 - p0);
        const double seglen = seg.length();
        if (seglen > 0) { // if capsule is not a disk
            const Vec2d segdir = seg / seglen;
            const Vec2d p0p = position - p0;
            const double tx = p0p.dot(segdir);
            // does p project in segment?
            if (tx >= 0 && tx <= seglen) {
                const double ty = p0p.det(segdir);
                // does p project in slice?
                shortestDistance = (std::min)(shortestDistance, std::abs(ty));
            }
        }

        if (!outlineOnly) {
            // does p belongs to quad ?
            // only works for convex or hourglass quads atm
            const Vec2d r0 = it0->sidePoint(0);
            const Vec2d r0p = position - r0;
            if (it0->normal().det(r0p) <= 0) {
                const Vec2d l1 = it1->sidePoint(1);
                const Vec2d l1p = position - l1;
                if (it1->normal().det(l1p) >= 0) {
                    const Vec2d r1r0 = (r0 - it1->sidePoint(0));
                    const Vec2d l0l1 = (l1 - it0->sidePoint(1));
                    const bool a = r1r0.det(r0p) >= 0;
                    const bool b = l0l1.det(l1p) >= 0;
                    // approximate detection of hourglass case
                    // (false-positives but no false-negatives)
                    //if (r1r0.dot(l0l1) > 0) {
                    //    // naive test for "p in quad?" in the hourglass case
                    //    if (a || b) {
                    //        return true;
                    //    }
                    //}
                    //else
                    if (a && b) {
                        if (outDistance) {
                            *outDistance = 0;
                        }
                        return true;
                    }
                }
            }
        }
    }

    if (shortestDistance < tol) {
        if (outDistance) {
            *outDistance = shortestDistance;
        }
        return true;
    }

    return false;
}

namespace detail {

graphics::GeometryViewPtr
loadMeshGraphics(graphics::Engine* /*engine*/, const StuvMesh2d& /*mesh*/) {
    // TODO
    return {};
}

} // namespace detail

VacKeyEdge::~VacKeyEdge() {
    for (Int i = 0; i < 2; ++i) {
        VacKeyVertex* const vertex = verticesInfo_[i].element;
        if (vertex) {
            VacJoinHalfedge he(this, i == 0, 0);
            vertex->removeJoinHalfedge_(he);
            verticesInfo_[i].element = nullptr;
        }
    }
}

void VacKeyEdge::setTesselationMode(int mode) {
    int newMode = core::clamp(mode, 0, 3);
    if (edgeTesselationModeRequested_ != newMode) {
        edgeTesselationModeRequested_ = newMode;
        dirtyInputSampling_();
    }
}

std::optional<core::StringId> VacKeyEdge::domTagName() const {
    return dom::strings::edge;
}

geometry::Rect2d VacKeyEdge::boundingBox(core::AnimTime t) const {
    if (frameData_.time() == t) {
        return frameData_.bbox_;
    }
    return geometry::Rect2d::empty;
}

bool VacKeyEdge::isSelectableAt(
    const geometry::Vec2d& position,
    bool outlineOnly,
    double tol,
    double* outDistance,
    core::AnimTime t) const {

    if (frameData_.time() == t) {
        return frameData_.isSelectableAt(position, outlineOnly, tol, outDistance);
    }
    return false;
}

const VacKeyEdgeFrameData* VacKeyEdge::computeFrameData(VacEdgeComputationStage stage) {
    bool success = true;
    switch (stage) {
    case VacEdgeComputationStage::StrokeMesh:
        success = computeStrokeMesh_();
        break;
    case VacEdgeComputationStage::PreJoinGeometry:
        success = computePreJoinGeometry_();
        break;
    case VacEdgeComputationStage::PostJoinGeometry:
        success = computePostJoinGeometry_();
        break;
    case VacEdgeComputationStage::Clear:
        break;
    }
    return success ? &frameData_ : nullptr;
}

const VacEdgeCellFrameData*
VacKeyEdge::computeFrameDataAt(core::AnimTime t, VacEdgeComputationStage stage) {
    if (frameData_.time() == t) {
        return computeFrameData(stage);
    }
    return nullptr;
}

void VacKeyEdge::onPaintPrepare(core::AnimTime /*t*/, PaintOptions /*flags*/) {
    // todo, use paint options to not compute everything or with lower quality
    computeStrokeMesh_();
}

void VacKeyEdge::onPaintDraw(
    graphics::Engine* engine,
    core::AnimTime t,
    PaintOptions flags) const {

    VacKeyEdgeFrameData& data = frameData_;
    vacomplex::KeyEdge* ke = vacKeyEdgeNode();
    if (!ke || t != ke->time()) {
        return;
    }

    // if not already done (should we leave preparePaint_ optional?)
    const_cast<VacKeyEdge*>(this)->computeStrokeMesh_();

    using namespace graphics;
    namespace ds = dom::strings;

    const dom::Element* const domElement = this->domElement();
    // XXX "implicit" cells' domElement would be the composite ?

    constexpr PaintOptions strokeOptions = {PaintOption::Selected, PaintOption::Draft};

    // XXX todo: reuse geometry objects, create buffers separately (attributes waiting in EdgeGraphics).

    core::Color color = domElement->getAttribute(ds::color).getColor();

    EdgeGraphics& graphics = data.graphics_;
    bool hasNewStrokeGraphics = false;
    if ((flags.hasAny(strokeOptions) || !flags.has(PaintOption::Outline))
        && !graphics.strokeGeometry()) {

        hasNewStrokeGraphics = true;

        graphics.setStrokeGeometry(
            engine->createDynamicTriangleStripView(BuiltinGeometryLayout::XYUV_iRGBA));
        graphics.setJoinGeometry(engine->createDynamicTriangleStripView(
            BuiltinGeometryLayout::XYUV_iRGBA, IndexFormat::UInt32));

        GeometryViewCreateInfo createInfo = {};
        createInfo.setBuiltinGeometryLayout(BuiltinGeometryLayout::XYUV_iRGBA);
        createInfo.setPrimitiveType(PrimitiveType::TriangleStrip);
        createInfo.setVertexBuffer(0, graphics.strokeGeometry()->vertexBuffer(0));
        BufferPtr selectionInstanceBuffer = engine->createVertexBuffer(Int(4) * 4);
        createInfo.setVertexBuffer(1, selectionInstanceBuffer);
        graphics.setSelectionGeometry(engine->createGeometryView(createInfo));

        geometry::Vec2fArray strokeVertices;
        geometry::Vec2fArray joinVertices;
        core::Array<UInt32> joinIndices;

        if (data.samples_.size() >= 2) {

            const detail::EdgeJoinPatchMergeLocation& mergeLocation0 =
                data.patches_[0].mergeLocation;
            const detail::EdgeJoinPatchMergeLocation& mergeLocation1 =
                data.patches_[1].mergeLocation;

            auto standaloneSamples = core::Span(data.samples_);

            std::array<float, 2> mergeS = {
                0, static_cast<float>(standaloneSamples.last().s())};

            if (mergeLocation0.halfedgeNextSampleIndex > 0 && mergeLocation0.t < 1.0) {
                const geometry::CurveSample& s = mergeLocation0.sample;
                mergeS[0] = static_cast<float>(s.s());
                geometry::Vec2d p0 = s.leftPoint();
                geometry::Vec2d p1 = s.rightPoint();
                strokeVertices.emplaceLast(geometry::Vec2f(p0));
                strokeVertices.emplaceLast(
                    static_cast<float>(s.s()), static_cast<float>(-s.leftHalfwidth()));
                strokeVertices.emplaceLast(geometry::Vec2f(p1));
                strokeVertices.emplaceLast(
                    static_cast<float>(s.s()), static_cast<float>(s.rightHalfwidth()));
            }

            if ((mergeLocation0.halfedgeNextSampleIndex
                 + mergeLocation1.halfedgeNextSampleIndex)
                < standaloneSamples.length()) {

                auto coreSamples = core::Span(
                    standaloneSamples.begin() + mergeLocation0.halfedgeNextSampleIndex,
                    standaloneSamples.end() - mergeLocation1.halfedgeNextSampleIndex);
                for (const geometry::CurveSample& s : coreSamples) {
                    geometry::Vec2d p0 = s.leftPoint();
                    geometry::Vec2d p1 = s.rightPoint();
                    strokeVertices.emplaceLast(geometry::Vec2f(p0));
                    strokeVertices.emplaceLast(static_cast<float>(s.s()), -1.f);
                    //static_cast<float>(-s.leftHalfwidth()));
                    strokeVertices.emplaceLast(geometry::Vec2f(p1));
                    strokeVertices.emplaceLast(static_cast<float>(s.s()), 1.f);
                    //static_cast<float>(s.rightHalfwidth()));
                }
            }

            if (mergeLocation1.halfedgeNextSampleIndex > 0 && mergeLocation1.t < 1.0) {
                const geometry::CurveSample& s = mergeLocation1.sample;
                mergeS[1] = static_cast<float>(s.s());
                geometry::Vec2d p0 = s.leftPoint();
                geometry::Vec2d p1 = s.rightPoint();
                strokeVertices.emplaceLast(geometry::Vec2f(p0));
                strokeVertices.emplaceLast(
                    static_cast<float>(s.s()), static_cast<float>(-s.leftHalfwidth()));
                strokeVertices.emplaceLast(geometry::Vec2f(p1));
                strokeVertices.emplaceLast(
                    static_cast<float>(s.s()), static_cast<float>(s.rightHalfwidth()));
            }

            UInt32 joinIndex = core::int_cast<UInt32>(joinVertices.length());
            for (Int i = 0; i < 2; ++i) {
                const auto& patch = data.patches_[i];
                for (Int side = 0; side < 2; ++side) {
                    if (joinIndex > 0) {
                        joinIndices.emplaceLast(-1);
                    }
                    for (const auto& s : patch.sideSamples[side]) {
                        geometry::Vec2d cp = s.centerPoint;
                        geometry::Vec2d sp = s.sidePoint;
                        geometry::Vec2f spf(sp);
                        float sign = (side != i) ? -1.f : 1.f;
                        joinVertices.emplaceLast(spf);
                        joinVertices.emplaceLast(
                            geometry::Vec2f(s.sideSTUV[0], sign * s.sideSTUV[1]));
                        joinVertices.emplaceLast(geometry::Vec2f(cp));
                        joinVertices.emplaceLast(s.centerSU[0], 0.f);
                        // XXX use isLeft to make the strip CCW.
                        joinIndices.emplaceLast(joinIndex);
                        joinIndices.emplaceLast(joinIndex + 1);
                        joinIndex += 2;
                    }
                }
            }
        }

        engine->updateBufferData(
            graphics.strokeGeometry()->vertexBuffer(0), //
            std::move(strokeVertices));

        engine->updateBufferData(
            graphics.joinGeometry()->vertexBuffer(0), //
            std::move(joinVertices));
        engine->updateBufferData(
            graphics.joinGeometry()->indexBuffer(), //
            std::move(joinIndices));
    }
    if (graphics.strokeGeometry()
        && (data.hasPendingColorChange_ || hasNewStrokeGraphics)) {
        engine->updateBufferData(
            graphics.strokeGeometry()->vertexBuffer(1), //
            core::Array<float>({color.r(), color.g(), color.b(), color.a()}));
        engine->updateBufferData(
            graphics.joinGeometry()->vertexBuffer(1), //
            core::Array<float>({color.g(), color.b(), color.r(), color.a()}));
    }

    constexpr PaintOptions centerlineOptions = {
        PaintOption::Outline, PaintOption::Selected};
    bool hasNewCenterlineGraphics = false;
    if (flags.hasAny(centerlineOptions) && !graphics.centerlineGeometry()) {
        hasNewCenterlineGraphics = true;
        graphics.setCenterlineGeometry(engine->createDynamicTriangleStripView(
            BuiltinGeometryLayout::XYDxDy_iXYRotWRGBA));

        GeometryViewCreateInfo createInfo = {};
        createInfo.setBuiltinGeometryLayout(BuiltinGeometryLayout::XYDxDy_iXYRotWRGBA);
        createInfo.setPrimitiveType(PrimitiveType::TriangleStrip);
        createInfo.setVertexBuffer(0, graphics.centerlineGeometry()->vertexBuffer(0));
        BufferPtr selectionInstanceBuffer = engine->createVertexBuffer(0);
        createInfo.setVertexBuffer(1, selectionInstanceBuffer);
        graphics.setSelectionGeometry(engine->createGeometryView(createInfo));

        core::FloatArray lineInstData;
        lineInstData.extend({0.f, 0.f, 1.f, 2.f, 0.02f, 0.64f, 1.0f, 1.f});

        geometry::Vec4fArray lineVertices;
        for (const geometry::CurveSample& s : data.samples_) {
            geometry::Vec2f p = geometry::Vec2f(s.position());
            geometry::Vec2f n = geometry::Vec2f(s.normal());
            // clang-format off
            lineVertices.emplaceLast(p.x(), p.y(), -n.x(), -n.y());
            lineVertices.emplaceLast(p.x(), p.y(),  n.x(),  n.y());
            // clang-format on
        }

        engine->updateBufferData(
            graphics.centerlineGeometry()->vertexBuffer(0), std::move(lineVertices));
        engine->updateBufferData(
            graphics.centerlineGeometry()->vertexBuffer(1), std::move(lineInstData));
    }
    if (graphics.selectionGeometry()
        && (data.hasPendingColorChange_ || hasNewCenterlineGraphics)) {
        engine->updateBufferData(
            graphics.selectionGeometry()->vertexBuffer(1), //
            core::Array<float>(
                {0.f,
                 0.f,
                 1.f,
                 2.f,
                 0.0f, //std::round(1.0f - color.r()),
                 0.7f, //std::round(1.0f - color.g()),
                 1.0f, //std::round(1.0f - color.b()),
                 1.f}));
    }

    constexpr PaintOptions pointsOptions = {PaintOption::Outline};

    if (flags.hasAny(pointsOptions) && !controlPointsGeometry_) {
        controlPointsGeometry_ = engine->createDynamicTriangleStripView(
            BuiltinGeometryLayout::XYDxDy_iXYRotWRGBA);

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
        const Int numPoints = controlPoints_.length();
        const float dl = 1.f / numPoints;
        for (Int j = 0; j < numPoints; ++j) {
            geometry::Vec2f p = geometry::Vec2f(controlPoints_[j]);
            float l = j * dl;
            pointInstData.extend(
                {p.x(),
                 p.y(),
                 0.f,
                 1.5f,
                 (l > 0.5f ? 2 * (1.f - l) : 1.f),
                 0.f,
                 (l < 0.5f ? 2 * l : 1.f),
                 1.f});
        }

        engine->updateBufferData(
            controlPointsGeometry_->vertexBuffer(0), std::move(pointVertices));
        engine->updateBufferData(
            controlPointsGeometry_->vertexBuffer(1), std::move(pointInstData));
    }

    data.hasPendingColorChange_ = false;

    if (flags.has(PaintOption::Selected)) {
        engine->setProgram(graphics::BuiltinProgram::SreenSpaceDisplacement);
        engine->draw(graphics.selectionGeometry());
    }
    else if (!flags.has(PaintOption::Outline)) {
        engine->setProgram(graphics::BuiltinProgram::Simple);
        engine->draw(graphics.strokeGeometry());
        engine->draw(graphics.joinGeometry());
    }

    if (flags.has(PaintOption::Outline)) {
        engine->setProgram(graphics::BuiltinProgram::SreenSpaceDisplacement);
        engine->draw(graphics.centerlineGeometry());
        engine->drawInstanced(controlPointsGeometry_);
    }
}

void VacKeyEdge::onDependencyChanged_(Element* /*dependency*/, ChangeFlags changes) {
    if (changes.has(ChangeFlag::VertexPosition)) {
        dirtyPreJoinGeometry_();
    }
}

void VacKeyEdge::onDependencyRemoved_(Element* dependency) {
    for (VertexInfo& vi : verticesInfo_) {
        if (vi.element == dependency) {
            vi.element = nullptr;
        }
    }
}

ElementStatus VacKeyEdge::updateFromDom_(Workspace* workspace) {
    namespace ds = dom::strings;
    // TODO: update using owning composite when it is implemented
    dom::Element* const domElement = this->domElement();
    if (!domElement) {
        // TODO: throw ?
        onUpdateError_();
        return ElementStatus::InternalError;
    }

    // update dependencies
    std::array<std::optional<Element*>, 2> verticesOpt = {
        workspace->getElementFromPathAttribute(domElement, ds::startvertex, ds::vertex),
        workspace->getElementFromPathAttribute(domElement, ds::endvertex, ds::vertex)};

    std::array<VacKeyVertex*, 2> newVertices = {};
    for (Int i = 0; i < 2; ++i) {
        newVertices[i] = dynamic_cast<VacKeyVertex*>(verticesOpt[i].value_or(nullptr));
    }

    updateVertices_(newVertices);

    // What's the cleanest way to report/notify that this edge has actually changed ?
    // What are the different categories of changes that matter to dependents ?
    // For instance an edge wanna know if a vertex moves or has a new style (new join)

    if (verticesOpt[0].has_value() != verticesOpt[1].has_value()) {
        onUpdateError_();
        return ElementStatus::InvalidAttribute;
    }

    bool isClosed = true;
    if (verticesOpt[0].has_value()) {
        for (Int i = 0; i < 2; ++i) {
            if (!newVertices[i]) {
                onUpdateError_();
                return ElementStatus::UnresolvedDependency;
            }
        }
        isClosed = false;
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
    Element* parentElement = parent();
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

    ChangeFlags changeFlags = {};

    const auto& points = domElement->getAttribute(ds::positions).getVec2dArray();
    const auto& widths = domElement->getAttribute(ds::widths).getDoubleArray();

    bool hasInputGeometryChanged = true;
    bool hasBoundaryChanged = true;
    if (ke) {
        if (&ke->points() == &points.get() && &ke->widths() == &widths.get()) {
            hasInputGeometryChanged = false;
        }
        if (vacKvs[0] == ke->startVertex() && vacKvs[1] == ke->endVertex()) {
            hasBoundaryChanged = false;
        }
        else {
            // must rebuild
            removeVacNode();
            ke = nullptr;
        }
    }

    // create/rebuild/update vac node
    if (!ke) {
        if (isClosed) {
            ke = topology::ops::createKeyClosedEdge(points, widths, parentGroup);
        }
        else {
            ke = topology::ops::createKeyOpenEdge(
                vacKvs[0], vacKvs[1], points, widths, parentGroup);
        }
        if (!ke) {
            onUpdateError_();
            return ElementStatus::InvalidAttribute;
        }
        setVacNode(ke);
    }
    else if (hasInputGeometryChanged) {
        topology::ops::setKeyEdgeCurvePoints(ke, points);
        topology::ops::setKeyEdgeCurveWidths(ke, widths);
    }

    // dirty cached data
    if (hasInputGeometryChanged) {
        dirtyInputSampling_(false);
    }
    else if (hasBoundaryChanged) {
        dirtyPreJoinGeometry_(false);
    }

    const auto& color = domElement->getAttribute(ds::color).getColor();
    if (frameData_.color_ != color) {
        frameData_.color_ = color;
        frameData_.hasPendingColorChange_ = true;
        pendingNotifyChanges_.set(ChangeFlag::Color);
    }

    notifyChanges_();
    return ElementStatus::Ok;
}

void VacKeyEdge::updateFromVac_() {
    namespace ds = dom::strings;
    vacomplex::KeyEdge* ke = vacKeyEdgeNode();
    if (!ke) {
        if (status() != ElementStatus::Ok) {
            // Element is already corrupt, no need to fail loudly.
            return;
        }
        // TODO: error or throw ?
        return;
    }

    dom::Element* const domElement = this->domElement();
    if (!domElement) {
        // TODO: use owning composite when implemented
        return;
    }

    const auto& points = domElement->getAttribute(ds::positions).getVec2dArray();
    if (ke->points() != points) {
        domElement->setAttribute(ds::positions, points);
    }

    const auto& widths = domElement->getAttribute(ds::widths).getDoubleArray();
    if (ke->widths() != widths) {
        domElement->setAttribute(ds::widths, widths);
    }

    const Workspace* w = workspace();
    std::array<VacKeyVertex*, 2> oldVertices = {
        verticesInfo_[0].element, verticesInfo_[1].element};
    // TODO: check bool(ke->startVertex()) == bool(newVertices[0])
    //             bool(ke->endVertex()) == bool(newVertices[1])
    std::array<VacKeyVertex*, 2> newVertices = {
        static_cast<VacKeyVertex*>(w->findVacElement(ke->startVertex())),
        static_cast<VacKeyVertex*>(w->findVacElement(ke->endVertex())),
    };
    updateVertices_(newVertices);

    // TODO: check domElement() != nullptr
    if (oldVertices[0] != newVertices[0]) {
        domElement->setAttribute(
            ds::startvertex, newVertices[0]->domElement()->getPathFromId());
    }
    if (oldVertices[1] != newVertices[1]) {
        domElement->setAttribute(
            ds::startvertex, newVertices[1]->domElement()->getPathFromId());
    }
}

void VacKeyEdge::updateVertices_(const std::array<VacKeyVertex*, 2>& newVertices) {
    for (Int i = 0; i < 2; ++i) {
        VacKeyVertex* const oldVertex = verticesInfo_[i].element;
        VacKeyVertex* const otherVertex = verticesInfo_[1 - i].element;
        VacKeyVertex* const newVertex = newVertices[i];
        if (oldVertex != newVertex) {
            if (oldVertex != otherVertex) {
                removeDependency(oldVertex);
            }
            if (newVertex != otherVertex) {
                addDependency(newVertex);
            }
            VacJoinHalfedge he(this, i == 0, 0);
            if (oldVertex) {
                oldVertex->removeJoinHalfedge_(he);
            }
            if (newVertex) {
                newVertex->addJoinHalfedge_(he);
            }
            verticesInfo_[i].element = newVertex;
        }
    }
}

void VacKeyEdge::notifyChanges_() {
    pendingNotifyChanges_.unset(alreadyNotifiedChanges_);
    if (pendingNotifyChanges_) {
        alreadyNotifiedChanges_.set(pendingNotifyChanges_);
        notifyChangesToDependents(pendingNotifyChanges_);
        if (pendingNotifyChanges_.has(ChangeFlag::EdgePreJoinGeometry)) {
            for (VertexInfo& vi : verticesInfo_) {
                if (vi.element) {
                    vi.element->onJoinEdgePreJoinGeometryChanged_(this);
                }
            }
        }
    }
}

bool VacKeyEdge::computeInputSampling_() {
    VacKeyEdgeFrameData& data = frameData_;
    if (!isInputSamplingDirty_) {
        return true;
    }
    VGC_ASSERT(!data.isComputing_);

    vacomplex::KeyEdge* ke = vacKeyEdgeNode();
    if (!ke) {
        return false;
    }

    data.isComputing_ = true;

    double maxAngle = 0.05;
    Int minQuads = 1;
    Int maxQuads = 64;
    switch (edgeTesselationModeRequested_) {
    case 0:
        break;
    case 1:
        maxQuads = 1;
        break;
    case 2:
        minQuads = 2;
        maxQuads = 8;
        break;
    default:
        minQuads = 64;
        maxQuads = 64;
        break;
    }
    edgeTesselationMode_ = edgeTesselationModeRequested_;

    geometry::Curve curve;
    curve.setPositions(ke->points());
    curve.setWidths(ke->widths());
    geometry::CurveSamplingParameters samplingParams = {};
    samplingParams.setMaxAngle(maxAngle * 0.5); // matches triangulate()
    samplingParams.setMinIntraSegmentSamples(minQuads - 1);
    samplingParams.setMaxIntraSegmentSamples(maxQuads - 1);
    curve.sampleRange(samplingParams, inputSamples_);
    if (inputSamples_.length()) {
        auto it = inputSamples_.begin();
        geometry::Vec2d lastPoint = it->position();
        double s = 0;
        for (++it; it != inputSamples_.end(); ++it) {
            geometry::Vec2d point = it->position();
            s += (point - lastPoint).length();
            it->setS(s);
            lastPoint = point;
        }
    }
    samplingVersion_++;

    for (const geometry::Vec2d& p : curve.positions()) {
        controlPoints_.emplaceLast(geometry::Vec2f(p));
    }

    isInputSamplingDirty_ = false;
    data.isComputing_ = false;
    return true;
}

bool VacKeyEdge::computePreJoinGeometry_() {
    VacKeyEdgeFrameData& data = frameData_;
    if (data.stage_ >= VacEdgeComputationStage::PreJoinGeometry) {
        return true;
    }
    VGC_ASSERT(!data.isComputing_);

    vacomplex::KeyEdge* ke = vacKeyEdgeNode();
    if (!ke) {
        return false;
    }

    if (!computeInputSampling_()) {
        return false;
    }

    data.isComputing_ = true;

    // TODO: compute vertices pos and snap edge geometry
    data.samples_ = inputSamples_;
    data.bbox_ = geometry::Rect2d::empty;
    for (auto& sample : data.samples_) {
        data.bbox_.uniteWith(sample.position());
    }

    alreadyNotifiedChanges_.unset(ChangeFlag::EdgePreJoinGeometry);
    data.stage_ = VacEdgeComputationStage::PreJoinGeometry;
    data.isComputing_ = false;
    return true;
}

bool VacKeyEdge::computePostJoinGeometry_() {
    VacEdgeCellFrameData& data = frameData_;
    if (data.stage_ >= VacEdgeComputationStage::PostJoinGeometry) {
        return true;
    }
    VGC_ASSERT(!data.isComputing_);

    if (!computePreJoinGeometry_()) {
        return false;
    }

    data.isComputing_ = true;

    // XXX shouldn't do it for draft -> add quality enum for current cached geometry
    VacKeyVertex* v0 = verticesInfo_[0].element;
    if (v0) {
        v0->computeJoin_();
    }
    VacKeyVertex* v1 = verticesInfo_[1].element;
    if (v1 && v1 != v0) {
        v1->computeJoin_();
    }

    alreadyNotifiedChanges_.unset(ChangeFlag::EdgePostJoinGeometry);
    data.stage_ = VacEdgeComputationStage::PostJoinGeometry;
    data.isComputing_ = false;
    return true;
}

bool VacKeyEdge::computeStrokeMesh_() {
    VacEdgeCellFrameData& data = frameData_;
    if (data.stage_ >= VacEdgeComputationStage::StrokeMesh) {
        return true;
    }
    VGC_ASSERT(!data.isComputing_);

    if (!computePostJoinGeometry_()) {
        return false;
    }

    data.isComputing_ = true;

    // TODO: use mesh builder
    // TODO: implement overlaps removal pass

    alreadyNotifiedChanges_.unset(ChangeFlag::EdgeStrokeMesh);
    data.stage_ = VacEdgeComputationStage::StrokeMesh;
    data.isComputing_ = false;
    return true;
}

void VacKeyEdge::dirtyInputSampling_(bool notifyDependents) {
    if (!isInputSamplingDirty_) {
        controlPoints_.clear();
        controlPointsGeometry_.reset();
        inputSamples_.clear();
        edgeTesselationMode_ = -1;
        dirtyPreJoinGeometry_(notifyDependents);
        isInputSamplingDirty_ = true;
    }
}

void VacKeyEdge::dirtyPreJoinGeometry_(bool notifyDependents) {
    if (!alreadyNotifiedChanges_.has(ChangeFlag::EdgePreJoinGeometry)) {
        frameData_.resetToStage(VacEdgeComputationStage::Clear);
        pendingNotifyChanges_.set(
            {ChangeFlag::EdgePreJoinGeometry,
             ChangeFlag::EdgePostJoinGeometry,
             ChangeFlag::EdgeStrokeMesh});
        if (notifyDependents) {
            notifyChanges_();
        }
    }
}

void VacKeyEdge::dirtyPostJoinGeometry_(bool notifyDependents) {
    if (!alreadyNotifiedChanges_.has(ChangeFlag::EdgePostJoinGeometry)) {
        frameData_.resetToStage(VacEdgeComputationStage::PreJoinGeometry);
        pendingNotifyChanges_.set(
            {ChangeFlag::EdgePostJoinGeometry, ChangeFlag::EdgeStrokeMesh});
        if (notifyDependents) {
            notifyChanges_();
        }
    }
}

void VacKeyEdge::dirtyStrokeMesh_(bool notifyDependents) {
    if (!alreadyNotifiedChanges_.has(ChangeFlag::EdgeStrokeMesh)) {
        frameData_.resetToStage(VacEdgeComputationStage::PostJoinGeometry);
        pendingNotifyChanges_.set({ChangeFlag::EdgeStrokeMesh});
        if (notifyDependents) {
            notifyChanges_();
        }
    }
}

// called by one of the end vertex
void VacKeyEdge::dirtyJoinDataAtVertex_(const VacVertexCell* vertexCell) {
    if (!alreadyNotifiedChanges_.has(ChangeFlag::EdgePreJoinGeometry)) {
        dirtyPostJoinGeometry_(false);
        if (verticesInfo_[0].element == vertexCell) {
            frameData_.patches_[0].clear();
        }
        if (verticesInfo_[1].element == vertexCell) {
            frameData_.patches_[1].clear();
        }
        notifyChanges_();
    }
}

void VacKeyEdge::onUpdateError_() {
    removeVacNode();
    dirtyInputSampling_();
}

} // namespace vgc::workspace
