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
#include <vgc/vacomplex/exceptions.h>
#include <vgc/vacomplex/logcategories.h>

namespace vgc::vacomplex::detail {

Operations::Operations(Complex* complex)
    : complex_(complex) {

    // Ensure complex is non-null
    if (!complex) {
        throw LogicError("Cannot instantiate a VAC `Operations` with a null complex.");
    }

    // Increment version
    complex->version_ += 1;
}

Group* Operations::createRootGroup() {

    Group* group = createNode_<Group>(complex());

    // diff
    if (complex()->isDiffEnabled_) {
        complex()->diff_.onNodeDiff(group, NodeDiffFlag::Created);
    }
    complex()->nodeCreated().emit(group, {});

    return group;
}

Group* Operations::createGroup(Group* parentGroup, Node* nextSibling) {

    Group* group = createNode_<Group>(complex());
    parentGroup->insertChildUnchecked(nextSibling, group);

    // diff
    if (complex()->isDiffEnabled_) {
        complex()->diff_.onNodeDiff(group, NodeDiffFlag::Created);
        complex()->diff_.onNodeDiff(parentGroup, NodeDiffFlag::ChildrenChanged);
    }
    complex()->nodeCreated().emit(group, {});

    return group;
}

KeyVertex* Operations::createKeyVertex(
    const geometry::Vec2d& position,
    Group* parentGroup,
    Node* nextSibling,
    core::Span<Node*> operationSourceNodes,
    core::AnimTime t) {

    KeyVertex* kv = createNode_<KeyVertex>(t);
    kv->position_ = position;
    parentGroup->insertChildUnchecked(nextSibling, kv);

    // diff
    if (complex()->isDiffEnabled_) {
        complex()->diff_.onNodeDiff(kv, NodeDiffFlag::Created);
        complex()->diff_.onNodeDiff(parentGroup, NodeDiffFlag::ChildrenChanged);
    }
    complex()->nodeCreated().emit(kv, operationSourceNodes);

    return kv;
}

KeyEdge* Operations::createKeyOpenEdge(
    KeyVertex* startVertex,
    KeyVertex* endVertex,
    const geometry::SharedConstVec2dArray& points,
    const core::SharedConstDoubleArray& widths,
    Group* parentGroup,
    Node* nextSibling,
    core::Span<Node*> operationSourceNodes,
    core::AnimTime t) {

    KeyEdge* ke = createNode_<KeyEdge>(t);
    parentGroup->insertChildUnchecked(nextSibling, ke);

    // init cell
    ke->startVertex_ = startVertex;
    ke->endVertex_ = endVertex;
    ke->points_ = points.getShared();
    ke->widths_ = widths.getShared();
    ke->boundary_.assign({startVertex, endVertex});
    // add edge to new vertices star
    startVertex->star_.append(ke);
    if (endVertex != startVertex) {
        endVertex->star_.append(ke);
    }

    // diff
    if (complex()->isDiffEnabled_) {
        complex()->diff_.onNodeDiff(ke, NodeDiffFlag::Created);
        complex()->diff_.onNodeDiff(parentGroup, NodeDiffFlag::ChildrenChanged);
        complex()->diff_.onNodeDiff(startVertex, NodeDiffFlag::StarChanged);
        complex()->diff_.onNodeDiff(endVertex, NodeDiffFlag::StarChanged);
    }
    complex()->nodeCreated().emit(ke, operationSourceNodes);

    return ke;
}

KeyEdge* Operations::createKeyClosedEdge(
    const geometry::SharedConstVec2dArray& points,
    const core::SharedConstDoubleArray& widths,
    Group* parentGroup,
    Node* nextSibling,
    core::Span<Node*> operationSourceNodes,
    core::AnimTime t) {

    KeyEdge* ke = createNode_<KeyEdge>(t);
    parentGroup->insertChildUnchecked(nextSibling, ke);

    ke->points_ = points.getShared();
    ke->widths_ = widths.getShared();

    // init cell
    // ...

    // diff
    if (complex()->isDiffEnabled_) {
        complex()->diff_.onNodeDiff(ke, NodeDiffFlag::Created);
        complex()->diff_.onNodeDiff(parentGroup, NodeDiffFlag::ChildrenChanged);
    }
    complex()->nodeCreated().emit(ke, operationSourceNodes);

    return ke;
}

// Assumes `cycles` are valid.
// Assumes `nextSibling` is either `nullptr` or a child of `parentGroup`.
KeyFace* Operations::createKeyFace(
    core::Array<KeyCycle> cycles,
    Group* parentGroup,
    Node* nextSibling,
    core::Span<Node*> operationSourceNodes,
    core::AnimTime t) {

    KeyFace* kf = createNode_<KeyFace>(t);
    parentGroup->insertChildUnchecked(nextSibling, kf);

    // init cell
    kf->cycles_ = std::move(cycles);
    core::Array<Cell*> boundary = {};
    for (const KeyCycle& cycle : kf->cycles_) {
        KeyVertex* kv = cycle.steinerVertex_;
        if (kv) {
            if (!boundary.contains(kv)) {
                kv->star_.append(kf);
                // diff star
                if (complex()->isDiffEnabled_) {
                    complex()->diff_.onNodeDiff(kv, NodeDiffFlag::StarChanged);
                }
                boundary.append(kv);
            }
        }
        for (const KeyHalfedge& halfedge : cycle.halfedges_) {
            KeyEdge* ke = halfedge.edge();
            if (!boundary.contains(ke)) {
                ke->star_.append(kf);
                // diff star
                if (complex()->isDiffEnabled_) {
                    complex()->diff_.onNodeDiff(ke, NodeDiffFlag::StarChanged);
                }
                boundary.append(ke);
            }
        }
    }
    kf->boundary_.assign(std::move(boundary));

    // diff
    if (complex()->isDiffEnabled_) {
        complex()->diff_.onNodeDiff(kf, NodeDiffFlag::Created);
        complex()->diff_.onNodeDiff(parentGroup, NodeDiffFlag::ChildrenChanged);
    }
    complex()->nodeCreated().emit(kf, operationSourceNodes);

    return kf;
}

void Operations::removeNode(Node* node, bool removeFreeVertices) {

    ComplexDiff& diff = complex()->diff_;
    const bool diffEnabled = complex()->isDiffEnabled_;
    const bool isRoot = (complex()->rootGroup() == node);

    std::unordered_set<Node*> toRemoveNodes;

    // only remove `node` if it is not the root node
    if (!isRoot) {
        toRemoveNodes.insert(node);
    }

    // collect all dependent nodes
    collectDependentNodes_(node, toRemoveNodes);

    std::unordered_set<Node*> freeKeyVertices;
    std::unordered_set<Node*> freeInbetweenVertices;

    // flag removal
    for (Node* toRemoveNode : toRemoveNodes) {
        toRemoveNode->isBeingDestroyed_ = true;
    }

    auto hasEmptyStar = [](Cell* cell) {
        bool isStarEmpty = true;
        for (Cell* starCell : cell->star()) {
            if (!starCell->isBeingDestroyed_) {
                isStarEmpty = false;
                break;
            }
        }
        return isStarEmpty;
    };

    for (Node* toRemoveNode : toRemoveNodes) {
        if (toRemoveNode->isCell()) {
            Cell* cell = toRemoveNode->toCellUnchecked();
            for (Cell* boundaryCell : cell->boundary()) {
                // skip if cell is flag'd for removal
                if (boundaryCell->isBeingDestroyed_) {
                    continue;
                }
                if (removeFreeVertices
                    && boundaryCell->spatialType() == CellSpatialType::Vertex
                    && hasEmptyStar(boundaryCell)) {

                    switch (boundaryCell->cellType()) {
                    case CellType::KeyVertex:
                        freeKeyVertices.insert(boundaryCell);
                        break;
                    case CellType::InbetweenVertex:
                        freeInbetweenVertices.insert(boundaryCell);
                        break;
                    default:
                        break;
                    }

                    boundaryCell->isBeingDestroyed_ = true;
                }
                if (!boundaryCell->isBeingDestroyed_) {
                    boundaryCell->star_.removeOne(cell);
                    if (diffEnabled) {
                        diff.onNodeDiff(boundaryCell, NodeDiffFlag::StarChanged);
                    }
                }
            }
        }
    }

    if (removeFreeVertices) {
        // it requires a second pass since inbetween vertices are in star of key vertices
        for (Node* vn : freeInbetweenVertices) {
            Cell* cell = vn->toCellUnchecked();
            for (Cell* boundaryCell : cell->boundary()) {
                if (boundaryCell->isBeingDestroyed_) {
                    continue;
                }
                if (hasEmptyStar(boundaryCell)) {
                    freeKeyVertices.insert(boundaryCell->toKeyVertexUnchecked());
                    boundaryCell->isBeingDestroyed_ = true;
                }
                else {
                    boundaryCell->star_.removeOne(cell);
                    if (diffEnabled) {
                        diff.onNodeDiff(boundaryCell, NodeDiffFlag::StarChanged);
                    }
                }
            }
        }
        toRemoveNodes.merge(freeKeyVertices);
        toRemoveNodes.merge(freeInbetweenVertices);
    }

    for (Node* toRemoveNode : toRemoveNodes) {
        if (diffEnabled) {
            Group* parentGroup = toRemoveNode->parentGroup();
            if (parentGroup) {
                diff.onNodeDiff(parentGroup, NodeDiffFlag::ChildrenChanged);
            }
            diff.onNodeRemoved(toRemoveNode);
        }
        toRemoveNode->unlink();
        // Note: must not cause recursion.
        complex()->nodeAboutToBeRemoved().emit(toRemoveNode);
        complex()->nodes_.erase(toRemoveNode->id());
    }

    if (isRoot) {
        // we did not remove root group but cleared its children
        Group* group = node->toGroupUnchecked();
        if (group->numChildren()) {
            group->resetChildrenNoUnlink();
            if (diffEnabled) {
                diff.onNodeDiff(node, NodeDiffFlag::ChildrenChanged);
            }
        }
    }
}

void Operations::removeNodeSmart(Node* /*node*/, bool /*removeFreeVertices*/) {
    // todo later
    throw core::RuntimeError("not implemented");
}

void Operations::moveToGroup(Node* node, Group* parentGroup, Node* nextSibling) {

    Group* oldParent = node->parentGroup();
    bool inserted = parentGroup->insertChildUnchecked(nextSibling, node);
    if (!inserted) {
        return;
    }

    // diff
    if (complex()->isDiffEnabled_) {
        if (oldParent != parentGroup) {
            complex()->diff_.onNodeDiff(node, NodeDiffFlag::Reparented);
        }
        complex()->diff_.onNodeDiff(parentGroup, NodeDiffFlag::ChildrenChanged);
    }
}

// dev note: update boundary before star

void Operations::setKeyVertexPosition(KeyVertex* kv, const geometry::Vec2d& pos) {
    if (pos == kv->position_) {
        // same data
        return;
    }

    kv->position_ = pos;

    dirtyGeometry_(kv); // it also emits the geometry change event
}

void Operations::setKeyEdgeCurvePoints(
    KeyEdge* ke,
    const geometry::SharedConstVec2dArray& points) {

    KeyEdge::SharedConstPoints sPoints = points.getShared();
    if (sPoints == ke->points_) {
        // same data
        return;
    }

    ke->points_ = std::move(sPoints);
    ++ke->dataVersion_;

    ke->dirtyInputSampling_();
    dirtyGeometry_(ke); // it also emits the geometry change event
}

void Operations::setKeyEdgeCurveWidths(
    KeyEdge* ke,
    const core::SharedConstDoubleArray& widths) {

    KeyEdge::SharedConstWidths sWidths = widths.getShared();
    if (sWidths == ke->widths_) {
        // same data
        return;
    }

    ke->widths_ = std::move(sWidths);
    ++ke->dataVersion_;

    ke->dirtyInputSampling_();
    dirtyGeometry_(ke); // it also emits the geometry change event
}

void Operations::setKeyEdgeSamplingParameters(
    KeyEdge* ke,
    const geometry::CurveSamplingParameters& parameters) {

    if (parameters == ke->samplingParameters_) {
        // same data
        return;
    }

    ke->samplingParameters_ = parameters;
    ++ke->dataVersion_;

    ke->dirtyInputSampling_();
    dirtyGeometry_(ke); // it also emits the geometry change event
}

void Operations::collectDependentNodes_(
    Node* node,
    std::unordered_set<Node*>& dependentNodes) {

    if (node->isGroup()) {
        Group* g = node->toGroupUnchecked();
        for (Node* n : *g) {
            if (dependentNodes.insert(n).second) {
                collectDependentNodes_(n, dependentNodes);
            }
        }
    }
    else {
        Cell* c = node->toCellUnchecked();
        for (Node* n : c->star()) {
            if (dependentNodes.insert(n).second) {
                collectDependentNodes_(n, dependentNodes);
            }
        }
    }
}

void Operations::dirtyGeometry_(Cell* cell) {
    if (cell->isGeometryDirty_) {
        return;
    }

    core::Array<Cell*> dirtyList;

    dirtyList.append(cell);
    cell->isGeometryDirty_ = true;

    for (Cell* starCell : cell->star_) {
        if (!starCell->isGeometryDirty_) {
            // There is no need to call dirtyGeometry_ for starCell
            // since starCell.star() is a subset of cell.star().
            dirtyList.append(starCell);
            starCell->isGeometryDirty_ = true;
        }
    }

    for (Cell* dirtyCell : dirtyList) {
        if (complex()->isDiffEnabled_) {
            complex()->diff_.onNodeDiff(dirtyCell, NodeDiffFlag::GeometryChanged);
        }
        complex()->nodeModified().emit(dirtyCell, NodeDiffFlag::GeometryChanged);
    }
}

} // namespace vgc::vacomplex::detail
