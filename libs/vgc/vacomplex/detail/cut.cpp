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

#include <vgc/vacomplex/detail/operations.h>

#include <algorithm> // reverse, unique

#include <vgc/core/algorithms.h> // sort
#include <vgc/core/random.h>
#include <vgc/geometry/intersect.h>

namespace vgc::vacomplex::detail {

namespace {

struct IndexedCurveParameter {
    geometry::CurveParameter param;
    Int index;

    bool operator<(const IndexedCurveParameter& other) const {
        return param < other.param;
    }

    // Implicit conversion to CurveParameter
    operator geometry::CurveParameter() const {
        return param;
    }
};

core::Array<IndexedCurveParameter>
sortParameters(core::ConstSpan<geometry::CurveParameter> parameters) {
    Int index = 0;
    core::Array<IndexedCurveParameter> res(parameters, [&index](const auto& param) {
        return IndexedCurveParameter{param, index++};
    });
    core::sort(res);
    return res;
}

} // namespace

CutEdgeResult
Operations::cutEdge(KeyEdge* ke, core::ConstSpan<geometry::CurveParameter> parameters_) {

    Int n = parameters_.length();
    if (n == 0) {
        return CutEdgeResult({}, {});
    }

    KeyEdgeData& oldData = ke->data();
    const geometry::AbstractStroke2d* oldStroke = oldData.stroke();
    Group* parentGroup = ke->parentGroup();
    Node* nextSibling = ke->nextSibling();
    core::AnimTime time = ke->time();

    // Create a copy and sort parameters in increasing geometric order, while
    // preserving the info of their original index before sorting.
    //
    core::Array<IndexedCurveParameter> parameters = sortParameters(parameters_);

    // Create the new vertices and edges (geometry-sorted).
    //
    core::Array<KeyVertex*> newVertices;
    core::Array<KeyEdge*> newEdges;
    newVertices.reserve(n);
    if (ke->isClosed()) {

        // Create the KeyEdgeData
        core::Array<KeyEdgeData> newEdgesData;
        newEdgesData.reserve(n);
        geometry::CurveParameter p1 = parameters.last();
        bool areAllParametersEqual =
            (parameters.first().param == parameters.last().param);
        Int numWraps = areAllParametersEqual ? 1 : 0;
        for (geometry::CurveParameter p2 : parameters) {
            newEdgesData.append(KeyEdgeData::fromSlice(oldData, p1, p2, numWraps));
            p1 = p2;
            numWraps = 0;
        }

        // Create the new vertices
        for (const KeyEdgeData& data : newEdgesData) {
            geometry::Vec2d position = data.stroke()->endPosition();
            KeyVertex* v = createKeyVertex(position, parentGroup, nextSibling, time);
            newVertices.append(v);
        }

        // Create the new edges
        newEdges.reserve(n);
        KeyVertex* v1 = newVertices.last();
        for (Int i = 0; i < n; ++i) {
            KeyEdgeData& data = newEdgesData.getUnchecked(i);
            KeyVertex* v2 = newVertices.getUnchecked(i);
            KeyEdge* e = createKeyOpenEdge(v1, v2, std::move(data), parentGroup, ke);
            newEdges.append(e);
            v1 = v2;
        }
    }
    else {
        newEdges.reserve(n + 1);
        Int numWraps = 0;

        // Create the new vertices and the first n new edges
        geometry::CurveParameter p1 = oldStroke->startParameter();
        KeyVertex* v1 = ke->startVertex();
        for (geometry::CurveParameter p2 : parameters) {
            KeyEdgeData data = KeyEdgeData::fromSlice(oldData, p1, p2, numWraps);
            geometry::Vec2d position = data.stroke()->endPosition();
            KeyVertex* v2 = createKeyVertex(position, parentGroup, nextSibling, time);
            KeyEdge* e = createKeyOpenEdge(v1, v2, std::move(data), parentGroup, ke);
            newVertices.append(v2);
            newEdges.append(e);
            p1 = p2;
            v1 = v2;
        }

        // Create the last edge
        geometry::CurveParameter p2 = oldStroke->endParameter();
        KeyVertex* v2 = ke->endVertex();
        KeyEdgeData data = KeyEdgeData::fromSlice(oldData, p1, p2, numWraps);
        KeyEdge* e = createKeyOpenEdge(v1, v2, std::move(data), parentGroup, ke);
        newEdges.append(e);
    }

    // Express the sequence of new edges as a KeyPath and its reversed path.
    //
    KeyPath path(core::Array<KeyHalfedge>(newEdges, [](const auto& edge) { //
        return KeyHalfedge(edge, true);
    }));
    KeyPath reversedPath = path.reversed();

    // Substitute all usages of ke by the new edges in incident faces.
    // We need to take a copy of the star since it is modified during the iteration.
    //
    for (Cell* starCell : ke->star().copy()) {
        KeyFace* kf = starCell->toKeyFace();
        if (!kf) {
            continue;
        }
        for (KeyCycle& cycle : kf->cycles_) {
            if (cycle.steinerVertex()) {
                continue;
            }
            core::Array<KeyHalfedge>& halfedges = cycle.halfedges_;
            for (auto it = halfedges.begin(); it != halfedges.end(); ++it) {
                if (it->edge() == ke) {
                    bool direction = it->direction();
                    it = halfedges.erase(it);
                    if (direction) {
                        it = halfedges.insert(it, path.halfedges());
                    }
                    else {
                        it = halfedges.insert(it, reversedPath.halfedges());
                    }
                    // Make `it` point to the last inserted element rather than the first
                    it += newEdges.length() - 1;
                }
            }
            VGC_ASSERT(cycle.isValid());
        }

        removeFromBoundary_(kf, ke);
        addToBoundary_(kf, path);
    }

    // Delete old edge
    hardDelete(ke);

    // Apply permutation so that the output vertices are in the same order as
    // the input CurveParameters.
    //
    // Note: for simplicity, we do this by using a separate array. If we later
    // want to avoid the extra memory allocation, there exist more complex
    // algorithms that can do this in-place.
    //
    VGC_ASSERT(newVertices.length() == n);
    VGC_ASSERT(parameters.length() == n);
    core::Array<KeyVertex*> outputVertices(n);
    for (Int i = 0; i < n; ++i) {
        Int inputIndex = parameters[i].index;
        outputVertices[inputIndex] = newVertices[i];
    }

    return CutEdgeResult(std::move(outputVertices), std::move(newEdges));
}

namespace {

core::Array<KeyFaceVertexUsageIndex>
getVertexIndexCandidates(KeyFace* kf, KeyVertex* kv1) {
    core::Array<KeyFaceVertexUsageIndex> res;
    Int i = 0;
    for (const auto& cycle : kf->cycles()) {
        KeyVertex* sv = cycle.steinerVertex();
        if (sv) {
            if (sv == kv1) {
                res.emplaceLast(i, 0);
            }
        }
        else {
            Int j = 0;
            for (const auto& h : cycle.halfedges()) {
                KeyVertex* kv = h.startVertex();
                if (kv == kv1) {
                    res.emplaceLast(i, j);
                }
                ++j;
            }
        }
        ++i;
    }
    return res;
}

} // namespace

CutFaceResult Operations::cutGlueFace(
    KeyFace* kf,
    const KeyEdge* ke,
    OneCycleCutPolicy oneCycleCutPolicy,
    TwoCycleCutPolicy twoCycleCutPolicy) {

    if (!ke->isClosed()) {

        // Get candidate vertex-usages for the cut
        KeyVertex* startVertex = ke->startVertex();
        KeyVertex* endVertex = ke->endVertex();
        core::Array<KeyFaceVertexUsageIndex> startIndexCandidates =
            getVertexIndexCandidates(kf, startVertex);
        core::Array<KeyFaceVertexUsageIndex> endIndexCandidates =
            getVertexIndexCandidates(kf, endVertex);

        // If the start and/or end vertex is not already in the boundary of the
        // face, cut the face with the vertex. Note that this does not
        // invalidate pre-existing KeyFaceVertexUsageIndex, as the new vertex
        // is appended as a new Steiner cycle after existing cycles.
        //
        if (startIndexCandidates.isEmpty()) {
            cutGlueFaceWithVertex(kf, startVertex);
            startIndexCandidates = getVertexIndexCandidates(kf, startVertex);
            VGC_ASSERT(!startIndexCandidates.isEmpty());
        }
        if (endIndexCandidates.isEmpty()) {
            cutGlueFaceWithVertex(kf, endVertex);
            endIndexCandidates = getVertexIndexCandidates(kf, endVertex);
            VGC_ASSERT(!endIndexCandidates.isEmpty());
        }

        // TODO: find best usages
        // XXX: based on policies ?
        KeyFaceVertexUsageIndex startIndex = startIndexCandidates.first();
        KeyFaceVertexUsageIndex endIndex = endIndexCandidates.first();

        KeyHalfedge khe(const_cast<KeyEdge*>(ke), true);

        if (startVertex == endVertex) {
            // TODO: choose halfedge direction based on twoCycleCutPolicy.
        }

        return cutGlueFace(
            kf, khe, startIndex, endIndex, oneCycleCutPolicy, twoCycleCutPolicy);
    }
    else {
        KeyHalfedge khe(const_cast<KeyEdge*>(ke), true);

        // Do cut-glue with closed edge.

        if (oneCycleCutPolicy == OneCycleCutPolicy::Auto) {
            // TODO: find best policy.
            oneCycleCutPolicy = OneCycleCutPolicy::Disk;
        }

        switch (oneCycleCutPolicy) {
        case OneCycleCutPolicy::Auto:
        case OneCycleCutPolicy::Disk: {
            // Let's assume our edge is in the interior of the face.
            core::Array<KeyCycle> cycles1;
            core::Array<KeyCycle> cycles2;

            KeyCycle newCycle({khe});

            // TODO: use KeyFace winding rule.
            geometry::WindingRule windingRule = geometry::WindingRule::Odd;
            constexpr Int numSamplesPerContenanceTest = 20;
            constexpr double ratioThreshold = 0.5;

            cycles1.append(newCycle);
            cycles2.append(newCycle.reversed());

            for (const auto& cycle : kf->cycles()) {
                if (newCycle.interiorContainedRatio(
                        cycle, windingRule, numSamplesPerContenanceTest)
                    > ratioThreshold) {
                    cycles1.append(cycle);
                }
                else {
                    cycles2.append(cycle);
                }
            }

            // Create the faces
            KeyFace* kf1 =
                createKeyFace(std::move(cycles1), kf->parentGroup(), kf, kf->time());
            kf1->data().setProperties(kf->data().properties());
            KeyFace* kf2 =
                createKeyFace(std::move(cycles2), kf->parentGroup(), kf, kf->time());
            kf2->data().setProperties(kf->data().properties());

            // TODO: substitute in inbetween faces.

            // Delete original face.
            hardDelete(kf);

            return CutFaceResult(kf1, khe.edge(), kf2);
        }
        case OneCycleCutPolicy::Mobius: {
            KeyCycle newCycle({khe, khe});
            addCycleToFace(kf, std::move(newCycle));
            return CutFaceResult(kf, khe.edge(), kf);
        }
        case OneCycleCutPolicy::Torus: {
            KeyCycle newCycle({khe});
            addCycleToFace(kf, newCycle);
            addCycleToFace(kf, newCycle.reversed());
            return CutFaceResult(kf, khe.edge(), kf);
        }
        }

        throw LogicError("cutGlueFace: invalid oneCycleCutPolicy.");
    }
}

namespace {

/// Returns a length that can be considered negligible relative to the size of
/// the given polylines.
///
double getEpsilon(geometry::Vec2dArray& polyline1, geometry::Vec2dArray& polyline2) {
    geometry::Rect2d bb1 = geometry::Rect2d::computeBoundingBox(polyline1);
    geometry::Rect2d bb2 = geometry::Rect2d::computeBoundingBox(polyline2);
    double magnitude = bb1.width() + bb1.height() + bb2.width() + bb2.height();
    return 1e-10 * magnitude;
}

void removeDuplicates(geometry::Vec2dArray& polyline, double epsilon = 0) {
    double eps2 = epsilon * epsilon;
    auto last = std::unique(
        polyline.begin(),
        polyline.end(),
        [eps2](const geometry::Vec2d& v1, const geometry::Vec2d& v2) {
            return (v1 - v2).squaredLength() <= eps2;
        });
    polyline.erase(last, polyline.cend());
}

// Removes any common non-zero-length common subset at the end of the given polylines:
//
//               INPUT     OUTPUT
//
//                   x-x         x
//                  /           /
// polyline1:  x---x       x---x
//
//                 x---x       x-x
//                /           /
// polyline2:  x-x         x-x
//
// Post-condition:
// - one of the polyline is empty, or
// - the distance between the last points of the polylines is greater than epsilon, or
// - the last points of the polyline are equal and the distance between the
//   second-last point (if any) of each polyline to the last segment of the other polyline is greater than epsilon
//
void trimSharedEnd(
    geometry::Vec2dArray& polyline1,
    geometry::Vec2dArray& polyline2,
    double epsilon = 0) {

    Int n1 = polyline1.length();
    Int n2 = polyline2.length();
    if (n1 == 0 || n2 == 0) {
        return;
    }
    double eps2 = epsilon * epsilon;
    if ((polyline1[n1 - 1] - polyline2[n2 - 1]).squaredLength() > eps2) {
        return;
    }
    polyline1.last() = polyline2.last();

    while (n1 >= 2 && n2 >= 2) {

        // Get the last segment AB of each polyline
        geometry::Vec2d& a1 = polyline1[n1 - 1];
        geometry::Vec2d& a2 = polyline2[n2 - 1];
        geometry::Vec2d& b1 = polyline1[n1 - 2];
        geometry::Vec2d& b2 = polyline2[n2 - 2];
        VGC_ASSERT(a1 == a2);
        const geometry::Vec2d& a = a1;

        // Assuming AB1 is smaller than AB2, we want to compute whether
        // B1 is within the epsilon-sized capsule around AB2:
        //
        //   .--------------------.
        //  .   A              B2  .
        //  |   o--------------o   |  ^
        //  `                      '  | eps
        //   `--------------------'   v
        //                     <--->
        //                      eps

        // Test whether B1 and B2 are within epsilon of each other.
        // In this case, we consider the two segments equal.
        //
        //   .--------------.-----.
        //  .   A          .   B2  .       A             B1,B2
        //  |   o----------|---o B1|  =>   o--------------o
        //  `              `     o '
        //   `--------------`-----'
        //
        if ((b1 - b2).squaredLength() <= eps2) {
            b1 = b2;
            --n1;
            --n2;
            continue;
        }

        // Test whether B1 (or B2) is within epsilon of A.
        // In this case, we consider it equal to A.
        //
        //   .----.---------.-----.
        //  .   A  .       .   B2  .      A,B1            B2
        //  |B1 o--|-------|---o   |  =>   o--------------o
        //  ` o    '       `       '
        //   `----'---------`-----'
        //
        geometry::Vec2d ab1 = b1 - a;
        geometry::Vec2d ab2 = b2 - a;
        double l1Squared = ab1.squaredLength();
        double l2Squared = ab2.squaredLength();
        if (l1Squared <= eps2 || l2Squared <= eps2) {
            if (l1Squared <= eps2) {
                b1 = a;
                --n1;
            }
            if (l2Squared <= eps2) {
                b2 = a;
                --n2;
            }
            continue;
        }

        // We now know that A, B1, and B2 are all separated by at least epsilon.

        // Fast return if AB1 and AB2 point to opposite directions.
        //
        //     /|
        //  B1 /|
        //  o  /| A            B2
        //     /o--------------o
        //     /|
        //
        double dot = ab1.dot(ab2);
        if (dot < 0) {
            break;
        }

        // Let AB1 be the shorter of the two.
        // This means that we now know that B1 is somewhere in this space:
        //
        //  /|       B1     |/
        //  /|       o      |/
        //  /| A          B2|/
        //  /o--------------o/
        //  /|              |/
        //
        if (l1Squared > l2Squared) {
            std::swap(ab1, ab2);
            std::swap(l1Squared, l2Squared);
        }

        // Project B1 to AB2 line
        //
        //         B1
        //         o
        //  A      |h     B2
        //  o------o------o
        //         C
        //
        geometry::Vec2d ac = dot / l2Squared * ab2;
        geometry::Vec2d cb1 = ab1 - ac;
        double hSquared = cb1.squaredLength();

        // If B1 is within epsilon of AB2, replace A1 and B1
        // by C and keep iterating. Otherwise, we're done.
        //
        if (hSquared <= eps2) {
            geometry::Vec2d c = a + ac;
            a1 = c;
            a2 = c;
            continue;
        }
        else {
            break;
        }
    }

    polyline1.resize(n1);
    polyline2.resize(n2);
}

// Determine an appropriate OneCycleCutPolicy based on geometric heuristics.
//
// Example:
//
//        o---->----o     path1: from v1 to v2 going
//        |         |            through the "top" part
//        o--o v1   o v2
//        |         |     path2: from v2 to v1 going
//        o----<----o            through the "bottom" part
//
OneCycleCutPolicy computeOneCycleCutPolicy(const KeyPath& path1, const KeyPath& path2) {

    if (path1.halfedges().isEmpty() || path2.halfedges().isEmpty()) {
        return OneCycleCutPolicy::Disk;
    }

    // Sample paths as polylines and remove almost-duplicates samples.
    //
    geometry::Vec2dArray poly1 = path1.sampleCenterline();
    geometry::Vec2dArray poly2 = path2.sampleCenterline();
    double eps = getEpsilon(poly1, poly2);
    double minSegmentLength = 100 * eps;
    removeDuplicates(poly1, minSegmentLength);
    removeDuplicates(poly2, minSegmentLength);
    if (poly1.isEmpty() || poly2.isEmpty()) {
        return OneCycleCutPolicy::Disk;
    }

    // It's actually easier to work with path2 reversed for this algorithm, so
    // that they both start at the same vertex and end at the same vertex.
    //
    //           path1
    //        o---->----o
    //        |         |
    //        o--o v1   o v2
    //        |         |
    //        o---->----o
    //           path2
    //
    std::reverse(poly2.begin(), poly2.end());

    // Enforce that they do start and end at the same position. Normally, this
    // is already the case initially, but the duplicate removal step may have
    // changed that.
    //
    poly1.first() = poly2.first();
    poly1.last() = poly2.last();

    // Trim the common start/end between the polylines
    trimSharedEnd(poly1, poly2, eps);
    std::reverse(poly1.begin(), poly1.end());
    std::reverse(poly2.begin(), poly2.end());
    trimSharedEnd(poly1, poly2, eps);

    // Apply random perturbation until there are no special cases
    UInt32 seed = 0;
    core::PseudoRandomUniform<double> rng(-0.1 * eps, 0.1 * eps, seed);
    Int numAttempts = 0;
    Int maxNumAttempts = 10;
    Int numIntersections = 0;
    bool tryAgain = true;
    while (tryAgain && ++numAttempts <= maxNumAttempts) {

        // Apply random perturbations.
        //
        // In case of shared edges, this means that the two paths will cross
        // many times (around half the number of shared segments), but the
        // number of intersection will be guaranteed to be even.
        //
        for (Int i = 1; i + 1 < poly1.length(); ++i) {
            poly1.getUnchecked(i) += geometry::Vec2d(rng(), rng());
        }
        for (Int j = 1; j + 1 < poly2.length(); ++j) {
            poly2.getUnchecked(j) += geometry::Vec2d(rng(), rng());
        }

        // Count the number of intersections.
        //
        tryAgain = false;
        Int n1 = poly1.length();
        Int n2 = poly2.length();
        for (Int i = 0; i < n1 - 1; ++i) {
            Int jStart = (i == 0) ? 1 : 0;              // don't count shared start point
            Int jEnd = (i == n1 - 2) ? n2 - 2 : n2 - 1; // don't count shared end point
            for (Int j = jStart; j < jEnd; ++j) {
                geometry::Segment2dIntersection inter = geometry::segmentIntersection(
                    poly1[i], poly1[i + 1], poly2[j], poly2[j + 1]);

                using SIT = geometry::SegmentIntersectionType;
                switch (inter.type()) {
                case SIT::Empty:
                    // nothing to do
                    break;

                case SIT::Point:
                    // TODO: what if the intersection is near the beginning or end?
                    // how to avoid double counting, or missing both due to numerical errors?
                    ++numIntersections;
                    break;

                case SIT::Segment:
                    // our heuristic cannot work in this case, so let's try again.
                    tryAgain = true;

                    // Exit out of the two nested loops
                    i = poly1.length();
                    j = poly2.length();

                    // Fallback to Disk if maxNumAttempts reached
                    numIntersections = 0;
                }
            }
        }
    }

    if (numIntersections % 2 == 1) {
        return OneCycleCutPolicy::Mobius;
    }
    else {
        return OneCycleCutPolicy::Disk;
    }
}

} // namespace

CutFaceResult Operations::cutGlueFace(
    KeyFace* kf,
    const KeyHalfedge& khe,
    KeyFaceVertexUsageIndex startIndex,
    KeyFaceVertexUsageIndex endIndex,
    OneCycleCutPolicy oneCycleCutPolicy,
    TwoCycleCutPolicy twoCycleCutPolicy) {

    Int cycleIndex1 = startIndex.cycleIndex();
    Int cycleIndex2 = endIndex.cycleIndex();

    // Get precision for numerical errors
    kf->boundingBox();

    if (cycleIndex1 == cycleIndex2) {
        // One-cycle cut case

        KeyCycle& cycle = kf->cycles_[cycleIndex1];

        // If one path must be empty, it will be path2.
        KeyPath path1 =
            cycle.subPath(endIndex.componentIndex(), startIndex.componentIndex(), true);
        KeyPath path2 =
            cycle.subPath(startIndex.componentIndex(), endIndex.componentIndex());

        if (oneCycleCutPolicy == OneCycleCutPolicy::Auto) {
            oneCycleCutPolicy = computeOneCycleCutPolicy(path1, path2);
        }

        switch (oneCycleCutPolicy) {
        case OneCycleCutPolicy::Auto:
        case OneCycleCutPolicy::Disk: {
            core::Array<KeyCycle> cycles1;
            core::Array<KeyCycle> cycles2;

            KeyCycle newCycle1;
            {
                KeyPath path = path1;
                path.append(khe);
                newCycle1 = KeyCycle(std::move(path));
            }
            VGC_ASSERT(newCycle1.isValid());

            KeyCycle newCycle2;
            {
                KeyPath path = path2;
                path.append(khe.opposite());
                newCycle2 = KeyCycle(std::move(path));
            }
            VGC_ASSERT(newCycle2.isValid());

            cycles1.append(newCycle1);
            cycles2.append(newCycle2);

            // TODO: use KeyFace winding rule.
            geometry::WindingRule windingRule = geometry::WindingRule::Odd;
            constexpr Int numSamplesPerContenanceTest = 20;

            Int cycleIndex = -1;
            for (const auto& otherCycle : kf->cycles()) {
                ++cycleIndex;
                if (cycleIndex == cycleIndex1) {
                    continue;
                }
                double r1 = newCycle1.interiorContainedRatio(
                    otherCycle, windingRule, numSamplesPerContenanceTest);
                double r2 = newCycle2.interiorContainedRatio(
                    otherCycle, windingRule, numSamplesPerContenanceTest);
                if (r1 >= r2) {
                    cycles1.append(otherCycle);
                }
                else {
                    cycles2.append(otherCycle);
                }
            }

            // Create the faces
            KeyFace* kf1 =
                createKeyFace(std::move(cycles1), kf->parentGroup(), kf, kf->time());
            kf1->data().setProperties(kf->data().properties());
            KeyFace* kf2 =
                createKeyFace(std::move(cycles2), kf->parentGroup(), kf, kf->time());
            kf2->data().setProperties(kf->data().properties());

            // TODO: substitute in inbetween faces.

            // Delete original face.
            hardDelete(kf);

            return CutFaceResult(kf1, khe.edge(), kf2);
        }
        case OneCycleCutPolicy::Mobius: {
            KeyCycle newCycle;
            {
                KeyPath path = path1;
                path.append(khe);
                path.extendReversed(path2);
                path.append(khe);
                newCycle = KeyCycle(std::move(path));
            }
            VGC_ASSERT(newCycle.isValid());

            kf->cycles_[cycleIndex1] = std::move(newCycle);
            addToBoundary_(kf, khe.edge());

            return CutFaceResult(kf, khe.edge(), kf);
        }
        case OneCycleCutPolicy::Torus: {
            KeyCycle newCycle1;
            {
                KeyPath path = path1;
                path.append(khe);
                newCycle1 = KeyCycle(std::move(path));
            }
            VGC_ASSERT(newCycle1.isValid());

            KeyCycle newCycle2;
            {
                KeyPath path = path2;
                path.append(khe.opposite());
                newCycle2 = KeyCycle(std::move(path));
            }
            VGC_ASSERT(newCycle2.isValid());

            kf->cycles_[cycleIndex1] = std::move(newCycle1);
            kf->cycles_.append(std::move(newCycle2));
            addToBoundary_(kf, khe.edge());

            return CutFaceResult(kf, khe.edge(), kf);
        }
        }

        throw LogicError("cutGlueFace: invalid oneCycleCutPolicy.");
    }
    else {
        // Two-cycle cut case

        const KeyCycle& cycle1 = kf->cycles_[cycleIndex1];
        const KeyCycle& cycle2 = kf->cycles_[cycleIndex2];

        KeyPath path1 = cycle1.rotated(startIndex.componentIndex());
        KeyPath path2 = cycle2.rotated(endIndex.componentIndex());

        if (twoCycleCutPolicy == TwoCycleCutPolicy::Auto) {

            // Default fallback policy.
            twoCycleCutPolicy = TwoCycleCutPolicy::ReverseNone;

            if (!path1.isSingleVertex() && !path2.isSingleVertex()) {
                // Around each end vertex, the plane can be seen as divided into sectors by
                // the incident halfedges. In each of these sectors we want the winding to
                // remain unchanged if possible.

                core::Array<const KeyCycle*> otherCycles;
                Int cycleIndex = -1;
                for (const auto& otherCycle : kf->cycles()) {
                    ++cycleIndex;
                    if (cycleIndex == cycleIndex1 || cycleIndex == cycleIndex2) {
                        continue;
                    }
                    if (otherCycle.steinerVertex()) {
                        continue;
                    }
                    otherCycles.append(&otherCycle);
                }

                // assumes h2 = h1.previous().opposite()
                auto toSectorPoint =
                    [kf](
                        const geometry::Vec2d& p,
                        const RingKeyHalfedge& rh1,
                        const RingKeyHalfedge& rh2) -> std::optional<geometry::Vec2d> {
                    double angle1 = rh1.angle();
                    double angle2 = rh2.angle();
                    if (rh2 < rh1) {
                        angle2 += 2 * core::pi;
                    }
                    else if (angle2 == angle1) {
                        return std::nullopt;
                    }
                    double angle = (angle1 + angle2) * 0.5;

                    geometry::Rect2d kfBbox = kf->boundingBox();
                    double delta = (std::max)(kfBbox.width(), kfBbox.height());
                    delta *= core::epsilon;

                    return p + geometry::Vec2d(std::cos(angle), std::sin(angle)) * delta;
                };

                struct WindingSample {
                    geometry::Vec2d position;
                    std::array<Int, 4> numbers;
                };

                core::Array<WindingSample> windingSamples;

                // TODO: use KeyFace winding rule.
                geometry::WindingRule windingRule = geometry::WindingRule::Odd;

                auto processRing = [&](KeyVertex* kv) {
                    geometry::Vec2d p = kv->position();
                    core::Array<RingKeyHalfedge> ring =
                        khe.startVertex()->ringHalfedges();

                    auto prevIt = ring.end() - 1;
                    for (auto it = ring.begin(); it != ring.end(); prevIt = it, ++it) {
                        if (std::optional<geometry::Vec2d> sp =
                                toSectorPoint(p, *prevIt, *it)) {

                            geometry::Vec2d spPos = sp.value();
                            Int number0 = 0;
                            for (const auto* otherCycle : otherCycles) {
                                number0 += otherCycle->computeWindingNumberAt(spPos);
                            }
                            Int number1 = cycle1.computeWindingNumberAt(spPos);
                            Int number2 = cycle2.computeWindingNumberAt(spPos);
                            WindingSample& sample = windingSamples.emplaceLast();
                            sample.position = spPos;
                            sample.numbers[0] = number0 + number1 + number2;
                            sample.numbers[1] = number0 - number1 + number2;
                            sample.numbers[2] = number0 + number1 - number2;
                            sample.numbers[3] = number0 - number1 - number2;
                        }
                    }
                };

                processRing(khe.startVertex());
                if (khe.endVertex() != khe.startVertex()) {
                    processRing(khe.endVertex());
                }

                switch (windingRule) {
                case geometry::WindingRule::Odd: {
                    // All combinaisons keep the appearance,
                    // but we prefer lower numbers and zeros.
                    using Sums = std::array<Int, 3>;
                    Sums zeroSums = {0, 0, 0};
                    std::array<Sums, 4> sumsPerPolicy = {
                        zeroSums, zeroSums, zeroSums, zeroSums};
                    for (const WindingSample& sample : windingSamples) {
                        Int number0 = sample.numbers[0];
                        for (Int i = 0; i < 4; ++i) {
                            Int number = sample.numbers[i];
                            Int absNumber = std::abs(number);
                            if (absNumber % 2 == 0) {
                                sumsPerPolicy[i][0] += std::abs(number);
                            }
                            else {
                                sumsPerPolicy[i][1] += std::abs(number);
                            }
                            if (number != 0) {
                                if (number0 == 0) {
                                    sumsPerPolicy[i][2] += 1;
                                }
                                else if (number * number0 < 0) {
                                    sumsPerPolicy[i][2] += 1;
                                }
                            }
                        }
                    }
                    Int minIndex = std::distance(
                        sumsPerPolicy.begin(),
                        std::min_element(sumsPerPolicy.begin(), sumsPerPolicy.end()));
                    switch (minIndex) {
                    case 0:
                        twoCycleCutPolicy = TwoCycleCutPolicy::ReverseNone;
                        break;
                    case 1:
                        twoCycleCutPolicy = TwoCycleCutPolicy::ReverseStart;
                        break;
                    case 2:
                        twoCycleCutPolicy = TwoCycleCutPolicy::ReverseEnd;
                        break;
                    case 3:
                        twoCycleCutPolicy = TwoCycleCutPolicy::ReverseBoth;
                        break;
                    }
                }
                case geometry::WindingRule::NonZero:
                case geometry::WindingRule::Positive:
                case geometry::WindingRule::Negative:
                    // TODO
                    break;
                }

                // geometry::isWindingNumberSatisfyingRule

                /*if (khe.startVertex() != khe.endVertex()) {
                    geometry::Vec2d p2 = khe.endVertex()->position();
                    core::Array<RingKeyHalfedge> ring2 =
                        khe.endVertex()->computeRingHalfedges();
                }*/

                // TODO
            }
        }

        VGC_ASSERT(twoCycleCutPolicy != TwoCycleCutPolicy::Auto);

        switch (twoCycleCutPolicy) {
        case TwoCycleCutPolicy::Auto:
        case TwoCycleCutPolicy::ReverseNone: {
            break;
        }
        case TwoCycleCutPolicy::ReverseStart: {
            path1.reverse();
            break;
        }
        case TwoCycleCutPolicy::ReverseEnd: {
            path2.reverse();
            break;
        }
        case TwoCycleCutPolicy::ReverseBoth: {
            path1.reverse();
            path2.reverse();
            break;
        }
        }

        path1.append(khe);
        path1.extend(path2);
        path1.append(khe.opposite());

        KeyCycle newCycle(std::move(path1));
        VGC_ASSERT(newCycle.isValid());

        kf->cycles_[cycleIndex1] = std::move(newCycle);
        kf->cycles_.removeAt(cycleIndex2);
        addToBoundary_(kf, khe.edge());

        return CutFaceResult(kf, khe.edge(), kf);
    }
}

CutFaceResult Operations::cutFaceWithClosedEdge(
    KeyFace* kf,
    KeyEdgeData&& data,
    OneCycleCutPolicy oneCycleCutPolicy) {

    KeyEdge* ke = createKeyClosedEdge(
        std::move(data), kf->parentGroup(), kf->nextSibling(), kf->time());
    return cutGlueFace(kf, ke, oneCycleCutPolicy);
}

CutFaceResult Operations::cutFaceWithOpenEdge(
    KeyFace* kf,
    KeyEdgeData&& data,
    KeyFaceVertexUsageIndex startIndex,
    KeyFaceVertexUsageIndex endIndex,
    OneCycleCutPolicy oneCycleCutPolicy,
    TwoCycleCutPolicy twoCycleCutPolicy) {

    KeyVertex* kv1 = kf->vertex(startIndex);
    KeyVertex* kv2 = kf->vertex(endIndex);
    KeyEdge* ke = createKeyOpenEdge(
        kv1, kv2, std::move(data), kf->parentGroup(), kf->nextSibling());
    KeyHalfedge khe(ke, true);
    return cutGlueFace(
        kf, khe, startIndex, endIndex, oneCycleCutPolicy, twoCycleCutPolicy);
}

CutFaceResult Operations::cutFaceWithOpenEdge(
    KeyFace* kf,
    KeyEdgeData&& data,
    KeyVertex* startVertex,
    KeyVertex* endVertex,
    OneCycleCutPolicy oneCycleCutPolicy,
    TwoCycleCutPolicy twoCycleCutPolicy) {

    KeyEdge* ke = createKeyOpenEdge(
        startVertex, endVertex, std::move(data), kf->parentGroup(), kf->nextSibling());
    return cutGlueFace(kf, ke, oneCycleCutPolicy, twoCycleCutPolicy);
}

void Operations::cutGlueFaceWithVertex(KeyFace* kf, KeyVertex* kv) {
    // Append last so that it does not invalidate existing KeyFaceVertexUsageIndex
    kf->cycles_.append(KeyCycle(kv));
    addToBoundary_(kf, kv);
}

KeyVertex* Operations::cutFaceWithVertex(KeyFace* kf, const geometry::Vec2d& position) {
    KeyVertex* newKv =
        createKeyVertex(position, kf->parentGroup(), kf->nextSibling(), kf->time());
    cutGlueFaceWithVertex(kf, newKv);
    return newKv;
}

} // namespace vgc::vacomplex::detail
