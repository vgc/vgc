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

#include <vgc/vacomplex/operations.h>

#include <vgc/vacomplex/detail/operationsimpl.h>

namespace vgc::vacomplex::ops {

namespace {

void checkIsChildOrNull(Node* node, Group* expectedParent) {
    if (node && node->parentGroup() != expectedParent) {
        throw NotAChildError(node, expectedParent);
    }
}

} // namespace

Group* createGroup(Group* parentGroup, Node* nextSibling) {
    if (!parentGroup) {
        throw LogicError("createGroup: parentGroup is nullptr.");
    }
    checkIsChildOrNull(nextSibling, parentGroup);
    detail::Operations ops(parentGroup->complex());
    return ops.createGroup(parentGroup, nextSibling);
}

KeyVertex* createKeyVertex(
    const geometry::Vec2d& position,
    Group* parentGroup,
    Node* nextSibling,
    core::AnimTime t) {

    if (!parentGroup) {
        throw LogicError("createKeyVertex: parentGroup is nullptr.");
    }
    checkIsChildOrNull(nextSibling, parentGroup);
    detail::Operations ops(parentGroup->complex());
    return ops.createKeyVertex(position, parentGroup, nextSibling, {}, t);
}

KeyEdge* createKeyClosedEdge(
    const geometry::SharedConstVec2dArray& points,
    const core::SharedConstDoubleArray& widths,
    Group* parentGroup,
    Node* nextSibling,
    core::AnimTime t) {

    if (!parentGroup) {
        throw LogicError("createKeyClosedEdge: parentGroup is nullptr.");
    }
    checkIsChildOrNull(nextSibling, parentGroup);
    detail::Operations ops(parentGroup->complex());
    return ops.createKeyClosedEdge(points, widths, parentGroup, nextSibling, {}, t);
}

KeyEdge* createKeyOpenEdge(
    KeyVertex* startVertex,
    KeyVertex* endVertex,
    const geometry::SharedConstVec2dArray& points,
    const core::SharedConstDoubleArray& widths,
    Group* parentGroup,
    Node* nextSibling,
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

    Complex* complex = parentGroup->complex();

    if (complex != startVertex->complex()) {
        throw LogicError(
            "createKeyOpenEdge: given `parentGroup` and `startVertex` are not "
            "in the same `Complex`.");
    }
    if (complex != endVertex->complex()) {
        throw LogicError("createKeyOpenEdge: given `parentGroup` and `endVertex` are not "
                         "in the same `Complex`.");
    }
    if (t != startVertex->time()) {
        throw LogicError(
            "createKeyOpenEdge: given `startVertex` is not at the given time `t`.");
    }
    if (t != endVertex->time()) {
        throw LogicError(
            "createKeyOpenEdge: given `endVertex` is not at the given time `t`.");
    }

    detail::Operations ops(complex);
    core::Span<Node*> sourceNodes;
    return ops.createKeyOpenEdge(
        startVertex, endVertex, points, widths, parentGroup, nextSibling, sourceNodes, t);
}

KeyFace* createKeyFace(
    core::Array<KeyCycle> cycles,
    Group* parentGroup,
    Node* nextSibling,
    core::AnimTime t) {

    if (!parentGroup) {
        throw LogicError("createKeyFace: parentGroup is nullptr.");
    }
    checkIsChildOrNull(nextSibling, parentGroup);

    for (const KeyCycle& cycle : cycles) {
        if (!cycle.isValid()) {
            throw LogicError("createKeyFace: invalid input cycle.");
        }
    }

    detail::Operations ops(parentGroup->complex());
    core::Span<Node*> sourceNodes;
    return ops.createKeyFace(std::move(cycles), parentGroup, nextSibling, sourceNodes, t);
}

KeyFace*
createKeyFace(KeyCycle cycle, Group* parentGroup, Node* nextSibling, core::AnimTime t) {
    core::Array<KeyCycle> cycles(1, std::move(cycle));
    return createKeyFace(std::move(cycles), parentGroup, nextSibling, t);
}

void removeNode(Node* node, bool removeFreeVertices) {
    if (!node) {
        throw LogicError("removeNode: node is nullptr.");
    }
    detail::Operations ops(node->complex());
    ops.removeNode(node, removeFreeVertices);
}

void removeNodeSmart(Node* node, bool removeFreeVertices) {
    if (!node) {
        throw LogicError("removeNodeSmart: node is nullptr.");
    }
    detail::Operations ops(node->complex());
    ops.removeNodeSmart(node, removeFreeVertices);
}

void moveToGroup(Node* node, Group* parentGroup, Node* nextSibling) {
    if (!node) {
        throw LogicError("moveToGroup: node is nullptr.");
    }
    if (!parentGroup) {
        throw LogicError("moveToGroup: parentGroup is nullptr.");
    }
    checkIsChildOrNull(nextSibling, parentGroup);
    detail::Operations ops(node->complex());
    ops.moveToGroup(node, parentGroup, nextSibling);
}

void setKeyVertexPosition(KeyVertex* vertex, const geometry::Vec2d& pos) {
    if (!vertex) {
        throw LogicError("setKeyVertexPosition: vertex is nullptr.");
    }
    detail::Operations ops(vertex->complex());
    return ops.setKeyVertexPosition(vertex, pos);
}

void setKeyEdgeCurvePoints(KeyEdge* edge, const geometry::SharedConstVec2dArray& points) {
    if (!edge) {
        throw LogicError("setKeyEdgeCurvePoints: edge is nullptr.");
    }
    detail::Operations ops(edge->complex());
    return ops.setKeyEdgeCurvePoints(edge, points);
}

void setKeyEdgeCurveWidths(KeyEdge* edge, const core::SharedConstDoubleArray& widths) {
    if (!edge) {
        throw LogicError("setKeyEdgeCurveWidths: edge is nullptr.");
    }
    detail::Operations ops(edge->complex());
    return ops.setKeyEdgeCurveWidths(edge, widths);
}

VGC_VACOMPLEX_API
void setKeyEdgeSamplingParameters(
    KeyEdge* edge,
    const geometry::CurveSamplingParameters& parameters) {

    if (!edge) {
        throw LogicError("setKeyEdgeSamplingParameters: edge is nullptr.");
    }
    detail::Operations ops(edge->complex());
    return ops.setKeyEdgeSamplingParameters(edge, parameters);
}

} // namespace vgc::vacomplex::ops
