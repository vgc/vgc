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

#include <vgc/geometry/bezier.h>
#include <vgc/geometry/vec2d.h>
#include <vgc/geometry/vec3d.h>
#include <vgc/graphics/detail/shapeutil.h>
#include <vgc/workspace/colors.h>
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

geometry::Rect2d VacKeyVertex::boundingBox(core::AnimTime t) const {
    // TODO: use cached position in frame data
    vacomplex::KeyVertex* kv = vacKeyVertexNode();
    if (kv && frameData_.time() == t) {
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
    core::AnimTime t) const {

    vacomplex::KeyVertex* kv = vacKeyVertexNode();
    if (kv && frameData_.time() == t) {
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

bool VacKeyVertex::isSelectableInRect(const geometry::Rect2d& rect, core::AnimTime t)
    const {

    vacomplex::KeyVertex* kv = vacKeyVertexNode();
    if (kv && frameData_.time() == t) {
        geometry::Vec2d pos = vacKeyVertexNode()->position();
        return rect.contains(pos);
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

    // TODO: share the disk mesh

    VacVertexCellFrameData& data = frameData_;
    vacomplex::KeyVertex* kv = vacKeyVertexNode();
    if (!kv || t != kv->time()) {
        return;
    }

    geometry::Vec2f posF(kv->position());

    if (flags.hasAny(PaintOption::Outline | PaintOption::Selected)) {

        constexpr float selectionDiskRadius = 6.f;
        constexpr Int selectionDiskNumSides = 16;

        if (!data.selectionGeometry_) {
            data.selectionGeometry_ = graphics::detail::createScreenSpaceDisk(
                engine,
                posF,
                selectionDiskRadius,
                colors::selection,
                selectionDiskNumSides);
        }
        else if (data.isSelectionGeometryDirty_) {
            graphics::detail::updateScreenSpaceDisk(
                engine,
                data.selectionGeometry_,
                posF,
                selectionDiskRadius,
                colors::selection);
        }

        engine->setProgram(graphics::BuiltinProgram::ScreenSpaceDisplacement);
        engine->draw(data.selectionGeometry_);
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
        kv = vacomplex::ops::createKeyVertex(position, parentNode->toGroupUnchecked());
        setVacNode(kv);
        // position should already be dirty
    }
    else if (kv->position() != position) {
        vacomplex::ops::setKeyVertexPosition(kv, position);
        dirtyPosition_();
    }

    return ElementStatus::Ok;
}

void VacKeyVertex::updateFromVac_(vacomplex::ModifiedNodeFlags flags) {
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

    using vacomplex::ModifiedNodeFlag;
    if (flags.has(ModifiedNodeFlag::GeometryChanged)) {
        const auto& position = domElement->getAttribute(ds::position).getVec2d();
        if (kv->position() != position) {
            domElement->setAttribute(ds::position, kv->position());
            dirtyPosition_();
        }
    }
    else if (flags.has(ModifiedNodeFlag::MeshChanged)) {
        frameData_.isSelectionGeometryDirty_ = true;
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

/*

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
    return (std::min)(index, samples.length() / 3);
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

*/

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
        Int numSamples = samples.length();

        if (numSamples < 2) {
            continue;
        }

        detail::VacJoinHalfedgeFrameData& heData =
            joinData.halfedgesFrameData_.emplaceLast(he);
        heData.edgeData_ = edgeData;

        const bool isReverse = he.isReverse();
        if (!isReverse) {
            geometry::CurveSample sample = samples.getUnchecked(0);
            heData.halfwidths_ = sample.halfwidths();
            heData.joinSample_ = sample;
            heData.joinPreviousSample_ = samples.getUnchecked(1);
        }
        else {
            geometry::CurveSample sample = samples.getUnchecked(numSamples - 1);
            heData.halfwidths_[0] = sample.halfwidth(1);
            heData.halfwidths_[1] = sample.halfwidth(0);
            heData.joinSample_ = geometry::CurveSample(
                sample.position(), -sample.normal(), heData.halfwidths_, 0);
            geometry::CurveSample previousSample = samples.getUnchecked(numSamples - 2);
            geometry::Vec2d previousHw = previousSample.halfwidths();
            heData.joinPreviousSample_ = geometry::CurveSample(
                previousSample.position(),
                -previousSample.normal(),
                geometry::Vec2d(previousHw[1], previousHw[0]),
                sample.s() - previousSample.s());
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

    using geometry::CurveSample;
    using geometry::Vec2d;
    using geometry::Vec3d;
    using geometry::Vec3f;

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

            CurveSample joinSample = halfedgeData.joinSample_;
            Vec2d normal = halfedgeData.joinSample_.normal();
            Vec2d capDir = -halfedgeData.joinSample_.tangent();

            detail::EdgeJoinPatch& patch =
                halfedgeData.edgeData_->patches_[halfedgeData.isReverse() ? 1 : 0];
            patch.isCap = true;

            patch.sideSamples[0].clear();
            patch.sideSamples[1].clear();

            Vec2d capOrigin = joinSample.position();

            // hard-coded configuration (atm)
            bool isStyleRadial = true;
            bool isCircular = false;

            if (isCircular) {
                if (isStyleRadial) {
                    // constant S; radial gradient T, V
                    Vec2d base = capDir * maxHalfwidth;
                    for (Int i = 0; i <= 32; ++i) {
                        detail::EdgeJoinPatchSample& ps0 =
                            patch.sideSamples[0].emplaceLast();
                        detail::EdgeJoinPatchSample& ps1 =
                            patch.sideSamples[1].emplaceLast();

                        double a = core::pi * 0.5 * i / 32.f;
                        double x = std::cos(a);
                        double y = std::sin(a);
                        double h0 = joinSample.halfwidth(0) * y;
                        double h1 = joinSample.halfwidth(1) * y;

                        Vec2d midPoint = capOrigin + x * base;

                        ps0.centerPoint = capOrigin;
                        ps1.centerPoint = ps0.centerPoint;
                        ps0.centerSTV = geometry::Vec3f(0, 0, 0);
                        ps1.centerSTV = ps0.centerSTV;

                        ps0.sidePoint = midPoint + h0 * normal;
                        ps1.sidePoint = midPoint - h1 * normal;
                        ps0.sideSTV = geometry::Vec3f(
                            0,
                            static_cast<float>(
                                (ps0.sidePoint - ps0.centerPoint).length()),
                            1.f);
                        ps1.sideSTV = geometry::Vec3f(
                            0,
                            static_cast<float>(
                                (ps1.sidePoint - ps1.centerPoint).length()),
                            1.f);
                    }
                }
                else {
                    // constant S; directional gradient T, V
                    patch.extensionS = static_cast<float>(maxHalfwidth);
                    for (Int i = 0; i <= 32; ++i) {
                        detail::EdgeJoinPatchSample& ps0 =
                            patch.sideSamples[0].emplaceLast();
                        detail::EdgeJoinPatchSample& ps1 =
                            patch.sideSamples[1].emplaceLast();

                        double a = core::pi * 0.5 * i / 32.f;
                        double x = std::sin(a);
                        double y = std::cos(a);
                        double h0 = joinSample.halfwidth(0) * y;
                        double h1 = joinSample.halfwidth(1) * y;
                        double s = x * maxHalfwidth;
                        float sf = static_cast<float>(s);

                        Vec2d midPoint = capOrigin + capDir * s;

                        ps0.centerPoint = midPoint;
                        ps1.centerPoint = ps0.centerPoint;
                        ps0.centerSTV = geometry::Vec3f(sf, 0, 0);
                        ps1.centerSTV = ps0.centerSTV;

                        ps0.sidePoint = midPoint + h0 * normal;
                        ps1.sidePoint = midPoint - h1 * normal;
                        ps0.sideSTV = geometry::Vec3f(
                            sf, static_cast<float>(h0), static_cast<float>(y));
                        ps1.sideSTV = geometry::Vec3f(
                            sf, static_cast<float>(h1), static_cast<float>(y));
                    }
                }
            }
            else { // !isCircular
                constexpr double oneThird = 1.0 / 3.0;

                geometry::CurveSample joinSamplePrev = halfedgeData.joinPreviousSample_;
                Vec2d cp = joinSample.position();
                Vec2d sp0 = joinSample.sidePoint(0);
                Vec2d sp1 = joinSample.sidePoint(1);

                bool isT0Normalizable = false;
                bool isT1Normalizable = false;
                Vec2d t0 = (sp0 - joinSamplePrev.sidePoint(0))
                               .normalized(&isT0Normalizable, core::epsilon);
                Vec2d t1 = (sp1 - joinSamplePrev.sidePoint(1))
                               .normalized(&isT1Normalizable, core::epsilon);
                Vec2d halfwidths = joinSample.halfwidths();

                // fix-up tangents if there is a cusp
                if (!isT0Normalizable) {
                    t0 = capDir;
                }
                else if (t0.dot(capDir) < 0) {
                    t0 = -t0;
                }
                if (!isT1Normalizable) {
                    t1 = capDir;
                }
                else if (t1.dot(capDir) < 0) {
                    t1 = -t1;
                }

                Int numPoints = 63;
                std::array<const Vec2d, 4> controlPoints = {
                    sp0,
                    sp0 + t0 * (oneThird * (1 + capDir.dot(t0))) * 2.0 * halfwidths[0],
                    sp1 + t1 * (oneThird * (1 + capDir.dot(t1))) * 2.0 * halfwidths[1],
                    sp1};

                geometry::Vec2dArray points;
                points.reserve(numPoints + 1); // +1 for patch split sample

                Int splitIndex = 0;
                {
                    Int i = 0;
                    for (; i < numPoints; ++i) {
                        double u = static_cast<double>(i + 1) / (numPoints + 1);
                        Vec2d p =
                            geometry::cubicBezierPosCasteljau<Vec2d>(controlPoints, u);
                        Vec2d op = p - capOrigin;
                        double side = op.dot(normal);
                        if (side < 0) {
                            splitIndex = i;
                            const Vec2d d1 = capDir;
                            const Vec2d d2 = (points.last() - p).normalized();
                            double delta = d1.det(d2);
                            if (std::abs(delta) > core::epsilon) {
                                double inv_delta = 1 / delta;
                                Vec2d pSplit = p + d2 * op.det(d1) * inv_delta;
                                points.append(pSplit);
                            }
                            points.append(p);
                            ++i;
                            break;
                        }
                        points.append(p);
                    }
                    for (; i < numPoints; ++i) {
                        double u = static_cast<double>(i + 1) / (numPoints + 1);
                        Vec2d p =
                            geometry::cubicBezierPosCasteljau<Vec2d>(controlPoints, u);
                        points.append(p);
                    }
                }
                numPoints = points.length();

                if (isStyleRadial) {
                    // constant S; radial gradient T, V
                    Vec2d base = capDir * maxHalfwidth;
                    bool previousIsSide0 = true;

                    Int indexOfSep = 0;

                    for (Int i = splitIndex; i >= 0; --i) {
                        detail::EdgeJoinPatchSample& ps =
                            patch.sideSamples[0].emplaceLast();
                        ps.centerPoint = capOrigin;
                        ps.centerSTV = geometry::Vec3f(0, 0, 0);
                        ps.sidePoint = points[i];
                        ps.sideSTV = geometry::Vec3f(
                            0,
                            static_cast<float>((ps.sidePoint - capOrigin).length()),
                            1.f);
                    }

                    for (Int i = splitIndex; i < numPoints; ++i) {
                        detail::EdgeJoinPatchSample& ps =
                            patch.sideSamples[1].emplaceLast();
                        ps.centerPoint = capOrigin;
                        ps.centerSTV = geometry::Vec3f(0, 0, 0);
                        ps.sidePoint = points[i];
                        ps.sideSTV = geometry::Vec3f(
                            0,
                            static_cast<float>((ps.sidePoint - capOrigin).length()),
                            1.f);
                    }
                }
                else {
                    // We want a strip in the direction of capDir.
                    // It is not trivial since we have to use a
                    // sweep-line like algorithm.

                    struct CrossSection {
                        Vec2d pos0;
                        Vec2d pos1;
                        double s;
                        double t0;
                        double t1;
                    };

                    double minS = 0;
                    Int minSIndex = 0;
                    core::Array<CrossSection> crossSections(numPoints, core::noInit);
                    for (Int i = 0; i < numPoints; ++i) {
                        Vec2d p = points[i];
                        Vec2d d = p - capOrigin;
                        double s = -d.dot(capDir);
                        crossSections[i].pos0 = p;
                        crossSections[i].s = s;
                        crossSections[i].t0 = d.dot(normal);
                        if (s < minS) {
                            minSIndex = i;
                            minS = s;
                        }
                    }
                    crossSections[splitIndex].t0 = 0;

                    // complete the cross sections by computing the point on the other side
                    {
                        CrossSection cs1 = {sp1, {}, 0, halfwidths[1], 0};
                        Int i2 = numPoints - 1;
                        for (Int i = 0; i < minSIndex; ++i) {
                            while (crossSections[i2].s > crossSections[i].s) {
                                cs1 = crossSections[i2--];
                            }
                            const CrossSection& cs2 = crossSections[i2];
                            double t = (crossSections[i].s - cs1.s) / (cs2.s - cs1.s);
                            double ot = 1 - t;
                            crossSections[i].pos1 = ot * cs1.pos0 + t * cs2.pos0;
                            crossSections[i].t1 = ot * cs1.t0 + t * cs2.t0;
                        }
                        crossSections[minSIndex].pos1 = crossSections[minSIndex].pos0;
                        crossSections[minSIndex].t1 = crossSections[minSIndex].t0;
                        cs1 = {sp0, {}, 0, halfwidths[0], 0};
                        i2 = 0;
                        for (Int i = numPoints - 1; i > minSIndex; --i) {
                            while (crossSections[i2].s > crossSections[i].s) {
                                cs1 = crossSections[i2++];
                            }
                            crossSections[i].pos1 = crossSections[i].pos0;
                            crossSections[i].t1 = crossSections[i].t0;
                            const CrossSection& cs2 = crossSections[i2];
                            double t = (crossSections[i].s - cs1.s) / (cs2.s - cs1.s);
                            double ot = 1 - t;
                            crossSections[i].pos0 = ot * cs1.pos0 + t * cs2.pos0;
                            crossSections[i].t0 = ot * cs1.t0 + t * cs2.t0;
                        }
                    }

                    // remove half the cross sections on the "small" side
                    double joinWidth = halfwidths[0] + halfwidths[1];
                    bool isSortedInReverseOrder = false;
                    if (halfwidths[1] + crossSections[minSIndex].t0 > joinWidth * 0.5) {
                        crossSections.removeFirst(minSIndex);
                    }
                    else {
                        crossSections.removeRange(minSIndex + 1, crossSections.length());
                        isSortedInReverseOrder = true;
                    }

                    // create patches

                    auto appendPatchSamplesFromCrossSection =
                        [&](const CrossSection& cs) {
                            //
                            double t0 = std::abs(cs.t0);
                            double t1 = std::abs(cs.t1);
                            float s = static_cast<float>(cs.s);
                            Vec3f stv0(
                                s,
                                static_cast<float>(t0),
                                static_cast<float>(t0 / halfwidths[0]));
                            Vec3f stv1(
                                s,
                                static_cast<float>(t1),
                                static_cast<float>(t1 / halfwidths[1]));
                            Vec2d splitPoint = {};
                            if (cs.t0 >= 0) {
                                detail::EdgeJoinPatchSample& ps =
                                    patch.sideSamples[0].emplaceLast();
                                ps.sidePoint = cs.pos0;
                                ps.sideSTV = stv0;
                                if (cs.t1 < 0) {
                                    splitPoint = capOrigin - cs.s * capDir;
                                    ps.centerPoint = splitPoint;
                                    ps.centerSTV = Vec3f(s, 0.f, 0.f);
                                }
                                else {
                                    if (cs.t1 == 0) {
                                        splitPoint = cs.pos1;
                                    }
                                    ps.centerPoint = cs.pos1;
                                    ps.centerSTV = stv1;
                                }
                            }
                            if (cs.t1 <= 0) {
                                detail::EdgeJoinPatchSample& ps =
                                    patch.sideSamples[1].emplaceLast();
                                ps.sidePoint = cs.pos1;
                                ps.sideSTV = stv1;
                                if (cs.t0 > 0) {
                                    // splitPoint is already computed.
                                    ps.centerPoint = splitPoint;
                                    ps.centerSTV = Vec3f(s, 0.f, 0.f);
                                }
                                else {
                                    ps.centerPoint = cs.pos0;
                                    ps.centerSTV = stv0;
                                }
                            }
                        };

                    if (isSortedInReverseOrder) {
                        auto it = crossSections.rbegin();
                        for (; it != crossSections.rend(); ++it) {
                            const CrossSection& cs = *it;
                            appendPatchSamplesFromCrossSection(cs);
                        }
                    }
                    else {
                        for (const CrossSection& cs : crossSections) {
                            appendPatchSamplesFromCrossSection(cs);
                        }
                    }
                }

                // add patch-samples at join
                {
                    detail::EdgeJoinPatchSample& ps = patch.sideSamples[0].emplaceLast();
                    ps.centerPoint = capOrigin;
                    ps.sidePoint = sp0;
                    ps.sideSTV[1] = static_cast<float>(halfwidths[0]);
                    ps.sideSTV[2] = 1.f;
                }
                {
                    detail::EdgeJoinPatchSample& ps = patch.sideSamples[1].emplaceLast();
                    ps.centerPoint = capOrigin;
                    ps.sidePoint = sp1;
                    ps.sideSTV[1] = static_cast<float>(halfwidths[1]);
                    ps.sideSTV[2] = 1.f;
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
                * (std::max)(heData.patchCutLimits_[0], heData.patchCutLimits_[1]);
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
                    else {
                        workingSamples.emplaceLast(sample);
                        previousSqDist = sqDist;
                    }
                }
            }
            else { // isReverse
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
                    else {
                        workingSamples.emplaceLast(
                            sample.position(),
                            -sample.normal(),
                            geometry::Vec2d(sample.halfwidth(1), sample.halfwidth(0)),
                            s);
                        previousSqDist = sqDist;
                    }
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

            detail::BoxModelBorder borderA(
                vertexPosition
                    + halfedgeDataA->outgoingTangent_.orthogonalized()
                          * halfedgeDataA->halfwidths_[0],
                halfedgeDataA->outgoingTangent_);
            sidePatchDataA0.border = borderA;

            detail::BoxModelBorder borderB(
                vertexPosition
                    - halfedgeDataB->outgoingTangent_.orthogonalized()
                          * halfedgeDataB->halfwidths_[1],
                halfedgeDataB->outgoingTangent_);
            sidePatchDataB1.border = borderB;

            sidePatchDataA0.clear();
            sidePatchDataB1.clear();

            sidePatchDataA0.joinHalfwidth = halfedgeDataA->halfwidths_[0];
            sidePatchDataB1.joinHalfwidth = halfedgeDataB->halfwidths_[1];

            std::optional<Vec2d> isect =
                borderA.computeIntersectionParametersWith(borderB);
            if (isect.has_value()) {
                Vec2d ts = isect.value();
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
                        Vec2d farCorner = borderA.origin;
                        detail::BoxModelBorder splitBorder(
                            vertexPosition, (farCorner - vertexPosition).normalized());
                        Vec2d split =
                            splitBorder.computeIntersectionParametersWith(borderB)
                                .value_or(Vec2d());

                        sidePatchDataA0.joinHalfwidth = split[0];
                        sidePatchDataA0.filletLength =
                            halfedgeDataA->patchLength_ / interpolationLimitCoefficient;
                        if (ts[0] > 0 && ts[0] < sidePatchDataA0.filletLength) {
                            sidePatchDataA0.filletLength = ts[0];
                        }

                        double tB = split[1];
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
                        Vec2d farCorner = borderB.origin;
                        detail::BoxModelBorder splitRay(
                            vertexPosition, (farCorner - vertexPosition).normalized());
                        Vec2d split =
                            splitRay.computeIntersectionParametersWith(borderA).value_or(
                                Vec2d());

                        sidePatchDataB1.joinHalfwidth = split[0];
                        sidePatchDataB1.filletLength =
                            halfedgeDataB->patchLength_ / interpolationLimitCoefficient;
                        if (ts[1] > 0 && ts[1] < sidePatchDataB1.filletLength) {
                            sidePatchDataB1.filletLength = ts[1];
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

            detail::BoxModelBorder centerBorder(
                vertexPosition, halfedgeData.outgoingTangent_);
            Vec2d centerBorderNormal = halfedgeData.outgoingTangent_.orthogonalized();

            const double tFilletMax = maxFilletLength / halfedgeData.patchLength_;
            const double sMax = workingSamples.last().s();
            const double sFilletMax = tFilletMax * sMax;

            // straighten samples
            if (sMax > 0) {
                auto it = workingSamples.begin();
                auto previousIt = it;
                for (; it != workingSamples.end(); previousIt = it++) {
                    const double s = it->s();
                    if (s > sFilletMax) {
                        double previousS = previousIt->s();
                        double d = (sFilletMax / sMax) * halfedgeData.patchLength_;
                        const double t = (sFilletMax - previousS) / (s - previousS);
                        const double ot = 1 - t;
                        geometry::CurveSample newSample(
                            centerBorder.pointAt(d),
                            centerBorderNormal,
                            previousIt->halfwidths() * ot + it->halfwidths() * t,
                            sFilletMax);
                        it = workingSamples.emplace(it, newSample);
                        previousIt = it++;
                        break;
                    }
                    double ts = s / sMax;
                    double d = ts * halfedgeData.patchLength_;
                    it->setPosition(centerBorder.pointAt(d));
                    it->setNormal(centerBorderNormal);
                    geometry::Vec2f hwf(it->halfwidths());
                    if (s == sFilletMax) {
                        previousIt = it++;
                        break;
                    }
                }
                // lerp samples
                const double sInterp = sMax - sFilletMax;
                if (sInterp > 0) {
                    for (; it != workingSamples.end(); previousIt = it++) {
                        double s = it->s();
                        double d = (s / sMax) * halfedgeData.patchLength_;
                        const double t = (s - sFilletMax) / sInterp;
                        const double ot = 1 - t;
                        Vec2d rayPoint = centerBorder.pointAt(d);
                        it->setPosition(rayPoint * ot + it->position() * t);
                        it->setNormal(
                            (centerBorderNormal * ot + it->normal() * t).normalized());
                    }
                }
            }

            for (Int i = 0; i < 2; ++i) {
                core::Array<detail::EdgeJoinPatchSample> patchSamples;
                const auto& sidePatchData = halfedgeData.sidePatchData_[i];
                //const auto& otherSidePatchData = halfedgeData.sidePatchData_[1 - i];
                const double halfwidth = halfedgeData.halfwidths_[i];

                // extension
                if (sidePatchData.extLength > 0) {
                    // todo: miter limit
                    float sf = static_cast<float>(-sidePatchData.extLength);
                    float hwf = static_cast<float>(halfwidth);
                    auto& p = patchSamples.emplaceLast();
                    p.centerPoint = vertexPosition;
                    p.centerSTV = geometry::Vec3f(sf, 0, 0);
                    p.sidePoint = sidePatchData.border.pointAt(sf);
                    p.sideSTV = geometry::Vec3f(sf, hwf, 1.f);
                }

                double newS = 0;

                const double tFillet =
                    sidePatchData.filletLength / halfedgeData.patchLength_;
                const double sFillet = tFillet * sMax;

                //const double tFillet2 =
                //    otherSidePatchData.filletLength / halfedgeData.patchLength_;
                //const double sFillet2 = tFillet2 * sMax;

                const int normalMultiplier = i ? -1 : 1;

                auto it = workingSamples.begin();
                auto previousIt = it;
                if constexpr (0) { // debug
                    for (; it != workingSamples.end(); previousIt = it++) {
                        float sf = static_cast<float>(it->s());
                        auto& p = patchSamples.emplaceLast();
                        p.centerPoint = it->position();
                        double hw = it->halfwidth(i);
                        float hwf = static_cast<float>(hw);
                        p.sidePoint =
                            p.centerPoint + normalMultiplier * hw * it->normal();
                        p.sideSTV = geometry::Vec3f(sf, hwf, 1.f);
                        p.centerSTV = geometry::Vec3f(sf, 0, 0);
                    }
                }
                else if (sidePatchData.isCutFillet) {
                    if (sFillet > 0) {
                        // lerp halfwidths from join halfwidth to cut halfwidth
                        for (; it != workingSamples.end(); previousIt = it++) {
                            double s = it->s();
                            if (s > sFillet) {
                                double d = (sFillet / sMax) * halfedgeData.patchLength_;
                                auto& p = patchSamples.emplaceLast();
                                Vec2d pos = centerBorder.pointAt(d);
                                newS += (pos - previousIt->position()).length();
                                float newSf = static_cast<float>(newS);
                                p.centerPoint = centerBorder.pointAt(d);
                                p.sidePoint = sidePatchData.border.pointAt(d);
                                p.sideSTV = geometry::Vec3f(
                                    newSf, static_cast<float>(halfwidth), 1.f);
                                p.centerSTV = geometry::Vec3f(newSf, 0, 0);
                                break;
                            }
                            Vec2d pos = it->position();
                            newS += (pos - previousIt->position()).length();
                            float newSf = static_cast<float>(newS);
                            const double t = s / sFillet;
                            const double ot = 1 - t;
                            auto& p = patchSamples.emplaceLast();
                            p.centerPoint = pos;
                            double hw = sidePatchData.joinHalfwidth * ot + halfwidth * t;
                            float hwf = static_cast<float>(hw);
                            p.sidePoint = p.centerPoint
                                          + normalMultiplier * hw * centerBorderNormal;
                            p.sideSTV = geometry::Vec3f(newSf, hwf, 1.f);
                            p.centerSTV = geometry::Vec3f(newSf, 0, 0);
                            if (s == sFillet) {
                                previousIt = it++;
                                break;
                            }
                        }
                    }
                    // lerp halfwidths from cut halfwidth to original halfwidth
                    //Int fixIndex = 0;
                    const double sInterp2 = sMax - sFillet;
                    for (; it != workingSamples.end(); previousIt = it++) {
                        Vec2d pos = it->position();
                        double s = it->s();
                        newS += (pos - previousIt->position()).length();
                        float newSf = static_cast<float>(newS);
                        //const double d = (s / sMax) * halfedgeData.patchLength_;
                        const double t = (s - sFillet) / sInterp2;
                        const double ot = 1 - t;
                        // temporary fix
                        //if (s == sFillet2 && sFillet2 > 0 && sFillet2 != sFillet) {
                        //    fixIndex = patchSamples.size();
                        //    auto& p = patchSamples.emplaceLast(patchSamples.last());
                        //    p.centerPoint = pos;
                        //    p.centerSU = geometry::Vec2f(s, s);
                        //    continue;
                        //}
                        auto& p = patchSamples.emplaceLast();
                        p.centerPoint = pos;
                        double hw = halfwidth * ot + it->halfwidth(i) * t;
                        float hwf = static_cast<float>(hw);
                        p.sidePoint =
                            p.centerPoint + normalMultiplier * hw * it->normal();
                        p.sideSTV = geometry::Vec3f(newSf, hwf, 1.f);
                        p.centerSTV = geometry::Vec3f(newSf, 0, 0);
                    }
                }
                else {
                    // lerp halfwidths from box halfwidth to original halfwidth
                    // additional lerp from join halfwidth to computed halfwidth between 0 and sFillet
                    //Int fixIndex = 0;
                    for (; it != workingSamples.end(); previousIt = it++) {
                        Vec2d pos = it->position();
                        double s = it->s();
                        newS += (pos - previousIt->position()).length();
                        float newSf = static_cast<float>(newS);
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
                        p.centerPoint = pos;
                        double hw = halfwidth * ot + it->halfwidth(i) * t;
                        if (sFillet > 0 && s < sFillet) {
                            // this works but looks like magic written this way
                            const double t1 = sFillet / sMax;
                            const double ot1 = 1 - t1;
                            double mhw = halfwidth * ot1 + it->halfwidth(i) * t1;

                            const double t2 = s / sFillet;
                            const double ot2 = 1 - t2;
                            hw = sidePatchData.joinHalfwidth * ot2 + mhw * t2;
                        }
                        float hwf = static_cast<float>(hw);
                        p.sidePoint =
                            p.centerPoint + normalMultiplier * hw * it->normal();
                        p.sideSTV = geometry::Vec3f(newSf, hwf, 1.f);
                        p.centerSTV = geometry::Vec3f(newSf, 0, 0);
                    }
                }

                // fill data
                bool isReverse = halfedgeData.isReverse();
                detail::EdgeJoinPatch& patch =
                    halfedgeData.edgeData_->patches_[isReverse ? 1 : 0];

                patch.extensionS =
                    (std::max)(patch.extensionS, -patchSamples.first().centerSTV[0]);
                patch.extensionS =
                    (std::max)(patch.extensionS, -patchSamples.first().sideSTV[0]);
                patch.overrideS =
                    (std::max)(patch.overrideS, patchSamples.last().centerSTV[0]);

                patch.sideSamples[i] = std::move(patchSamples);
            }
        }
    }

    data.isJoinComputed_ = true;
    data.isComputing_ = false;
}

void VacKeyVertex::dirtyPosition_() {
    if (frameData_.isPositionComputed_) {
        frameData_.isPositionComputed_ = false;
        frameData_.isSelectionGeometryDirty_ = true;
        notifyChangesToDependents(ChangeFlag::VertexPosition);
    }
}

void VacKeyVertex::dirtyJoin_() {
    // join data is stored in edges
}

void VacKeyVertex::onUpdateError_() {
    removeVacNode();
    dirtyPosition_();
}

} // namespace vgc::workspace
