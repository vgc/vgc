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

#include <vgc/core/span.h>
#include <vgc/geometry/triangle2d.h>
#include <vgc/workspace/colors.h>
#include <vgc/workspace/edgegeometry.h>
#include <vgc/workspace/freehandedgegeometry.h>
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
            strokeBbox_ = geometry::Rect2d::empty;
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
            bbox_ = geometry::Rect2d::empty;
            patches_[0].clear();
            patches_[1].clear();
            sampling_.reset();
            graphics_.clearCenterlineGeometry();
            graphics_.clearSelectionGeometry();
            selectionTestCacheRect_ = geometry::Rect2d::empty;
            selectionTestCacheResult_ = false;
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
    inflatedBbox.uniteWith(strokeBbox_);
    if (!inflatedBbox.contains(position)) {
        return false;
    }

    if (!outlineOnly) {
        bool isContained = false;
        // strip iteration
        const geometry::Vec2dArray& strokeVertices = this->stroke_.xyVertices();
        const core::IntArray& strokeIndices = this->stroke_.indices();
        Int j = 0;
        bool isTriangleDefined = false;
        geometry::Triangle2d triangle;
        for (Int i : strokeIndices) {
            if (i == StuvMesh2d::primitiveRestartIndex) {
                // new strip
                isTriangleDefined = false;
                j = 0;
            }
            else {
                triangle[j] = strokeVertices[i];
                j += 1;

                if (j == 3) {
                    isTriangleDefined = true;
                    j = 0;
                }

                if (isTriangleDefined && triangle.contains(position)) {
                    isContained = true;
                    break;
                }
            }
        }
        if (isContained) {
            if (outDistance) {
                *outDistance = 0;
            }
            return true;
        }
    }

    // use "binary search"-style tree/array of bboxes?

    if (!sampling_ || sampling_->samples().isEmpty()) {
        return false;
    }

    double shortestDistance = core::DoubleInfinity;

    const geometry::StrokeSample2dArray& samples = sampling_->samples();
    auto it1 = samples.begin();
    // is p in sample outline-mode-selection disk?
    shortestDistance =
        (std::min)(shortestDistance, (it1->position() - position).length());

    for (auto it0 = it1++; it1 != samples.end(); it0 = it1++) {
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
    }

    if (shortestDistance <= tol) {
        if (outDistance) {
            *outDistance = shortestDistance;
        }
        return true;
    }

    return false;
}

bool VacEdgeCellFrameData::isSelectableInRect(const geometry::Rect2d& rect) const {
    using Vec2d = geometry::Vec2d;

    if (rect.isEmpty()) {
        return false;
    }

    if (selectionTestCacheRect_ == rect) {
        return selectionTestCacheResult_;
    }

    if (bbox_.isEmpty() || !bbox_.intersects(rect)) {
        return false;
    }

    // use "binary search"-style tree/array of bboxes?

    if (!sampling_ || sampling_->samples().isEmpty()) {
        return false;
    }

    const geometry::StrokeSample2dArray& samples = sampling_->samples();

    struct SamplePositionGetter {
        Vec2d operator()(const geometry::StrokeSample2d& s) {
            return s.position();
        }
    };

    bool result =
        rect.intersectsPolyline(samples.begin(), samples.end(), SamplePositionGetter());

    // We only cache the result if it reaches the intersectsPolyline() test because
    // it seems better to only cache "costly" result.
    selectionTestCacheRect_ = rect;
    selectionTestCacheResult_ = result;

    return result;
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
            VacJoinHalfedge he(this, i > 0, 0);
            vertex->removeJoinHalfedge_(he);
            verticesInfo_[i].element = nullptr;
        }
    }
}

void VacKeyEdge::setTesselationMode(geometry::CurveSamplingQuality mode) {
    if (edgeTesselationMode_ != mode) {
        edgeTesselationMode_ = mode;
        vacomplex::KeyEdge* ke = vacKeyEdgeNode();
        if (ke) {
            vacomplex::ops::setKeyEdgeSamplingQuality(ke, edgeTesselationMode_);
        }
        dirtyPreJoinGeometry_();
    }
}

std::optional<core::StringId> VacKeyEdge::domTagName() const {
    return dom::strings::edge;
}

geometry::Rect2d VacKeyEdge::boundingBox(core::AnimTime t) const {
    if (frameData_.time() == t) {
        const_cast<VacKeyEdge*>(this)->computePreJoinGeometry_();
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

bool VacKeyEdge::isSelectableInRect(const geometry::Rect2d& rect, core::AnimTime t)
    const {

    if (frameData_.time() == t) {
        return frameData_.isSelectableInRect(rect);
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

    bool isPaintingOffsetLine0 = false;
    bool isPaintingOffsetLine1 = false;

    // TODO: reuse buffers and geometry views

    VacKeyEdgeFrameData& data = frameData_;
    vacomplex::KeyEdge* ke = vacKeyEdgeNode();
    if (!ke || t != ke->time()) {
        return;
    }

    using geometry::Vec2d;

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

        graphics.setStrokeGeometry(engine->createDynamicTriangleStripView(
            BuiltinGeometryLayout::XYUV_iRGBA, IndexFormat::UInt32));
        graphics.setJoinGeometry(engine->createDynamicTriangleStripView(
            BuiltinGeometryLayout::XYUV_iRGBA, IndexFormat::UInt32));

        geometry::Vec2fArray strokeVertices;
        core::Array<UInt32> strokeIndices;
        // join geometry is merged in stroke
        //geometry::Vec2fArray joinVertices;
        //core::Array<UInt32> joinIndices;

        strokeVertices.reserve(data.stroke_.numVertices());
        strokeIndices.reserve(data.stroke_.numIndices());

        auto itXy = data.stroke_.xyVertices().begin();
        auto itStuv = data.stroke_.stuvVertices().begin();
        for (; itXy != data.stroke_.xyVertices().end(); ++itXy, ++itStuv) {
            strokeVertices.append(geometry::Vec2f(*itXy));
            strokeVertices.append(geometry::Vec2f(itStuv->z(), itStuv->w()));
        }

        for (Int index : data.stroke_.indices()) {
            strokeIndices.append(static_cast<UInt32>(index));
        }

        engine->updateBufferData(
            graphics.strokeGeometry()->vertexBuffer(0), //
            std::move(strokeVertices));
        engine->updateBufferData(
            graphics.strokeGeometry()->indexBuffer(), //
            std::move(strokeIndices));

        //engine->updateBufferData(
        //    graphics.joinGeometry()->vertexBuffer(0), //
        //    std::move(joinVertices));
        //engine->updateBufferData(
        //    graphics.joinGeometry()->indexBuffer(), //
        //    std::move(joinIndices));
    }
    if (graphics.strokeGeometry()
        && (data.hasPendingColorChange_ || hasNewStrokeGraphics)) {
        engine->updateBufferData(
            graphics.strokeGeometry()->vertexBuffer(1), //
            core::Array<float>({color.r(), color.g(), color.b(), color.a()}));
        //engine->updateBufferData(
        //    graphics.joinGeometry()->vertexBuffer(1), //
        //    core::Array<float>({color.r(), color.g(), color.b(), color.a()}));
    }

    constexpr PaintOptions centerlineOptions = {
        PaintOption::Outline, PaintOption::Selected};

    bool hasNewCenterlineGraphics = false;

    if ((flags.hasAny(centerlineOptions) || isPaintingOffsetLine0
         || isPaintingOffsetLine1)
        && !graphics.centerlineGeometry()) {

        hasNewCenterlineGraphics = true;
        graphics.setCenterlineGeometry(engine->createDynamicTriangleStripView(
            BuiltinGeometryLayout::XYDxDy_iXYRotWRGBA));

        if (isPaintingOffsetLine0) {
            graphics.setOffsetLineGeometry(
                0,
                engine->createDynamicTriangleStripView(
                    BuiltinGeometryLayout::XYDxDy_iXYRotWRGBA));
        }
        if (isPaintingOffsetLine1) {
            graphics.setOffsetLineGeometry(
                1,
                engine->createDynamicTriangleStripView(
                    BuiltinGeometryLayout::XYDxDy_iXYRotWRGBA));
        }

        GeometryViewCreateInfo createInfo = {};
        createInfo.setBuiltinGeometryLayout(BuiltinGeometryLayout::XYDxDy_iXYRotWRGBA);
        createInfo.setPrimitiveType(PrimitiveType::TriangleStrip);
        createInfo.setVertexBuffer(0, graphics.centerlineGeometry()->vertexBuffer(0));
        BufferPtr selectionInstanceBuffer = engine->createVertexBuffer(0);
        createInfo.setVertexBuffer(1, selectionInstanceBuffer);
        graphics.setSelectionGeometry(engine->createGeometryView(createInfo));

        // X, Y, Rot, Width, R, G, B, A
        core::FloatArray lineInstData({0.f, 0.f, 1.f, 2.f, 0.02f, 0.64f, 1.0f, 1.f});

        geometry::Vec4fArray lineVertices;
        geometry::Vec4fArray offsetLine0Vertices;
        geometry::Vec4fArray offsetLine1Vertices;

        constexpr Int nCap = 2;
        const geometry::StrokeSample2dArray& preJoinSamples = data.preJoinSamples();
        if (!preJoinSamples.isEmpty()) {
            {
                // start cap
                geometry::Vec2f p = geometry::Vec2f(preJoinSamples.first().position());
                geometry::Vec2f n = geometry::Vec2f(preJoinSamples.first().normal());
                geometry::Vec2f d = n.orthogonalized();
                float aDelta = static_cast<float>(core::pi) / (nCap * 2.f + 1.f);
                for (Int i = 0; i < nCap; ++i) {
                    float a = aDelta * (nCap - i);
                    float x = std::sin(a);
                    float y = std::cos(a);
                    geometry::Vec2f dx = x * d;
                    geometry::Vec2f sp1 = dx - n * y;
                    geometry::Vec2f sp2 = dx + n * y;
                    lineVertices.emplaceLast(p.x(), p.y(), sp1.x(), sp1.y());
                    lineVertices.emplaceLast(p.x(), p.y(), sp2.x(), sp2.y());
                }
            }
            for (const geometry::StrokeSample2d& s : preJoinSamples) {
                geometry::Vec2f p = geometry::Vec2f(s.position());
                geometry::Vec2f n = geometry::Vec2f(s.normal());
                // clang-format off
                lineVertices.emplaceLast(p.x(), p.y(), -n.x(), -n.y());
                lineVertices.emplaceLast(p.x(), p.y(),  n.x(),  n.y());
                if (isPaintingOffsetLine0) {
                    geometry::Vec2f p0 = geometry::Vec2f(s.offsetPoint(0));
                    offsetLine0Vertices.emplaceLast(p0.x(), p0.y(), -n.x(), -n.y());
                    offsetLine0Vertices.emplaceLast(p0.x(), p0.y(),  n.x(),  n.y());
                }
                if (isPaintingOffsetLine1) {
                    geometry::Vec2f p1 = geometry::Vec2f(s.offsetPoint(1));
                    offsetLine1Vertices.emplaceLast(p1.x(), p1.y(), -n.x(), -n.y());
                    offsetLine1Vertices.emplaceLast(p1.x(), p1.y(),  n.x(),  n.y());
                }
                // clang-format on
            }
            {
                // end cap
                geometry::Vec2f p = geometry::Vec2f(preJoinSamples.last().position());
                geometry::Vec2f n = geometry::Vec2f(preJoinSamples.last().normal());
                geometry::Vec2f d = -n.orthogonalized();
                float aDelta = static_cast<float>(core::pi) / (nCap * 2.f + 1.f);
                for (Int i = 0; i < nCap; ++i) {
                    float a = aDelta * (i + 1);
                    float x = std::sin(a);
                    float y = std::cos(a);
                    geometry::Vec2f dx = x * d;
                    geometry::Vec2f sp1 = dx - n * y;
                    geometry::Vec2f sp2 = dx + n * y;
                    lineVertices.emplaceLast(p.x(), p.y(), sp1.x(), sp1.y());
                    lineVertices.emplaceLast(p.x(), p.y(), sp2.x(), sp2.y());
                }
            }
        }

        engine->updateBufferData(
            graphics.centerlineGeometry()->vertexBuffer(0), std::move(lineVertices));
        engine->updateBufferData(
            graphics.centerlineGeometry()->vertexBuffer(1), std::move(lineInstData));

        if (isPaintingOffsetLine0) {
            // X, Y, Rot, Width, R, G, B, A
            core::FloatArray offsetLineInstData(
                {0.f, 0.f, 1.f, 1.5f, 0.64f, 1.0f, 0.02f, 1.f});
            engine->updateBufferData(
                graphics.offsetLineGeometry(0)->vertexBuffer(0),
                std::move(offsetLine0Vertices));
            engine->updateBufferData(
                graphics.offsetLineGeometry(0)->vertexBuffer(1),
                std::move(offsetLineInstData));
        }
        if (isPaintingOffsetLine1) {
            // X, Y, Rot, Width, R, G, B, A
            core::FloatArray offsetLineInstData(
                {0.f, 0.f, 1.f, 1.5f, 1.0f, 0.02f, 0.64f, 1.f});
            engine->updateBufferData(
                graphics.offsetLineGeometry(1)->vertexBuffer(0),
                std::move(offsetLine1Vertices));
            engine->updateBufferData(
                graphics.offsetLineGeometry(1)->vertexBuffer(1),
                std::move(offsetLineInstData));
        }
    }
    if (graphics.selectionGeometry()
        && (data.hasPendingColorChange_ || hasNewCenterlineGraphics)) {

        const core::Color& c = colors::selection;
        core::FloatArray bufferData = {0.f, 0.f, 1.f, 2.f, c.r(), c.g(), c.b(), c.a()};
        const BufferPtr& buffer = graphics.selectionGeometry()->vertexBuffer(1);
        engine->updateBufferData(buffer, std::move(bufferData));
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
        engine->setProgram(graphics::BuiltinProgram::ScreenSpaceDisplacement);
        engine->draw(graphics.selectionGeometry());
    }
    else if (!flags.has(PaintOption::Outline)) {
        engine->setProgram(graphics::BuiltinProgram::Simple /*TexturedDebug*/);
        engine->draw(graphics.strokeGeometry());
        //engine->draw(graphics.joinGeometry());
    }

    if (isPaintingOffsetLine0) {
        engine->setProgram(graphics::BuiltinProgram::ScreenSpaceDisplacement);
        engine->draw(graphics.offsetLineGeometry(0));
    }

    if (isPaintingOffsetLine1) {
        engine->setProgram(graphics::BuiltinProgram::ScreenSpaceDisplacement);
        engine->draw(graphics.offsetLineGeometry(1));
    }

    if (flags.has(PaintOption::Outline)) {
        engine->setProgram(graphics::BuiltinProgram::ScreenSpaceDisplacement);
        engine->draw(graphics.centerlineGeometry());
        engine->drawInstanced(controlPointsGeometry_);
    }
}

ElementStatus
VacKeyEdge::onDependencyChanged_(Element* /*dependency*/, ChangeFlags changes) {
    ElementStatus status = this->status();
    if (status == ElementStatus::Ok) {
        if (changes.has(ChangeFlag::VertexPosition)) {
            dirtyPreJoinGeometry_();
        }
    }
    return status;
}

ElementStatus VacKeyEdge::onDependencyRemoved_(Element* dependency) {
    ElementStatus status = this->status();
    for (VertexInfo& vi : verticesInfo_) {
        if (vi.element == dependency) {
            vi.element = nullptr;
            if (status == ElementStatus::Ok) {
                status = ElementStatus::UnresolvedDependency;
            }
        }
    }
    return status;
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
    // XXX Do we need std::optional here? getElementFromPathAttribute currently
    // does not return an std::optional<Element*>, but directly a nullptr, so
    // the "or" part of "value_or" below is in fact never used.
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

    // update VAC to get vertex nodes
    std::array<vacomplex::KeyVertex*, 2> kvs = {};
    for (Int i = 0; i < 2; ++i) {
        VacKeyVertex* kvElement = newVertices[i];
        if (kvElement) {
            workspace->updateElementFromDom(kvElement);
            kvs[i] = kvElement->vacKeyVertexNode();
            if (kvElement->hasError() || !kvs[i]) {
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

    //const auto& points = domElement->getAttribute(ds::positions).getVec2dArray();
    //const auto& widths = domElement->getAttribute(ds::widths).getDoubleArray();

    bool hasGeometryChanged = true;
    bool hasBoundaryChanged = true;
    if (ke) {
        if (kvs[0] == ke->startVertex() && kvs[1] == ke->endVertex()) {
            hasBoundaryChanged = false;
        }
        else {
            // must rebuild
            removeVacNode();
            ke = nullptr;
        }
    }

    // create/rebuild/update VAC node
    if (!ke) {
        auto geometry =
            std::make_shared<workspace::FreehandEdgeGeometry>(isClosed);
        geometry->updateFromDomEdge_(domElement);
        if (isClosed) {
            ke = vacomplex::ops::createKeyClosedEdge(std::move(geometry), parentGroup);
        }
        else {
            ke = vacomplex::ops::createKeyOpenEdge(
                kvs[0], kvs[1], std::move(geometry), parentGroup);
        }
        if (!ke) {
            onUpdateError_();
            return ElementStatus::InvalidAttribute;
        }
        vacomplex::ops::setKeyEdgeSamplingQuality(ke, edgeTesselationMode_);
        setVacNode(ke);
    }
    else {
        auto geometry = dynamic_cast<workspace::EdgeGeometry*>(ke->geometry());
        if (!geometry || !geometry->updateFromDomEdge_(domElement)) {
            hasGeometryChanged = false;
        }
    }

    // dirty cached data
    if (hasGeometryChanged || hasBoundaryChanged) {
        dirtyPreJoinGeometry_(false);
    }

    const auto& color = domElement->getAttribute(ds::color).getColor();
    if (frameData_.color_ != color) {
        frameData_.color_ = color;
        frameData_.hasPendingColorChange_ = true;
        notifyChanges_({ChangeFlag::Color}, false);
    }

    notifyChanges_({}, true);
    return ElementStatus::Ok;
}

void VacKeyEdge::updateFromVac_(vacomplex::NodeModificationFlags flags) {
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

    if (flags.has(vacomplex::NodeModificationFlag::GeometryChanged)) {
        auto geometry = dynamic_cast<const workspace::EdgeGeometry*>(ke->geometry());
        if (geometry) {
            // todo: if geometry type changed, remove previous geometry's attributes.
            geometry->writeToDomEdge_(domElement);
            // todo: dirty only if really changed ?
            dirtyPreJoinGeometry_();
        }
    }
    else if (flags.has(vacomplex::NodeModificationFlag::MeshChanged)) {
        dirtyPreJoinGeometry_();
    }

    const Workspace* w = workspace();

    if (flags.has(vacomplex::NodeModificationFlag::BoundaryChanged)) {
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
            VacJoinHalfedge he(this, i > 0, 0);
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

// TODO: handle the following case:
//  1) dirty without notify: pendingNotifyChanges_.set(A)
//  2) computeA: alreadyNotifiedChanges_.unset(A)
//     -> does nothing
//  3) notify: alreadyNotifiedChanges_.set(pendingNotifyChanges_)
//     -> corrupts the flags and the requester of computeA won't know about the
//        next dirty.
//
void VacKeyEdge::notifyChanges_(ChangeFlags changes, bool immediately) {
    changes.unset(alreadyNotifiedChanges_);
    alreadyNotifiedChanges_.set(changes);
    pendingNotifyChanges_.set(changes);
    if (immediately && pendingNotifyChanges_) {
        notifyChangesToDependents(pendingNotifyChanges_);
        if (pendingNotifyChanges_.has(ChangeFlag::EdgePreJoinGeometry)) {
            for (VertexInfo& vi : verticesInfo_) {
                if (vi.element) {
                    vi.element->onJoinEdgePreJoinGeometryChanged_(this);
                }
            }
        }
        pendingNotifyChanges_.clear();
    }
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

    auto fhGeometry =
        dynamic_cast<const workspace::FreehandEdgeGeometry*>(ke->geometry());
    if (fhGeometry) {
        for (const geometry::Vec2d& p : fhGeometry->positions()) {
            controlPoints_.emplaceLast(geometry::Vec2f(p));
        }
    }

    data.isComputing_ = true;

    data.sampling_ = ke->samplingShared();
    data.bbox_ = ke->centerlineBoundingBox();

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

    StuvMesh2d& stroke = data.stroke_;
    stroke.reset(true);

    using geometry::Vec2d;
    using geometry::Vec4f;
    constexpr Int restartIndex = -1;

    if (data.sampling_ && data.sampling_->samples().size() >= 2) {

        const geometry::StrokeSample2dArray& samples = data.sampling_->samples();

        const auto& patch0 = data.patches_[0];
        const auto& patch1 = data.patches_[1];

        const detail::EdgeJoinPatchMergeLocation& mergeLocation0 = patch0.mergeLocation;
        const detail::EdgeJoinPatchMergeLocation& mergeLocation1 = patch1.mergeLocation;

        double totalS =
            patch0.extensionS + patch0.overrideS + patch1.overrideS + patch1.extensionS;
        if (mergeLocation1.halfedgeNextSampleIndex > 0) {
            totalS += mergeLocation1.sample.s();
        }
        else {
            totalS += samples.last().s();
        }
        if (mergeLocation0.halfedgeNextSampleIndex > 0) {
            totalS -= mergeLocation0.sample.s();
        }
        if (totalS == 0) {
            totalS = core::DoubleInfinity;
        }

        core::Span<const geometry::StrokeSample2d> prejoinSamples(samples);

        Int previousTrioStartIndex = 0;
        Int trioStartIndex = 0;
        bool hasPreviousSample = false;

        std::array<Vec2d, 3> ps = {};
        double previousS = 0;
        double offsetS = patch0.extensionS;

        // TODO: Rescale V values of the patch to makes V values match at merge location.
        //       This will become necessary when we implement parametrization styles.

        for (Int side = 0; side < 2; ++side) {
            float sign = (side == 0) ? 1.f : -1.f;
            for (const auto& sideSample : patch0.sideSamples[side]) {
                // side vertex
                double sideS = offsetS + sideSample.sideSTV[0];
                float sideSf = static_cast<float>(sideS);
                float sideU = static_cast<float>(sideS / totalS);
                Int idx = stroke.appendVertex(
                    sideSample.sidePoint,
                    Vec4f(
                        sideSf,
                        sideSample.sideSTV[1],
                        sideU,
                        sign * sideSample.sideSTV[2]));
                // center vertex
                double centerS = offsetS + sideSample.centerSTV[0];
                float centerSf = static_cast<float>(centerS);
                float centerU = static_cast<float>(centerS / totalS);
                stroke.appendVertex(
                    sideSample.centerPoint,
                    Vec4f(
                        centerSf,
                        sideSample.centerSTV[1],
                        centerU,
                        sign * sideSample.centerSTV[2]));
                // indices in strip
                if (side == 0) {
                    stroke.appendIndicesUnchecked({idx + 1, idx});
                }
                else {
                    stroke.appendIndicesUnchecked({idx, idx + 1});
                }
            }
            stroke.appendPrimitiveRestartIndex();
        }

        offsetS = patch0.extensionS + patch0.overrideS;

        auto tessIterSample = [&](const geometry::StrokeSample2d& sample) {
            std::array<Vec2d, 3> qs = {
                sample.offsetPoint(0), sample.position(), sample.offsetPoint(1)};

            double s = offsetS + sample.s();
            float sf = static_cast<float>(s);
            float u = static_cast<float>(s / totalS);

            float hwf0 = static_cast<float>(sample.halfwidth(0));
            float hwf1 = static_cast<float>(sample.halfwidth(1));

            trioStartIndex = stroke.appendVertex(qs[0], Vec4f(sf, hwf0, u, 1));
            stroke.appendVertex(qs[1], Vec4f(sf, 0, u, 0));
            stroke.appendVertex(qs[2], Vec4f(sf, hwf1, u, -1));

            if (!hasPreviousSample) {
                hasPreviousSample = true;
                previousTrioStartIndex = trioStartIndex;
                ps = qs;
                previousS = s;
                return;
            }

            // There are 3 different cases:
            // - convex quad (most common)
            // - concave quad (a point is inside the triangle of the others)
            // - hourglass (2 opposite sides intersect)
            //
            // ┌─── x
            // │
            // │     p2 ──────── q2
            // y      │           │
            //    ── p1 ─ edge ─ q1──→
            //        │           │
            //       p0 ──────── q0
            //

            // We first check if it is convex.
            // It is the only case in which diagonals intersect each other.
            //
            // ┌─── x
            // │
            // │     p2 ───→─── q2
            // y      │          │
            //        │          │
            //       p0 ───→─── q0
            //

            Vec2d p0q0 = (qs[0] - ps[0]);
            Vec2d p0q2 = (qs[2] - ps[0]);
            Vec2d p0p2 = (ps[2] - ps[0]);
            Vec2d p2q0 = (qs[0] - ps[2]);
            Vec2d p2q2 = (qs[2] - ps[2]);

            double det_p0q2_p0q0 = p0q2.det(p0q0);
            double det_p0q2_p0p2 = p0q2.det(p0p2);
            double det_p2q0_p2q2 = p2q0.det(p2q2);
            double det_p2q0_p2p0 = p2q0.det(-p0p2);

            if ((det_p0q2_p0q0 >= 0 && det_p0q2_p0p2 <= 0) //
                && (det_p2q0_p2q2 <= 0 && det_p2q0_p2p0 >= 0)) {

                // regular quad
                for (Int i = 0; i < 3; ++i) {
                    stroke.appendIndexUnchecked(previousTrioStartIndex + i); // pi
                    stroke.appendIndexUnchecked(trioStartIndex + i);         // qi
                }
                stroke.appendIndexUnchecked(restartIndex);
            }
            else if (
                (det_p0q2_p0q0 <= 0 && det_p0q2_p0p2 >= 0) //
                && (det_p2q0_p2q2 >= 0 && det_p2q0_p2p0 <= 0)) {

                // backfacing regular quad
                for (Int i = 0; i < 3; ++i) {
                    stroke.appendIndexUnchecked(trioStartIndex + i);         // qi
                    stroke.appendIndexUnchecked(previousTrioStartIndex + i); // pi
                }
                stroke.appendIndexUnchecked(restartIndex);
            }
            else {
                // Otherwise it is a special case and we process each side of the centerline separately.

                for (Int i = 0; i < 2; ++i) {

                    // First, check convex cases.
                    //
                    // ┌─── x
                    // │
                    // │     a1 ───→─── b1
                    // y      │          │
                    //        │          │
                    //       a0 ───→─── b0
                    //

                    Vec2d a0 = ps[0 + i];
                    Vec2d b0 = qs[0 + i];
                    Vec2d a1 = ps[1 + i];
                    Vec2d b1 = qs[1 + i];
                    double v0 = 1.f - i;
                    double v1 = 0.f - i;
                    Int a0i = previousTrioStartIndex + i;
                    Int a1i = a0i + 1;
                    Int b0i = trioStartIndex + i;
                    Int b1i = b0i + 1;

                    Vec2d a0b0 = (b0 - a0);
                    Vec2d a0b1 = (b1 - a0);
                    Vec2d a0a1 = (a1 - a0);
                    Vec2d a1b0 = (b0 - a1);
                    Vec2d a1b1 = (b1 - a1);
                    Vec2d b0b1 = (b1 - b0);

                    double det_a0b1_a0b0 = a0b1.det(a0b0);
                    double det_a0b1_a0a1 = a0b1.det(a0a1);
                    double det_a1b0_a1b1 = a1b0.det(a1b1);
                    double det_a1b0_a1a0 = a1b0.det(-a0a1);

                    if ((det_a0b1_a0b0 >= 0 && det_a0b1_a0a1 <= 0) //
                        && (det_a1b0_a1b1 <= 0 && det_a1b0_a1a0 >= 0)) {

                        // regular quad
                        stroke.appendIndicesUnchecked({a0i, b0i, a1i, b1i, restartIndex});
                        continue;
                    }

                    if ((det_a0b1_a0b0 <= 0 && det_a0b1_a0a1 >= 0) //
                        && (det_a1b0_a1b1 >= 0 && det_a1b0_a1a0 <= 0)) {

                        // backfacing regular quad
                        stroke.appendIndicesUnchecked({b0i, a0i, b1i, a1i, restartIndex});
                        continue;
                    }

                    // From here, we know that no 3 points are aligned

                    // Check for the hourglass case.
                    // There are 2 pairs of sides that can intersect, and in 2 directions.
                    // That is 2*2 sub-cases.

                    // Hourglass cases A and B
                    //
                    // ┌─── x
                    // │
                    // │    b1 ─←─ a1    a1 ─→─ b1
                    // y     ╲     ╱      ╲     ╱
                    //        ╲   ╱        ╲   ╱
                    //         ╲ ╱          ╲ ╱
                    //          ╳            ╳
                    //         ╱ ╲          ╱ ╲
                    //        ╱   ╲        ╱   ╲
                    //       ╱     ╲      ╱     ╲
                    //      a0 ─→─ b0    b0 ─←─ a0
                    //
                    //         (A)          (B)
                    //

                    double det_a0a1_a0b0 = a0a1.det(a0b0);
                    double det_b0b1_b0a0 = b0b1.det(-a0b0);
                    double det_b0b1_b0a1 = b0b1.det(-a1b0);

                    bool a1a0b0_positive = det_a0a1_a0b0 >= 0;
                    bool a1a0b1_positive = det_a0b1_a0a1 <= 0;
                    bool b1b0a0_positive = det_b0b1_b0a0 >= 0;
                    bool b1b0a1_positive = det_b0b1_b0a1 >= 0;

                    if (a1a0b1_positive != a1a0b0_positive
                        && b1b0a0_positive != b1b0a1_positive) {

                        // In this case we compute the intersection
                        // to build 2 triangles (+ 2 stitch triangles).

                        // Solve 2x2 system using Cramer's rule.
                        double delta = a0a1.det(b0b1);
                        double inv_delta = 1 / delta;
                        double x0 = a0b0.det(b0b1) * inv_delta;
                        double x1 = a0b0.det(a0a1) * inv_delta;
                        double a = a0.det(a1);
                        double b = b0.det(b1);

                        Vec2d c = (a0a1 * b - b0b1 * a) * inv_delta;

                        double xm = (x0 + x1) * 0.5;
                        double cv = v0 + xm * (v1 - v0);

                        double sm = (previousS + s) * 0.5;
                        float smf = static_cast<float>(sm);
                        float um = static_cast<float>(sm / totalS);
                        float cvf = static_cast<float>(cv);

                        double hwm = (a0a1.length() + b0b1.length()) * 0.5 * xm;
                        float hwmf = static_cast<float>(hwm);

                        Int pIndex = stroke.appendVertex(c, Vec4f(smf, hwmf, um, cvf));

                        if (b1b0a1_positive) {
                            // case A
                            stroke.appendIndicesUnchecked(
                                {a0i, b0i, pIndex, restartIndex});
                            stroke.appendIndicesUnchecked(
                                {pIndex, a1i, b1i, restartIndex});
                        }
                        else {
                            // case B
                            stroke.appendIndicesUnchecked(
                                {b0i, a0i, pIndex, restartIndex});
                            stroke.appendIndicesUnchecked(
                                {pIndex, b1i, a1i, restartIndex});
                        }

                        // degenerate triangles are necessary to stitch the
                        // geometry without leaving holes.

                        stroke.appendIndicesUnchecked({a0i, pIndex, a1i, restartIndex});
                        stroke.appendIndicesUnchecked({b0i, pIndex, b1i, restartIndex});

                        continue;
                    }

                    // Hourglass cases C and D
                    // (when centerline does a 180°)
                    //
                    // ┌─── x
                    // │
                    // │    a1      b0    b0      a1
                    // y     │╲     ╱│     │╲     ╱│
                    //       │ ╲   ╱ │     │ ╲   ╱ │
                    //       │  ╲ ╱  │     │  ╲ ╱  │
                    //       │   ╳   │     │   ╳   │
                    //       │  ╱ ╲  │     │  ╱ ╲  │
                    //       │ ╱   ╲ │     │ ╱   ╲ │
                    //       │╱     ╲│     │╱     ╲│
                    //      a0      b1    b1      a0
                    //
                    //         (C)           (D)
                    //

                    double det_a1b1_a1a0 = a1b1.det(-a0a1);

                    bool b0a0b1_positive = det_a0b1_a0b0 <= 0;
                    bool b0a0a1_positive = det_a0a1_a0b0 <= 0;
                    bool b1a1a0_positive = det_a1b1_a1a0 >= 0;
                    bool b1a1b0_positive = det_a1b0_a1b1 <= 0;

                    if (b0a0b1_positive != b0a0a1_positive
                        && b1a1a0_positive != b1a1b0_positive) {

                        // In this case we consider the sample b to be
                        // the discontinuity and stitch to it as if it
                        // were in the opposite orientation for the
                        // current quad (forming a regular quad).

                        if (b0a0b1_positive) {
                            // case C
                            stroke.appendIndicesUnchecked(
                                {a0i, b1i, a1i, b0i, restartIndex});
                        }
                        else {
                            // case D
                            stroke.appendIndicesUnchecked(
                                {a0i, a1i, b0i, b1i, restartIndex});
                        }

                        continue;
                    }

                    // Remains the concave quad case.
                    // There are 8 sub-cases: each point can be in the
                    // triangle formed of the 3 others, and in 2 orientations.
                    //
                    // ┌─── x
                    // │
                    // │    b1             b0             a0             a1
                    // y     │╲             │╲             │╲             │╲
                    //       │ ╲            │ ╲            │ ╲            │ ╲
                    //       │  b0 ─ a0     │  a0 ─ a1     │  a1 ─ b1     │  b1 ─ b0
                    //       │     .`       │     .`       │     .`       │     .`
                    //       │   .`         │   .`         │   .`         │   .`
                    //       │ .`           │ .`           │ .`           │ .`
                    //      a1   (A)       b1   (B)       b0   (C)       a0   (D)
                    //
                    //      a0             a1             b1             b0
                    //       │╲             │╲             │╲             │╲
                    //       │ ╲            │ ╲            │ ╲            │ ╲
                    //       │  b0 ─ b1     │  a0 ─ b0     │  a1 ─ a0     │  b1 ─ a1
                    //       │     .`       │     .`       │     .`       │     .`
                    //       │   .`         │   .`         │   .`         │   .`
                    //       │ .`           │ .`           │ .`           │ .`
                    //      a1   (E)       b1   (F)       b0   (G)       a0   (H)
                    //

                    // We can reuse the dets computed for previous cases

                    if (det_a0b1_a0b0 >= 0) {
                        // b0 is on the positive side of a0b1
                        // cases B, C, D, E
                        if (det_a0b1_a0a1 >= 0) {
                            // a1 is on the positive side of a0b1
                            // cases C, E
                            if (det_a1b0_a1a0 >= 0) {
                                // a0 is on the positive side of a1b0
                                // case C
                                stroke.appendIndicesUnchecked(
                                    {a0i, b0i, a1i, b1i, restartIndex});
                            }
                            else {
                                // a0 is on the negative side of a1b0
                                // case E
                                stroke.appendIndicesUnchecked(
                                    {a0i, a1i, b0i, b1i, restartIndex});
                            }
                        }
                        else {
                            // a1 is on the negative side of a0b1
                            // cases B, D
                            if (det_a1b0_a1a0 >= 0) {
                                // a0 is on the positive side of a1b0
                                // case D
                                stroke.appendIndicesUnchecked(
                                    {a1i, a0i, b1i, b0i, restartIndex});
                            }
                            else {
                                // a0 is on the negative side of a1b0
                                // case B
                                stroke.appendIndicesUnchecked(
                                    {b0i, b1i, a0i, a1i, restartIndex});
                            }
                        }
                    }
                    else {
                        // b0 is on the negative side of a0b1
                        // cases A, F, G, H
                        if (det_a0b1_a0a1 <= 0) {
                            // a1 is on the negative side of a0b1
                            // cases A, G
                            if (det_a1b0_a1a0 >= 0) {
                                // a0 is on the positive side of a1b0
                                // case A
                                stroke.appendIndicesUnchecked(
                                    {b1i, a1i, b0i, a0i, restartIndex});
                            }
                            else {
                                // a0 is on the negative side of a1b0
                                // case G
                                stroke.appendIndicesUnchecked(
                                    {b1i, b0i, a1i, a0i, restartIndex});
                            }
                        }
                        else {
                            // a1 is on the positive side of a0b1
                            // cases F, H
                            if (det_a1b0_a1a0 >= 0) {
                                // a0 is on the positive side of a1b0
                                // case F
                                stroke.appendIndicesUnchecked(
                                    {a1i, b1i, a0i, b0i, restartIndex});
                            }
                            else {
                                // a0 is on the negative side of a1b0
                                // case H
                                stroke.appendIndicesUnchecked(
                                    {b0i, a0i, b1i, a1i, restartIndex});
                            }
                        }
                    }
                }
            }

            previousTrioStartIndex = trioStartIndex;
            ps = qs;
            previousS = s;
        };

        const geometry::StrokeSample2d& mergeSample0 = mergeLocation0.sample;
        offsetS -= mergeSample0.s();

        if (mergeLocation0.halfedgeNextSampleIndex > 0 && mergeLocation0.t < 1.0) {
            tessIterSample(mergeSample0);
        }

        if ((mergeLocation0.halfedgeNextSampleIndex
             + mergeLocation1.halfedgeNextSampleIndex)
            < prejoinSamples.length()) {

            auto coreSamples = core::Span(
                prejoinSamples.begin() + mergeLocation0.halfedgeNextSampleIndex,
                prejoinSamples.end() - mergeLocation1.halfedgeNextSampleIndex);

            for (const geometry::StrokeSample2d& coreSample : coreSamples) {
                tessIterSample(coreSample);
            }
        }

        if (mergeLocation1.halfedgeNextSampleIndex > 0 && mergeLocation1.t < 1.0) {
            tessIterSample(mergeLocation1.sample);
        }

        offsetS = totalS - patch1.extensionS;

        for (Int side = 0; side < 2; ++side) {
            float sign = (side == 0) ? -1.f : 1.f;
            for (const auto& sideSample : patch1.sideSamples[side]) {
                // side vertex
                double sideS = offsetS - sideSample.sideSTV[0];
                float sideSf = static_cast<float>(sideS);
                float sideU = static_cast<float>(sideS / totalS);
                Int idx = stroke.appendVertex(
                    sideSample.sidePoint,
                    Vec4f(
                        sideSf,
                        sideSample.sideSTV[1],
                        sideU,
                        sign * sideSample.sideSTV[2]));
                // center vertex
                double centerS = offsetS - sideSample.centerSTV[0];
                float centerSf = static_cast<float>(centerS);
                float centerU = static_cast<float>(centerS / totalS);
                stroke.appendVertex(
                    sideSample.centerPoint,
                    Vec4f(
                        centerSf,
                        sideSample.centerSTV[1],
                        centerU,
                        sign * sideSample.centerSTV[2]));
                // indices in strip
                if (side == 0) {
                    stroke.appendIndicesUnchecked({idx + 1, idx});
                }
                else {
                    stroke.appendIndicesUnchecked({idx, idx + 1});
                }
            }
            stroke.appendPrimitiveRestartIndex();
        }
    }

    data.strokeBbox_ = geometry::Rect2d::empty;
    for (const geometry::Vec2d& v : stroke.xyVertices()) {
        data.strokeBbox_.uniteWith(v);
    }

    alreadyNotifiedChanges_.unset(ChangeFlag::EdgeStrokeMesh);
    data.stage_ = VacEdgeComputationStage::StrokeMesh;
    data.isComputing_ = false;
    return true;
}

void VacKeyEdge::dirtyPreJoinGeometry_(bool notifyDependentsImmediately) {
    if (frameData_.stage() > VacEdgeComputationStage::Clear) {
        controlPoints_.clear();
        controlPointsGeometry_.reset();
        frameData_.resetToStage(VacEdgeComputationStage::Clear);
        notifyChanges_(
            {ChangeFlag::EdgePreJoinGeometry,
             ChangeFlag::EdgePostJoinGeometry,
             ChangeFlag::EdgeStrokeMesh},
            notifyDependentsImmediately);
    }
}

void VacKeyEdge::dirtyPostJoinGeometry_(bool notifyDependentsImmediately) {
    if (frameData_.stage() > VacEdgeComputationStage::PreJoinGeometry) {
        frameData_.resetToStage(VacEdgeComputationStage::PreJoinGeometry);
        notifyChanges_(
            {ChangeFlag::EdgePostJoinGeometry, ChangeFlag::EdgeStrokeMesh},
            notifyDependentsImmediately);
    }
}

void VacKeyEdge::dirtyStrokeMesh_(bool notifyDependentsImmediately) {
    if (frameData_.stage() > VacEdgeComputationStage::PostJoinGeometry) {
        frameData_.resetToStage(VacEdgeComputationStage::PostJoinGeometry);
        notifyChanges_({ChangeFlag::EdgeStrokeMesh}, notifyDependentsImmediately);
    }
}

// called by one of the end vertex
void VacKeyEdge::dirtyJoinDataAtVertex_(const VacVertexCell* vertexCell) {
    if (frameData_.stage() >= VacEdgeComputationStage::PreJoinGeometry) {
        dirtyPostJoinGeometry_(false);
        if (verticesInfo_[0].element == vertexCell) {
            frameData_.patches_[0].clear();
        }
        if (verticesInfo_[1].element == vertexCell) {
            frameData_.patches_[1].clear();
        }
        notifyChanges_({});
    }
}

void VacKeyEdge::onUpdateError_() {
    removeVacNode();
    dirtyPreJoinGeometry_();
}

} // namespace vgc::workspace
