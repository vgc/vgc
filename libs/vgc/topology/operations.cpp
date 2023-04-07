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

#include <vgc/topology/operations.h>

#include <vgc/topology/detail/operationsimpl.h>

namespace vgc::topology::ops {

namespace {

void checkIsChildOrNull(VacNode* node, VacGroup* expectedParent) {
    if (node && node->parentGroup() != expectedParent) {
        throw NotAChildError(node, expectedParent);
    }
}

} // namespace

VacGroup* createVacGroup(VacGroup* parentGroup, VacNode* nextSibling) {
    if (!parentGroup) {
        throw LogicError("createVacGroup: parentGroup is nullptr.");
    }
    checkIsChildOrNull(nextSibling, parentGroup);

    return detail::Operations::createVacGroup(parentGroup, nextSibling);
}

KeyVertex* createKeyVertex(
    const geometry::Vec2d& position,
    VacGroup* parentGroup,
    VacNode* nextSibling,
    core::AnimTime t) {

    if (!parentGroup) {
        throw LogicError("createKeyVertex: parentGroup is nullptr.");
    }
    checkIsChildOrNull(nextSibling, parentGroup);

    return detail::Operations::createKeyVertex(position, parentGroup, nextSibling, {}, t);
}

KeyEdge* createKeyClosedEdge(
    const geometry::SharedConstVec2dArray& points,
    const core::SharedConstDoubleArray& widths,
    VacGroup* parentGroup,
    VacNode* nextSibling,
    core::AnimTime t) {

    if (!parentGroup) {
        throw LogicError("createKeyClosedEdge: parentGroup is nullptr.");
    }
    checkIsChildOrNull(nextSibling, parentGroup);

    return detail::Operations::createKeyClosedEdge(
        points, widths, parentGroup, nextSibling, {}, t);
}

KeyEdge* createKeyOpenEdge(
    KeyVertex* startVertex,
    KeyVertex* endVertex,
    const geometry::SharedConstVec2dArray& points,
    const core::SharedConstDoubleArray& widths,
    VacGroup* parentGroup,
    VacNode* nextSibling,
    core::AnimTime t) {

    if (!parentGroup) {
        throw LogicError("createKeyOpenEdge: parentGroup is nullptr.");
    }
    checkIsChildOrNull(nextSibling, parentGroup);

    if (!startVertex) {
        throw LogicError("createKeyOpenEdge: startVertex is nullptr.");
    }
    if (!endVertex) {
        throw LogicError("createKeyOpenEdge: endVertex is nullptr.");
    }

    Vac* vac = parentGroup->vac();

    if (vac != startVertex->vac()) {
        throw LogicError(
            "createKeyOpenEdge: given `parentGroup` and `startVertex` are not "
            "in the same `Vac`.");
    }
    if (vac != endVertex->vac()) {
        throw LogicError("createKeyOpenEdge: given `parentGroup` and `endVertex` are not "
                         "in the same `Vac`.");
    }
    if (t != startVertex->time()) {
        throw LogicError(
            "createKeyOpenEdge: given `startVertex` is not at the given time `t`.");
    }
    if (t != endVertex->time()) {
        throw LogicError(
            "createKeyOpenEdge: given `endVertex` is not at the given time `t`.");
    }

    return detail::Operations::createKeyOpenEdge(
        startVertex, endVertex, points, widths, parentGroup, nextSibling, {}, t);
}

KeyFace* createKeyFace(
    core::Array<KeyCycle> cycles,
    VacGroup* parentGroup,
    VacNode* nextSibling,
    core::AnimTime t) {

    if (!parentGroup) {
        throw LogicError("createKeyFace: parentGroup is nullptr.");
    }
    checkIsChildOrNull(nextSibling, parentGroup);

    for (const KeyCycle& cycle : cycles) {
        if (!cycle.isValid()) {
            throw LogicError(
                "createKeyFace: at least one of the input cycles is not valid.");
        }
    }

    return detail::Operations::createKeyFace(
        std::move(cycles), parentGroup, nextSibling, {}, t);
}

KeyFace* createKeyFace(
    KeyCycle cycle,
    VacGroup* parentGroup,
    VacNode* nextSibling,
    core::AnimTime t) {

    if (!parentGroup) {
        throw LogicError("createKeyFace: parentGroup is nullptr.");
    }
    checkIsChildOrNull(nextSibling, parentGroup);

    if (!cycle.isValid()) {
        throw LogicError("createKeyFace: the input cycle is not valid.");
    }

    return detail::Operations::createKeyFace(
        core::Array<KeyCycle>(1, std::move(cycle)), parentGroup, nextSibling, {}, t);
}

void removeNode(VacNode* node, bool removeFreeVertices) {
    if (!node) {
        throw LogicError("removeNode: node is nullptr.");
    }
    detail::Operations::removeNode(node, removeFreeVertices);
}

void removeNodeSmart(VacNode* node, bool removeFreeVertices) {
    if (!node) {
        throw LogicError("removeNodeSmart: node is nullptr.");
    }
    detail::Operations::removeNodeSmart(node, removeFreeVertices);
}

void moveToGroup(VacNode* node, VacGroup* parentGroup, VacNode* nextSibling) {
    if (!node) {
        throw LogicError("moveToGroup: node is nullptr.");
    }
    if (!parentGroup) {
        throw LogicError("moveToGroup: parentGroup is nullptr.");
    }
    checkIsChildOrNull(nextSibling, parentGroup);

    return detail::Operations::moveToGroup(node, parentGroup, nextSibling);
}

void setKeyVertexPosition(KeyVertex* vertex, const geometry::Vec2d& pos) {
    if (!vertex) {
        throw LogicError("setKeyVertexPosition: vertex is nullptr.");
    }
    return detail::Operations::setKeyVertexPosition(vertex, pos);
}

void setKeyEdgeCurvePoints(KeyEdge* edge, const geometry::SharedConstVec2dArray& points) {
    if (!edge) {
        throw LogicError("setKeyEdgeCurvePoints: edge is nullptr.");
    }
    return detail::Operations::setKeyEdgeCurvePoints(edge, points);
}

void setKeyEdgeCurveWidths(KeyEdge* edge, const core::SharedConstDoubleArray& widths) {
    if (!edge) {
        throw LogicError("setKeyEdgeCurveWidths: edge is nullptr.");
    }
    return detail::Operations::setKeyEdgeCurveWidths(edge, widths);
}

} // namespace vgc::topology::ops
