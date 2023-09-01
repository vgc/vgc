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

#include <vgc/vacomplex/detail/operationsimpl.h>

#include <unordered_set>
#include <algorithm> // std::reverse

#include <vgc/core/array.h>
#include <vgc/vacomplex/exceptions.h>
#include <vgc/vacomplex/keyedgedata.h>
#include <vgc/vacomplex/logcategories.h>

namespace vgc::vacomplex::detail {

Operations::Operations(Complex* complex)
    : complex_(complex) {

    // Ensure complex is non-null
    if (!complex) {
        throw LogicError("Cannot instantiate a VAC `Operations` with a null complex.");
    }

    if (++complex->numOperationsInProgress_ == 1) {
        // Increment version
        complex->version_ += 1;
    }
}

Operations::~Operations() {
    Complex* complex = this->complex();
    // TODO: try/catch
    if (--complex->numOperationsInProgress_ == 0) {
        // Do geometric updates
        for (const ModifiedNodeInfo& info : complex->opDiff_.modifiedNodes_) {
            if (info.flags().has(NodeModificationFlag::BoundaryMeshChanged)
                && !info.flags().has(NodeModificationFlag::GeometryChanged)) {
                //
                // Let the cell snap to its boundary..
                Cell* cell = info.node()->toCell();
                if (cell && cell->updateGeometryFromBoundary_()) {
                    onNodeModified_(cell, NodeModificationFlag::GeometryChanged);
                }
            }
        }
        complex->nodesChanged().emit(complex->opDiff_);
        complex->opDiff_.clear();
    }
}

Group* Operations::createRootGroup() {
    Group* group = createNode_<Group>(complex());
    return group;
}

Group* Operations::createGroup(Group* parentGroup, Node* nextSibling) {
    Group* group = createNodeAt_<Group>(parentGroup, nextSibling, complex());
    return group;
}

KeyVertex* Operations::createKeyVertex(
    const geometry::Vec2d& position,
    Group* parentGroup,
    Node* nextSibling,
    core::AnimTime t) {

    KeyVertex* kv = createNodeAt_<KeyVertex>(parentGroup, nextSibling, t);

    // Topological attributes
    // -> None

    // Geometric attributes
    kv->position_ = position;

    return kv;
}

KeyEdge* Operations::createKeyOpenEdge(
    KeyVertex* startVertex,
    KeyVertex* endVertex,
    std::unique_ptr<KeyEdgeData>&& data,
    Group* parentGroup,
    Node* nextSibling) {

    KeyEdge* ke = createNodeAt_<KeyEdge>(parentGroup, nextSibling, startVertex->time());

    // Topological attributes
    ke->startVertex_ = startVertex;
    ke->endVertex_ = endVertex;
    addToBoundary_(ke, startVertex);
    addToBoundary_(ke, endVertex);

    // Geometric attributes
    ke->setData_(std::move(data));

    return ke;
}

KeyEdge* Operations::createKeyClosedEdge(
    std::unique_ptr<KeyEdgeData>&& data,
    Group* parentGroup,
    Node* nextSibling,
    core::AnimTime t) {

    KeyEdge* ke = createNodeAt_<KeyEdge>(parentGroup, nextSibling, t);

    // Topological attributes
    // -> None

    // Geometric attributes
    ke->setData_(std::move(data));

    return ke;
}

// Assumes `cycles` are valid.
// Assumes `nextSibling` is either `nullptr` or a child of `parentGroup`.
KeyFace* Operations::createKeyFace(
    core::Array<KeyCycle> cycles,
    Group* parentGroup,
    Node* nextSibling,
    core::AnimTime t) {

    KeyFace* kf = createNodeAt_<KeyFace>(parentGroup, nextSibling, t);

    // Topological attributes
    kf->cycles_ = std::move(cycles);
    for (const KeyCycle& cycle : kf->cycles()) {
        addToBoundary_(kf, cycle);
    }

    // Geometric attributes
    // -> None

    return kf;
}

void Operations::hardDelete(Node* node, bool deleteIsolatedVertices) {

    std::unordered_set<Node*> nodesToDestroy;

    // When hard-deleting the root, we delete all nodes below the root, but
    // preserve the root itself since we have the invariant that there is
    // always a root.
    //
    const bool isRoot = (complex()->rootGroup() == node);
    if (!isRoot) {
        nodesToDestroy.insert(node);
    }

    // Recursively collect all dependent nodes:
    // - children of groups
    // - star cells of cells
    //
    collectDependentNodes_(node, nodesToDestroy);

    // Flag all cells that are about to be deleted.
    //
    for (Node* nodeToDestroy : nodesToDestroy) {
        nodeToDestroy->isBeingDeleted_ = true;
    }

    // Helper function that tests if the star of a cell will become empty after
    // deleting all cells flagged for deletion.
    //
    auto hasEmptyStar = [](Cell* cell) {
        bool isStarEmpty = true;
        for (Cell* starCell : cell->star()) {
            if (!starCell->isBeingDeleted_) {
                isStarEmpty = false;
                break;
            }
        }
        return isStarEmpty;
    };

    // Update star of cells in the boundary of deleted cells.
    //
    // For example, if we delete an edge, we should remove the edge
    // for the star of its end vertices.
    //
    // In this step, we also detect vertices which are about to become
    // isolated, and delete these if deleteIsolatedVertices is true. Note that
    // there is no need to collectDependentNodes_(isolatedVertex), since being
    // isolated means having an empty star, which means that the vertex has no
    // dependent nodes.
    //
    // Note: we store the isolated vertices as set<Node*> instead of set<Cell*>
    // so that we can later merge with nodesToDelete.
    //
    std::unordered_set<Node*> isolatedKeyVertices;
    std::unordered_set<Node*> isolatedInbetweenVertices;
    for (Node* nodeToDestroy : nodesToDestroy) {
        if (nodeToDestroy->isCell()) {
            Cell* cell = nodeToDestroy->toCellUnchecked();
            for (Cell* boundaryCell : cell->boundary()) {
                if (boundaryCell->isBeingDeleted_) {
                    continue;
                }
                if (deleteIsolatedVertices
                    && boundaryCell->spatialType() == CellSpatialType::Vertex
                    && hasEmptyStar(boundaryCell)) {

                    switch (boundaryCell->cellType()) {
                    case CellType::KeyVertex:
                        isolatedKeyVertices.insert(boundaryCell);
                        break;
                    case CellType::InbetweenVertex:
                        isolatedInbetweenVertices.insert(boundaryCell);
                        break;
                    default:
                        break;
                    }
                    boundaryCell->isBeingDeleted_ = true;
                }
                if (!boundaryCell->isBeingDeleted_) {
                    boundaryCell->star_.removeOne(cell);
                    onNodeModified_(boundaryCell, NodeModificationFlag::StarChanged);
                }
            }
        }
    }

    // Deleting isolated inbetween vertices might indirectly cause key vertices
    // to become isolated, so we detect these in a second pass.
    //
    //       ke1
    // kv1 -------- kv2          Scenario: user hard deletes ie1
    //  |            |
    //  |iv1         | iv2        -> This directly makes iv1, iv2, and iv3 isolated
    //  |            |               (but does not directly make kv5 isolated, since
    //  |    ie1     kv5              the star of kv5 still contained iv2 and iv3)
    //  |            |
    //  |            | iv3
    //  |            |
    // kv3 ------- kv4
    //       ke2
    //
    if (deleteIsolatedVertices) {
        for (Node* inbetweenVertexNode : isolatedInbetweenVertices) {
            Cell* inbetweenVertex = inbetweenVertexNode->toCellUnchecked();
            for (Cell* keyVertex : inbetweenVertex->boundary()) {
                if (keyVertex->isBeingDeleted_) {
                    continue;
                }
                if (hasEmptyStar(keyVertex)) {
                    isolatedKeyVertices.insert(keyVertex);
                    keyVertex->isBeingDeleted_ = true;
                }
                else {
                    keyVertex->star_.removeOne(inbetweenVertex);
                    onNodeModified_(keyVertex, NodeModificationFlag::StarChanged);
                }
            }
        }
        nodesToDestroy.merge(isolatedKeyVertices);
        nodesToDestroy.merge(isolatedInbetweenVertices);
    }

    destroyNodes_(nodesToDestroy);
}

void Operations::softDelete(Node* /*node*/, bool /*deleteIsolatedVertices*/) {
    // TODO
    throw core::LogicError("Soft Delete topological operator is not implemented yet.");
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

    // TODO: define source operation
    KeyVertex* newKv = createKeyVertex(position, parentGroup, nextSibling, kv0->time());

    std::unordered_set<KeyVertex*> seen;
    for (KeyVertex* kv : kvs) {
        bool inserted = seen.insert(kv).second;
        if (inserted) {
            substitute_(kv, newKv);
            hardDelete(kv, false);
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
                result.append(t * s1.position() + (1 - t) * s0->position());
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
        vacomplex::KeyEdge* ke0 = kes[0];
        vacomplex::KeyEdge* ke1 = kes[1];
        vacomplex::KeyVertex* ke00 = ke0->startVertex();
        vacomplex::KeyVertex* ke01 = ke0->endVertex();
        vacomplex::KeyVertex* ke10 = ke1->startVertex();
        vacomplex::KeyVertex* ke11 = ke1->endVertex();
        bool isAnyLoop = (ke00 == ke01) || (ke10 == ke11);
        bool isBestDirectionKnown = false;
        bool direction1 = true;
        if (!isAnyLoop) {
            if ((ke00 == ke10) || (ke01 == ke11)) {
                // If the two edges have the same start vertex or the same
                // end vertex, we glue them in their intrinsic direction.
                direction1 = true;
                isBestDirectionKnown = true;
            }
            else if ((ke00 == ke11) || (ke01 == ke10)) {
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
        const geometry::StrokeSample2dArray& strokeSamples = ke->strokeSampling().samples();
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

    core::Array<vacomplex::KeyHalfedge> khs;
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
        const geometry::StrokeSample2dArray& strokeSamples = ke->strokeSampling().samples();
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
        const geometry::StrokeSample2dArray& strokeSamples = ke->strokeSampling().samples();
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
                    Int iSample = (iCostSample * costSampleStride + k) % numSamples;
                    Int iSampleR = numSamples - 1 - iSample;
                    const geometry::Vec2d& s0i = s0.getUnchecked(iSample);
                    costEjk += (s0i - s1.getUnchecked(iSample)).squaredLength();
                    costEjRk += (s0i - s1.getUnchecked(iSampleR)).squaredLength();
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

    core::Array<vacomplex::KeyHalfedge> khs;
    khs.reserve(n);
    for (Int i = 0; i < n; ++i) {
        khs.emplaceLast(kes[i], bestDirections[i]);
    }

    return glueKeyClosedEdges_(khs, bestUOffsets);
}

core::Array<KeyEdge*> Operations::unglueKeyEdges(KeyEdge* targetKe) {
    core::Array<KeyEdge*> result;
    if (countUses_(targetKe) <= 1) {
        result.append(targetKe);
        return result;
    }

    // TODO: handle temporal star.

    // Helper
    auto duplicateTargetKe = [this, targetKe, &result]() {
        KeyEdge* newKe = nullptr;
        std::unique_ptr<KeyEdgeData> dataDuplicate = targetKe->data()->clone();
        if (targetKe->isClosed()) {
            // TODO: define source operation
            newKe = createKeyClosedEdge(
                std::move(dataDuplicate),
                targetKe->parentGroup(),
                targetKe->nextSibling());
        }
        else {
            // TODO: define source operation
            newKe = createKeyOpenEdge(
                targetKe->startVertex(),
                targetKe->endVertex(),
                std::move(dataDuplicate),
                targetKe->parentGroup(),
                targetKe->nextSibling());
        }
        result.append(newKe);
        return newKe;
    };

    // Helper. Assumes targetKe's star will be cleared later.
    auto removeTargetKeFromBoundary = [this, targetKe](Cell* boundedCell) {
        boundedCell->boundary_.removeOne(targetKe);
        onBoundaryChanged_(boundedCell);
    };

    // Substitute targetKe by a duplicate in each of its use.
    // Note: star is copied for safety since it may be modified in loop.
    for (Cell* cell : core::Array(targetKe->star_)) {
        switch (cell->cellType()) {
        case CellType::KeyFace: {
            KeyFace* kf = cell->toKeyFaceUnchecked();
            for (KeyCycle& cycle : kf->cycles_) {
                if (cycle.steinerVertex_) {
                    continue;
                }
                KeyHalfedge first = cycle.halfedges().first();
                if (!first.isClosed()) {
                    for (KeyHalfedge& kheRef : cycle.halfedges_) {
                        if (kheRef.edge() == targetKe) {
                            KeyEdge* newKe = duplicateTargetKe();
                            kheRef = KeyHalfedge(newKe, kheRef.direction());
                            addToBoundary_(kf, newKe);
                        }
                    }
                }
                else if (first.edge() == targetKe) {
                    KeyEdge* newKe = duplicateTargetKe();
                    for (KeyHalfedge& kheRef : cycle.halfedges_) {
                        kheRef = KeyHalfedge(newKe, kheRef.direction());
                    }
                    addToBoundary_(kf, newKe);
                    // TODO: instead of having a copy of the edge used N times
                    // use a single edge with its geometry looped N times.
                    // See Boris Dalstein's thesis page 187.
                }
            }
            removeTargetKeFromBoundary(kf);
            break;
        }
        default:
            throw core::LogicError(
                "unglueKeyEdges() doesn't support temporal cells in edge star.");
        }
    }

    // Delete targetKe
    targetKe->star_.clear();
    hardDelete(targetKe, true);

    return result;
}

core::Array<KeyVertex*> Operations::unglueKeyVertices(
    KeyVertex* targetKv,
    core::Array<std::pair<core::Id, core::Array<KeyEdge*>>>& ungluedKeyEdges) {

    core::Array<KeyVertex*> result;
    if (countUses_(targetKv) <= 1) {
        result.append(targetKv);
        return result;
    }

    // TODO: handle temporal star.

    // Unglue incident key edges.
    for (Cell* cell : core::Array(targetKv->star_)) {
        switch (cell->cellType()) {
        case CellType::KeyEdge: {
            KeyEdge* ke = cell->toKeyEdgeUnchecked();
            core::Id id = ke->id();
            core::Array<KeyEdge*> a = unglueKeyEdges(ke);
            if (a.length() > 1) {
                ungluedKeyEdges.emplaceLast(id, std::move(a));
            }
            break;
        }
        default:
            break;
        }
    }

    // Helper
    auto duplicateTargetKv = [this, targetKv, &result]() {
        // TODO: define source operation
        KeyVertex* newKv = createKeyVertex(
            targetKv->position(),
            targetKv->parentGroup(),
            targetKv->nextSibling(),
            targetKv->time());
        result.append(newKv);
        return newKv;
    };

    // Helper. Assumes targetKv's star will be cleared later.
    auto removeTargetKvFromBoundary = [this, targetKv](Cell* boundedCell) {
        boundedCell->boundary_.removeOne(targetKv);
        onBoundaryChanged_(boundedCell);
    };

    // Helper. Assumes the replaced key vertex is `targetKv` and that
    // targetKv's star will be cleared later.
    auto substituteTargetKvAtStartOrEndOfKhe = //
        [this, targetKv, &removeTargetKvFromBoundary](
            const KeyHalfedge& khe, bool startVertex, KeyVertex* newKv) {
            //
            KeyEdge* ke = khe.edge();

            KeyVertex* endKv = nullptr;
            if (khe.direction() == startVertex) {
                endKv = ke->endVertex();
                ke->startVertex_ = newKv;
            }
            else {
                endKv = ke->startVertex();
                ke->endVertex_ = newKv;
            }

            if (endKv != targetKv) {
                removeTargetKvFromBoundary(ke);
            }

            addToBoundary_(ke, newKv);
        };

    // Substitute targetKv by a duplicate in each of its use.
    // Note: star is copied for safety since it may be modified in loop.
    for (Cell* cell : core::Array(targetKv->star_)) {
        switch (cell->cellType()) {
        case CellType::KeyEdge: {
            KeyEdge* ke = cell->toKeyEdgeUnchecked();
            bool hasFaceInStar = false;
            for (Cell* keStarCell : ke->star()) {
                if (keStarCell->cellType() == CellType::KeyFace) {
                    hasFaceInStar = true;
                    break;
                }
            }
            if (!hasFaceInStar) {
                if (ke->isStartVertex(targetKv)) {
                    KeyVertex* newKv = duplicateTargetKv();
                    ke->startVertex_ = newKv;
                    addToBoundary_(ke, newKv);
                }
                if (ke->isEndVertex(targetKv)) {
                    KeyVertex* newKv = duplicateTargetKv();
                    ke->endVertex_ = newKv;
                    addToBoundary_(ke, newKv);
                }
                removeTargetKvFromBoundary(ke);
            }
            break;
        }
        case CellType::KeyFace: {
            KeyFace* kf = cell->toKeyFaceUnchecked();
            for (KeyCycle& cycle : kf->cycles_) {
                if (cycle.steinerVertex()) {
                    if (cycle.steinerVertex() == targetKv) {
                        KeyVertex* newKv = duplicateTargetKv();
                        cycle.steinerVertex_ = newKv;
                        addToBoundary_(kf, newKv);
                    }
                    continue;
                }
                Int numHalfedges = cycle.halfedges_.length();
                // substitute at face corner uses
                for (Int i = 0; i < numHalfedges; ++i) {
                    KeyHalfedge& khe1 = cycle.halfedges_[i];
                    if (khe1.startVertex() == targetKv) {
                        Int previousKheIndex = (i - 1 + numHalfedges) % numHalfedges;
                        KeyHalfedge& khe0 = cycle.halfedges_[previousKheIndex];

                        // (?)---khe0-->(targetKv)---khe1-->(?)
                        KeyVertex* newKv = duplicateTargetKv();
                        substituteTargetKvAtStartOrEndOfKhe(khe0, false, newKv);
                        substituteTargetKvAtStartOrEndOfKhe(khe1, true, newKv);
                        // (?)---khe0-->( newKv  )---khe1-->(?)

                        addToBoundary_(kf, newKv);
                    }
                }
            }
            removeTargetKvFromBoundary(kf);
            break;
        }
        default:
            throw core::LogicError(
                "unglueKeyVertices() doesn't support temporal cells in edge star.");
        }
    }

    // Delete targetKv
    targetKv->star_.clear();
    hardDelete(targetKv, false);

    return result;
}

void Operations::moveToGroup(Node* node, Group* parentGroup, Node* nextSibling) {
    if (nextSibling) {
        insertNodeBeforeSibling_(node, nextSibling);
    }
    else {
        insertNodeAsLastChild_(node, parentGroup);
    }
}

void Operations::moveBelowBoundary(Node* node) {
    Cell* cell = node->toCell();
    if (!cell) {
        return;
    }
    const auto& boundary = cell->boundary();
    if (boundary.length() == 0) {
        // nothing to do.
        return;
    }
    // currently keeping the same parent
    Node* oldParentNode = cell->parent();
    Node* newParentNode = oldParentNode;
    if (!newParentNode) {
        // `boundary.length() > 0` previously checked.
        newParentNode = (*boundary.begin())->parent();
    }
    if (!newParentNode) {
        return;
    }
    Group* newParent = newParentNode->toGroupUnchecked();
    Node* nextSibling = newParent->firstChild();
    while (nextSibling) {
        bool found = false;
        for (Cell* boundaryCell : boundary) {
            if (nextSibling == boundaryCell) {
                found = true;
                break;
            }
        }
        if (found) {
            break;
        }
        nextSibling = nextSibling->nextSibling();
    }
    if (nextSibling) {
        insertNodeBeforeSibling_(node, nextSibling);
    }
    else {
        // all boundary cells are in another group
        // TODO: use set of ancestors of boundary cells
        insertNodeAsLastChild_(node, newParent);
    }
}

// dev note: update boundary before star

void Operations::setKeyVertexPosition(KeyVertex* kv, const geometry::Vec2d& pos) {
    if (pos == kv->position_) {
        // same data
        return;
    }

    kv->position_ = pos;

    onGeometryChanged_(kv);
}

void Operations::setKeyEdgeData(KeyEdge* ke, std::unique_ptr<KeyEdgeData>&& data) {

    KeyEdge* previousKe = data->keyEdge();
    VGC_ASSERT(!previousKe);

    ke->setData_(std::move(data));

    onGeometryChanged_(ke);
}

void Operations::setKeyEdgeSamplingQuality(
    KeyEdge* ke,
    geometry::CurveSamplingQuality quality) {

    if (quality == ke->samplingQuality_) {
        // same data
        return;
    }

    ke->samplingQuality_ = quality;
    dirtyMesh_(ke);
}

void Operations::onNodeCreated_(Node* node) {
    complex_->opDiff_.onNodeCreated(node);
}

void Operations::onNodeInserted_(
    Node* node,
    Node* oldParent,
    NodeInsertionType insertionType) {
    complex_->opDiff_.onNodeInserted(node, oldParent, insertionType);
}

void Operations::onNodeModified_(Node* node, NodeModificationFlags diffFlags) {
    complex_->opDiff_.onNodeModified(node, diffFlags);
}

void Operations::onNodePropertyModified_(Node* node, core::StringId name) {
    complex_->opDiff_.onNodePropertyModified(node, name);
}

void Operations::insertNodeBeforeSibling_(Node* node, Node* nextSibling) {
    Group* oldParent = node->parentGroup();
    Group* newParent = nextSibling->parentGroup();
    if (newParent->insertChildUnchecked(nextSibling, node)) {
        onNodeInserted_(node, oldParent, NodeInsertionType::BeforeSibling);
    }
}

void Operations::insertNodeAfterSibling_(Node* node, Node* previousSibling) {
    Group* oldParent = node->parentGroup();
    Group* newParent = previousSibling->parentGroup();
    Node* nextSibling = previousSibling->nextSibling();
    if (newParent->insertChildUnchecked(nextSibling, node)) {
        onNodeInserted_(node, oldParent, NodeInsertionType::AfterSibling);
    }
}

void Operations::insertNodeAsFirstChild_(Node* node, Group* parent) {
    Group* oldParent = node->parentGroup();
    Node* nextSibling = parent->firstChild();
    if (parent->insertChildUnchecked(nextSibling, node)) {
        onNodeInserted_(node, oldParent, NodeInsertionType::FirstChild);
    }
}

void Operations::insertNodeAsLastChild_(Node* node, Group* parent) {
    Group* oldParent = node->parentGroup();
    if (parent->appendChild(node)) {
        onNodeInserted_(node, oldParent, NodeInsertionType::LastChild);
    }
}

Node* Operations::findTopMost(core::Span<Node*> nodes) {
    // currently only looking under a single parent
    // TODO: tree-wide top most.
    if (nodes.isEmpty()) {
        return nullptr;
    }
    Node* node0 = nodes[0];
    Group* parent = node0->parentGroup();
    Node* topMostNode = parent->lastChild();
    while (topMostNode) {
        if (nodes.contains(topMostNode)) {
            break;
        }
        topMostNode = topMostNode->previousSibling();
    }
    return topMostNode;
}

// Assumes node has no children.
// maybe we should also handle star/boundary changes here
void Operations::destroyNode_(Node* node) {
    [[maybe_unused]] Group* group = node->toGroup();
    VGC_ASSERT(!group || group->numChildren() == 0);
    Group* parentGroup = node->parentGroup();
    core::Id nodeId = node->id();
    node->unparent();
    complex()->nodes_.erase(nodeId);
    complex_->opDiff_.onNodeDestroyed(nodeId);
    if (parentGroup) {
        complex_->opDiff_.onNodeModified(
            parentGroup, NodeModificationFlag::ChildrenChanged);
    }
}

// Assumes that all descendants of all `nodes` are also in `nodes`.
void Operations::destroyNodes_(const std::unordered_set<Node*>& nodes) {
    // debug check
    for (Node* node : nodes) {
        Group* group = node->toGroup();
        if (group) {
            for (Node* child : *group) {
                VGC_ASSERT(nodes.count(child)); // == contains
            }
        }
    }
    for (Node* node : nodes) {
        Group* parentGroup = node->parentGroup();
        node->unparent();
        complex_->opDiff_.onNodeDestroyed(node->id());
        if (parentGroup) {
            complex_->opDiff_.onNodeModified(
                parentGroup, NodeModificationFlag::ChildrenChanged);
        }
    }
    for (Node* node : nodes) {
        complex()->nodes_.erase(node->id());
    }
}

void Operations::onBoundaryChanged_(Cell* cell) {
    onNodeModified_(cell, NodeModificationFlag::BoundaryChanged);
    onBoundaryMeshChanged_(cell);
}

void Operations::onGeometryChanged_(Cell* cell) {
    onNodeModified_(cell, NodeModificationFlag::GeometryChanged);
    dirtyMesh_(cell);
}

void Operations::onPropertyChanged_(Cell* cell, core::StringId name) {
    onNodePropertyModified_(cell, name);
}

void Operations::onBoundaryMeshChanged_(Cell* cell) {
    onNodeModified_(cell, NodeModificationFlag::BoundaryMeshChanged);
    dirtyMesh_(cell);
}

void Operations::dirtyMesh_(Cell* cell) {
    if (cell->hasMeshBeenQueriedSinceLastDirtyEvent_) {
        cell->hasMeshBeenQueriedSinceLastDirtyEvent_ = false;
        cell->dirtyMesh_();
        onNodeModified_(cell, NodeModificationFlag::MeshChanged);
        for (Cell* starCell : cell->star()) {
            // No need for recursion since starCell.star() is a subset
            // of cell.star().
            onNodeModified_(starCell, NodeModificationFlag::BoundaryMeshChanged);
            if (starCell->hasMeshBeenQueriedSinceLastDirtyEvent_) {
                starCell->hasMeshBeenQueriedSinceLastDirtyEvent_ = false;
                starCell->dirtyMesh_();
                onNodeModified_(starCell, NodeModificationFlag::MeshChanged);
            }
        }
    }
}

void Operations::addToBoundary_(Cell* boundedCell, Cell* boundingCell) {
    if (!boundingCell) {
        throw core::LogicError("Cannot add null cell to boundary.");
    }
    else if (!boundedCell) {
        throw core::LogicError("Cannot modify the boundary of a null cell.");
    }
    else if (!boundedCell->boundary_.contains(boundingCell)) {
        boundedCell->boundary_.append(boundingCell);
        boundingCell->star_.append(boundedCell);
        onBoundaryChanged_(boundedCell);
        onNodeModified_(boundingCell, NodeModificationFlag::StarChanged);
    }
}

void Operations::addToBoundary_(FaceCell* face, const KeyCycle& cycle) {
    if (cycle.steinerVertex()) {
        // Steiner cycle
        addToBoundary_(face, cycle.steinerVertex());
    }
    else if (cycle.halfedges().first().isClosed()) {
        // Simple cycle
        addToBoundary_(face, cycle.halfedges().first().edge());
    }
    else {
        // Non-simple cycle
        for (const KeyHalfedge& halfedge : cycle.halfedges()) {
            addToBoundary_(face, halfedge.edge());
            addToBoundary_(face, halfedge.endVertex());
        }
    }
}

void Operations::substitute_(KeyVertex* oldVertex, KeyVertex* newVertex) {
    bool hasStar = false;
    for (Cell* starCell : oldVertex->star()) {
        *starCell->boundary_.find(oldVertex) = newVertex;
        newVertex->star_.append(starCell);
        starCell->substituteKeyVertex_(oldVertex, newVertex);
        onBoundaryChanged_(starCell);
        hasStar = true;
    }
    if (hasStar) {
        oldVertex->star_.clear();
        onNodeModified_(oldVertex, NodeModificationFlag::StarChanged);
        onNodeModified_(newVertex, NodeModificationFlag::StarChanged);
    }
}

void Operations::substitute_(const KeyHalfedge& oldKhe, const KeyHalfedge& newKhe) {

    KeyEdge* const oldKe = oldKhe.edge();
    KeyEdge* const newKe = newKhe.edge();
    bool hasStar = false;
    for (Cell* starCell : oldKe->star()) {
        *starCell->boundary_.find(oldKe) = newKe;
        newKe->star_.append(starCell);
        starCell->substituteKeyHalfedge_(oldKhe, newKhe);
        onBoundaryChanged_(starCell);
        hasStar = true;
    }
    if (hasStar) {
        oldKe->star_.clear();
        onNodeModified_(oldKe, NodeModificationFlag::StarChanged);
        onNodeModified_(newKe, NodeModificationFlag::StarChanged);
    }
}

void Operations::collectDependentNodes_(
    Node* node,
    std::unordered_set<Node*>& dependentNodes) {

    if (node->isGroup()) {
        // Delete all children of the group
        Group* group = node->toGroupUnchecked();
        for (Node* child : *group) {
            if (dependentNodes.insert(child).second) {
                collectDependentNodes_(child, dependentNodes);
            }
        }
    }
    else {
        // Delete all cells in the star of the cell
        Cell* cell = node->toCellUnchecked();
        for (Cell* starCell : cell->star()) {
            // No need for recursion since starCell.star() is a subset
            // of cell.star().
            dependentNodes.insert(starCell);
        }
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
        KeyEdgeData* kd = kh.edge()->data();
        if (!kd) {
            // missing data
            return nullptr;
        }
        khds.emplaceLast(kd, kh.direction());
    }
    std::unique_ptr<KeyEdgeData> newData = KeyEdgeData::fromGlueOpen(khds);
    VGC_ASSERT(newData && newData->stroke());

    std::array<geometry::Vec2d, 2> endPositions = newData->stroke()->endPositions();

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
        newData->snap(endVertexPosition, endVertexPosition);
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
        substitute_(kh, newKh);
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
        KeyEdgeData* kd = kh.edge()->data();
        if (!kd) {
            // missing data
            return nullptr;
        }
        khds.emplaceLast(kd, kh.direction());
    }

    Node* topMostEdge = findTopMost(edgeNodes);
    Group* parentGroup = topMostEdge->parentGroup();
    Node* nextSibling = topMostEdge->nextSibling();

    std::unique_ptr<KeyEdgeData> newData = KeyEdgeData::fromGlueClosed(khds, uOffsets);
    VGC_ASSERT(newData && newData->stroke());

    KeyEdge* newKe = createKeyClosedEdge(std::move(newData), parentGroup, nextSibling);

    KeyHalfedge newKh(newKe, true);
    for (const KeyHalfedge& kh : khs) {
        substitute_(kh, newKh);
        // It is important that no 2 halfedges refer to the same edge.
        hardDelete(kh.edge(), true);
    }

    return newKe;
}

Int Operations::countSteinerUses_(KeyVertex* kv) {
    Int count = 0;
    for (Cell* starCell : kv->star()) {
        KeyFace* kf = starCell->toKeyFace();
        if (!kf) {
            continue;
        }
        for (const KeyCycle& cycle : kf->cycles()) {
            if (cycle.steinerVertex() == kv) {
                ++count;
            }
        }
    }
    return count;
}

Int Operations::countUses_(KeyVertex* kv) {
    Int count = 0;
    for (Cell* starCell : kv->star()) {
        switch (starCell->cellType()) {
        case CellType::KeyEdge: {
            KeyEdge* ke = starCell->toKeyEdgeUnchecked();
            bool hasFaceInStar = false;
            for (Cell* keStarCell : ke->star()) {
                if (keStarCell->cellType() == CellType::KeyFace) {
                    hasFaceInStar = true;
                    break;
                }
            }
            if (!hasFaceInStar) {
                if (ke->isStartVertex(kv)) {
                    ++count;
                }
                if (ke->isEndVertex(kv)) {
                    ++count;
                }
            }
            break;
        }
        case CellType::KeyFace: {
            KeyFace* kf = starCell->toKeyFaceUnchecked();
            for (const KeyCycle& cycle : kf->cycles()) {
                if (cycle.steinerVertex()) {
                    if (cycle.steinerVertex() == kv) {
                        ++count;
                    }
                    continue;
                }
                for (const KeyHalfedge& khe : cycle.halfedges()) {
                    if (khe.startVertex() == kv) {
                        ++count;
                    }
                }
            }
            break;
        }
        default:
            break;
        }
    }
    return count;
}

Int Operations::countUses_(KeyEdge* ke) {
    Int count = 0;
    for (Cell* starCell : ke->star()) {
        KeyFace* kf = starCell->toKeyFace();
        if (!kf) {
            continue;
        }
        for (const KeyCycle& cycle : kf->cycles()) {
            if (cycle.steinerVertex()) {
                continue;
            }
            for (const KeyHalfedge& khe : cycle.halfedges()) {
                if (khe.edge() == ke) {
                    ++count;
                }
            }
        }
    }
    return count;
}

} // namespace vgc::vacomplex::detail
