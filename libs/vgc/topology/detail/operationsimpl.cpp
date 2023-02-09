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

#include <vgc/topology/detail/operationsimpl.h>
#include <vgc/topology/exceptions.h>
#include <vgc/topology/logcategories.h>

namespace vgc::topology::detail {

VacGroup* Operations::createRootGroup(Vac* vac, core::Id id) {

    VacGroup* p = new VacGroup(vac, id);
    vac->insertNode(id, std::unique_ptr<VacGroup>(p));

    // diff
    vac->incrementVersion();
    if (vac->diffEnabled_) {
        vac->diff_.onNodeDiff(p, VacNodeDiffFlag::Created);
    }

    return p;
}

VacGroup*
Operations::createVacGroup(core::Id id, VacGroup* parentGroup, VacNode* nextSibling) {

    Vac* vac = parentGroup->vac();
    VacGroup* p = new VacGroup(vac, id);
    vac->insertNode(id, std::unique_ptr<VacGroup>(p));
    parentGroup->insertChildUnchecked(nextSibling, p);

    // diff
    vac->incrementVersion();
    if (vac->diffEnabled_) {
        vac->diff_.onNodeDiff(p, VacNodeDiffFlag::Created);
        vac->diff_.onNodeDiff(parentGroup, VacNodeDiffFlag::ChildrenChanged);
    }

    return p;
}

KeyVertex* Operations::createKeyVertex(
    core::Id id,
    VacGroup* parentGroup,
    VacNode* nextSibling,
    core::AnimTime t) {

    Vac* vac = parentGroup->vac();
    KeyVertex* p = new KeyVertex(id, t);
    vac->insertNode(id, std::unique_ptr<KeyVertex>(p));
    parentGroup->insertChildUnchecked(nextSibling, p);

    // diff
    vac->incrementVersion();
    if (vac->diffEnabled_) {
        vac->diff_.onNodeDiff(p, VacNodeDiffFlag::Created);
        vac->diff_.onNodeDiff(parentGroup, VacNodeDiffFlag::ChildrenChanged);
    }

    return p;
}

KeyEdge* Operations::createKeyOpenEdge(
    core::Id id,
    VacGroup* parentGroup,
    KeyVertex* startVertex,
    KeyVertex* endVertex,
    VacNode* nextSibling,
    core::AnimTime t) {

    Vac* vac = parentGroup->vac();
    KeyEdge* p = new KeyEdge(id, t);
    vac->insertNode(id, std::unique_ptr<KeyEdge>(p));
    parentGroup->insertChildUnchecked(nextSibling, p);

    // init cell
    p->startVertex_ = startVertex;
    p->endVertex_ = endVertex;
    p->boundary_.assign({startVertex, endVertex});
    // add edge to new vertices star
    startVertex->star_.emplaceLast(p);
    if (endVertex != startVertex) {
        endVertex->star_.emplaceLast(p);
    }

    // diff
    vac->incrementVersion();
    if (vac->diffEnabled_) {
        vac->diff_.onNodeDiff(p, VacNodeDiffFlag::Created);
        vac->diff_.onNodeDiff(parentGroup, VacNodeDiffFlag::ChildrenChanged);
        vac->diff_.onNodeDiff(startVertex, VacNodeDiffFlag::StarChanged);
        vac->diff_.onNodeDiff(endVertex, VacNodeDiffFlag::StarChanged);
    }

    return p;
}

KeyEdge* Operations::createKeyClosedEdge(
    core::Id id,
    VacGroup* parentGroup,
    VacNode* nextSibling,
    core::AnimTime t) {

    Vac* vac = parentGroup->vac();
    KeyEdge* p = new KeyEdge(id, t);
    vac->insertNode(id, std::unique_ptr<KeyEdge>(p));
    parentGroup->insertChildUnchecked(nextSibling, p);

    // init cell
    // ...

    // diff
    vac->incrementVersion();
    if (vac->diffEnabled_) {
        vac->diff_.onNodeDiff(p, VacNodeDiffFlag::Created);
        vac->diff_.onNodeDiff(parentGroup, VacNodeDiffFlag::ChildrenChanged);
    }

    return p;
}

void Operations::removeNode(VacNode* node, bool removeFreeVertices) {

    Vac* vac = node->vac();
    VacDiff& diff = vac->diff_;
    const bool diffEnabled = vac->diffEnabled_;
    const bool isRoot = (vac->rootGroup() == node);

    std::unordered_set<VacNode*> toRemoveNodes;

    // only remove `node` if it is not the root node
    if (!isRoot) {
        toRemoveNodes.insert(node);
    }

    // collect all dependent nodes
    collectDependentNodes_(node, toRemoveNodes);

    std::unordered_set<VacNode*> freeKeyVertices;
    std::unordered_set<VacNode*> freeInbetweenVertices;

    // flag removal
    for (VacNode* n : toRemoveNodes) {
        n->isBeingDestroyed_ = true;
    }

    auto hasEmptyStar = [](VacCell* cell) {
        bool isStarEmpty = true;
        for (VacCell* starCell : cell->star()) {
            if (!starCell->isBeingDestroyed_) {
                isStarEmpty = false;
                break;
            }
        }
        return isStarEmpty;
    };

    for (VacNode* n : toRemoveNodes) {
        if (n->isCell()) {
            VacCell* cell = n->toCellUnchecked();
            for (VacCell* boundaryCell : cell->boundary()) {
                // skip if cell is flag'd for removal
                if (boundaryCell->isBeingDestroyed_) {
                    continue;
                }
                if (removeFreeVertices
                    && boundaryCell->spatialType() == CellSpatialType::Vertex
                    && hasEmptyStar(boundaryCell)) {

                    switch (boundaryCell->cellType()) {
                    case VacCellType::KeyVertex:
                        freeKeyVertices.insert(boundaryCell);
                        break;
                    case VacCellType::InbetweenVertex:
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
                        diff.onNodeDiff(boundaryCell, VacNodeDiffFlag::StarChanged);
                    }
                }
            }
        }
    }

    if (removeFreeVertices) {
        // it requires a second pass since inbetween vertices are in star of key vertices
        for (VacNode* vn : freeInbetweenVertices) {
            VacCell* cell = vn->toCellUnchecked();
            for (VacCell* boundaryCell : cell->boundary()) {
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
                        diff.onNodeDiff(boundaryCell, VacNodeDiffFlag::StarChanged);
                    }
                }
            }
        }
        toRemoveNodes.merge(freeKeyVertices);
        toRemoveNodes.merge(freeInbetweenVertices);
    }

    for (VacNode* n : toRemoveNodes) {
        vac->onNodeAboutToBeRemoved().emit(n);
    }

    for (VacNode* n : toRemoveNodes) {
        if (diffEnabled) {
            VacGroup* parentGroup = n->parentGroup();
            if (parentGroup) {
                diff.onNodeDiff(parentGroup, VacNodeDiffFlag::ChildrenChanged);
            }
            diff.onNodeRemoved(n);
        }
        n->unlink();
        vac->nodes_.erase(n->id());
    }

    if (isRoot) {
        // we did not remove root group but cleared its children
        VacGroup* g = node->toGroupUnchecked();
        if (g->numChildren()) {
            g->resetChildrenNoUnlink();
            if (diffEnabled) {
                diff.onNodeDiff(node, VacNodeDiffFlag::ChildrenChanged);
            }
        }
    }
}

void Operations::removeNodeSmart(VacNode* /*node*/, bool /*removeFreeVertices*/) {
    // todo later
    throw core::RuntimeError("not implemented");
}

void Operations::moveToGroup(VacNode* node, VacGroup* parentGroup, VacNode* nextSibling) {

    Vac* vac = parentGroup->vac();
    VacGroup* oldParent = node->parentGroup();
    bool inserted = parentGroup->insertChildUnchecked(nextSibling, node);
    if (!inserted) {
        return;
    }

    // diff
    vac->incrementVersion();
    if (vac->diffEnabled_) {
        if (oldParent != parentGroup) {
            vac->diff_.onNodeDiff(node, VacNodeDiffFlag::Reparented);
        }
        vac->diff_.onNodeDiff(parentGroup, VacNodeDiffFlag::ChildrenChanged);
    }
}

// dev note: update boundary before star

void Operations::setKeyVertexPosition(KeyVertex* v, const geometry::Vec2d& pos) {
    v->position_ = pos;
    Vac* vac = v->vac();
    if (vac) {
        // inc version
        vac->incrementVersion();
        // update diff
        if (vac->diffEnabled_) {
            vac->diff_.onNodeDiff(v, VacNodeDiffFlag::GeometryChanged);
        }
    }
}

void Operations::setKeyEdgeCurvePoints(
    KeyEdge* e,
    const geometry::SharedConstVec2dArray& points) {

    KeyEdge::SharedConstPoints sPoints = points.getShared();
    if (sPoints == e->points_) {
        // same data
        return;
    }

    e->points_ = std::move(sPoints);
    ++e->dataVersion_;
    Vac* vac = e->vac();
    if (vac) {
        // inc version
        vac->incrementVersion();
        // update diff
        if (vac->diffEnabled_) {
            vac->diff_.onNodeDiff(e, VacNodeDiffFlag::GeometryChanged);
        }
    }
}

void Operations::setKeyEdgeCurveWidths(
    KeyEdge* e,
    const core::SharedConstDoubleArray& widths) {

    KeyEdge::SharedConstWidths sWidths = widths.getShared();
    if (sWidths == e->widths_) {
        // same data
        return;
    }

    e->widths_ = std::move(sWidths);
    ++e->dataVersion_;
    Vac* vac = e->vac();
    if (vac) {
        // inc version
        vac->incrementVersion();
        // update diff
        if (vac->diffEnabled_) {
            vac->diff_.onNodeDiff(e, VacNodeDiffFlag::GeometryChanged);
        }
    }
}

void Operations::collectDependentNodes_(
    VacNode* node,
    std::unordered_set<VacNode*>& dependentNodes) {

    if (node->isGroup()) {
        VacGroup* g = node->toGroupUnchecked();
        for (VacNode* n : *g) {
            if (dependentNodes.insert(n).second) {
                collectDependentNodes_(n, dependentNodes);
            }
        }
    }
    else {
        VacCell* c = node->toCellUnchecked();
        for (VacNode* n : c->star()) {
            if (dependentNodes.insert(n).second) {
                collectDependentNodes_(n, dependentNodes);
            }
        }
    }
}

} // namespace vgc::topology::detail
