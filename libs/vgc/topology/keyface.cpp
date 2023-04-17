// Copyright 2023 The VGC Developers
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

#include <vgc/topology/keyface.h>
#include <vgc/topology/vac.h>

#include <unordered_set>

#include <vgc/geometry/curves2d.h>

namespace vgc::topology {

namespace {

struct KeyHalfedgeCandidate {
    KeyHalfedgeCandidate(
        KeyEdge* edge,
        bool direction,
        double distance,
        float angleScore,
        bool isBackFacing) noexcept

        : halfedge(edge, direction)
        , distance(distance)
        , angleScore(angleScore)
        , isBackFacing(isBackFacing) {
    }

    KeyHalfedgeCandidate(const KeyHalfedge& he)
        : halfedge(he) {
    }

    KeyHalfedge halfedge;
    double distance = 0;
    float angleScore = 0;
    bool isBackFacing = false;
    Int32 windingContribution = 0;
    bool hasComputedWindingContribution = false;

    bool operator==(const KeyHalfedgeCandidate& b) const {
        return halfedge == b.halfedge;
    }

    bool operator==(const KeyHalfedge& b) const {
        return halfedge == b;
    }
};

struct KeyHalfedgeCandidateHash {
    size_t operator()(const KeyHalfedgeCandidate& c) const noexcept {
        return std::hash<vgc::topology::KeyHalfedge>{}(c.halfedge);
    }
};

struct KeyHalfedgeCandidateCompare {
    bool operator()(const KeyHalfedgeCandidate& a, const KeyHalfedgeCandidate& b) {
        if (a.isBackFacing != b.isBackFacing) {
            return b.isBackFacing;
        }
        else if (a.distance != b.distance) {
            return a.distance < b.distance;
        }
        else {
            return a.angleScore > b.angleScore;
        }
    }
};

/// Returns the contribution to the winding number at the given `point` of
/// the given `keyHalfedge`.
///
/// The sum of the results of this function for all halfedges of a cycle is
/// the winding number.
///
Int32 computeWindingContribution(
    const KeyHalfedge& keyHalfedge,
    const geometry::Vec2d& point) {

    const KeyEdge* ke = keyHalfedge.edge();
    const geometry::CurveSampleArray& samples = ke->sampling().samples();
    const geometry::Rect2d& bbox = ke->samplingBoundingBox();

    // The winding number of a closed curve C (cycle) at point P = (Px, Py)
    // is the total number of times C travels positively around P.
    //
    // If Y points down and X points right, positive means clockwise.
    //
    // Example: the winding number of the trigonometric circle defined as
    // C(t) = (cos(t), sin(t)), for t from 0 to 1, is 1 for any point inside,
    // and 0 for points outside.
    //
    // It can be computed by looking for all intersections of C with a
    // half line L starting at P. When C crosses L positively as seen from P
    // we count +1, when it crosses L the other way we count -1.
    //
    // We use an axis-aligned half-line that simplifies the computation of
    // intersections.
    //
    // In particular, for a vertical half-line L defined as
    // {(x, y): x = Px, y >= Py}:
    //
    // - If the bounding box of a halfedge H is strictly on one side of P in X,
    // or below P (box.maxY() <= Py), then it cannot cross the half-line and
    // does not participate in the winding number (apart from being part of the
    // cycle) whatever the shape of the halfedge H inside that box.
    //
    // - If the bounding box of a halfedge H is above P (box.minY() >= Py),
    // then we know that the winding number would remain the same whatever the
    // shape of the interior of the curve inside that box. We can thus use
    // the straight line between its endpoints to compute the crossing with L.
    // In fact, only the X component of its endpoints is relevant.
    //
    // - Otherwise, we have to intersect the halfedge curve with the half-line
    // the usual way.
    //
    // To not miss a crossing nor count one twice, care must be taken about
    // samples exactly on H. To deal with this problem we consider Hx to be
    // between Px's floating point value and the next (core::nextafter(Px)).
    //
    // As future optimization, curves could implement an acceleration structure
    // for intersections in the form of a tree of sub-spans and bounding boxes
    // that we could benefit from. Such sub-spans would follow the same rules
    // as halfedges (listed above).

    double px = point.x();
    double py = point.y();

    if (bbox.xMax() + core::epsilon < px) {
        return 0;
    }

    if (bbox.xMin() - core::epsilon > px) {
        return 0;
    }

    if (bbox.yMax() < py) {
        return 0;
    }

    // from here, bbox overlaps the half-line

    Int32 contribution = 0;

    if (bbox.yMin() > py) {
        // bbox overlaps the half-line but does not contain P.
        // => only X component of end points matters.

        // Note: We assume first and last sample positions to exactly match the
        // positions of start and end vertices.
        geometry::Vec2d p1 = samples.first().position();
        geometry::Vec2d p2 = samples.last().position();
        double x1 = p1.x();
        double x2 = p2.x();

        // winding contribution:
        // x1 <= px && x2 > px: -1
        // x2 <= px && x1 > px: +1
        // otherwise: 0
        bool x1Side = x1 <= px;
        bool x2Side = x2 <= px;
        if (x1Side == x2Side) {
            return 0;
        }
        else {
            //       P┌────────→ x
            //  true  │  false
            //  Side  H  Side
            //        │
            //        │    winding
            //        │    contribution:
            //      ──│─→    -1
            //      ←─│──    +1
            //        │
            //        y
            //
            contribution = 1 - 2 * x1Side;
        }
    }
    else {
        // bbox contains P.
        // => compute intersections with curve.

        // Note: We assume first and last sample positions to exactly match the
        // positions of start and end vertices.
        auto it = samples.begin();
        geometry::Vec2d pi = it->position();
        geometry::Vec2d pj = {};
        bool xiSide = (pi.x() <= px);
        bool xjSide = {};
        for (++it; it != samples.end(); ++it) {
            pj = it->position();
            xjSide = (pj.x() <= px);
            if (xiSide != xjSide) {
                bool yiSide = pi.y() < py;
                bool yjSide = pj.y() < py;
                if (yiSide == yjSide) {
                    if (!yiSide) {
                        contribution += (1 - 2 * xiSide);
                    }
                }
                else {
                    const geometry::Vec2d segment = pj - pi;
                    double delta = -segment.x();
                    geometry::Vec2d ppi = pi - point;
                    double inv_delta = 1 / delta;
                    double yIntersect = ppi.det(segment) * inv_delta;
                    if (yIntersect > py) {
                        contribution += (1 - 2 * xiSide);
                    }
                }
            }

            pi = pj;
            xiSide = xjSide;
        }
    }

    return keyHalfedge.direction() ? contribution : -contribution;
}

} // namespace

namespace detail {

// todo: return the face mesh too
core::Array<KeyCycle> computeKeyFaceCandidateAt(
    geometry::Vec2d position,
    vacomplex::Group* group,
    core::FloatArray& trianglesBuffer,
    geometry::WindingRule windingRule,
    core::AnimTime t) {

    using geometry::Vec2d;

    core::Array<KeyCycle> result = {};

    // From here, we try to find a list of cycles such that
    // the corresponding face would intersect with the cursor.

    std::unordered_set<KeyHalfedgeCandidate, KeyHalfedgeCandidateHash>
        cycleHalfedgeCandidates;

    // Compute candidate halfedges existing at time t, and child of given group.
    for (vacomplex::Node* child = group->firstChild(); child;
         child = child->nextSibling()) {

        vacomplex::Cell* cell = child->toCell();
        if (!cell) {
            continue;
        }
        vacomplex::KeyEdge* ke = cell->toKeyEdge();
        if (ke && ke->existsAt(t)) {
            const geometry::CurveSampleArray& sampling = ke->sampling().samples();
            geometry::DistanceToCurve d = geometry::distanceToCurve(sampling, position);

            constexpr double hpi = core::pi / 2;
            double a = d.angleFromTangent();
            float angleScore = static_cast<float>(hpi - std::abs(hpi - std::abs(a)));
            if (a < 0) {
                angleScore = -angleScore;
            }

            bool isEdgeBackFacing = a < 0;
            cycleHalfedgeCandidates.emplace(
                ke, true, d.distance(), angleScore, isEdgeBackFacing);
            cycleHalfedgeCandidates.emplace(
                ke, false, d.distance(), -angleScore, !isEdgeBackFacing);
        }
    }

    // First, we try to create such a face assuming that the
    // VGC is actually planar (cells are not overlapping).
    {
        // Find external boundary: the closest planar cycle containing mouse cursor.

        std::unordered_set<KeyHalfedgeCandidate, KeyHalfedgeCandidateHash>
            planarCycleHalfedgeCandidates = cycleHalfedgeCandidates;
        core::Array<KeyHalfedgeCandidate> planarCycleCandidate;
        double maxKeyHalfedgeCandidateDistance = core::DoubleInfinity;

        auto findNextPlanarCycleCandidate = [&]() -> bool {
            planarCycleCandidate.clear();
            while (!planarCycleHalfedgeCandidates.empty()) {
                // Find closest potential edge.
                auto it = std::min_element(
                    planarCycleHalfedgeCandidates.begin(),
                    planarCycleHalfedgeCandidates.end(),
                    KeyHalfedgeCandidateCompare());
                KeyHalfedgeCandidate keyHalfedgeCandidate = *it;

                if (keyHalfedgeCandidate.distance > maxKeyHalfedgeCandidateDistance) {
                    planarCycleHalfedgeCandidates.clear();
                    break;
                }

                planarCycleCandidate.append(keyHalfedgeCandidate);
                planarCycleHalfedgeCandidates.erase(it);

                // Find the corresponding planar map cycle if halfedge is open.
                if (!keyHalfedgeCandidate.halfedge.isClosed()) {

                    KeyHalfedge heFirst = keyHalfedgeCandidate.halfedge;
                    KeyHalfedge he = heFirst;
                    bool foundCycle = false;

                    Int maxIter = 2 * planarCycleHalfedgeCandidates.size() + 2;
                    for (Int i = 0; i < maxIter; ++i) {
                        // Find next halfedge in cycle.
                        he = he.next();
                        KeyHalfedge heStop = he;
                        auto heIt =
                            planarCycleHalfedgeCandidates.find(KeyHalfedgeCandidate(he));
                        // Iterate in ring until heFirst or a non-discarded
                        // candidate is found.
                        while (heIt == planarCycleHalfedgeCandidates.end()) {
                            if (he == heFirst) {
                                // Cycle completed: leave loop.
                                foundCycle = true;
                                break;
                            }
                            he = he.opposite().next();
                            if (he == heStop) {
                                // Exhausted ring. Dead end.
                                break;
                            }
                            heIt = planarCycleHalfedgeCandidates.find(
                                KeyHalfedgeCandidate(he));
                        }
                        if (foundCycle) {
                            // Cycle completed: leave loop.
                            break;
                        }
                        if (heIt == planarCycleHalfedgeCandidates.end()) {
                            // Dead end.
                            break;
                        }

                        // Insert and iterate.
                        planarCycleCandidate.append(*heIt);
                        planarCycleHalfedgeCandidates.erase(heIt);
                    }

                    if (!foundCycle) {
                        // Something bad happened. We should have found a cycle since
                        // (he.previous().next() == he).
                        planarCycleCandidate.clear();
                        continue;
                    }
                }

                // Cycle found, leave while loop.
                break;
            }
            return !planarCycleCandidate.isEmpty();
        };

        struct CycleWithWindingNumber {
            core::Array<KeyHalfedge> cycle;
            Int32 windingNumber;
        };

        core::Array<CycleWithWindingNumber> discardedCycles;
        CycleWithWindingNumber externalBoundaryCycle;

        while (findNextPlanarCycleCandidate()) {

            // Compute winding number to see if cursor is inside the cycle candidate.
            // Build the cycle in the same loop.

            core::Array<KeyHalfedge> cycle;
            Int32 windingNumber = 0;
            for (KeyHalfedgeCandidate& hec : planarCycleCandidate) {
                Int32 contribution = hec.windingContribution;
                if (!hec.hasComputedWindingContribution) {
                    contribution = computeWindingContribution(hec.halfedge, position);
                    hec.windingContribution = contribution;
                }
                windingNumber += contribution;
                cycle.emplaceLast(hec.halfedge);
            }

            if (geometry::isWindingNumberSatisfyingRule(windingNumber, windingRule)) {
                externalBoundaryCycle.cycle = std::move(cycle);
                externalBoundaryCycle.windingNumber = windingNumber;
                break;
            }

            auto& discardedCycle = discardedCycles.emplaceLast();
            discardedCycle.cycle = std::move(cycle);
            discardedCycle.windingNumber = windingNumber;
        }

        //struct PreviewKeyFace {};
        //PreviewKeyFace externalBoundary;

        // Find holes for this cycle, from closest to farthest.
        // Each new hole must lie ~50% inside the external boundary as well
        // as the face triangulation with current holes.

        // TODO: find holes using discarded cycles then new cycles
        // maybe compute max dist to P in current face to stop looking for new holes when the next closest
        // candidate edge is too far away..

        // We left the while loop, so either we found an external boundary, or there's no hope to find one
        //if (foundExternalBoundary) {
        //    // Great, so we know we have a valid planar face!
        //    // Plus, it's already stored as a PreviewFace :)
        //    *toBePaintedFace_ = externalBoundary;
        //    foundPlanarFace = true;
        //
        //    // Now, let's try to add holes to the external boundary
        //    QSet<KeyEdge*> potentialHoleEdges;
        //    foreach (KeyEdge* e, instantEdges()) {
        //        if (e->exists(time))
        //            potentialHoleEdges.insert(e);
        //    }
        //    CellSet cellsInExternalBoundary = externalBoundary.cycles()[0].cells();
        //    KeyEdgeSet edgesInExternalBoundary = cellsInExternalBoundary;
        //    foreach (KeyEdge* e, edgesInExternalBoundary)
        //        potentialHoleEdges.remove(e);
        //    QList<PreviewKeyFace> holes;
        //    while (!potentialHoleEdges.isEmpty()) {
        //        // Ordered by distance to mouse cursor p, add planar cycles gamma which:
        //        //   - Do not contain p
        //        //   - Are contained in external boundary
        //        //   - Are not contained in holes already added
        //        addHoleToPaintedFace(
        //            potentialHoleEdges, *toBePaintedFace_, distancesToEdges, x, y);
        //    }
        //}

        result.emplaceLast(KeyCycle(std::move(externalBoundaryCycle.cycle)));
        computeKeyFaceFillTriangles(result, trianglesBuffer, windingRule);

    } // planar face search

    //if (foundPlanarFace) {
    //    // Great, nothing to do! Everything has already been taken care of.
    //}
    //else {
    //    // TODO: try to find any valid face, even if it's not planar
    //}

    return result;
}

namespace {

bool computeKeyFaceFillTriangles(
    const core::Array<KeyCycle>& cycles,
    core::FloatArray& trianglesBuffer,
    const geometry::CurveSamplingParameters* parameters,
    geometry::WindingRule windingRule) {

    trianglesBuffer.clear();

    geometry::Curves2d curves2d;
    for (const vacomplex::KeyCycle& cycle : cycles) {
        KeyVertex* kv = cycle.steinerVertex();
        if (kv) {
            geometry::Vec2d p = kv->position();
            curves2d.moveTo(p[0], p[1]);
        }
        else {
            bool isFirst = true;
            for (const KeyHalfedge& khe : cycle.halfedges()) {
                const KeyEdge* ke = khe.edge();
                if (ke) {
                    const geometry::CurveSampleArray& samples =
                        parameters ? ke->computeSampling(*parameters)
                                   : ke->sampling().samples();
                    if (khe.direction()) {
                        for (const geometry::CurveSample& s : samples) {
                            geometry::Vec2d p = s.position();
                            if (isFirst) {
                                curves2d.moveTo(p[0], p[1]);
                                isFirst = false;
                            }
                            else {
                                curves2d.lineTo(p[0], p[1]);
                            }
                        }
                    }
                    else {
                        for (auto it = samples.rbegin(); it != samples.rend(); ++it) {
                            const geometry::CurveSample& s = *it;
                            geometry::Vec2d p = s.position();
                            if (isFirst) {
                                curves2d.moveTo(p[0], p[1]);
                                isFirst = false;
                            }
                            else {
                                curves2d.lineTo(p[0], p[1]);
                            }
                        }
                    }
                }
                else {
                    // TODO: log error
                    return false;
                }
            }
        }
        curves2d.close();
    }

    auto params = geometry::Curves2dSampleParams::adaptive();
    curves2d.fill(trianglesBuffer, params, windingRule);

    return true;
}

} // namespace

bool computeKeyFaceFillTriangles(
    const core::Array<KeyCycle>& cycles,
    core::FloatArray& trianglesBuffer,
    geometry::WindingRule windingRule) {

    return computeKeyFaceFillTriangles(cycles, trianglesBuffer, nullptr, windingRule);
}

bool computeKeyFaceFillTriangles(
    const core::Array<KeyCycle>& cycles,
    core::FloatArray& trianglesBuffer,
    const geometry::CurveSamplingParameters& parameters,
    geometry::WindingRule windingRule) {

    return computeKeyFaceFillTriangles(cycles, trianglesBuffer, &parameters, windingRule);
}

} // namespace detail

} // namespace vgc::topology
