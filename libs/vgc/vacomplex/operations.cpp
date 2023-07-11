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
    const std::shared_ptr<KeyEdgeGeometry>& geometry,
    Group* parentGroup,
    Node* nextSibling,
    core::AnimTime t) {

    if (!parentGroup) {
        throw LogicError("createKeyClosedEdge: parentGroup is nullptr.");
    }
    checkIsChildOrNull(nextSibling, parentGroup);
    detail::Operations ops(parentGroup->complex());
    return ops.createKeyClosedEdge(geometry, parentGroup, nextSibling, {}, t);
}

KeyEdge* createKeyOpenEdge(
    KeyVertex* startVertex,
    KeyVertex* endVertex,
    const std::shared_ptr<KeyEdgeGeometry>& geometry,
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
    return ops.createKeyOpenEdge(
        startVertex,
        endVertex,
        geometry,
        parentGroup,
        nextSibling,
        NodeSourceOperation());
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
    return ops.createKeyFace(
        std::move(cycles), parentGroup, nextSibling, NodeSourceOperation(), t);
}

KeyFace*
createKeyFace(KeyCycle cycle, Group* parentGroup, Node* nextSibling, core::AnimTime t) {
    core::Array<KeyCycle> cycles(1, std::move(cycle));
    return createKeyFace(std::move(cycles), parentGroup, nextSibling, t);
}

void hardDelete(Node* node, bool deleteIsolatedVertices) {
    if (!node) {
        throw LogicError("hardDelete: node is nullptr.");
    }
    detail::Operations ops(node->complex());
    ops.hardDelete(node, deleteIsolatedVertices);
}

void softDelete(Node* node, bool deleteIsolatedVertices) {
    if (!node) {
        throw LogicError("softDelete: node is nullptr.");
    }
    detail::Operations ops(node->complex());
    ops.softDelete(node, deleteIsolatedVertices);
}

KeyVertex*
glueKeyVertices(core::Span<KeyVertex*> vertices, const geometry::Vec2d& position) {
    if (vertices.isEmpty()) {
        throw LogicError("glueKeyVertices: requires at least 1 vertex.");
    }
    KeyVertex* kv0 = vertices[0];

    if (vertices.contains(nullptr)) {
        throw LogicError("glueKeyVertices: a vertex in vertices is nullptr.");
    }

    Complex* complex = kv0->complex();
    core::AnimTime t0 = kv0->time();
    for (KeyVertex* vertex : vertices.subspan(1)) {
        if (vertex->complex() != complex) {
            throw LogicError("glueKeyVertices: a key vertex is from a different complex "
                             "than the others.");
        }
        if (vertex->time() != t0) {
            throw LogicError("glueKeyVertices: a key vertex is from a different time "
                             "than the others.");
        }
    }

    detail::Operations ops(complex);
    return ops.glueKeyVertices(vertices, position);
}

KeyEdge* glueKeyOpenEdges(
    core::Span<KeyHalfedge> khes,
    std::shared_ptr<KeyEdgeGeometry> geometry,
    const geometry::Vec2d& startPosition,
    const geometry::Vec2d& endPosition) {

    if (khes.isEmpty()) {
        throw LogicError("glueKeyOpenEdges: requires at least 1 halfedge.");
    }
    KeyHalfedge khe0 = khes[0];
    if (khe0.isClosed()) {
        throw LogicError("glueKeyOpenEdges: a key halfedge is from a closed edge.");
    }

    Complex* complex = khe0.edge()->complex();
    core::AnimTime t0 = khe0.edge()->time();
    std::unordered_set<KeyEdge*> seen;
    seen.insert(khe0.edge());

    for (const KeyHalfedge& khe : khes.subspan(1)) {
        KeyEdge* ke = khe.edge();
        bool inserted = seen.insert(ke).second;
        if (!inserted) {
            throw LogicError("glueKeyOpenEdges: cannot glue two key halfedges that use "
                             "the same key edge.");
        }
        if (ke->complex() != complex) {
            throw LogicError("glueKeyOpenEdges: a key halfedge is from a different "
                             "complex than the others.");
        }
        if (ke->time() != t0) {
            throw LogicError("glueKeyOpenEdges: a key halfedge is from a different time "
                             "than the others.");
        }
        if (ke->isClosed()) {
            throw LogicError("glueKeyOpenEdges: a key halfedge is from a closed edge.");
        }
    }

    detail::Operations ops(complex);
    return ops.glueKeyOpenEdges(khes, std::move(geometry), startPosition, endPosition);
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

void moveBelowBoundary(Node* node) {
    if (!node) {
        throw LogicError("moveBelowBoundary: node is nullptr.");
    }
    detail::Operations ops(node->complex());
    ops.moveBelowBoundary(node);
}

void setKeyVertexPosition(KeyVertex* vertex, const geometry::Vec2d& pos) {
    if (!vertex) {
        throw LogicError("setKeyVertexPosition: vertex is nullptr.");
    }
    detail::Operations ops(vertex->complex());
    return ops.setKeyVertexPosition(vertex, pos);
}

void setKeyEdgeGeometry(KeyEdge* edge, const std::shared_ptr<KeyEdgeGeometry>& geometry) {
    if (!edge) {
        throw LogicError("setKeyEdgeGeometry: edge is nullptr.");
    }
    detail::Operations ops(edge->complex());
    return ops.setKeyEdgeGeometry(edge, geometry);
}

void setKeyEdgeSamplingQuality(KeyEdge* edge, geometry::CurveSamplingQuality quality) {
    if (!edge) {
        throw LogicError("setKeyEdgeSamplingQuality: edge is nullptr.");
    }
    detail::Operations ops(edge->complex());
    return ops.setKeyEdgeSamplingQuality(edge, quality);
}

} // namespace vgc::vacomplex::ops
