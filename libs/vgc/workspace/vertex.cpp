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

void VacVertexCellFrameData::debugPaint_(graphics::Engine* engine) {

    using namespace graphics;
    using detail::VacJoinHalfedgeFrameData;

    if (!joinDebugQuadRenderGeometry_) {
        joinDebugQuadRenderGeometry_ = engine->createDynamicTriangleStripView(
            BuiltinGeometryLayout::XYRGB, IndexFormat::UInt16);
        geometry::Vec2f p(position_);
        core::FloatArray vertices({
            p.x() - 5, p.y() - 5, 0, 0, 0, //
            p.x() - 5, p.y() + 5, 0, 0, 0, //
            p.x() + 5, p.y() - 5, 0, 0, 0, //
            p.x() + 5, p.y() + 5, 0, 0, 0, //
        });
        engine->updateVertexBufferData(joinDebugQuadRenderGeometry_, std::move(vertices));
        core::Array<UInt16> lineIndices({0, 1, 2, 3});
        engine->updateBufferData(
            joinDebugQuadRenderGeometry_->indexBuffer(), std::move(lineIndices));
    }

    if (!joinDebugLinesRenderGeometry_ && joinData_.halfedgesFrameData_.length()) {
        joinDebugLinesRenderGeometry_ = engine->createDynamicTriangleStripView(
            BuiltinGeometryLayout::XYDxDy_iXYRotWRGBA, IndexFormat::UInt16);

        core::FloatArray lineInstData;
        lineInstData.extend({0.f, 0.f, 1.f, 1.5f, 0.64f, 0.02f, 1.0f, 1.f});

        float lineLength = 100.f;

        geometry::Vec4fArray lineVertices;
        core::Array<UInt16> lineIndices;

        for (const VacJoinHalfedgeFrameData& heData : joinData_.halfedgesFrameData_) {
            geometry::Vec2f p(position_);
            double angle0 = heData.angle();
            double angle1 = heData.angle() + heData.angleToNext();
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
            geometry::Vec2f n = d.orthogonalized();
            d *= lineLength;
            // clang-format off
            Int i = lineVertices.length();
            lineVertices.emplaceLast(p.x(), p.y(), -n.x()        , -n.y()        );
            lineVertices.emplaceLast(p.x(), p.y(),  n.x()        ,  n.y()        );
            lineVertices.emplaceLast(p.x(), p.y(), -n.x() + d.x(), -n.y() + d.y());
            lineVertices.emplaceLast(p.x(), p.y(),  n.x() + d.x(),  n.y() + d.y());
            lineIndices.extend(
                {static_cast<UInt16>(i),
                static_cast<UInt16>(i + 1),
                static_cast<UInt16>(i + 2),
                static_cast<UInt16>(i + 3),
                static_cast<UInt16>(-1)});
            // clang-format on
        }

        engine->updateBufferData(
            joinDebugLinesRenderGeometry_->indexBuffer(), //
            std::move(lineIndices));
        engine->updateBufferData(
            joinDebugLinesRenderGeometry_->vertexBuffer(0), std::move(lineVertices));
        engine->updateBufferData(
            joinDebugLinesRenderGeometry_->vertexBuffer(1), std::move(lineInstData));
    }

    if (joinDebugLinesRenderGeometry_) {
        engine->setProgram(graphics::BuiltinProgram::SreenSpaceDisplacement);
        engine->draw(joinDebugLinesRenderGeometry_);
    }

    if (joinDebugQuadRenderGeometry_ && joinData_.halfedgesFrameData_.length() == 1) {
        engine->setProgram(graphics::BuiltinProgram::Simple);
        engine->draw(joinDebugQuadRenderGeometry_);
    }
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
        // TODO: replace dynamic_cast
        VacEdgeCell* edgeCell = dynamic_cast<VacEdgeCell*>(edge);
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

void VacVertexCell::dirtyJoinHalfedgesJoinData_(const VacEdgeCell* excluded) const {
    for (const VacJoinHalfedge& joinHalfedge : joinHalfedges_) {
        if (joinHalfedge.edgeCell() != excluded) {
            joinHalfedge.edgeCell()->dirtyJoinDataAtVertex_(this);
        }
    }
}

// this vertex is the halfedge end vertex
void VacVertexCell::addJoinHalfedge_(const VacJoinHalfedge& joinHalfedge) {
    joinHalfedges_.emplaceLast(joinHalfedge);
    onAddJoinHalfedge_(joinHalfedge);
}

// this vertex is the halfedge end vertex
void VacVertexCell::removeJoinHalfedge_(const VacJoinHalfedge& joinHalfedge) {
    auto equal = VacJoinHalfedge::GrouplessEqualTo();
    bool removed = joinHalfedges_.removeOneIf(
        [&](const VacJoinHalfedge& x) { return equal(joinHalfedge, x); });
    if (removed) {
        onRemoveJoinHalfedge_(joinHalfedge);
    }
}

std::optional<core::StringId> VacKeyVertex::domTagName() const {
    return dom::strings::vertex;
}

geometry::Rect2d VacKeyVertex::boundingBox(core::AnimTime /*t*/) const {
    // TODO: use cached position in frame data
    vacomplex::KeyVertex* kv = vacKeyVertexNode();
    if (kv) {
        geometry::Vec2d pos = vacKeyVertexNode()->position();
        return geometry::Rect2d(pos, pos);
    }
    return geometry::Rect2d::empty;
}

bool VacKeyVertex::isSelectableAt(
    const geometry::Vec2d& p,
    bool /*outlineOnly*/,
    double tol,
    double* outDistance,
    core::AnimTime /*t*/) const {

    vacomplex::KeyVertex* kv = vacKeyVertexNode();
    if (kv) {
        geometry::Vec2d pos = vacKeyVertexNode()->position();
        double d = (p - pos).length();
        if (d < tol) {
            if (outDistance) {
                *outDistance = d;
            }
            return true;
        }
    }
    return false;
}

const VacVertexCellFrameData* VacKeyVertex::computeFrameDataAt(core::AnimTime t) {
    if (frameData_.time() == t) {
        computePosition_();
        return &frameData_;
    }
    return nullptr;
}

void VacKeyVertex::onPaintDraw(
    graphics::Engine* engine,
    core::AnimTime t,
    PaintOptions flags) const {

    if (flags.has(PaintOption::Outline) && frameData_.time() == t) {
        const_cast<VacKeyVertex*>(this)->computeJoin_();
        frameData_.debugPaint_(engine);
    }
}

ElementStatus VacKeyVertex::updateFromDom_(Workspace* /*workspace*/) {
    namespace ds = dom::strings;
    dom::Element* const domElement = this->domElement();
    if (!domElement) {
        // TODO: throw ?
        onUpdateError_();
        return ElementStatus::InternalError;
    }

    const auto& position = domElement->getAttribute(ds::position).getVec2d();

    vacomplex::KeyVertex* kv = vacKeyVertexNode();
    if (!kv) {
        VacElement* parentElement = parentVacElement();
        if (!parentElement) {
            onUpdateError_();
            return ElementStatus::ErrorInParent;
        }
        vacomplex::Node* parentNode = parentElement->vacNode();
        if (!parentNode) {
            onUpdateError_();
            return ElementStatus::ErrorInParent;
        }
        kv = topology::ops::createKeyVertex(position, parentNode->toGroupUnchecked());
        setVacNode(kv);
        // position should already be dirty
    }
    else if (kv->position() != position) {
        topology::ops::setKeyVertexPosition(kv, position);
        dirtyPosition_();
    }

    return ElementStatus::Ok;
}

void VacKeyVertex::updateFromVac_() {
    namespace ds = dom::strings;
    vacomplex::KeyVertex* kv = vacKeyVertexNode();
    if (!kv) {
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

    const auto& position = domElement->getAttribute(ds::position).getVec2d();
    if (kv->position() != position) {
        domElement->setAttribute(ds::position, kv->position());
        dirtyPosition_();
    }
}

void VacKeyVertex::onAddJoinHalfedge_(const VacJoinHalfedge& joinHalfedge) {
    frameData_.clearJoinData_();
    dirtyJoinHalfedgesJoinData_(joinHalfedge.edgeCell());
}

void VacKeyVertex::onRemoveJoinHalfedge_(const VacJoinHalfedge& joinHalfedge) {
    frameData_.clearJoinData_();
    dirtyJoinHalfedgesJoinData_(joinHalfedge.edgeCell());
}

void VacKeyVertex::onJoinEdgePreJoinGeometryChanged_(const VacEdgeCell* edge) {
    frameData_.clearJoinData_();
    dirtyJoinHalfedgesJoinData_(edge);
}

void VacKeyVertex::computePosition_() {
    VacVertexCellFrameData& data = frameData_;
    if (data.isPositionComputed_ || data.isComputing_) {
        return;
    }
    vacomplex::VertexCell* v = vacVertexCellNode();
    if (!v) {
        return;
    }

    data.isComputing_ = true;

    data.position_ = v->position(data.time());

    data.isPositionComputed_ = true;
    data.isComputing_ = false;
}

void VacKeyVertex::computeJoin_() {
    VacVertexCellFrameData& data = frameData_;
    VGC_ASSERT(!data.isComputing_);

    if (data.isJoinComputed_) {
        return;
    }

    computePosition_();

    data.isComputing_ = true;

    detail::VacJoinFrameData& joinData = data.joinData_;
    geometry::Vec2d vertexPosition = data.position();

    // collect standalone edge data and halfwidths at join
    for (const VacJoinHalfedge& he : joinHalfedges()) {
        VacEdgeCell* cell = he.edgeCell();
        auto edgeData = const_cast<VacEdgeCellFrameData*>(cell->computeFrameDataAt(
            data.time(), VacEdgeComputationStage::PreJoinGeometry));
        if (!edgeData) {
            // huh ?
            continue;
        }

        const geometry::CurveSampleArray& samples = edgeData->preJoinSamples();
        if (samples.length() < 2) {
            continue;
        }

        detail::VacJoinHalfedgeFrameData& heData =
            joinData.halfedgesFrameData_.emplaceLast(he);
        heData.edgeData_ = edgeData;

        const bool isReverse = he.isReverse();
        if (!isReverse) {
            geometry::CurveSample sample = samples.first();
            heData.halfwidths_ = sample.halfwidths();
        }
        else {
            geometry::CurveSample sample = samples.last();
            heData.halfwidths_[0] = sample.halfwidth(1);
            heData.halfwidths_[1] = sample.halfwidth(0);
        }
    }

    // compute patch limits (future: hook for plugins)
    for (detail::VacJoinHalfedgeFrameData& heData : joinData.halfedgesFrameData_) {
        // We tested experimentally a per-slice equation, but it prevents us from
        // defining the angular order based on the result.
        // To keep more freedom we limit the input to halfwidths per edge.
        //
        // Let's keep it simple for now and use a single coefficient and multiply
        // each halfwidth to obtain the length limit.
        //
        constexpr double cutLimitCoefficient = 4.0;
        heData.patchCutLimits_ = cutLimitCoefficient * heData.halfwidths_;
    }

    // we define the interpolation length based on the cut limit
    //constexpr double interpolationLimitCoefficient = 1.5;

    const Int numHalfedges = joinData.halfedgesFrameData_.length();
    if (numHalfedges == 0) {
        // nothing to do
    }
    else if (numHalfedges == 1) {
        // cap, todo
        //const detail::VacJoinHalfedgeFrameData& halfedgeData = data.halfedgesData_[0];
        //Int basePatchIndex = halfedgeData.halfedge().isReverse() ? 2 : 0;
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
            data.joinData_.halfedgesFrameData_.begin(),
            data.joinData_.halfedgesFrameData_.end(),
            [](const detail::VacJoinHalfedgeFrameData& a,
               const detail::VacJoinHalfedgeFrameData& b) {
                return a.angle() < b.angle();
            });

        detail::VacJoinHalfedgeFrameData* halfedgeDataA =
            &data.joinData_.halfedgesFrameData_.last();
        detail::VacJoinHalfedgeFrameData* halfedgeDataB =
            &data.joinData_.halfedgesFrameData_.first();
        double angleA = halfedgeDataA->angle() - core::pi * 2;
        double angleB = 0;
        for (Int i = 0; i < data.joinData_.halfedgesFrameData_.length();
             ++i, halfedgeDataA = halfedgeDataB++, angleA = angleB) {

            angleB = halfedgeDataB->angle();
            halfedgeDataA->angleToNext_ = angleB - angleA;
        }
    }

    data.isJoinComputed_ = true;
    data.isComputing_ = false;
}

void VacKeyVertex::dirtyPosition_() {
    if (frameData_.isPositionComputed_) {
        frameData_.isPositionComputed_ = false;
        notifyChangesToDependents(ChangeFlag::VertexPosition);
    }
}

void VacKeyVertex::dirtyJoin_() {
    if (frameData_.isPositionComputed_) {
        frameData_.isPositionComputed_ = false;
        notifyChangesToDependents(ChangeFlag::VertexPosition);
    }
}

void VacKeyVertex::onUpdateError_() {
    removeVacNode();
    dirtyPosition_();
}

} // namespace vgc::workspace
