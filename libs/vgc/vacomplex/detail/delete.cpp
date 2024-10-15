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

#include <unordered_set>

#include <vgc/vacomplex/algorithms.h> // opening, closure
#include <vgc/vacomplex/inbetweenedge.h>
#include <vgc/vacomplex/inbetweenface.h>
#include <vgc/vacomplex/inbetweenvertex.h>

namespace vgc::vacomplex::detail {

template<typename TNodeRange>
void Operations::deleteWithDependents_(
    TNodeRange nodes,
    bool deleteIsolatedVertices,
    bool tryRepairingStarCells) {

    Group* rootGroup = complex()->rootGroup();

    // First collect all descendants
    std::unordered_set<Node*> descendants;

    for (Node* node : nodes) {
        for (Node* descendant : node->descendants()) {
            descendants.insert(descendant);
        }
        // When hard-deleting the root, we delete all nodes below the root, but
        // preserve the root itself since we have the invariant that there is
        // always a root.
        //
        if (node != rootGroup) {
            descendants.insert(node);
        }
    }

    delete_(descendants, deleteIsolatedVertices, tryRepairingStarCells);
}

void Operations::delete_(
    std::unordered_set<Node*> descendants,
    bool deleteIsolatedVertices,
    bool tryRepairingStarCells) {

    std::unordered_set<Node*> nodesToDestroy;

    // Flag all descendants as about to be deleted.
    //
    for (Node* descendant : descendants) {
        descendant->isBeingDeleted_ = true;
        nodesToDestroy.insert(descendant);
    }

    // Delete star of nodes.
    core::Array<KeyFace*> starFaces;
    for (Node* descendant : descendants) {
        Cell* cell = descendant->toCell();
        if (!cell) {
            continue;
        }
        for (Cell* starCell : cell->star()) {
            switch (starCell->cellType()) {
            case CellType::KeyFace: {
                KeyFace* kf = starCell->toKeyFaceUnchecked();
                if (!starFaces.contains(kf)) {
                    starFaces.append(kf);
                }
                break;
            }
            default: {
                if (!starCell->isBeingDeleted_) {
                    starCell->isBeingDeleted_ = true;
                    nodesToDestroy.insert(starCell);
                }
                break;
            }
            }
        }
    }

    // TODO: use KeyFace winding rule.
    geometry::WindingRule windingRule = geometry::WindingRule::Odd;
    constexpr Int numSamplesPerContainTest = 20;
    constexpr double ratioThreshold = 0.5;

    KeyCycle tmpCycle;
    for (KeyFace* kf : starFaces) {
        if (kf->isBeingDeleted_) {
            continue;
        }
        if (!tryRepairingStarCells) {
            kf->isBeingDeleted_ = true;
            nodesToDestroy.insert(kf);
            continue;
        }

        struct RepairedCycle {
            KeyCycle cycle;
            Int originalIndex;
            bool isUnchanged;
        };
        core::Array<RepairedCycle> repairedCycles;

        for (Int i = 0; i != kf->cycles_.length(); ++i) {
            const KeyCycle& kc = kf->cycles_[i];
            KeyVertex* sv = kc.steinerVertex();
            if (sv) {
                if (!sv->isBeingDeleted_) {
                    repairedCycles.append({kc, i, true});
                }
                continue;
            }
            tmpCycle = kc;
            Int numRemoved = tmpCycle.halfedges_.removeIf(
                [](const KeyHalfedge& he) { return he.edge()->isBeingDeleted_; });
            bool isUnchanged = numRemoved == 0;
            if (isUnchanged || tmpCycle.isValid()) {
                repairedCycles.append({tmpCycle, i, isUnchanged});
            }
        }

        // If a repaired cycle is contained in a deleted one, delete it.
        for (auto it = repairedCycles.begin(); it != repairedCycles.end();) {
            const KeyCycle& repairedCycle = it->cycle;
            KeyVertex* steinerVertex = repairedCycle.steinerVertex();
            if (steinerVertex) {
                bool keep = true;
                geometry::Vec2d pos = steinerVertex->position();
                for (Int i = 0; i != kf->cycles_.length(); ++i) {
                    const KeyCycle& originalCycle = kf->cycles_[i];
                    RepairedCycle* rc = repairedCycles.search(
                        [i](const RepairedCycle& rc) { return rc.originalIndex == i; });
                    if (rc && rc->isUnchanged) {
                        continue;
                    }
                    if (originalCycle.interiorContains(pos, windingRule)) {
                        if (!rc || !rc->cycle.interiorContains(pos, windingRule)) {
                            keep = false;
                            break;
                        }
                    }
                }
                if (!keep) {
                    it = repairedCycles.erase(it);
                }
                else {
                    ++it;
                }
            }
            else {
                bool keep = true;
                core::Array<geometry::Vec2d> samples =
                    repairedCycle.sampleUniformly(numSamplesPerContainTest);
                for (Int i = 0; i != kf->cycles_.length(); ++i) {
                    const KeyCycle& originalCycle = kf->cycles_[i];
                    RepairedCycle* rc = repairedCycles.search(
                        [=](const RepairedCycle& rc) { return rc.originalIndex == i; });
                    if (rc && rc->isUnchanged) {
                        continue;
                    }
                    double ratio =
                        originalCycle.interiorContainedRatio(samples, windingRule);
                    if (ratio > ratioThreshold) {
                        if (!rc) {
                            keep = false;
                            break;
                        }
                        double ratio2 =
                            rc->cycle.interiorContainedRatio(samples, windingRule);
                        if (ratio2 <= ratioThreshold) {
                            keep = false;
                            break;
                        }
                    }
                }
                if (!keep) {
                    repairedCycles.erase(it);
                    // restart tests since previous cycles could have been
                    // contained and thus saved by the current one.
                    it = repairedCycles.begin();
                }
                else {
                    ++it;
                }
            }
        }

        if (repairedCycles.isEmpty()) {
            kf->isBeingDeleted_ = true;
            nodesToDestroy.insert(kf);
        }
        else {
            for (Cell* boundaryCell : kf->boundary().copy()) {
                removeFromBoundary_(kf, boundaryCell);
            }
            kf->cycles_.clear();
            for (RepairedCycle& repairedCycle : repairedCycles) {
                addToBoundary_(kf, repairedCycle.cycle);
                kf->cycles_.append(std::move(repairedCycle.cycle));
            }
        }
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
        onNodeModified_(parentGroup, NodeModificationFlag::ChildrenChanged);
    }
    if (node->isCell()) {
        complex_->temporaryCellSet_.removeOne(node->toCellUnchecked());
    }
    onNodeDestroyed_(node->id());
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
            onNodeModified_(parentGroup, NodeModificationFlag::ChildrenChanged);
        }
    }
    for (Node* node : nodes) {
        onNodeDestroyed_(node->id());
        complex_->nodes_.erase(node->id());
    }
}

void Operations::hardDelete(core::ConstSpan<Node*> nodes, bool deleteIsolatedVertices) {

    deleteWithDependents_(nodes, deleteIsolatedVertices, false);
}

void Operations::hardDelete(Node* node, bool deleteIsolatedVertices) {

    deleteWithDependents_(std::array{node}, deleteIsolatedVertices, false);
}

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
void Operations::softDelete(core::ConstSpan<Node*> nodes, bool deleteIsolatedVertices) {

    VGC_UNUSED(deleteIsolatedVertices);

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
            typename c20::remove_cvref_t<decltype(cells)>::value_type>;
        for (CellType*& cell : cells) {
            bool wasUncut = false;
            if constexpr (std::is_same_v<CellType, KeyVertex>) {
                wasUncut = uncutAtKeyVertex(cell, smoothJoins).success;
            }
            if constexpr (std::is_same_v<CellType, KeyEdge>) {
                wasUncut = uncutAtKeyEdge(cell).success;
            }
            if (wasUncut) {
                cell = nullptr;
            }
        }
        cells.removeAll(nullptr);
    };

    auto deleteCells = [this](auto&& cells) {
        // Note: deleteIsolatedVertices could remove cells that are
        // in `cells` and it would cause a crash.
        deleteWithDependents_(cells, false, true);
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
        deleteCells(kfs);
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
        deleteCells(kes);
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
        deleteCells(kvs);
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

} // namespace vgc::vacomplex::detail
