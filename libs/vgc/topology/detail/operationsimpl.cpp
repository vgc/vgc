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

namespace vgc::topology::detail {

void checkIndexInRange(VacGroup* group, Int index) {
    if (index < 0 || index > group->numChildren()) {
        throw core::IndexError(core::format(
            "Child index {} out of range for insertion in group (end index currently is "
            "{}).",
            index,
            group->numChildren()));
    }
}

VacGroup*
Operations::createVacGroup(core::Id id, VacGroup* parentGroup, VacNode* nextSibling) {

    Vac* vac = parentGroup->vac();
    VacGroup* p = new VacGroup(vac, id);
    vac->nodes_[p->id_] = std::unique_ptr<VacGroup>(p);
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
    vac->nodes_[p->id_] = std::unique_ptr<KeyVertex>(p);
    parentGroup->insertChildUnchecked(nextSibling, p);

    // diff
    vac->incrementVersion();
    if (vac->diffEnabled_) {
        vac->diff_.onNodeDiff(p, VacNodeDiffFlag::Created);
        vac->diff_.onNodeDiff(parentGroup, VacNodeDiffFlag::ChildrenChanged);
    }

    return p;
}

KeyEdge* Operations::createKeyEdge(
    core::Id id,
    VacGroup* parentGroup,
    KeyVertex* startVertex,
    KeyVertex* endVertex,
    VacNode* nextSibling,
    core::AnimTime t) {

    Vac* vac = parentGroup->vac();
    KeyEdge* p = new KeyEdge(id, t);
    vac->nodes_[p->id_] = std::unique_ptr<KeyEdge>(p);
    parentGroup->insertChildUnchecked(nextSibling, p);

    // init cell
    p->startVertex_ = startVertex;
    p->endVertex_ = endVertex;
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
    vac->nodes_[p->id_] = std::unique_ptr<KeyEdge>(p);
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

    e->points_ = points.getShared();
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

    e->widths_ = widths.getShared();
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

} // namespace vgc::topology::detail
