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

namespace {

bool isMultiJoinEnabled_ = false;

} // namespace

bool isMultiJoinEnabled() {
    return isMultiJoinEnabled_;
}

void setMultiJoinEnabled(bool enabled) {
    isMultiJoinEnabled_ = enabled;
}

} // namespace detail

void VacVertexCellFrameData::debugPaint_(graphics::Engine* /*engine*/) {

    using namespace graphics;
    using detail::VacJoinHalfedgeFrameData;
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

void VacKeyVertex::updateFromVac_(vacomplex::NodeDiffFlags diffs) {
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

    using vacomplex::NodeDiffFlag;
    if (diffs.hasAny({NodeDiffFlag::GeometryChanged, NodeDiffFlag::Created})) {
        const auto& position = domElement->getAttribute(ds::position).getVec2d();
        if (kv->position() != position) {
            domElement->setAttribute(ds::position, kv->position());
            dirtyPosition_();
        }
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

namespace {

template<bool fromEnd>
Int findJoinPatchLimit(
    VacEdgeCellFrameData* edgeData,
    double halfwidthArcRatio,
    Int side) {

    Int index = 0;
    const geometry::CurveSampleArray& samples = edgeData->preJoinSamples();
    if constexpr (!fromEnd) {
        for (auto it = samples.begin(); it != samples.end(); ++it, ++index) {
            const double hw = it->halfwidth(side);
            const double s = it->s();
            if (s * halfwidthArcRatio > hw) {
                break;
            }
        }
    }
    else {
        const double endS = samples.last().s();
        for (auto it = samples.rbegin(); it != samples.rend(); ++it, ++index) {
            const double hw = it->halfwidth(side);
            const double s = endS - it->s();
            if (s * halfwidthArcRatio > hw) {
                break;
            }
        }
    }
    return std::min(index, samples.length() / 3);
}

Int findJoinPatchLimit(
    VacEdgeCellFrameData* edgeData,
    double halfwidthArcRatio,
    Int side,
    bool fromEnd = false) {

    if (fromEnd) {
        return findJoinPatchLimit<true>(edgeData, halfwidthArcRatio, side);
    }
    else {
        return findJoinPatchLimit<false>(edgeData, halfwidthArcRatio, side);
    }
}

} // namespace

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
            heData.joinSample_ = sample;
        }
        else {
            geometry::CurveSample sample = samples.last();
            heData.halfwidths_[0] = sample.halfwidth(1);
            heData.halfwidths_[1] = sample.halfwidth(0);
            heData.joinSample_ = sample;
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

    const Int numHalfedges = joinData.halfedgesFrameData_.length();
    if (numHalfedges == 0) {
        // nothing to do
    }
    else if (numHalfedges == 1 || !detail::isMultiJoinEnabled()) {
        // caps

        for (detail::VacJoinHalfedgeFrameData& halfedgeData :
             data.joinData_.halfedgesFrameData_) {

            // only "round" cap for now.

            double maxHalfwidth =
                (std::max)(halfedgeData.halfwidths_[0], halfedgeData.halfwidths_[1]);

            geometry::CurveSample joinSample = halfedgeData.joinSample_;
            geometry::Vec2d normal = halfedgeData.joinSample_.normal();
            geometry::Vec2d dir = -halfedgeData.joinSample_.tangent();
            if (halfedgeData.isReverse()) {
                normal = -normal;
                dir = -dir;
            }

            detail::EdgeJoinPatch& patch =
                halfedgeData.edgeData_->patches_[halfedgeData.isReverse() ? 1 : 0];
            patch.isCap = true;
            //patch.sideSamples[0].clear();
            //patch.sideSamples[1].clear();

            float s = 0.f;
            float u = 0.f;
            if (halfedgeData.isReverse()) {
                s = static_cast<float>(
                    halfedgeData.edgeData_->sampling_->samples().last().s());
                u = 1.f;
            }

            geometry::Vec2d base = dir * maxHalfwidth;

            bool isStyleRadial = true;
            if (isStyleRadial) {
                // constant S; radial gradient T, V
                for (Int i = 0; i <= 32; ++i) {
                    detail::EdgeJoinPatchSample& ps0 = patch.sideSamples[0].emplaceLast();
                    detail::EdgeJoinPatchSample& ps1 = patch.sideSamples[1].emplaceLast();

                    double a = core::pi * 0.5 * i / 32.f;
                    double x = std::cos(a);
                    double y = std::sin(a);
                    double h0 = joinSample.halfwidth(0) * y;
                    double h1 = joinSample.halfwidth(1) * y;
                    geometry::Vec2d midPoint = joinSample.position() + x * base;

                    ps0.centerPoint = joinSample.position();
                    ps1.centerPoint = ps0.centerPoint;
                    ps0.centerSU = geometry::Vec2f{s, u};
                    ps1.centerSU = ps0.centerSU;

                    ps0.sidePoint = midPoint + h0 * normal;
                    ps1.sidePoint = midPoint - h1 * normal;
                    ps0.sideSTUV = geometry::Vec4f{
                        s,
                        static_cast<float>((ps0.sidePoint - ps0.centerPoint).length()),
                        u,
                        1.f};
                    ps1.sideSTUV = geometry::Vec4f{
                        s,
                        static_cast<float>((ps1.sidePoint - ps1.centerPoint).length()),
                        u,
                        1.f};
                }
            }
            else {
                // constant S; directional gradient T, V
                for (Int i = 0; i <= 32; ++i) {
                    detail::EdgeJoinPatchSample& ps0 = patch.sideSamples[0].emplaceLast();
                    detail::EdgeJoinPatchSample& ps1 = patch.sideSamples[1].emplaceLast();

                    double a = core::pi * 0.5 * i / 32.f;
                    double x = std::sin(a);
                    double y = std::cos(a);
                    double h0 = joinSample.halfwidth(0) * y;
                    double h1 = joinSample.halfwidth(1) * y;
                    geometry::Vec2d midPoint = joinSample.position() + x * base;

                    ps0.centerPoint = midPoint;
                    ps1.centerPoint = ps0.centerPoint;
                    ps0.centerSU = geometry::Vec2f{s, u};
                    ps1.centerSU = ps0.centerSU;

                    ps0.sidePoint = midPoint + h0 * normal;
                    ps1.sidePoint = midPoint - h1 * normal;
                    ps0.sideSTUV = geometry::Vec4f{
                        s, static_cast<float>(h0), u, static_cast<float>(y)};
                    ps1.sideSTUV = geometry::Vec4f{
                        s, static_cast<float>(h1), u, static_cast<float>(y)};
                }
            }
        }
    }
    else {
        // Our current method considers incident straight lines of constant widths
        // and interpolates the original samples toward the computed samples.
        // This brings a few problems:
        // - The original samples projected onto the straight line model
        //   must remain in order. Otherwise it would result in a self-overlap.
        // - If the centerline is not contained in between the straight model outlines
        //   the interpolated outlines would cross it.
        //   We can either adapt the centerline or limit the patch length.

        // Limitations to work on:
        // - The two joins of a collapsing edge produce overlaps.
        //   In the context of animation we have to prevent popping when the two vertices
        //   become one.

        // we define the interpolation length based on the cut limit
        constexpr double interpolationLimitCoefficient = 1.5;

        // compute the straight line model tangents and fix limits
        for (detail::VacJoinHalfedgeFrameData& heData : joinData.halfedgesFrameData_) {
            // We approximate the tangent using the position of the first sample
            //
            const geometry::CurveSampleArray& preJoinSamples =
                heData.edgeData_->preJoinSamples();
            const bool isReverse = heData.isReverse();
            const double endS = preJoinSamples.last().s();
            // the patch cannot use more than half of the edge
            const double maxS = endS * 0.5;
            // we'll interpolate the center-line too and it is common to both sides
            const double patchLengthLimit =
                interpolationLimitCoefficient
                * std::max(heData.patchCutLimits_[0], heData.patchCutLimits_[1]);
            const double sqPatchLengthLimit = patchLengthLimit * patchLengthLimit;
            double patchLength = patchLengthLimit;
            //
            core::Array<geometry::CurveSample>& workingSamples = heData.workingSamples_;
            geometry::Vec2d outgoingTangent;
            workingSamples.clear();
            if (!isReverse) {
                auto it = preJoinSamples.begin();
                Int i = 0;
                auto previousIt = it;
                double previousSqDist = 0;
                for (; it != preJoinSamples.end(); previousIt = it++, ++i) {
                    const geometry::CurveSample& sample = *it;
                    const geometry::Vec2d position = it->position();
                    if (i == 1) {
                        outgoingTangent = (position - vertexPosition).normalized();
                    }
                    const double sqDist = (vertexPosition - position).squaredLength();
                    const double s = it->s();
                    double tStop = 2;
                    if (s > maxS) {
                        const double previousS = previousIt->s();
                        tStop = (maxS - previousS) / (s - previousS);
                    }
                    if (sqDist > sqPatchLengthLimit) {
                        const double distance = std::sqrt(sqDist);
                        const double previousDistance = std::sqrt(previousSqDist);
                        tStop = (std::min)(
                            tStop,
                            (patchLengthLimit - previousDistance)
                                / (distance - previousDistance));
                    }
                    if (tStop <= 1) {
                        geometry::CurveSample mergeSample =
                            geometry::lerp(*previousIt, *it, tStop);
                        const double distance =
                            (vertexPosition - mergeSample.position()).length();
                        workingSamples.emplaceLast(mergeSample);
                        patchLength = (std::min)(patchLengthLimit, distance);
                        detail::EdgeJoinPatchMergeLocation& mergeLocation =
                            heData.edgeData_->patches_[0].mergeLocation;
                        mergeLocation.halfedgeNextSampleIndex = i;
                        mergeLocation.t = tStop;
                        mergeLocation.sample = mergeSample;
                        break;
                    }
                    workingSamples.emplaceLast(sample);
                    previousSqDist = sqDist;
                }
            }
            else {
                //std::array<bool, 2> sideDone = {};
                auto it = preJoinSamples.rbegin();
                Int i = 0;
                auto previousIt = it;
                double previousSqDist = 0;
                for (; it != preJoinSamples.rend(); previousIt = it++, ++i) {
                    const geometry::CurveSample& sample = *it;
                    const geometry::Vec2d position = it->position();
                    if (i == 1) {
                        outgoingTangent = (position - vertexPosition).normalized();
                    }
                    const double sqDist = (vertexPosition - position).squaredLength();
                    const double s = endS - it->s();
                    double tStop = 2;
                    if (s > maxS) {
                        // lerp a new sample
                        const double previousS = endS - previousIt->s();
                        tStop = (maxS - previousS) / (s - previousS);
                    }
                    if (sqDist > sqPatchLengthLimit) {
                        const double distance = std::sqrt(sqDist);
                        const double previousDistance = std::sqrt(previousSqDist);
                        tStop = (std::min)(
                            tStop,
                            (patchLengthLimit - previousDistance)
                                / (distance - previousDistance));
                    }
                    if (tStop <= 1) {
                        geometry::CurveSample mergeSample =
                            geometry::lerp(*previousIt, *it, tStop);
                        const double distance =
                            (vertexPosition - mergeSample.position()).length();
                        workingSamples.emplaceLast(
                            mergeSample.position(),
                            -mergeSample.normal(),
                            geometry::Vec2d(
                                mergeSample.halfwidth(1), mergeSample.halfwidth(0)),
                            endS - mergeSample.s());
                        patchLength = (std::min)(patchLengthLimit, distance);
                        detail::EdgeJoinPatchMergeLocation& mergeLocation =
                            heData.edgeData_->patches_[1].mergeLocation;
                        mergeLocation.halfedgeNextSampleIndex = i;
                        mergeLocation.t = tStop;
                        mergeLocation.sample = mergeSample;
                        break;
                    }
                    workingSamples.emplaceLast(
                        sample.position(),
                        -sample.normal(),
                        geometry::Vec2d(sample.halfwidth(1), sample.halfwidth(0)),
                        s);
                    previousSqDist = sqDist;
                }
            }
            heData.patchLength_ = patchLength;
            const double patchCutLimit = patchLength / interpolationLimitCoefficient;
            heData.patchCutLimits_[0] =
                (std::min)(heData.patchCutLimits_[0], patchCutLimit);
            heData.patchCutLimits_[1] =
                (std::min)(heData.patchCutLimits_[1], patchCutLimit);

            // outgoingTangent = (workingSamples.last().position() - vertexPosition).normalized();
            heData.outgoingTangent_ = outgoingTangent;

            double angle = outgoingTangent.angle();
            if (angle < 0) {
                angle += core::pi * 2;
            }
            heData.angle_ = angle;
        }

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

            auto& sidePatchDataA0 = halfedgeDataA->sidePatchData_[0];
            auto& sidePatchDataB1 = halfedgeDataB->sidePatchData_[1];

            detail::BoxModelBorder borderA = {
                vertexPosition
                    + halfedgeDataA->outgoingTangent_.orthogonalized()
                          * halfedgeDataA->halfwidths_[0],
                halfedgeDataA->outgoingTangent_};
            sidePatchDataA0.border = borderA;

            detail::BoxModelBorder borderB = {
                vertexPosition
                    - halfedgeDataB->outgoingTangent_.orthogonalized()
                          * halfedgeDataB->halfwidths_[1],
                halfedgeDataB->outgoingTangent_};
            sidePatchDataB1.border = borderB;

            sidePatchDataA0.clear();
            sidePatchDataB1.clear();

            sidePatchDataA0.joinHalfwidth = halfedgeDataA->halfwidths_[0];
            sidePatchDataB1.joinHalfwidth = halfedgeDataB->halfwidths_[1];

            std::optional<geometry::Vec2d> isect =
                borderA.computeIntersectionParametersWith(borderB);
            if (isect.has_value()) {
                geometry::Vec2d ts = isect.value();
                if (ts[0] > 0 && ts[1] > 0) {
                    sidePatchDataA0.filletLength =
                        (std::min)(ts[0], halfedgeDataA->patchCutLimits_[0]);
                    sidePatchDataA0.isCutFillet = true;
                    sidePatchDataA0.joinHalfwidth = 0;
                    sidePatchDataB1.filletLength =
                        (std::min)(ts[1], halfedgeDataB->patchCutLimits_[1]);
                    sidePatchDataB1.isCutFillet = true;
                    sidePatchDataB1.joinHalfwidth = 0;
                }
                else if (ts[0] < 0 && ts[1] < 0) {
                    sidePatchDataA0.extLength = -ts[0];
                    sidePatchDataB1.extLength = -ts[1];
                }
                else {
                    if (halfedgeDataA->halfwidths_[0] > halfedgeDataB->halfwidths_[1]) {
                        geometry::Vec2d farCorner = borderA.origin;
                        detail::BoxModelBorder splitBorder{
                            vertexPosition, (farCorner - vertexPosition).normalized()};
                        geometry::Vec2d split =
                            splitBorder.computeIntersectionParametersWith(borderB)
                                .value_or(geometry::Vec2d());

                        sidePatchDataA0.joinHalfwidth = split[0];
                        sidePatchDataA0.filletLength = halfedgeDataA->patchLength_;
                        if (ts[0] > 0) {
                            sidePatchDataA0.filletLength =
                                (std::min)(ts[0], halfedgeDataA->patchLength_);
                        }

                        const double tB = split[1];
                        if (tB > 0) {
                            sidePatchDataB1.filletLength =
                                (std::min)(tB, halfedgeDataB->patchCutLimits_[1]);
                            sidePatchDataB1.isCutFillet = true;
                            sidePatchDataB1.joinHalfwidth = 0;
                        }
                        else {
                            sidePatchDataB1.extLength = -tB;
                        }
                    }
                    else {
                        geometry::Vec2d farCorner = borderB.origin;
                        detail::BoxModelBorder splitRay{
                            vertexPosition, (farCorner - vertexPosition).normalized()};
                        geometry::Vec2d split =
                            splitRay.computeIntersectionParametersWith(borderA).value_or(
                                geometry::Vec2d());

                        sidePatchDataB1.joinHalfwidth = split[0];
                        sidePatchDataB1.filletLength = halfedgeDataB->patchLength_;
                        if (ts[1] > 0) {
                            sidePatchDataB1.filletLength =
                                (std::min)(ts[1], halfedgeDataB->patchLength_);
                        }

                        const double tA = split[1];
                        if (tA > 0) {
                            sidePatchDataA0.filletLength =
                                (std::min)(tA, halfedgeDataA->patchCutLimits_[0]);
                            sidePatchDataA0.isCutFillet = true;
                            sidePatchDataA0.joinHalfwidth = 0;
                        }
                        else {
                            sidePatchDataA0.extLength = -tA;
                        }
                    }
                }
            }
            else {
                // todo
            }
        }

        // now create the actual patches
        for (auto& halfedgeData : data.joinData_.halfedgesFrameData_) {
            geometry::CurveSampleArray& workingSamples = halfedgeData.workingSamples_;

            const double maxFilletLength = (std::max)(
                halfedgeData.sidePatchData_[0].filletLength,
                halfedgeData.sidePatchData_[1].filletLength);

            detail::BoxModelBorder centerRay = {
                vertexPosition, halfedgeData.outgoingTangent_};
            geometry::Vec2d centerRayNormal =
                halfedgeData.outgoingTangent_.orthogonalized();

            const double tFilletMax = maxFilletLength / halfedgeData.patchLength_;
            const double sMax = workingSamples.last().s();
            const double sFilletMax = tFilletMax * sMax;
            // straighten samples
            auto it = workingSamples.begin();
            auto previousIt = it;
            for (; it != workingSamples.end(); previousIt = it++) {
                const double s = it->s();
                if (s > sFilletMax) {
                    const double previousS = previousIt->s();
                    const double d = (sFilletMax / sMax) * halfedgeData.patchLength_;
                    const double t = (sFilletMax - previousS) / (s - previousS);
                    const double ot = 1 - t;
                    geometry::CurveSample newSample(
                        centerRay.pointAt(d),
                        centerRayNormal,
                        previousIt->halfwidths() * ot + it->halfwidths() * t,
                        sFilletMax);
                    it = workingSamples.emplace(it, newSample);
                    previousIt = it++;
                    break;
                }
                const double d = (s / sMax) * halfedgeData.patchLength_;
                it->setPosition(centerRay.pointAt(d));
                it->setNormal(centerRayNormal);
                geometry::Vec2f hwf(it->halfwidths());
                if (s == sFilletMax) {
                    previousIt = it++;
                    break;
                }
            }
            // lerp samples
            const double sInterp = sMax - sFilletMax;
            for (; it != workingSamples.end(); previousIt = it++) {
                const double s = it->s();
                const double d = (s / sMax) * halfedgeData.patchLength_;
                const double t = (s - sFilletMax) / sInterp;
                const double ot = 1 - t;
                geometry::Vec2d rayPoint = centerRay.pointAt(d);
                it->setPosition(rayPoint * ot + it->position() * t);
                it->setNormal((centerRayNormal * ot + it->normal() * t).normalized());
            }

            for (Int i = 0; i < 2; ++i) {
                core::Array<detail::EdgeJoinPatchSample> patchSamples;
                const auto& sidePatchData = halfedgeData.sidePatchData_[i];
                //const auto& otherSidePatchData = halfedgeData.sidePatchData_[1 - i];
                const double halfwidth = halfedgeData.halfwidths_[i];

                // extension
                if (sidePatchData.extLength > 0) {
                    // todo: miter limit
                    auto& p = patchSamples.emplaceLast();
                    p.centerPoint = vertexPosition;
                    p.sidePoint = sidePatchData.border.pointAt(-sidePatchData.extLength);
                    p.sideSTUV =
                        geometry::Vec4f(0, static_cast<float>(halfwidth), 0, 1.f);
                    p.centerSU = geometry::Vec2f(0, 0);
                }

                const double tFillet =
                    sidePatchData.filletLength / halfedgeData.patchLength_;
                const double sFillet = tFillet * sMax;

                //const double tFillet2 =
                //    otherSidePatchData.filletLength / halfedgeData.patchLength_;
                //const double sFillet2 = tFillet2 * sMax;

                const int normalMultiplier = i ? -1 : 1;

                it = workingSamples.begin();
                if constexpr (0) { // debug
                    for (; it != workingSamples.end(); ++it) {
                        const double s = it->s();
                        auto& p = patchSamples.emplaceLast();
                        p.centerPoint = it->position();
                        double hw = it->halfwidth(i);
                        p.sidePoint =
                            p.centerPoint + normalMultiplier * hw * it->normal();
                        p.sideSTUV = geometry::Vec4f(
                            static_cast<float>(s),
                            static_cast<float>(hw),
                            static_cast<float>(s),
                            1.f);
                        p.centerSU =
                            geometry::Vec2f(static_cast<float>(s), static_cast<float>(s));
                    }
                }
                else if (sidePatchData.isCutFillet) {
                    if (sFillet > 0) {
                        // lerp halfwidths from join halfwidth to cut halfwidth
                        previousIt = it;
                        for (; it != workingSamples.end(); previousIt = it++) {
                            const double s = it->s();
                            if (s > sFillet) {
                                const double d =
                                    (sFillet / sMax) * halfedgeData.patchLength_;
                                auto& p = patchSamples.emplaceLast();
                                p.centerPoint = centerRay.pointAt(d);
                                p.sidePoint = sidePatchData.border.pointAt(d);
                                p.sideSTUV = geometry::Vec4f(
                                    static_cast<float>(sFillet),
                                    static_cast<float>(halfwidth),
                                    static_cast<float>(sFillet),
                                    1.f);
                                p.centerSU = geometry::Vec2f(
                                    static_cast<float>(sFillet),
                                    static_cast<float>(sFillet));
                                break;
                            }
                            const double t = s / sFillet;
                            const double ot = 1 - t;
                            auto& p = patchSamples.emplaceLast();
                            p.centerPoint = it->position();
                            const double hw =
                                sidePatchData.joinHalfwidth * ot + halfwidth * t;
                            p.sidePoint =
                                p.centerPoint + normalMultiplier * hw * centerRayNormal;
                            p.sideSTUV = geometry::Vec4f(
                                static_cast<float>(s),
                                static_cast<float>(hw),
                                static_cast<float>(s),
                                1.f);
                            p.centerSU = geometry::Vec2f(
                                static_cast<float>(s), static_cast<float>(s));
                            if (s == sFillet) {
                                previousIt = it++;
                                break;
                            }
                        }
                    }
                    // lerp halfwidths from cut halfwidth to original halfwidth
                    //Int fixIndex = 0;
                    const double sInterp2 = sMax - sFillet;
                    for (; it != workingSamples.end(); ++it) {
                        const double s = it->s();
                        //const double d = (s / sMax) * halfedgeData.patchLength_;
                        const double t = (s - sFillet) / sInterp2;
                        const double ot = 1 - t;
                        // temporary fix
                        //if (s == sFillet2 && sFillet2 > 0 && sFillet2 != sFillet) {
                        //    fixIndex = patchSamples.size();
                        //    auto& p = patchSamples.emplaceLast(patchSamples.last());
                        //    p.centerPoint = it->position();
                        //    p.centerSU = geometry::Vec2f(s, s);
                        //    continue;
                        //}
                        auto& p = patchSamples.emplaceLast();
                        p.centerPoint = it->position();
                        const double hw = halfwidth * ot + it->halfwidth(i) * t;
                        p.sidePoint =
                            p.centerPoint + normalMultiplier * hw * it->normal();
                        p.sideSTUV = geometry::Vec4f(
                            static_cast<float>(s),
                            static_cast<float>(hw),
                            static_cast<float>(s),
                            1.f);
                        p.centerSU =
                            geometry::Vec2f(static_cast<float>(s), static_cast<float>(s));
                    }
                }
                else {
                    // lerp halfwidths from box halfwidth to original halfwidth
                    // additional lerp from join halfwidth to computed halfwidth between 0 and sFillet
                    //Int fixIndex = 0;
                    for (; it != workingSamples.end(); ++it) {
                        const double s = it->s();
                        const double t = s / sMax;
                        const double ot = 1 - t;
                        // temporary fix
                        //if (s == sFillet2 && sFillet2 > 0 && sFillet2 != sFillet) {
                        //    fixIndex = patchSamples.size();
                        //    auto& p = patchSamples.emplaceLast(patchSamples.last());
                        //    p.centerPoint = it->position();
                        //    p.centerSU = geometry::Vec2f(s, s);
                        //}
                        auto& p = patchSamples.emplaceLast();
                        p.centerPoint = it->position();
                        double hw = halfwidth * ot + it->halfwidth(i) * t;
                        if (sFillet > 0 && s < sFillet) {
                            // this works but looks like magic written this way
                            const double t1 = sFillet / sMax;
                            const double ot1 = 1 - t1;
                            const double mhw = halfwidth * ot1 + it->halfwidth(i) * t1;

                            const double t2 = s / sFillet;
                            const double ot2 = 1 - t2;
                            hw = sidePatchData.joinHalfwidth * ot2 + mhw * t2;
                        }
                        p.sidePoint =
                            p.centerPoint + normalMultiplier * hw * it->normal();
                        p.sideSTUV = geometry::Vec4f(
                            static_cast<float>(s),
                            static_cast<float>(hw),
                            static_cast<float>(s),
                            1.f);
                        p.centerSU =
                            geometry::Vec2f(static_cast<float>(s), static_cast<float>(s));
                    }
                }

                // fill data
                const bool isReverse = halfedgeData.isReverse();
                detail::EdgeJoinPatch& patch0 =
                    halfedgeData.edgeData_->patches_[isReverse ? 1 : 0];
                patch0.sideSamples[i] = std::move(patchSamples);
            }
        }
    }

    data.isJoinComputed_ = true;
    data.isComputing_ = false;
} // namespace vgc::workspace

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
