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

#include <vgc/vacomplex/exceptions.h>
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
    Group* group = createNode_<Group>(NodeSourceOperation(), complex());
    return group;
}

Group* Operations::createGroup(Group* parentGroup, Node* nextSibling) {
    Group* group =
        createNodeAt_<Group>(parentGroup, nextSibling, NodeSourceOperation(), complex());
    return group;
}

KeyVertex* Operations::createKeyVertex(
    const geometry::Vec2d& position,
    Group* parentGroup,
    Node* nextSibling,
    NodeSourceOperation sourceOperation,
    core::AnimTime t) {

    KeyVertex* kv =
        createNodeAt_<KeyVertex>(parentGroup, nextSibling, std::move(sourceOperation), t);

    // Topological attributes
    // -> None

    // Geometric attributes
    kv->position_ = position;

    return kv;
}

// TODO: replace points & widths with `std::unique_ptr<EdgeGeometry>&& geometry`.

KeyEdge* Operations::createKeyOpenEdge(
    KeyVertex* startVertex,
    KeyVertex* endVertex,
    const std::shared_ptr<KeyEdgeGeometry>& geometry,
    Group* parentGroup,
    Node* nextSibling,
    NodeSourceOperation sourceOperation) {

    KeyEdge* ke = createNodeAt_<KeyEdge>(
        parentGroup, nextSibling, std::move(sourceOperation), startVertex->time());

    // Topological attributes
    ke->startVertex_ = startVertex;
    ke->endVertex_ = endVertex;
    addToBoundary_(ke, startVertex);
    addToBoundary_(ke, endVertex);

    // Geometric attributes
    ke->geometry_ = geometry;
    geometry->edge_ = ke;

    return ke;
}

KeyEdge* Operations::createKeyClosedEdge(
    const std::shared_ptr<KeyEdgeGeometry>& geometry,
    Group* parentGroup,
    Node* nextSibling,
    NodeSourceOperation sourceOperation,
    core::AnimTime t) {

    KeyEdge* ke =
        createNodeAt_<KeyEdge>(parentGroup, nextSibling, std::move(sourceOperation), t);

    // Topological attributes
    // -> None

    // Geometric attributes
    ke->geometry_ = geometry;
    geometry->edge_ = ke;

    return ke;
}

// Assumes `cycles` are valid.
// Assumes `nextSibling` is either `nullptr` or a child of `parentGroup`.
KeyFace* Operations::createKeyFace(
    core::Array<KeyCycle> cycles,
    Group* parentGroup,
    Node* nextSibling,
    NodeSourceOperation sourceOperation,
    core::AnimTime t) {

    KeyFace* kf =
        createNodeAt_<KeyFace>(parentGroup, nextSibling, std::move(sourceOperation), t);

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
    KeyVertex* newKv = createKeyVertex(
        position, parentGroup, nextSibling, NodeSourceOperation{}, kv0->time());

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

KeyEdge* Operations::glueKeyOpenEdges(
    core::Span<KeyHalfedge> khes,
    std::shared_ptr<KeyEdgeGeometry> geometry,
    const geometry::Vec2d& startPosition,
    const geometry::Vec2d& endPosition) {

    if (khes.isEmpty()) {
        return nullptr;
    }

    Int n = khes.length();

    core::Array<KeyVertex*> startVertices;
    startVertices.reserve(n);
    for (const KeyHalfedge& khe : khes) {
        startVertices.append(khe.startVertex());
    }
    KeyVertex* startKv = glueKeyVertices(startVertices, startPosition);

    // Note: we can only list end vertices after the glue of
    // start vertices since it can substitute end vertices.
    core::Array<KeyVertex*> endVertices;
    endVertices.reserve(n);
    for (const KeyHalfedge& khe : khes) {
        endVertices.append(khe.endVertex());
    }
    KeyVertex* endKv = glueKeyVertices(endVertices, endPosition);

    // Location: top-most input edge
    core::Array<Node*> edgeNodes(n);
    for (Int i = 0; i < n; ++i) {
        edgeNodes[i] = khes[i].edge();
    }
    Node* topMostEdge = findTopMost(edgeNodes);
    Group* parentGroup = topMostEdge->parentGroup();
    Node* nextSibling = topMostEdge->nextSibling();

    // TODO: define source operation
    KeyEdge* newKe = createKeyOpenEdge(
        startKv,
        endKv,
        std::move(geometry),
        parentGroup,
        nextSibling,
        NodeSourceOperation{});

    KeyHalfedge newKhe(newKe, true);
    for (const KeyHalfedge& khe : khes) {
        substitute_(khe, newKhe);
        // It is important that no 2 halfedges refer to the same edge.
        hardDelete(khe.edge(), true);
    }

    return newKe;
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

void Operations::setKeyEdgeGeometry(
    KeyEdge* ke,
    const std::shared_ptr<KeyEdgeGeometry>& geometry) {

    KeyEdge* previousKe = geometry->edge_;
    ke->geometry_ = geometry;
    geometry->edge_ = ke;

    if (previousKe) {
        previousKe->geometry_ = nullptr;
        onGeometryChanged_(previousKe);
    }

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

void Operations::onNodeCreated_(Node* node, NodeSourceOperation sourceOperation) {
    complex_->opDiff_.onNodeCreated(node, std::move(sourceOperation));
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

void Operations::onBoundaryMeshChanged_(Cell* cell) {
    onNodeModified_(cell, NodeModificationFlag::BoundaryMeshChanged);
    dirtyMesh_(cell);
}

void Operations::dirtyMesh_(Cell* cell) {
    if (cell->hasMeshBeenQueriedSinceLastDirtyEvent_) {
        cell->hasMeshBeenQueriedSinceLastDirtyEvent_ = false;
        cell->dirtyMesh_();
        onNodeModified_(cell, NodeModificationFlag::MeshChanged);
        for (Cell* starCell : cell->star_) {
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
    for (Cell* starCell : oldVertex->star_) {
        *starCell->boundary_.find(oldVertex) = newVertex;
        newVertex->star_.append(starCell);
        onBoundaryChanged_(starCell);
        starCell->substituteKeyVertex_(oldVertex, newVertex);
    }
    oldVertex->star_.clear();
    onNodeModified_(oldVertex, NodeModificationFlag::StarChanged);
    onNodeModified_(newVertex, NodeModificationFlag::StarChanged);
}

void Operations::substitute_(
    const KeyHalfedge& oldHalfedge,
    const KeyHalfedge& newHalfedge) {

    KeyEdge* const oldEdge = oldHalfedge.edge();
    KeyEdge* const newEdge = newHalfedge.edge();
    for (Cell* starCell : oldEdge->star_) {
        *starCell->boundary_.find(oldEdge) = newEdge;
        newEdge->star_.append(starCell);
        onBoundaryChanged_(starCell);
        starCell->substituteKeyHalfedge_(oldHalfedge, newHalfedge);
    }
    oldEdge->star_.clear();
    onNodeModified_(oldEdge, NodeModificationFlag::StarChanged);
    onNodeModified_(newEdge, NodeModificationFlag::StarChanged);
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
        for (Node* starNode : cell->star()) {
            if (dependentNodes.insert(starNode).second) {
                collectDependentNodes_(starNode, dependentNodes);
            }
        }
    }
}

} // namespace vgc::vacomplex::detail
