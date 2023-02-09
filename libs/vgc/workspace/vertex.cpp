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

#include <vgc/workspace/vertex.h>

#include <vgc/workspace/edge.h>
#include <vgc/workspace/workspace.h>

namespace vgc::workspace {

namespace detail {

void VacVertexCellFrameData::debugPaint(graphics::Engine* engine) {

    using namespace graphics;
    using detail::VacJoinHalfedgeFrameData;

    if (!debugQuadRenderGeometry_) {
        debugQuadRenderGeometry_ = engine->createDynamicTriangleStripView(
            BuiltinGeometryLayout::XYRGB, IndexFormat::UInt16);
        geometry::Vec2f p(pos_);
        core::FloatArray vertices({
            p.x() - 5, p.y() - 5, 0, 0, 0, //
            p.x() - 5, p.y() + 5, 0, 0, 0, //
            p.x() + 5, p.y() - 5, 0, 0, 0, //
            p.x() + 5, p.y() + 5, 0, 0, 0, //
        });
        engine->updateVertexBufferData(debugQuadRenderGeometry_, std::move(vertices));
        core::Array<UInt16> lineIndices({0, 1, 2, 3});
        engine->updateBufferData(
            debugQuadRenderGeometry_->indexBuffer(), std::move(lineIndices));
    }

    if (!debugLinesRenderGeometry_ && halfedgesData_.length()) {
        debugLinesRenderGeometry_ = engine->createDynamicTriangleStripView(
            BuiltinGeometryLayout::XYDxDy_iXYRotRGBA, IndexFormat::UInt16);

        core::FloatArray lineInstData;
        lineInstData.extend({0.f, 0.f, 1.f, 0.64f, 0.02f, 1.0f, 1.f, 0.f /*padding*/});

        float lineHalfWidth = 1.5f;
        float lineLength = 100.f;

        geometry::Vec4fArray lineVertices;
        core::Array<UInt16> lineIndices;

        for (const VacJoinHalfedgeFrameData& s : halfedgesData_) {
            geometry::Vec2f p(pos_);
            double angle0 = s.angle();
            double angle1 = s.angle() + s.angleToNext();
            double midAngle = angle0 + angle1;
            if (angle0 > angle1) {
                midAngle += core::pi * 2;
            }
            midAngle *= 0.5;
            if (midAngle > core::pi * 2) {
                midAngle -= core::pi * 2;
            }
            geometry::Vec2f d(
                static_cast<float>(std::cos(midAngle)),
                static_cast<float>(std::sin(midAngle)));
            geometry::Vec2f n = d.orthogonalized() * lineHalfWidth;
            d *= lineLength;
            // clang-format off
            Int i = lineVertices.length();
            lineVertices.emplaceLast(p.x(), p.y(), -n.x(), -n.y());
            lineVertices.emplaceLast(p.x(), p.y(), n.x(), n.y());
            lineVertices.emplaceLast(p.x(), p.y(), -n.x() + d.x(), -n.y() + d.y());
            lineVertices.emplaceLast(p.x(), p.y(), n.x() + d.x(), n.y() + d.y());
            lineIndices.extend(
                {static_cast<UInt16>(i),
                static_cast<UInt16>(i + 1),
                static_cast<UInt16>(i + 2),
                static_cast<UInt16>(i + 3),
                static_cast<UInt16>(-1)});
            // clang-format on
        }

        engine->updateBufferData(
            debugLinesRenderGeometry_->indexBuffer(), //
            std::move(lineIndices));
        engine->updateBufferData(
            debugLinesRenderGeometry_->vertexBuffer(0), std::move(lineVertices));
        engine->updateBufferData(
            debugLinesRenderGeometry_->vertexBuffer(1), std::move(lineInstData));
    }

    if (debugLinesRenderGeometry_) {
        engine->setProgram(graphics::BuiltinProgram::SreenSpaceDisplacement);
        engine->draw(debugLinesRenderGeometry_);
    }

    if (debugQuadRenderGeometry_ && halfedgesData_.length() == 1) {
        engine->setProgram(graphics::BuiltinProgram::Simple);
        engine->draw(debugQuadRenderGeometry_);
    }
}

} // namespace detail

geometry::Rect2d VacKeyVertex::boundingBox(core::AnimTime /*t*/) const {
    vacomplex::KeyVertex* kv = vacKeyVertexNode();
    if (kv) {
        geometry::Vec2d pos = vacKeyVertexNode()->position({});
        return geometry::Rect2d(pos, pos);
    }
    return geometry::Rect2d::empty;
}

void VacVertexCell::computeJoin(core::AnimTime t) {
    detail::VacVertexCellFrameData* c = frameData(t);
    if (c) {
        computePos(*c);
        computeJoin(*c);
    }
}

void VacVertexCell::paint_(graphics::Engine* engine, core::AnimTime t, PaintOptions flags)
    const {

    if (flags.has(PaintOption::Outline)) {
        detail::VacVertexCellFrameData* fd = frameData(t);
        if (fd) {
            //const_cast<VacVertexCell*>(this)->computePos_(*fd);
            const_cast<VacVertexCell*>(this)->computeJoin(*fd);
            fd->debugPaint(engine);
        }
    }
}

detail::VacVertexCellFrameData* VacVertexCell::frameData(core::AnimTime t) const {
    vacomplex::VertexCell* cell = vacVertexCellNode();
    if (!cell) {
        return nullptr;
    }
    auto it = frameDataEntries_.begin();
    for (; it != frameDataEntries_.end(); ++it) {
        if (it->time() == t) {
            return &*it;
        }
    }
    if (cell->existsAt(t)) {
        it = frameDataEntries_.emplace(it, t);
        return &*it;
    }
    return nullptr;
}

void VacVertexCell::computePos(detail::VacVertexCellFrameData& data) {
    if (data.isPosComputed_ || data.isComputing_) {
        return;
    }
    vacomplex::VertexCell* v = vacVertexCellNode();
    if (!v) {
        return;
    }

    data.isComputing_ = true;
    VGC_DEBUG_TMP("VacVertexCell({})->computeJoin_", (void*)this);

    data.pos_ = v->position(data.time());

    data.isPosComputed_ = true;
    data.isComputing_ = false;
}

void VacVertexCell::computeJoin(detail::VacVertexCellFrameData& data) {
    if (data.isJoinComputed_ || data.isComputing_) {
        return;
    }

    computePos(data);

    data.isComputing_ = true;
    VGC_DEBUG_TMP("VacVertexCell({})->computeJoin_", (void*)this);

    for (const detail::VacJoinHalfedge& he : joinHalfedges_) {
        data.halfedgesData_.emplaceLast(he);
    }

    for (detail::VacJoinHalfedgeFrameData& heData : data.halfedgesData_) {

        VacEdgeCell* cell = heData.halfedge().edgeCell();
        VacEdgeCellFrameData* edgeData = cell->frameData(data.time());
        if (!edgeData) {
            // huh ?
            continue;
        }
        cell->computeStandaloneGeometry(*edgeData);

        const geometry::CurveSampleArray& samples = edgeData->samples_;
        if (samples.length() < 2) {
            continue;
        }
        geometry::CurveSample sample =
            samples[heData.halfedge().isReverse() ? samples.size() - 2 : 1];

        // XXX add xAxisAngle to Vec class
        geometry::Vec2d outgoingTangent = (sample.position() - data.pos()).normalized();
        double angle = geometry::Vec2d(1.f, 0).angle(outgoingTangent);
        if (angle < 0) {
            angle += core::pi * 2;
        }

        heData.edgeData_ = edgeData;
        heData.outgoingTangent_ = outgoingTangent;
        heData.angle_ = angle;
    }

    const Int numHalfedges = data.halfedgesData_.length();
    VGC_DEBUG_TMP("numHalfedges:{}", numHalfedges);
    if (numHalfedges == 0) {
        // nothing to do
    }
    else if (numHalfedges == 1) {
        // cap, todo
        //const detail::VacJoinHalfedgeFrameData& halfedgeData = data.halfedgesData_[0];
        //Int basePatchIndex = halfedgeData.halfedge().isReverse() ? 2 : 0;
        //VGC_DEBUG_TMP("basePatchIndex:{}", basePatchIndex);
        //VacEdgeCellFrameData* edgeData = halfedgeData.edgeData_;
        //if (edgeData) {
        //    Int numSamples = edgeData->samples_.length();
        //    edgeData->patches_[basePatchIndex].sampleOverride_ = numSamples / 3;
        //    edgeData->patches_[basePatchIndex + 1].sampleOverride_ = 0;
        //}
    }
    else {
        // sort by incident angle
        std::sort(
            data.halfedgesData_.begin(),
            data.halfedgesData_.end(),
            [](const detail::VacJoinHalfedgeFrameData& a,
               const detail::VacJoinHalfedgeFrameData& b) {
                return a.angle() < b.angle();
            });

        detail::VacJoinHalfedgeFrameData* previousHalfedge = &data.halfedgesData_.last();
        double previousAngle = previousHalfedge->angle() - core::pi;
        for (detail::VacJoinHalfedgeFrameData& halfedge : data.halfedgesData_) {
            halfedge.angleToNext_ = halfedge.angle() - previousAngle;
            previousAngle = halfedge.angle();
        }
    }

    data.isJoinComputed_ = true;
    data.isComputing_ = false;
}

ElementStatus VacKeyVertex::updateFromDom_(Workspace* /*workspace*/) {
    namespace ds = dom::strings;
    dom::Element* const domElement = this->domElement();

    vacomplex::KeyVertex* kv = vacKeyVertexNode();
    if (!kv) {
        kv = topology::ops::createKeyVertex(
            domElement->internalId(), parentVacElement()->vacNode()->toGroupUnchecked());
        setVacNode(kv);
    }

    const auto& position = domElement->getAttribute(ds::position).getVec2d();
    if (kv->position() != position) {
        topology::ops::setKeyVertexPosition(kv, position);
        onInputGeometryChanged();
    }

    return ElementStatus::Ok;
}

void VacVertexCell::rebuildJoinHalfedgesArray() const {

    vacomplex::VertexCell* v = vacVertexCellNode();
    if (!v) {
        joinHalfedges_.clear();
        return;
    }

    joinHalfedges_.clear();
    for (vacomplex::Cell* vacCell : v->star()) {
        vacomplex::EdgeCell* vacEdge = vacCell->toEdgeCell();
        VacElement* edge = static_cast<VacElement*>(workspace()->find(vacEdge->id()));
        // XXX replace dynamic_cast
        VacEdgeCell* edgeCell = dynamic_cast<VacKeyEdge*>(edge);
        if (edgeCell) {
            if (vacEdge->isStartVertex(v)) {
                joinHalfedges_.emplaceLast(edgeCell, 0, false);
            }
            if (vacEdge->isEndVertex(v)) {
                joinHalfedges_.emplaceLast(edgeCell, 0, true);
            }
        }
    }
}

void VacVertexCell::clearJoinHalfedgesJoinData() const {
    for (const detail::VacJoinHalfedge& joinHalfedge : joinHalfedges_) {
        if (joinHalfedge.isReverse()) {
            joinHalfedge.edgeCell()->clearEndJoinData();
        }
        else {
            joinHalfedge.edgeCell()->clearStartJoinData();
        }
    }
}

void VacVertexCell::addJoinHalfedge_(const detail::VacJoinHalfedge& joinHalfedge) {
    joinHalfedges_.emplaceLast(joinHalfedge);
    for (auto& entry : frameDataEntries_) {
        entry.clearJoinData();
    }
}

void VacVertexCell::removeJoinHalfedge_(const detail::VacJoinHalfedge& joinHalfedge) {
    auto equal = detail::VacJoinHalfedge::GrouplessEqualTo();
    joinHalfedges_.removeOneIf(
        [&](const detail::VacJoinHalfedge& x) { return equal(joinHalfedge, x); });
    for (auto& entry : frameDataEntries_) {
        entry.clearJoinData();
    }
}

void VacVertexCell::onInputGeometryChanged() {
    frameDataEntries_.clear();
    clearJoinHalfedgesJoinData();
}

void VacVertexCell::onJoinEdgeGeometryChanged_(VacEdgeCell* edge) {
    for (auto& entry : frameDataEntries_) {
        entry.clearJoinData();
    }
    clearJoinHalfedgesJoinData();
}

geometry::Rect2d VacInbetweenVertex::boundingBox(core::AnimTime t) const {
    vacomplex::InbetweenVertex* iv = vacInbetweenVertexNode();
    if (iv) {
        geometry::Vec2d pos = iv->position(t);
        return geometry::Rect2d(pos, pos);
    }
    return geometry::Rect2d();
}

ElementStatus VacInbetweenVertex::updateFromDom_(Workspace* /*workspace*/) {
    return ElementStatus::Ok;
}

void VacInbetweenVertex::preparePaint_(core::AnimTime /*t*/, PaintOptions /*flags*/) {
}

} // namespace vgc::workspace
