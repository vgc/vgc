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

#include <algorithm> // std::reverse
#include <unordered_set>

#include <vgc/core/array.h>
#include <vgc/vacomplex/exceptions.h>
#include <vgc/vacomplex/keyedgedata.h>
#include <vgc/vacomplex/logcategories.h>

namespace vgc::vacomplex {

namespace detail {

core::Array<Cell*> star(core::ConstSpan<Cell*> cells) {
    core::Array<Cell*> result;
    for (Cell* c : cells) {
        for (Cell* sc : c->star()) {
            if (!result.contains(sc) && !cells.contains(sc)) {
                result.append(sc);
            }
        }
    }
    return result;
};

core::Array<Cell*> opening(core::ConstSpan<Cell*> cells) {
    core::Array<Cell*> result(cells);
    for (Cell* c : cells) {
        for (Cell* sc : c->star()) {
            if (!result.contains(sc)) {
                result.append(sc);
            }
        }
    }
    return result;
};

core::Array<Cell*> closure(core::ConstSpan<Cell*> cells) {
    core::Array<Cell*> result(cells);
    for (Cell* c : cells) {
        for (Cell* bc : c->boundary()) {
            if (!result.contains(bc)) {
                result.append(bc);
            }
        }
    }
    return result;
};

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
        for (const CreatedNodeInfo& info : complex->opDiff_.createdNodes_) {
            Cell* cell = info.node()->toCell();
            if (cell) {
                switch (cell->cellType()) {
                case CellType::KeyEdge: {
                    KeyEdge* ke = cell->toKeyEdgeUnchecked();
                    if (ke->data()) {
                        ke->data()->finalizeConcat();
                    }
                    break;
                }
                case CellType::KeyFace: {
                    KeyFace* kf = cell->toKeyFaceUnchecked();
                    kf->data().finalizeConcat();
                    break;
                }
                default:
                    break;
                }
            }
        }
        complex->nodesChanged().emit(complex->opDiff_);
        complex->opDiff_.clear();
        complex->temporaryCellSet_.clear();
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

    // First collect all dependents

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
            for (Cell* boundaryCell : cell->boundary().copy()) {
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
            cell->star_.clear();
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

    core::Array<Node*> nodesToDestroyArray(nodesToDestroy);
    destroyNodes_(nodesToDestroyArray);
}

/*

softDeleteRec<Dim, MaxDim>(Array<Cell<Dim>> cells)

    for (Int i = Dim + 1; i < MaxDim && result; ++i) {
        Array<Cell<Dim>> notuncut;
        for (c : cells) {
            if (canUncut(c)) {
                uncut(c);
            }
            else {
                notuncut.append(c);
            }
        }
        Array<Cell<Dim+1>> directStar = directStar(notuncut);
        softDeleteRec<Dim, i>(directStar);
    }
}

*/

namespace {

class ClassifiedCells {
public:
    ClassifiedCells() noexcept = default;

    ClassifiedCells(core::ConstSpan<Cell*> cells) {
        insert(cells.begin(), cells.end());
    }

    bool insert(Cell* cell) {
        return insert_(cell);
    }

    template<typename Iter>
    void insert(Iter first, Iter last) {
        while (first != last) {
            insert_(*first);
            ++first;
        }
    }

    void insert(const CellRangeView& rangeView) {
        for (Cell* cell : rangeView) {
            insert_(cell);
        }
    }

    void clear() {
        kvs_.clear();
        kes_.clear();
        kfs_.clear();
        ivs_.clear();
        ies_.clear();
        ifs_.clear();
    }

    core::Array<KeyVertex*>& kvs() {
        return kvs_;
    }

    const core::Array<KeyVertex*>& kvs() const {
        return kvs_;
    }

    core::Array<KeyEdge*>& kes() {
        return kes_;
    }

    const core::Array<KeyEdge*>& kes() const {
        return kes_;
    }

    core::Array<KeyFace*>& kfs() {
        return kfs_;
    }

    const core::Array<KeyFace*>& kfs() const {
        return kfs_;
    }

    core::Array<InbetweenVertex*>& ivs() {
        return ivs_;
    }

    const core::Array<InbetweenVertex*>& ivs() const {
        return ivs_;
    }

    core::Array<InbetweenEdge*>& ies() {
        return ies_;
    }

    const core::Array<InbetweenEdge*>& ies() const {
        return ies_;
    }

    core::Array<InbetweenFace*>& ifs() {
        return ifs_;
    }

    const core::Array<InbetweenFace*>& ifs() const {
        return ifs_;
    }

private:
    core::Array<KeyVertex*> kvs_;
    core::Array<KeyEdge*> kes_;
    core::Array<KeyFace*> kfs_;
    core::Array<InbetweenVertex*> ivs_;
    core::Array<InbetweenEdge*> ies_;
    core::Array<InbetweenFace*> ifs_;

    bool insert_(Cell* cell) {
        switch (cell->cellType()) {
        case vacomplex::CellType::KeyVertex: {
            KeyVertex* kv = cell->toKeyVertexUnchecked();
            if (!kvs_.contains(kv)) {
                kvs_.append(kv);
                return true;
            }
            break;
        }
        case vacomplex::CellType::KeyEdge: {
            KeyEdge* ke = cell->toKeyEdgeUnchecked();
            if (!kes_.contains(ke)) {
                kes_.append(ke);
                return true;
            }
            break;
        }
        case vacomplex::CellType::KeyFace: {
            KeyFace* kf = cell->toKeyFaceUnchecked();
            if (!kfs_.contains(kf)) {
                kfs_.append(kf);
                return true;
            }
            break;
        }
        case vacomplex::CellType::InbetweenVertex: {
            InbetweenVertex* iv = cell->toInbetweenVertexUnchecked();
            if (!ivs_.contains(iv)) {
                ivs_.append(iv);
                return true;
            }
            break;
        }
        case vacomplex::CellType::InbetweenEdge: {
            InbetweenEdge* ie = cell->toInbetweenEdgeUnchecked();
            if (!ies_.contains(ie)) {
                ies_.append(ie);
                return true;
            }
            break;
        }
        case vacomplex::CellType::InbetweenFace: {
            InbetweenFace* if_ = cell->toInbetweenFaceUnchecked();
            if (!ifs_.contains(if_)) {
                ifs_.append(if_);
                return true;
            }
            break;
        }
        }
        return false;
    }
};

class ResolvedSelection {
public:
    ResolvedSelection(core::ConstSpan<Node*> nodes) {
        for (Node* node : nodes) {
            if (node->isGroup()) {
                Group* group = node->toGroupUnchecked();
                visitGroup_(group);
            }
        }
        for (Node* node : nodes) {
            if (node->isCell()) {
                Cell* cell = node->toCellUnchecked();
                if (!cells_.contains(cell)) {
                    cells_.append(cell);
                    topCells_.append(cell);
                }
            }
        }
    }

    const core::Array<Group*>& groups() const {
        return groups_;
    }
    const core::Array<Cell*>& cells() const {
        return cells_;
    }

    const core::Array<Group*>& topGroups() const {
        return topGroups_;
    }
    const core::Array<Cell*>& topCells() const {
        return topCells_;
    }

private:
    core::Array<Group*> groups_;
    core::Array<Cell*> cells_;

    core::Array<Group*> topGroups_;
    core::Array<Cell*> topCells_;

    void visitChildNode_(Node* node) {
        if (node->isGroup()) {
            Group* group = node->toGroupUnchecked();
            visitGroup_(group);
        }
        else {
            Cell* cell = node->toCellUnchecked();
            if (cells_.contains(cell)) {
                topCells_.removeOne(cell);
            }
            else {
                cells_.append(cell);
            }
        }
    }

    void visitGroup_(Group* group) {
        if (groups_.contains(group)) {
            topGroups_.removeOne(group);
        }
        else {
            groups_.append(group);
            topGroups_.append(group);
            for (Node* child : *group) {
                visitChildNode_(child);
            }
        }
    }
};

} // namespace

// deleteIsolatedVertices is not supported yet
void Operations::softDelete(
    core::ConstSpan<Node*> nodes,
    bool /*deleteIsolatedVertices*/) {

    if (nodes.isEmpty()) {
        return;
    }

    constexpr bool smoothJoins = false;

    // classifyCells
    ClassifiedCells classifiedStar;
    auto classifyStar = [&](auto& cells) {
        classifiedStar.clear();
        for (const auto& cell : cells) {
            classifiedStar.insert(cell->star());
        }
    };

    // cells is updated to contain only cells that could not be uncut.
    auto uncutCells = [&](auto& cells) {
        using CellType = std::remove_pointer_t<
            typename core::RemoveCVRef<decltype(cells)>::value_type>;
        for (CellType*& cell : cells) {
            bool wasUncut = false;
            if constexpr (std::is_same_v<CellType, KeyVertex>) {
                wasUncut = uncutAtKeyVertex(cell, smoothJoins).success;
                VGC_DEBUG_TMP("uncutAtKeyVertex->{}", wasUncut);
            }
            if constexpr (std::is_same_v<CellType, KeyEdge>) {
                wasUncut = uncutAtKeyEdge(cell).success;
                VGC_DEBUG_TMP("uncutAtKeyEdge->{}", wasUncut);
            }
            if (wasUncut) {
                cell = nullptr;
            }
        }
        cells.removeAll(nullptr);
    };

    auto hardDeleteCells = [=](auto& cells) {
        for (const auto& cell : cells) {
            // Note: deleteIsolatedVertices could remove cells that are
            // in `cells` and it would cause a crash.
            hardDelete(cell, false);
        }
        cells.clear();
    };

    // Resolve selection
    ResolvedSelection selection(nodes);
    ClassifiedCells selectionCells(selection.cells());

    Complex* complex = nodes.first()->complex();
    complex->temporaryCellSet_ = closure(opening(selection.cells()));

    // Faces
    {
        core::Array<KeyFace*> kfs(selectionCells.kfs());
        if (!kfs.isEmpty()) {
            uncutCells(kfs);
        }
        hardDeleteCells(kfs);
    }

    // Edges
    {
        core::Array<KeyEdge*> kes(selectionCells.kes());
        if (!kes.isEmpty()) {
            uncutCells(kes);
        }
        if (!kes.isEmpty()) {
            classifyStar(kes);
            core::Array<KeyFace*>& kfs = classifiedStar.kfs();
            uncutCells(kfs);
            uncutCells(kes);
        }
        hardDeleteCells(kes);
    }

    // Vertices
    {
        core::Array<KeyVertex*> kvs(selectionCells.kvs());
        if (!kvs.isEmpty()) {
            uncutCells(kvs);
        }
        if (!kvs.isEmpty()) {
            classifyStar(kvs);
            core::Array<KeyEdge*>& kes = classifiedStar.kes();
            uncutCells(kes);
            uncutCells(kvs);
        }
        if (!kvs.isEmpty()) {
            classifyStar(kvs);
            core::Array<KeyFace*>& kfs = classifiedStar.kfs();
            uncutCells(kfs);
            core::Array<KeyEdge*>& kes = classifiedStar.kes();
            uncutCells(kes);
            uncutCells(kvs);
        }
        hardDeleteCells(kvs);
    }

    // Groups
    for (Group* g : selection.topGroups()) {
        destroyChildlessNode_(g);
    }

    // Check closure for residual cells to remove such as isolated vertices.
    ClassifiedCells residualCells(complex->temporaryCellSet_);
    for (KeyVertex* kv : residualCells.kvs()) {
        if (kv->star().isEmpty()) {
            destroyChildlessNode_(kv);
        }
    }
}

core::Array<KeyCell*> Operations::simplify(
    core::Span<KeyVertex*> kvs,
    core::Span<KeyEdge*> kes,
    bool smoothJoins) {

    Complex* complex = nullptr;
    if (kvs.isEmpty()) {
        if (kes.isEmpty()) {
            return {};
        }
        complex = kes.first()->complex();
    }
    else {
        complex = kvs.first()->complex();
    }

    core::Array<KeyCell*> result;

    std::unordered_set<core::Id> resultEdgeIds;
    std::unordered_set<core::Id> resultFaceIds;

    for (KeyEdge* ke : kes) {
        UncutAtKeyEdgeResult uncutFace = uncutAtKeyEdge(ke);
        if (uncutFace.success) {
            resultFaceIds.erase(uncutFace.removedKfId1);
            resultFaceIds.erase(uncutFace.removedKfId2);
            if (uncutFace.resultKf) {
                resultFaceIds.insert(uncutFace.resultKf->id());
            }
        }
        else { // uncut failed
            resultEdgeIds.insert(ke->id());
        }
    }

    for (KeyVertex* kv : kvs) {
        UncutAtKeyVertexResult uncutEdge = uncutAtKeyVertex(kv, smoothJoins);
        if (uncutEdge.success) {
            resultEdgeIds.erase(uncutEdge.removedKeId1);
            resultEdgeIds.erase(uncutEdge.removedKeId2);
            if (uncutEdge.resultKe) {
                resultEdgeIds.insert(uncutEdge.resultKe->id());
            }
        }
        else {
            // uncut failed, return the vertex
            result.append(kv);
        }
    }

    for (core::Id id : resultEdgeIds) {
        Cell* cell = complex->findCell(id);
        if (cell) {
            KeyEdge* ke = cell->toKeyEdge();
            if (ke) {
                result.append(ke);
            }
        }
    }

    return result;
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

    // Substitute targetKe by a duplicate in each of its use.
    // Note: star is copied for safety since it may be modified in loop.
    for (Cell* cell : targetKe->star().copy()) {
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
            removeFromBoundary_(kf, targetKe);
            break;
        }
        default:
            throw core::LogicError(
                "unglueKeyEdges() doesn't support temporal cells in edge star.");
        }
    }

    // Delete targetKe
    hardDelete(targetKe);

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
    for (Cell* cell : targetKv->star().copy()) {
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

    // Helper. Assumes the replaced key vertex is `targetKv`.
    auto substituteTargetKvAtStartOrEndOfKhe = //
        [this, targetKv](const KeyHalfedge& khe, bool startVertex, KeyVertex* newKv) {
            //
            KeyEdge* ke = khe.edge();

            KeyVertex* otherEndKv = nullptr;
            if (khe.direction() == startVertex) {
                otherEndKv = ke->endVertex();
                ke->startVertex_ = newKv;
            }
            else {
                otherEndKv = ke->startVertex();
                ke->endVertex_ = newKv;
            }

            if (otherEndKv != targetKv) {
                removeFromBoundary_(ke, targetKv);
            }

            addToBoundary_(ke, newKv);
        };

    // Substitute targetKv by a duplicate in each of its use.
    // Note: star is copied for safety since it may be modified in loop.
    for (Cell* cell : targetKv->star().copy()) {
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
                removeFromBoundary_(ke, targetKv);
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
            removeFromBoundary_(kf, targetKv);
            break;
        }
        default:
            throw core::LogicError(
                "unglueKeyVertices() doesn't support temporal cells in edge star.");
        }
    }

    // Delete targetKv
    hardDelete(targetKv);

    return result;
}

UncutAtKeyVertexResult
Operations::uncutAtKeyVertex(KeyVertex* targetKv, bool smoothJoin) {

    UncutAtKeyVertexResult result = {};

    UncutAtKeyVertexInfo_ info = prepareUncutAtKeyVertex_(targetKv);
    if (!info.isValid) {
        return result;
    }

    KeyEdge* newKe = nullptr;

    if (info.kf) {
        info.kf->cycles_.removeAt(info.cycleIndex);
        removeFromBoundary_(info.kf, targetKv);

        result.success = true;
    }
    else if (info.khe1.edge() == info.khe2.edge()) {
        // Transform open edge into closed edge.
        KeyEdge* oldKe = info.khe1.edge();

        std::unique_ptr<KeyEdgeData> newData = oldKe->stealData_();
        newData->isClosed_ = true;
        newData->stroke_->close(smoothJoin);

        newKe = createKeyClosedEdge(
            std::move(newData), oldKe->parentGroup(), oldKe->nextSibling());
        result.resultKe = newKe;

        KeyHalfedge oldKhe(oldKe, true);
        KeyHalfedge newKhe(newKe, true);
        substitute_(oldKhe, newKhe);

        // Since substitute expects end vertices to be the same,
        // it didn't remove our targetKv from its star. So
        // we do it manually here.
        for (Cell* cell : targetKv->star().copy()) {
            removeFromBoundary_(cell, targetKv);
        }

        // Delete oldKe and targetKv
        result.removedKeId1 = oldKe->id();
        hardDelete(oldKe);

        result.success = true;
    }
    else {
        KeyEdgeData* ked1 = info.khe1.edge()->data();
        KeyEdgeData* ked2 = info.khe2.edge()->data();
        KeyVertex* kv1 = info.khe1.startVertex();
        KeyVertex* kv2 = info.khe2.endVertex();
        bool dir1 = info.khe1.direction();
        bool dir2 = info.khe2.direction();
        if (!ked1 || !ked2) {
            // missing geometry
            return result;
        }
        KeyHalfedgeData khd1(ked1, dir1);
        KeyHalfedgeData khd2(ked2, dir2);
        std::unique_ptr<KeyEdgeData> concatData =
            ked1->fromConcatStep(khd1, khd2, smoothJoin);

        std::array<Node*, 2> kes = {info.khe1.edge(), info.khe2.edge()};
        Node* bottomMostEdge = findBottomMost(kes);
        Group* parentGroup = bottomMostEdge->parentGroup();
        Node* nextSibling = bottomMostEdge;

        newKe =
            createKeyOpenEdge(kv1, kv2, std::move(concatData), parentGroup, nextSibling);
        result.resultKe = newKe;

        for (Cell* starCell : info.khe1.edge()->star().copy()) {
            KeyFace* kf = starCell->toKeyFace();
            if (!kf) {
                continue;
            }

            // Substitute.
            // we know that face cycles never uses khe1 or khe2 independently,
            // but always both consecutively.
            for (KeyCycle& cycle : kf->cycles_) {
                if (cycle.steinerVertex()) {
                    continue;
                }
                auto it = cycle.halfedges_.begin();
                while (it != cycle.halfedges_.end()) {
                    KeyHalfedge& khe = *it;
                    if (khe.endVertex() == targetKv) {
                        bool dir = khe.direction() == info.khe1.direction();
                        khe = KeyHalfedge(newKe, dir);
                        ++it;
                    }
                    else if (khe.startVertex() == targetKv) {
                        it = cycle.halfedges_.erase(it);
                    }
                    else {
                        ++it;
                    }
                }
            }

            removeFromBoundary_(kf, info.khe1.edge());
            removeFromBoundary_(kf, info.khe2.edge());
            removeFromBoundary_(kf, targetKv);
            addToBoundary_(kf, newKe);
        }

        // Delete khe1, khe2 and targetKv
        result.removedKeId1 = info.khe1.edge()->id();
        result.removedKeId2 = info.khe2.edge()->id();

        hardDelete(info.khe1.edge());
        hardDelete(info.khe2.edge());

        result.success = true;
    }

    if (result.success) {
        VGC_ASSERT(targetKv->star().isEmpty());
        hardDelete(targetKv);
    }

    return result;
}

UncutAtKeyEdgeResult Operations::uncutAtKeyEdge(KeyEdge* targetKe) {

    UncutAtKeyEdgeResult result = {};

    UncutAtKeyEdgeInfo_ info = prepareUncutAtKeyEdge_(targetKe);
    if (!info.isValid) {
        return result;
    }

    if (targetKe->isClosed()) {
        if (info.kf1 == info.kf2) {
            KeyFace* kf = info.kf1;

            kf->cycles_.removeIf([targetKe](const KeyCycle& cycle) {
                return !cycle.steinerVertex()
                       && cycle.halfedges().first().edge() == targetKe;
            });
            removeFromBoundary_(kf, targetKe);

            hardDelete(targetKe);

            result.resultKf = kf;
            result.success = true;
        }
        else {
            // make new face from 2 input faces
            core::Array<KeyCycle> newCycles = {};

            for (KeyCycle& cycle : info.kf1->cycles_) {
                if (cycle.steinerVertex()) {
                    newCycles.append(cycle);
                }
                else if (cycle.halfedges_.first().edge() != targetKe) {
                    newCycles.append(cycle);
                }
            }

            std::array<Node*, 2> kfs = {info.kf1, info.kf2};
            Node* bottomMostFace = findBottomMost(kfs);
            Group* parentGroup = bottomMostFace->parentGroup();
            Node* nextSibling = bottomMostFace;

            KeyFace* newKf =
                createKeyFace(std::move(newCycles), parentGroup, nextSibling);

            KeyFaceData::assignFromConcatStep(
                newKf->data(), info.kf1->data(), info.kf2->data());

            result.removedKfId1 = info.kf1->id();
            result.removedKfId2 = info.kf2->id();
            hardDelete(info.kf1);
            hardDelete(info.kf2);

            result.resultKf = newKf;
            result.success = true;
        }
    }
    else { // key open edge
        if (info.kf1 == info.kf2) {
            KeyFace* kf = info.kf1;

            if (info.cycleIndex1 == info.cycleIndex2) {
                KeyCycle& cycle = kf->cycles_[info.cycleIndex1];
                Int i1 = info.componentIndex1;
                Int i2 = info.componentIndex2;
                KeyPath p1 = subPath(cycle, i1 + 1, i2);
                KeyPath p2 = subPath(cycle, i2 + 1, i1);
                bool d1 = cycle.halfedges_[i1].direction();
                bool d2 = cycle.halfedges_[i2].direction();
                if (d1 == d2) {
                    p2.reverse();
                    kf->cycles_.append(KeyCycle(concatPath(p1, p2)));
                }
                else {
                    kf->cycles_.append(KeyCycle(std::move(p1)));
                    kf->cycles_.append(KeyCycle(std::move(p2)));
                }
                kf->cycles_.removeAt(info.cycleIndex1);
                removeFromBoundary_(kf, targetKe);
            }
            else {
                KeyCycle& cycle1 = kf->cycles_[info.cycleIndex1];
                KeyCycle& cycle2 = kf->cycles_[info.cycleIndex2];
                Int i1 = info.componentIndex1;
                Int i2 = info.componentIndex2;
                KeyPath p1 = subPath(cycle1, i1 + 1, i1);
                KeyPath p2 = subPath(cycle2, i2 + 1, i2);
                bool d1 = cycle1.halfedges_[i1].direction();
                bool d2 = cycle2.halfedges_[i2].direction();
                if (d1 == d2) {
                    p2.reverse();
                }

                kf->cycles_.append(KeyCycle(concatPath(p1, p2)));
                auto indices = std::minmax(info.cycleIndex1, info.cycleIndex2);
                kf->cycles_.removeAt(indices.second);
                kf->cycles_.removeAt(indices.first);
                removeFromBoundary_(kf, targetKe);
            }

            result.resultKf = kf;
            result.success = true;
        }
        else {
            KeyFace* kf1 = info.kf1;
            KeyFace* kf2 = info.kf2;
            KeyCycle& cycle1 = kf1->cycles_[info.cycleIndex1];
            KeyCycle& cycle2 = kf2->cycles_[info.cycleIndex2];
            Int i1 = info.componentIndex1;
            Int i2 = info.componentIndex2;
            KeyPath p1 = subPath(cycle1, i1 + 1, i1);
            KeyPath p2 = subPath(cycle2, i2 + 1, i2);
            bool d1 = cycle1.halfedges_[i1].direction();
            bool d2 = cycle2.halfedges_[i2].direction();
            if (d1 == d2) {
                p2.reverse();
            }

            core::Array<KeyCycle> newCycles;

            KeyCycle newCycle(concatPath(p1, p2));
            newCycles.append(std::move(newCycle));

            for (Int j = 0; j < kf1->cycles_.length(); ++j) {
                if (j != info.cycleIndex1) {
                    newCycles.append(kf1->cycles_[j]);
                }
            }
            for (Int j = 0; j < kf2->cycles_.length(); ++j) {
                if (j != info.cycleIndex2) {
                    newCycles.append(kf2->cycles_[j]);
                }
            }

            std::array<Node*, 2> kfs = {info.kf1, info.kf2};
            Node* bottomMostFace = findBottomMost(kfs);
            Group* parentGroup = bottomMostFace->parentGroup();
            Node* nextSibling = bottomMostFace;

            KeyFace* newKf =
                createKeyFace(std::move(newCycles), parentGroup, nextSibling);

            KeyFaceData::assignFromConcatStep(
                newKf->data(), info.kf1->data(), info.kf2->data());

            result.removedKfId1 = info.kf1->id();
            result.removedKfId2 = info.kf2->id();
            hardDelete(info.kf1);
            hardDelete(info.kf2);

            result.resultKf = newKf;
            result.success = true;
        }
    }

    hardDelete(targetKe);
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
    auto boundary = cell->boundary();
    if (boundary.isEmpty()) {
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

/* static */
Node* Operations::findTopMost(core::ConstSpan<Node*> nodes) {
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

/* static */
Node* Operations::findBottomMost(core::ConstSpan<Node*> nodes) {
    // currently only looking under a single parent
    // TODO: tree-wide bottom most.
    if (nodes.isEmpty()) {
        return nullptr;
    }
    Node* node0 = nodes[0];
    Group* parent = node0->parentGroup();
    Node* bottomMostNode = parent->firstChild();
    while (bottomMostNode) {
        if (nodes.contains(bottomMostNode)) {
            break;
        }
        bottomMostNode = bottomMostNode->nextSibling();
    }
    return bottomMostNode;
}

// Assumes node has no children.
// maybe we should also handle star/boundary changes here
void Operations::destroyChildlessNode_(Node* node) {
    [[maybe_unused]] Group* group = node->toGroup();
    if (group) {
        VGC_ASSERT(group->numChildren() == 0);
    }
    Group* parentGroup = node->parentGroup();
    if (parentGroup) {
        node->unparent();
        complex_->opDiff_.onNodeModified(
            parentGroup, NodeModificationFlag::ChildrenChanged);
    }
    if (node->isCell()) {
        complex_->temporaryCellSet_.removeOne(node->toCellUnchecked());
    }
    complex_->opDiff_.onNodeDestroyed(node->id());
    complex_->nodes_.erase(node->id());
}

// Assumes that all descendants of all `nodes` are also in `nodes`.
void Operations::destroyNodes_(core::ConstSpan<Node*> nodes) {
    // debug check
    for (Node* node : nodes) {
        Group* group = node->toGroup();
        if (group) {
            for (Node* child : *group) {
                VGC_ASSERT(nodes.contains(child));
            }
        }
    }
    for (Node* node : nodes) {
        Group* parentGroup = node->parentGroup();
        if (parentGroup) {
            node->unparent();
            complex_->opDiff_.onNodeModified(
                parentGroup, NodeModificationFlag::ChildrenChanged);
        }
    }
    for (Node* node : nodes) {
        complex_->opDiff_.onNodeDestroyed(node->id());
        complex_->nodes_.erase(node->id());
    }
}

void Operations::onGeometryChanged_(Cell* cell) {
    onNodeModified_(cell, NodeModificationFlag::GeometryChanged);
    dirtyMesh_(cell);
}

void Operations::onPropertyChanged_(Cell* cell, core::StringId name) {
    onNodePropertyModified_(cell, name);
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
        onNodeModified_(
            boundedCell,
            {NodeModificationFlag::BoundaryChanged,
             NodeModificationFlag::BoundaryMeshChanged});
        onNodeModified_(boundingCell, NodeModificationFlag::StarChanged);
        dirtyMesh_(boundedCell);
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

void Operations::removeFromBoundary_(Cell* boundedCell, Cell* boundingCell) {
    if (!boundingCell) {
        throw core::LogicError("Cannot remove null cell from boundary.");
    }
    else if (!boundedCell) {
        throw core::LogicError("Cannot modify the boundary of a null cell.");
    }
    else if (boundedCell->boundary_.contains(boundingCell)) {
        boundedCell->boundary_.removeOne(boundingCell);
        boundingCell->star_.removeOne(boundedCell);
        onNodeModified_(
            boundedCell,
            {NodeModificationFlag::BoundaryChanged,
             NodeModificationFlag::BoundaryMeshChanged});
        onNodeModified_(boundingCell, NodeModificationFlag::StarChanged);
        dirtyMesh_(boundedCell);
    }
}

void Operations::substitute_(KeyVertex* oldVertex, KeyVertex* newVertex) {
    if (newVertex != oldVertex) {
        for (Cell* starCell : oldVertex->star().copy()) {
            starCell->substituteKeyVertex_(oldVertex, newVertex);
            removeFromBoundary_(starCell, oldVertex);
            addToBoundary_(starCell, newVertex);
        }
    }
}

// it assumes end vertices are the same!
void Operations::substitute_(const KeyHalfedge& oldKhe, const KeyHalfedge& newKhe) {
    KeyEdge* const oldKe = oldKhe.edge();
    KeyEdge* const newKe = newKhe.edge();
    auto star = oldKe->star().copy();
    for (Cell* starCell : star) {
        starCell->substituteKeyHalfedge_(oldKhe, newKhe);
    }
    if (newKe != oldKe) {
        for (Cell* starCell : star) {
            removeFromBoundary_(starCell, oldKe);
            addToBoundary_(starCell, newKe);
        }
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

KeyPath Operations::subPath(const KeyCycle& cycle, Int first, Int last) {
    if (cycle.steinerVertex()) {
        return KeyPath(cycle.steinerVertex());
    }
    if (first == last) {
        KeyVertex* singleVertex = cycle.halfedges_[first].startVertex();
        return KeyPath(singleVertex);
    }
    else {
        Int n = cycle.halfedges_.length();
        first = ((first % n) + n) % n;
        last = ((last % n) + n) % n;
        core::Array<KeyHalfedge> halfedges = {};
        for (Int i = first; i != last; i = (i + 1) % n) {
            halfedges.append(cycle.halfedges_[i]);
        }
        return KeyPath(std::move(halfedges));
    }
}

KeyPath Operations::concatPath(const KeyPath& p1, const KeyPath& p2) {
    if (p1.singleVertex()) {
        return p2;
    }
    else if (p2.singleVertex()) {
        return p1;
    }
    else {
        core::Array<KeyHalfedge> halfedges = {};
        halfedges.reserve(p1.halfedges_.length());
        halfedges.extend(p1.halfedges_);
        halfedges.extend(p2.halfedges_);
        return KeyPath(std::move(halfedges));
    }
}

// Note: Uncut does not yet support incident inbetween cells. As a
// workaround, we do nothing, as if uncutting here isn't possible, even
// though maybe in theory it is. In the future, we should handle the cases
// where uncutting is actually possible despite the presence of incident
// inbetween cells.
Operations::UncutAtKeyVertexInfo_ Operations::prepareUncutAtKeyVertex_(KeyVertex* kv) {
    UncutAtKeyVertexInfo_ result = {};

    for (Cell* starCell : kv->star()) {
        switch (starCell->cellType()) {
        case CellType::KeyEdge: {
            KeyEdge* ke = starCell->toKeyEdgeUnchecked();
            if (ke->isStartVertex(kv)) {
                if (!result.khe1.edge()) {
                    result.khe1 = KeyHalfedge(ke, false);
                }
                else if (!result.khe2.edge()) {
                    result.khe2 = KeyHalfedge(ke, true);
                }
                else {
                    // Cannot uncut if kv is used more than twice as edge vertex.
                    return result;
                }
            }
            if (ke->isEndVertex(kv)) {
                if (!result.khe1.edge()) {
                    result.khe1 = KeyHalfedge(ke, true);
                }
                else if (!result.khe2.edge()) {
                    result.khe2 = KeyHalfedge(ke, false);
                }
                else {
                    // Cannot uncut if kv is used more than twice as edge vertex.
                    return result;
                }
            }
            break;
        }
        case CellType::KeyFace: {
            KeyFace* kf = starCell->toKeyFaceUnchecked();
            Int cycleIndex = -1;
            for (const KeyCycle& cycle : kf->cycles()) {
                ++cycleIndex;
                if (cycle.steinerVertex() == kv) {
                    if (result.kf) {
                        // Cannot uncut if kv is used more than once as steiner vertex.
                        return result;
                    }
                    result.kf = kf;
                    result.cycleIndex = cycleIndex;
                }
            }
            break;
        }
        case CellType::InbetweenVertex: {
            //InbetweenVertex* iv = starCell->toInbetweenVertexUnchecked();
            // Currently not supported.
            return result;
            break;
        }
        default:
            break;
        }
    }

    if (result.khe1.edge()) {
        if (!result.kf && result.khe2.edge()) {
            if (result.khe1.edge() != result.khe2.edge()) {
                // If edges are different:
                // (inverse op: cut open edge)
                //
                //                     ┌─←─┐
                //                     │   C
                // o ───A──→ X ───B──→ o ──┘
                //
                // Uncutting at X means replacing the chain AB by D.
                // Thus the cycle B*A*ABC would become D*DC but
                // the cycle B*BC would not be representable anymore.
                //
                // In other words, we want the edges to always be used
                // consecutively in the cycles they are part of.
                //
                for (Cell* starCell : kv->star()) {
                    KeyFace* kf = starCell->toKeyFace();
                    if (!kf) {
                        continue;
                    }
                    for (const KeyCycle& cycle : kf->cycles()) {
                        if (cycle.steinerVertex()) {
                            continue;
                        }
                        KeyEdge* previousKe = cycle.halfedges().last().edge();
                        for (const KeyHalfedge& khe : cycle.halfedges()) {
                            if (khe.startVertex() == kv) {
                                if (khe.edge() == previousKe) {
                                    // Cannot uncut if kv is used as a u-turn in cycle.
                                    return result;
                                }
                            }
                            previousKe = khe.edge();
                        }
                    }
                }
                result.isValid = true;
            }
            else {
                // (inverse op: cut closed edge)
                // the only incident edge is a loop, and we don't
                // kv to be used as a u-turn in any cycle.
                for (Cell* starCell : kv->star()) {
                    KeyFace* kf = starCell->toKeyFace();
                    if (!kf) {
                        continue;
                    }
                    for (const KeyCycle& cycle : kf->cycles()) {
                        if (cycle.steinerVertex()) {
                            continue;
                        }
                        if (cycle.halfedges().first().edge() != result.khe1.edge()) {
                            continue;
                        }
                        // All edges in this cycle are equal to result.khe1.edge().
                        // We require them to be in the same direction (no u-turn).
                        bool direction = cycle.halfedges().first().edge();
                        for (const KeyHalfedge& khe : cycle.halfedges()) {
                            if (khe.direction() != direction) {
                                // Cannot uncut if kv is used as a u-turn in cycle.
                                return result;
                            }
                        }
                    }
                }
                result.isValid = true;
            }
        }
    }
    else if (result.kf) {
        // (inverse op: cut face at vertex)
        result.isValid = true;
    }

    return result;
}

Operations::UncutAtKeyEdgeInfo_ Operations::prepareUncutAtKeyEdge_(KeyEdge* ke) {
    UncutAtKeyEdgeInfo_ result = {};

    for (Cell* starCell : ke->star()) {
        switch (starCell->cellType()) {
        case CellType::KeyFace: {
            KeyFace* kf = starCell->toKeyFaceUnchecked();
            Int cycleIndex = -1;
            for (const KeyCycle& cycle : kf->cycles()) {
                ++cycleIndex;
                if (cycle.steinerVertex()) {
                    continue;
                }
                Int componentIndex = -1;
                for (const KeyHalfedge& khe : cycle.halfedges()) {
                    ++componentIndex;
                    if (khe.edge() != ke) {
                        continue;
                    }
                    if (!result.kf1) {
                        result.kf1 = kf;
                        result.cycleIndex1 = cycleIndex;
                        result.componentIndex1 = componentIndex;
                    }
                    else if (!result.kf2) {
                        result.kf2 = kf;
                        result.cycleIndex2 = cycleIndex;
                        result.componentIndex2 = componentIndex;
                    }
                    else {
                        // Cannot uncut if used more than twice as face cycle component.
                        return result;
                    }
                }
            }
            break;
        }
        default:
            break;
        }
    }

    if (result.kf1 && result.kf2) {
        result.isValid = true;
    }

    return result;
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

} // namespace detail

} // namespace vgc::vacomplex
