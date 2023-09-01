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

#include <vgc/vacomplex/keyface.h>

#include <unordered_set>

#include <vgc/geometry/tesselator.h>
#include <vgc/vacomplex/complex.h>

namespace vgc::vacomplex {

namespace {

// Candidate first halfedge in the cycle
// discovery algorithms of paint bucket.
//
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

    bool operator==(const KeyHalfedgeCandidate& b) const {
        return halfedge == b.halfedge;
    }

    bool operator==(const KeyHalfedge& b) const {
        return halfedge == b;
    }
};

struct KeyHalfedgeCandidateHash {
    size_t operator()(const KeyHalfedgeCandidate& c) const noexcept {
        return std::hash<KeyHalfedge>{}(c.halfedge);
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
Int computeWindingContribution(
    const KeyHalfedge& keyHalfedge,
    const geometry::Vec2d& point) {

    const KeyEdge* ke = keyHalfedge.edge();

    const geometry::StrokeSampling2d& sampling = ke->strokeSampling();
    const geometry::StrokeSample2dArray& samples = sampling.samples();
    const geometry::Rect2d& bbox = sampling.centerlineBoundingBox();

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

    Int contribution = 0;

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
        geometry::Vec2d p1 = it->position();
        geometry::Vec2d p2 = {};
        bool x1Side = (p1.x() <= px);
        bool x2Side = {};
        for (++it; it != samples.end(); ++it) {
            p2 = it->position();
            x2Side = (p2.x() <= px);
            if (x1Side != x2Side) {
                bool y1Side = p1.y() < py;
                bool y2Side = p2.y() < py;
                if (y1Side == y2Side) {
                    if (!y1Side) {
                        contribution += (1 - 2 * x1Side);
                    }
                }
                else {
                    geometry::Vec2d p1p2 = p2 - p1;
                    geometry::Vec2d pp1 = p1 - point;
                    double t = pp1.det(p1p2);
                    bool xDir = std::signbit(p1p2.x());
                    bool tSign = std::signbit(t);
                    // If p lies exactly on p1p2, the result does not matter,
                    // thus we do not test for (t == 0).
                    if (tSign != xDir) {
                        contribution += (1 - 2 * x1Side);
                    }
                }
            }
            p1 = p2;
            x1Side = x2Side;
        }
    }

    return keyHalfedge.direction() ? contribution : -contribution;
}

Int computeWindingNumber(
    const core::Array<KeyHalfedge>& cycle,
    const geometry::Vec2d& point) {

    Int result = 0;
    for (const KeyHalfedge& keyHalfedge : cycle) {
        Int contribution = computeWindingContribution(keyHalfedge, point);
        result += contribution;
    }
    return result;
}

void samplePointsOnCycleUniformly(
    const core::Array<KeyHalfedge>& cycle,
    Int numSamples,
    core::Array<geometry::Vec2d>& outPoints) {

    double totalS = 0;
    for (const KeyHalfedge& khe : cycle) {
        totalS += khe.edge()->strokeSampling().samples().last().s();
    }
    double stepS = totalS / numSamples;
    double nextStepS = stepS / 2;
    for (const KeyHalfedge& khe : cycle) {
        const geometry::StrokeSample2dArray& samples = khe.edge()->strokeSampling().samples();
        double heS = samples.last().s();
        if (heS == 0) {
            // XXX TODO: handle cases where heS == 0 more appropriately.
            // For example, ensure that this function still returns at least one sample.
            continue;
        }
        if (khe.direction()) {
            auto it = samples.begin();
            geometry::Vec2d previousP = it->position();
            double previousS = 0;
            ++it;
            for (auto end = samples.end(); it != end; ++it) {
                geometry::Vec2d p = it->position();
                double nextS = it->s();
                while (nextS >= nextStepS) {
                    double t = (nextStepS - previousS) / (nextS - previousS);
                    double ot = (1 - t);
                    previousP = ot * previousP + t * p;
                    previousS = nextStepS;
                    nextStepS += stepS;
                    outPoints.emplaceLast(previousP);
                }
                previousP = p;
                previousS = nextS;
            }
        }
        else {
            auto rit = samples.rbegin();
            geometry::Vec2d previousP = rit->position();
            double previousS = heS - rit->s();
            ++rit;
            for (auto rend = samples.rend(); rit != rend; ++rit) {
                geometry::Vec2d p = rit->position();
                double nextS = heS - rit->s();
                while (nextS >= nextStepS) {
                    double t = (nextStepS - previousS) / (nextS - previousS);
                    double ot = (1 - t);
                    previousP = ot * previousP + t * p;
                    previousS = nextStepS;
                    nextStepS += stepS;
                    outPoints.emplaceLast(previousP);
                }
                previousP = p;
                previousS = nextS;
            }
        }
        nextStepS -= heS;
    }
}

} // namespace

namespace detail {

core::Array<KeyCycle> computeKeyFaceCandidateAt(
    geometry::Vec2d position,
    Group* group,
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
    for (Node* child = group->firstChild(); child; child = child->nextSibling()) {

        Cell* cell = child->toCell();
        if (!cell) {
            continue;
        }
        KeyEdge* ke = cell->toKeyEdge();
        if (ke && ke->existsAt(t)) {
            const geometry::StrokeSample2dArray& samples = ke->strokeSampling().samples();
            geometry::DistanceToCurve d =
                geometry::distanceToCurve(samples, position);

            constexpr double hpi = core::pi / 2;
            double a = d.angleFromTangent();
            float angleScore = static_cast<float>(hpi - std::abs(hpi - std::abs(a)));
            bool isEdgeBackFacing = false;
            if (a < 0) {
                angleScore = -angleScore;
                isEdgeBackFacing = true;
            }

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
        core::Array<KeyHalfedge> planarCycleCandidate;
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

                planarCycleCandidate.append(keyHalfedgeCandidate.halfedge);
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
                        planarCycleCandidate.append(heIt->halfedge);
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
            Int windingNumber;
        };

        core::Array<CycleWithWindingNumber> discardedCycles;
        CycleWithWindingNumber externalBoundaryCycle;

        while (findNextPlanarCycleCandidate()) {

            // Compute winding number to see if cursor is inside the cycle candidate.
            // Build the cycle in the same loop.

            Int windingNumber = computeWindingNumber(planarCycleCandidate, position);

            if (geometry::isWindingNumberSatisfyingRule(windingNumber, windingRule)) {
                externalBoundaryCycle.cycle.swap(planarCycleCandidate);
                externalBoundaryCycle.windingNumber = windingNumber;
                break;
            }

            auto& discardedCycle = discardedCycles.emplaceLast();
            discardedCycle.cycle.swap(planarCycleCandidate);
            discardedCycle.windingNumber = windingNumber;
        }

        if (!externalBoundaryCycle.cycle.isEmpty()) {

            // Find holes for this cycle, from closest to farthest.
            // Each new hole must lie ~50% inside the external boundary as well
            // as the face triangulation with current holes.

            Int totalWindingNumber = externalBoundaryCycle.windingNumber;
            core::Array<CycleWithWindingNumber> holeCycles; // accepted holes

            core::Array<geometry::Vec2d> holeCycleCandidatePoints;
            auto isValidHoleCycle =
                [&](const CycleWithWindingNumber& holeCycleCandidate) {
                    Int newWindingNumber =
                        totalWindingNumber + holeCycleCandidate.windingNumber;
                    if (!geometry::isWindingNumberSatisfyingRule(
                            newWindingNumber, windingRule)) {
                        return false;
                    }

                    holeCycleCandidatePoints.clear();
                    Int numPoints = 20;
                    samplePointsOnCycleUniformly(
                        holeCycleCandidate.cycle, numPoints, holeCycleCandidatePoints);

                    Int successes = 0;
                    Int fails = 0;
                    for (const geometry::Vec2d& point : holeCycleCandidatePoints) {
                        bool success = false;
                        Int windingNumber =
                            computeWindingNumber(externalBoundaryCycle.cycle, point);
                        if (geometry::isWindingNumberSatisfyingRule(
                                windingNumber, windingRule)) {
                            for (const CycleWithWindingNumber& holeCycle : holeCycles) {
                                windingNumber +=
                                    computeWindingNumber(holeCycle.cycle, point);
                            }
                            if (geometry::isWindingNumberSatisfyingRule(
                                    windingNumber, windingRule)) {
                                success = true;
                            }
                        }
                        if (success) {
                            ++successes;
                            if (static_cast<double>(successes) / numPoints >= 0.5) {
                                return true;
                            }
                        }
                        else {
                            ++fails;
                            if (static_cast<double>(fails) / numPoints > 0.5) {
                                return false;
                            }
                        }
                    }

                    return false;
                };

            std::unordered_set<KeyEdge*> usedEdges; // needed to reuse discarded cycles

            // erase opposite halfedges of external boundary cycle
            for (const KeyHalfedge& khe : externalBoundaryCycle.cycle) {
                usedEdges.insert(khe.edge());
                planarCycleHalfedgeCandidates.erase(KeyHalfedgeCandidate(khe.opposite()));
            }

            // find holes in cycles discarded earlier
            for (const CycleWithWindingNumber& cycle : discardedCycles) {
                bool usesAlreadyUsedEdge = false;
                for (const KeyHalfedge& khe : cycle.cycle) {
                    if (usedEdges.find(khe.edge()) != usedEdges.end()) {
                        usesAlreadyUsedEdge = true;
                        break;
                    }
                }
                if (!usesAlreadyUsedEdge && isValidHoleCycle(cycle)) {
                    holeCycles.emplaceLast(cycle);
                    for (const KeyHalfedge& khe : cycle.cycle) {
                        usedEdges.insert(khe.edge());
                        planarCycleHalfedgeCandidates.erase(
                            KeyHalfedgeCandidate(khe.opposite()));
                    }
                }
            }

            // compute max distance from position to external boundary to limit search
            maxKeyHalfedgeCandidateDistance = 0;
            for (const KeyHalfedge& khe : externalBoundaryCycle.cycle) {
                const geometry::Rect2d& bbox = khe.edge()->centerlineBoundingBox();
                for (Int i = 0; i < 4; ++i) {
                    double d = (bbox.corner(i) - position).length();
                    if (d > maxKeyHalfedgeCandidateDistance) {
                        maxKeyHalfedgeCandidateDistance = d;
                    }
                }
            }

            // find holes in new cycles
            while (findNextPlanarCycleCandidate()) {
                Int windingNumber = computeWindingNumber(planarCycleCandidate, position);
                CycleWithWindingNumber cycle = {};
                cycle.cycle.swap(planarCycleCandidate);
                cycle.windingNumber = windingNumber;
                planarCycleCandidate = core::Array<KeyHalfedge>();
                if (isValidHoleCycle(cycle)) {
                    for (const KeyHalfedge& khe : cycle.cycle) {
                        planarCycleHalfedgeCandidates.erase(
                            KeyHalfedgeCandidate(khe.opposite()));
                    }
                    holeCycles.emplaceLast(std::move(cycle));
                }
            }

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
            for (CycleWithWindingNumber& holeCycle : holeCycles) {
                result.emplaceLast(KeyCycle(std::move(holeCycle.cycle)));
            }
            computeKeyFaceFillTriangles(
                result,
                trianglesBuffer,
                geometry::CurveSamplingQuality::Disabled,
                windingRule);
            return result;
        }
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
    const geometry::CurveSamplingQuality* quality,
    geometry::WindingRule windingRule) {

    trianglesBuffer.clear();

    geometry::Tesselator tess;
    core::Array<double> coords;
    for (const KeyCycle& cycle : cycles) {
        coords.clear();
        KeyVertex* kv = cycle.steinerVertex();
        if (kv) {
            geometry::Vec2d p = kv->position();
            coords.extend({p[0], p[1]});
        }
        else {
            for (const KeyHalfedge& khe : cycle.halfedges()) {
                const KeyEdge* ke = khe.edge();
                if (ke) {
                    // Note: ke->computeSampling(*quality) returns an
                    // EdgeSampling by value. The const ref extend its
                    // lifetime.
                    const geometry::StrokeSampling2d& sampling =
                        quality ? ke->computeStrokeSampling(*quality) : ke->strokeSampling();
                    const geometry::StrokeSample2dArray& samples = sampling.samples();
                    if (khe.direction()) {
                        for (const geometry::StrokeSample2d& s : samples) {
                            geometry::Vec2d p = s.position();
                            coords.extend({p[0], p[1]});
                        }
                    }
                    else {
                        for (auto it = samples.rbegin(); it != samples.rend(); ++it) {
                            const geometry::StrokeSample2d& s = *it;
                            geometry::Vec2d p = s.position();
                            coords.extend({p[0], p[1]});
                        }
                    }
                }
                else {
                    // TODO: log error
                    return false;
                }
            }
        }
        tess.addContour(coords);
    }

    tess.tesselate(trianglesBuffer, windingRule);

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
    geometry::CurveSamplingQuality quality,
    geometry::WindingRule windingRule) {

    return computeKeyFaceFillTriangles(cycles, trianglesBuffer, &quality, windingRule);
}

} // namespace detail

void KeyFace::substituteKeyVertex_(KeyVertex* oldVertex, KeyVertex* newVertex) {
    for (KeyCycle& cycle : cycles_) {
        if (cycle.steinerVertex_ == oldVertex) {
            cycle.steinerVertex_ = newVertex;
        }
    }
}

void KeyFace::substituteKeyHalfedge_(
    const class KeyHalfedge& oldHalfedge,
    const class KeyHalfedge& newHalfedge) {

    for (KeyCycle& cycle : cycles_) {
        if (cycle.steinerVertex_) {
            continue;
        }
        for (KeyHalfedge& khe : cycle.halfedges_) {
            if (khe.edge() == oldHalfedge.edge()) {
                bool direction = newHalfedge.direction();
                if (khe.direction() != oldHalfedge.direction()) {
                    direction = !direction;
                }
                khe = KeyHalfedge(newHalfedge.edge(), direction);
            }
        }
    }
}

void KeyFace::debugPrint_(core::StringWriter& out) {

    out << core::format( //
        "{:<12}",
        "KeyFace");

    out << " cycles=[";
    bool isFirst = true;
    for (const KeyCycle& cycle : cycles()) {
        if (isFirst) {
            isFirst = false;
        }
        else {
            out << ", ";
        }
        cycle.debugPrint(out);
    }
    out << ']';
}

} // namespace vgc::vacomplex
