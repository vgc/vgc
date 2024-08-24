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

#include <algorithm> // reverse
#include <unordered_set>

namespace vgc::vacomplex::detail {

void Operations::substituteVertex_(KeyVertex* oldVertex, KeyVertex* newVertex) {
    if (newVertex == oldVertex) {
        return;
    }
    for (Cell* starCell : oldVertex->star().copy()) {
        starCell->substituteKeyVertex_(oldVertex, newVertex);
        removeFromBoundary_(starCell, oldVertex);
        addToBoundary_(starCell, newVertex);
    }
}

// It assumes end vertices are the same!
void Operations::substituteEdge_(const KeyHalfedge& oldKhe, const KeyHalfedge& newKhe) {
    if (oldKhe == newKhe) {
        return;
    }
    KeyEdge* const oldKe = oldKhe.edge();
    KeyEdge* const newKe = newKhe.edge();
    for (Cell* starCell : oldKe->star().copy()) {
        starCell->substituteKeyEdge_(oldKhe, newKhe);
        removeFromBoundary_(starCell, oldKe);
        addToBoundary_(starCell, newKe);
    }
}

KeyEdge* Operations::glueKeyOpenEdges_(core::ConstSpan<KeyHalfedge> khs) {

    if (khs.isEmpty()) {
        return nullptr;
    }

    Int n = khs.length();
    core::Array<KeyHalfedgeData> khds;
    khds.reserve(n);
    for (const KeyHalfedge& kh : khs) {
        KeyEdgeData& kd = kh.edge()->data();
        khds.emplaceLast(&kd, kh.direction());
    }
    KeyEdgeData newData = KeyEdgeData::fromGlueOpen(khds);
    VGC_ASSERT(newData.stroke());

    std::array<geometry::Vec2d, 2> endPositions = newData.stroke()->endPositions();

    core::Array<KeyVertex*> startVertices;
    startVertices.reserve(n);
    for (const KeyHalfedge& kh : khs) {
        startVertices.append(kh.startVertex());
    }
    KeyVertex* startKv = glueKeyVertices(startVertices, endPositions[0]);

    // Note: we can only list end vertices after the glue of
    // start vertices since it can substitute end vertices.
    core::Array<KeyVertex*> endVertices;
    endVertices.reserve(n);
    for (const KeyHalfedge& kh : khs) {
        endVertices.append(kh.endVertex());
    }
    geometry::Vec2d endVertexPosition = endPositions[1];
    if (endVertices.contains(startKv)) {
        // collapsing start and end to single vertex
        endVertexPosition = 0.5 * (endPositions[0] + endVertexPosition);
        newData.snapGeometry(endVertexPosition, endVertexPosition);
        startKv = nullptr;
    }
    KeyVertex* endKv = glueKeyVertices(endVertices, endVertexPosition);
    if (!startKv) {
        startKv = endKv;
    }

    // Location: top-most input edge
    core::Array<Node*> edgeNodes(n);
    for (Int i = 0; i < n; ++i) {
        edgeNodes[i] = khs[i].edge();
    }

    Node* topMostEdge = findTopMost(edgeNodes);
    Group* parentGroup = topMostEdge->parentGroup();
    Node* nextSibling = topMostEdge->nextSibling();

    KeyEdge* newKe =
        createKeyOpenEdge(startKv, endKv, std::move(newData), parentGroup, nextSibling);

    KeyHalfedge newKh(newKe, true);
    for (const KeyHalfedge& kh : khs) {
        substituteEdge_(kh, newKh);
        // It is important that no 2 halfedges refer to the same edge.
        hardDelete(kh.edge(), true);
    }

    return newKe;
}

KeyEdge* Operations::glueKeyClosedEdges_(
    core::ConstSpan<KeyHalfedge> khs,
    core::ConstSpan<double> uOffsets) {

    if (khs.isEmpty()) {
        return nullptr;
    }

    // Location: top-most input edge

    Int n = khs.length();
    core::Array<Node*> edgeNodes;
    edgeNodes.reserve(n);
    core::Array<KeyHalfedgeData> khds;
    khds.reserve(n);
    for (const KeyHalfedge& kh : khs) {
        edgeNodes.append(kh.edge());
        KeyEdgeData& kd = kh.edge()->data();
        khds.emplaceLast(&kd, kh.direction());
    }

    Node* topMostEdge = findTopMost(edgeNodes);
    Group* parentGroup = topMostEdge->parentGroup();
    Node* nextSibling = topMostEdge->nextSibling();

    KeyEdgeData newData = KeyEdgeData::fromGlueClosed(khds, uOffsets);
    VGC_ASSERT(newData.stroke());

    KeyEdge* newKe = createKeyClosedEdge(std::move(newData), parentGroup, nextSibling);

    KeyHalfedge newKh(newKe, true);
    for (const KeyHalfedge& kh : khs) {
        substituteEdge_(kh, newKh);
        // It is important that no 2 halfedges refer to the same edge.
        hardDelete(kh.edge(), true);
    }

    return newKe;
}

KeyVertex*
Operations::glueKeyVertices(core::Span<KeyVertex*> kvs, const geometry::Vec2d& position) {
    if (kvs.isEmpty()) {
        return nullptr;
    }
    KeyVertex* kv0 = kvs[0];

    bool hasDifferentKvs = false;
    for (KeyVertex* kv : kvs.subspan(1)) {
        if (kv != kv0) {
            hasDifferentKvs = true;
            break;
        }
    }

    if (!hasDifferentKvs) {
        setKeyVertexPosition(kv0, position);
        return kv0;
    }

    // Location: top-most input vertex
    Int n = kvs.length();
    core::Array<Node*> nodes(n);
    for (Int i = 0; i < n; ++i) {
        nodes[i] = kvs[i];
    }
    Node* topMostVertex = findTopMost(nodes);
    Group* parentGroup = topMostVertex->parentGroup();
    Node* nextSibling = topMostVertex->nextSibling();

    KeyVertex* newKv = createKeyVertex(position, parentGroup, nextSibling, kv0->time());

    std::unordered_set<KeyVertex*> seen;
    for (KeyVertex* kv : kvs) {
        bool inserted = seen.insert(kv).second;
        if (inserted) {
            substituteVertex_(kv, newKv);
            hardDelete(kv);
        }
    }

    return newKv;
}

namespace {

// Assumes `!samples.isEmpty()` and `numSamples >= 1`.
core::Array<geometry::Vec2d> computeApproximateUniformSamplingPositions(
    const geometry::StrokeSample2dArray& samples,
    Int numSamples) {

    core::Array<geometry::Vec2d> result;
    result.reserve(numSamples);
    result.append(samples.first().position());
    double l = samples.last().s();
    if (l > 0) {
        double deltaS = l / (numSamples - 1);
        double targetS = deltaS;
        const geometry::StrokeSample2d* s0 = &samples[0];
        for (const geometry::StrokeSample2d& s1 : samples) {
            double ds = s1.s() - s0->s();
            if (ds > 0 && targetS <= s1.s()) {
                double t = (targetS - s0->s()) / ds;
                result.append(core::fastLerp(s0->position(), s1.position(), t));
                targetS += deltaS;
            }
            s0 = &s1;
        }
    }
    while (result.length() < numSamples) {
        result.append(samples.last().position());
    }

    return result;
}

} // namespace

KeyEdge* Operations::glueKeyOpenEdges(core::ConstSpan<KeyHalfedge> khs) {
    return glueKeyOpenEdges_(khs);
}

KeyEdge* Operations::glueKeyOpenEdges(core::ConstSpan<KeyEdge*> kes) {

    Int n = kes.length();

    if (n == 0) {
        return nullptr;
    }
    else if (n == 1) {
        return kes[0];
    }

    // Detect which edge direction should be used for gluing.

    // Here, we handle the simple case where there are two edges that
    // already share at least one vertex.
    if (n == 2) {
        KeyEdge* ke0 = kes[0];
        KeyEdge* ke1 = kes[1];
        KeyVertex* ke00 = ke0->startVertex();
        KeyVertex* ke01 = ke0->endVertex();
        KeyVertex* ke10 = ke1->startVertex();
        KeyVertex* ke11 = ke1->endVertex();
        bool isAnyLoop = (ke00 == ke01) || (ke10 == ke11);
        bool isBestDirectionKnown = false;
        bool direction1 = true;
        if (!isAnyLoop) {
            bool shared00 = ke00 == ke10;
            bool shared11 = ke01 == ke11;
            bool shared01 = ke00 == ke11;
            bool shared10 = ke01 == ke10;
            if (shared00 != shared11) {
                // If the two edges have the same start vertex or the same
                // end vertex, we glue them in their intrinsic direction.
                direction1 = true;
                isBestDirectionKnown = true;
            }
            else if (shared01 != shared10) {
                // If the start (resp. end) vertex of ke0 is equal to the
                // end (resp. start) vertex of ke1, we want to glue them in reverse.
                direction1 = false;
                isBestDirectionKnown = true;
            }
        }
        if (isBestDirectionKnown) {
            std::array<KeyHalfedge, 2> khs = {
                KeyHalfedge(ke0, true), KeyHalfedge(ke1, direction1)};
            return glueKeyOpenEdges_(khs);
        }
    }

    constexpr Int numSamples = 10;

    core::Array<core::Array<geometry::Vec2d>> sampleArrays;
    sampleArrays.reserve(n);
    for (KeyEdge* ke : kes) {
        const geometry::StrokeSample2dArray& strokeSamples =
            ke->strokeSampling().samples();
        sampleArrays.append(
            computeApproximateUniformSamplingPositions(strokeSamples, numSamples));
    }

    core::Array<bool> bestDirections;
    core::Array<bool> tmpDirections(n);
    double bestCost = core::DoubleInfinity;

    for (Int i = 0; i < n; ++i) {
        double tmpCost = 0;
        const core::Array<geometry::Vec2d>& s0 = sampleArrays[i];
        tmpDirections[i] = true;
        for (Int j = 0; j < n; ++j) {
            if (j == i) {
                continue;
            }
            const core::Array<geometry::Vec2d>& s1 = sampleArrays[j];

            // costs per direction of edge j
            double costEj = 0;
            double costEjR = 0;

            for (Int iSample = 0; iSample < numSamples; ++iSample) {
                Int iSampleR = numSamples - 1 - iSample;
                const geometry::Vec2d& s0i = s0.getUnchecked(iSample);
                costEj += (s0i - s1.getUnchecked(iSample)).squaredLength();
                costEjR += (s0i - s1.getUnchecked(iSampleR)).squaredLength();
            }

            if (costEj <= costEjR) {
                tmpDirections[j] = true;
                tmpCost += costEj;
            }
            else {
                tmpDirections[j] = false;
                tmpCost += costEjR;
            }
        }
        if (tmpCost < bestCost) {
            bestDirections = tmpDirections;
            bestCost = tmpCost;
        }
    }

    core::Array<KeyHalfedge> khs;
    khs.reserve(n);
    for (Int i = 0; i < n; ++i) {
        khs.emplaceLast(kes[i], bestDirections[i]);
    }

    return glueKeyOpenEdges_(khs);
}

KeyEdge* Operations::glueKeyClosedEdges(core::ConstSpan<KeyHalfedge> khs) {
    Int n = khs.length();

    if (n == 0) {
        return nullptr;
    }
    else if (n == 1) {
        return khs[0].edge();
    }

    constexpr Int numCostSamples = 10;
    constexpr Int costSampleStride = 10;
    constexpr Int numSamples = numCostSamples * costSampleStride;

    core::Array<core::Array<geometry::Vec2d>> sampleArrays;
    core::Array<double> arclengths;
    sampleArrays.reserve(n);
    arclengths.reserve(n);
    for (const KeyHalfedge& kh : khs) {
        KeyEdge* ke = kh.edge();
        const geometry::StrokeSample2dArray& strokeSamples =
            ke->strokeSampling().samples();
        sampleArrays.append(
            computeApproximateUniformSamplingPositions(strokeSamples, numSamples + 1));
        core::Array<geometry::Vec2d>& samples = sampleArrays.last();
        if (!kh.direction()) {
            std::reverse(samples.begin(), samples.end());
        }
        // since it is closed, first and last are the same
        samples.removeLast();
        arclengths.append(strokeSamples.last().s());
    }

    core::Array<double> bestUOffsets;
    core::Array<double> tmpUOffsets(n);

    double bestCost = core::DoubleInfinity;
    double deltaU = 1. / numSamples;

    for (Int i = 0; i < n; ++i) {
        double tmpCost = 0;
        const core::Array<geometry::Vec2d>& s0 = sampleArrays[i];
        tmpUOffsets[i] = 0.;
        for (Int j = 0; j < n; ++j) {
            if (j == i) {
                continue;
            }
            const core::Array<geometry::Vec2d>& s1 = sampleArrays[j];

            // best cost for shifts of halfedge j
            double bestCostHj = core::DoubleInfinity;

            for (Int k = 0; k < numSamples; ++k) {
                // cost for halfedge j with shift k
                double costHjk = 0;
                for (Int iCostSample = 0; iCostSample < numCostSamples; ++iCostSample) {
                    // the shift must be reversed for use with AbstractStroke2d
                    Int iSample = (iCostSample * costSampleStride + k) % numSamples;
                    const geometry::Vec2d& s0i = s0.getUnchecked(iSample);
                    const geometry::Vec2d& s1i = s1.getUnchecked(iSample);
                    costHjk += (s0i - s1i).squaredLength();
                }
                if (costHjk < bestCostHj) {
                    tmpUOffsets[j] = deltaU * k;
                    bestCostHj = costHjk;
                }
            }

            tmpCost += bestCostHj;
        }
        if (tmpCost < bestCost) {
            bestUOffsets = tmpUOffsets;
            bestCost = tmpCost;
        }
    }

    return glueKeyClosedEdges_(khs, bestUOffsets);
}

KeyEdge* Operations::glueKeyClosedEdges(core::ConstSpan<KeyEdge*> kes) {
    Int n = kes.length();

    if (n == 0) {
        return nullptr;
    }
    else if (n == 1) {
        return kes[0];
    }

    constexpr Int numCostSamples = 10;
    constexpr Int costSampleStride = 10;
    constexpr Int numSamples = numCostSamples * costSampleStride;

    core::Array<core::Array<geometry::Vec2d>> sampleArrays;
    core::Array<double> arclengths;
    sampleArrays.reserve(n);
    arclengths.reserve(n);
    for (KeyEdge* ke : kes) {
        const geometry::StrokeSample2dArray& strokeSamples =
            ke->strokeSampling().samples();
        sampleArrays.append(
            computeApproximateUniformSamplingPositions(strokeSamples, numSamples + 1));
        // since it is closed, first and last are the same
        sampleArrays.last().removeLast();
        arclengths.append(strokeSamples.last().s());
    }

    core::Array<bool> bestDirections;
    core::Array<bool> tmpDirections(n);
    core::Array<double> bestUOffsets;
    core::Array<double> tmpUOffsets(n);

    double bestCost = core::DoubleInfinity;
    double deltaU = 1. / numSamples;

    for (Int i = 0; i < n; ++i) {
        double tmpCost = 0;
        const core::Array<geometry::Vec2d>& s0 = sampleArrays[i];
        tmpDirections[i] = true;
        tmpUOffsets[i] = 0.;
        for (Int j = 0; j < n; ++j) {
            if (j == i) {
                continue;
            }
            const core::Array<geometry::Vec2d>& s1 = sampleArrays[j];

            // best cost for (direction, shift) of edge j
            double bestCostEj = core::DoubleInfinity;

            for (Int k = 0; k < numSamples; ++k) {
                // costs per direction of edge j with shift k
                double costEjk = 0;
                double costEjRk = 0;
                for (Int iCostSample = 0; iCostSample < numCostSamples; ++iCostSample) {
                    // the shift must be reversed for use with AbstractStroke2d
                    Int iSample = iCostSample * costSampleStride;
                    Int jSample = (iSample + k) % numSamples;
                    Int jSampleR = numSamples - 1 - jSample;
                    const geometry::Vec2d& s0i = s0.getUnchecked(iSample);
                    costEjk += (s0i - s1.getUnchecked(jSample)).squaredLength();
                    costEjRk += (s0i - s1.getUnchecked(jSampleR)).squaredLength();
                }
                if (costEjk < bestCostEj) {
                    tmpUOffsets[j] = deltaU * k;
                    tmpDirections[j] = true;
                    bestCostEj = costEjk;
                }
                if (costEjRk < bestCostEj) {
                    tmpUOffsets[j] = deltaU * k;
                    tmpDirections[j] = false;
                    bestCostEj = costEjRk;
                }
            }

            tmpCost += bestCostEj;
        }
        if (tmpCost < bestCost) {
            bestDirections = tmpDirections;
            bestUOffsets = tmpUOffsets;
            bestCost = tmpCost;
        }
    }

    core::Array<KeyHalfedge> khs;
    khs.reserve(n);
    for (Int i = 0; i < n; ++i) {
        khs.emplaceLast(kes[i], bestDirections[i]);
    }

    return glueKeyClosedEdges_(khs, bestUOffsets);
}

} // namespace vgc::vacomplex::detail
