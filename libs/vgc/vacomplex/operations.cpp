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
    return ops.createKeyVertex(position, parentGroup, nextSibling, t);
}

KeyEdge* createKeyClosedEdge(
    std::unique_ptr<KeyEdgeData>&& data,
    Group* parentGroup,
    Node* nextSibling,
    core::AnimTime t) {

    if (!parentGroup) {
        throw LogicError("createKeyClosedEdge: parentGroup is nullptr.");
    }
    checkIsChildOrNull(nextSibling, parentGroup);
    detail::Operations ops(parentGroup->complex());
    return ops.createKeyClosedEdge(std::move(data), parentGroup, nextSibling, t);
}

KeyEdge* createKeyOpenEdge(
    KeyVertex* startVertex,
    KeyVertex* endVertex,
    std::unique_ptr<KeyEdgeData>&& data,
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
        startVertex, endVertex, std::move(data), parentGroup, nextSibling);
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
    return ops.createKeyFace(std::move(cycles), parentGroup, nextSibling, t);
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

namespace {

template<bool isClosed>
Complex* checkGlueKeyEdges_(core::ConstSpan<KeyHalfedge> khs) {

    constexpr std::string_view opName =
        isClosed ? "glueKeyClosedEdges" : "glueKeyOpenEdges";

    constexpr std::string_view wrongEdgeType =
        isClosed ? "an open edge" : "a closed edge";

    if (khs.isEmpty()) {
        throw LogicError(core::format( //
            "{}: requires at least 1 halfedge.",
            opName));
    }

    for (const KeyHalfedge& kh : khs) {
        if (kh.edge()->isClosed() != isClosed) {
            throw LogicError(core::format( //
                "{}: a key halfedge is from {}.",
                opName,
                wrongEdgeType));
        }
    }

    KeyHalfedge kh0 = khs[0];
    Complex* complex = kh0.edge()->complex();
    core::AnimTime t0 = kh0.edge()->time();
    std::unordered_set<KeyEdge*> seen;
    seen.insert(kh0.edge());

    for (const KeyHalfedge& kh : khs.subspan(1)) {
        KeyEdge* ke = kh.edge();
        bool inserted = seen.insert(ke).second;
        if (!inserted) {
            throw LogicError(core::format( //
                "{}: cannot glue two key halfedges that use the same key edge.",
                opName));
        }
        if (ke->complex() != complex) {
            throw LogicError(core::format( //
                "{}: a key halfedge is from a different complex than the others.",
                opName));
        }
        if (ke->time() != t0) {
            throw LogicError(core::format( //
                "{}: a key halfedge is from a different time than the others.",
                opName));
        }
    }

    return complex;
}

template<bool isClosed>
Complex* checkGlueKeyEdges_(core::ConstSpan<KeyEdge*> kes) {

    constexpr std::string_view opName =
        isClosed ? "glueKeyClosedEdges" : "glueKeyOpenEdges";

    constexpr std::string_view wrongEdgeType =
        isClosed ? "an open edge" : "a closed edge";

    if (kes.isEmpty()) {
        throw LogicError(core::format( //
            "{}: requires at least 1 edge.",
            opName));
    }

    for (KeyEdge* ke : kes) {
        if (ke->isClosed() != isClosed) {
            throw LogicError(core::format( //
                "{}: a key edge is from {}.",
                opName,
                wrongEdgeType));
        }
    }

    KeyEdge* ke0 = kes[0];
    Complex* complex = ke0->complex();
    core::AnimTime t0 = ke0->time();

    for (auto it = kes.begin() + 1; it != kes.end(); ++it) {
        KeyEdge* ke = *it;
        if (std::find(kes.begin(), it, ke) != it) {
            throw LogicError(core::format( //
                "{}: cannot glue a key edge to itself.",
                opName));
        }
        if (ke->complex() != complex) {
            throw LogicError(core::format( //
                "{}: a key edge is from a different complex than the others.",
                opName));
        }
        if (ke->time() != t0) {
            throw LogicError(core::format( //
                "{}: a key edge is from a different time than the others.",
                opName));
        }
    }

    return complex;
}

} // namespace

KeyEdge* glueKeyOpenEdges(
    core::Span<KeyHalfedge> khs) {

    constexpr bool isClosed = false;
    Complex* complex = checkGlueKeyEdges_<isClosed>(khs);
    detail::Operations ops(complex);
    return ops.glueKeyOpenEdges(khs);
}

KeyEdge* glueKeyOpenEdges(
    core::Span<KeyEdge*> kes) {

    constexpr bool isClosed = false;
    Complex* complex = checkGlueKeyEdges_<isClosed>(kes);
    detail::Operations ops(complex);
    return ops.glueKeyOpenEdges(kes);
}

KeyEdge*
glueKeyClosedEdges(core::Span<KeyHalfedge> khs) {
    constexpr bool isClosed = true;
    Complex* complex = checkGlueKeyEdges_<isClosed>(khs);
    detail::Operations ops(complex);
    return ops.glueKeyClosedEdges(khs);
}

KeyEdge*
glueKeyClosedEdges(core::Span<KeyEdge*> kes) {
    constexpr bool isClosed = true;
    Complex* complex = checkGlueKeyEdges_<isClosed>(kes);
    detail::Operations ops(complex);
    return ops.glueKeyClosedEdges(kes);
}

core::Array<KeyEdge*> unglueKeyEdges(KeyEdge* ke) {
    if (!ke) {
        throw LogicError("unglueKeyEdges: ke is nullptr.");
    }
    detail::Operations ops(ke->complex());
    return ops.unglueKeyEdges(ke);
}

core::Array<KeyVertex*> unglueKeyVertices(
    KeyVertex* kv,
    core::Array<std::pair<core::Id, core::Array<KeyEdge*>>>& ungluedKeyEdges) {

    if (!kv) {
        throw LogicError("unglueKeyVertices: kv is nullptr.");
    }
    detail::Operations ops(kv->complex());
    return ops.unglueKeyVertices(kv, ungluedKeyEdges);
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

void setKeyEdgeData(KeyEdge* edge, std::unique_ptr<KeyEdgeData>&& data) {
    if (!edge) {
        throw LogicError("setKeyEdgeData: edge is nullptr.");
    }
    detail::Operations ops(edge->complex());
    return ops.setKeyEdgeData(edge, std::move(data));
}

void setKeyEdgeSamplingQuality(KeyEdge* edge, geometry::CurveSamplingQuality quality) {
    if (!edge) {
        throw LogicError("setKeyEdgeSamplingQuality: edge is nullptr.");
    }
    detail::Operations ops(edge->complex());
    return ops.setKeyEdgeSamplingQuality(edge, quality);
}

} // namespace vgc::vacomplex::ops
