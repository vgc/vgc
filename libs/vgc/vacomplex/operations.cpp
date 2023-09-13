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

void softDelete(core::ConstSpan<Node*> nodes, bool deleteIsolatedVertices) {
    if (nodes.isEmpty()) {
        return;
    }
    Complex* complex0 = nodes.first()->complex();

    for (Node* node : nodes) {
        if (!node) {
            throw LogicError("softDelete: a node is nullptr.");
        }
        if (node->complex() != complex0) {
            throw LogicError(
                "softDelete: a node is from a different complex than the others.");
        }
    }

    detail::Operations ops(complex0);
    ops.softDelete(nodes, deleteIsolatedVertices);
}

core::Array<KeyCell*>
simplify(core::Span<KeyVertex*> kvs, core::Span<KeyEdge*> kes, bool smoothJoins) {

    Complex* complex = nullptr;
    core::AnimTime t0 = {};
    if (kvs.isEmpty()) {
        if (kes.isEmpty()) {
            return {};
        }
        complex = kes[0]->complex();
        t0 = kes[0]->time();
    }
    else {
        complex = kvs[0]->complex();
        t0 = kvs[0]->time();
    }

    for (auto it = kvs.begin() + 1; it != kvs.end(); ++it) {
        KeyVertex* kv = *it;
        if (std::find(kvs.begin(), it, kv) != it) {
            throw LogicError("simplify: duplicate vertex in list.");
        }
        if (kv->complex() != complex) {
            throw LogicError("simplify: a key vertex is from a different complex than "
                             "the others or edges.");
        }
        if (kv->time() != t0) {
            throw LogicError("simplify: a key vertex is from a different time than the "
                             "others or edges.");
        }
    }

    for (auto it = kes.begin() + 1; it != kes.end(); ++it) {
        KeyEdge* ke = *it;
        if (std::find(kes.begin(), it, ke) != it) {
            throw LogicError("simplify: duplicate edge in list.");
        }
        if (ke->complex() != complex) {
            throw LogicError("simplify: a key edge is from a different complex than "
                             "the others or vertices.");
        }
        if (ke->time() != t0) {
            throw LogicError("simplify: a key edge is from a different time than the "
                             "others or vertices.");
        }
    }

    detail::Operations ops(complex);
    return ops.simplify(kvs, kes, smoothJoins);
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

KeyEdge* glueKeyOpenEdges(core::Span<KeyHalfedge> khs) {

    constexpr bool isClosed = false;
    Complex* complex = checkGlueKeyEdges_<isClosed>(khs);
    detail::Operations ops(complex);
    return ops.glueKeyOpenEdges(khs);
}

KeyEdge* glueKeyOpenEdges(core::Span<KeyEdge*> kes) {

    constexpr bool isClosed = false;
    Complex* complex = checkGlueKeyEdges_<isClosed>(kes);
    detail::Operations ops(complex);
    return ops.glueKeyOpenEdges(kes);
}

KeyEdge* glueKeyClosedEdges(core::Span<KeyHalfedge> khs) {
    constexpr bool isClosed = true;
    Complex* complex = checkGlueKeyEdges_<isClosed>(khs);
    detail::Operations ops(complex);
    return ops.glueKeyClosedEdges(khs);
}

KeyEdge* glueKeyClosedEdges(core::Span<KeyEdge*> kes) {
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

VertexCutEdgeResult vertexCutEdge(KeyEdge* ke, const geometry::CurveParameter& parameter) {
    if (!ke) {
        throw LogicError("vertexCutEdge: ke is nullptr.");
    }
    detail::Operations ops(ke->complex());
    return ops.vertexCutEdge(ke, parameter);
}

Cell* uncutAtKeyVertex(KeyVertex* kv, bool smoothJoin) {
    if (!kv) {
        throw LogicError("uncutAtKeyVertex: kv is nullptr.");
    }
    detail::Operations ops(kv->complex());
    detail::UncutAtKeyVertexResult res = ops.uncutAtKeyVertex(kv, smoothJoin);
    if (res.success) {
        VGC_ASSERT(res.resultKe || res.resultKf);
        if (res.resultKe) {
            return res.resultKe;
        }
        else {
            return res.resultKf;
        }
    }
    else {
        return nullptr;
    }
}

Cell* uncutAtKeyEdge(KeyEdge* ke) {
    if (!ke) {
        throw LogicError("uncutAtKeyEdge: ke is nullptr.");
    }
    detail::Operations ops(ke->complex());
    detail::UncutAtKeyEdgeResult res = ops.uncutAtKeyEdge(ke);
    if (res.success) {
        VGC_ASSERT(res.resultKf);
        return res.resultKf;
    }
    else {
        return nullptr;
    }
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

namespace {

template<typename T, typename Range>
void arrayUniteWith(core::Array<T>& dst, const Range& src) {
    for (const auto& srcNode : src) {
        if (!dst.contains(srcNode)) {
            dst.append(srcNode);
        }
    }
}

template<typename T, typename Range>
void arrayDifferenceWith(core::Array<T>& dst, const Range& src) {
    for (const auto& srcNode : src) {
        dst.removeOne(srcNode);
    }
}

Group* checkRaiseLowerPreConditions(core::ConstSpan<Node*> targets) {

    if (targets.isEmpty()) {
        return nullptr;
    }

    if (targets.contains(nullptr)) {
        throw LogicError("Cannot raise/lower nodes: one of nodes is null.");
    }

    Node* node0 = targets.first();
    Group* group0 = node0->parentGroup();
    Complex* complex0 = node0->complex();
    for (Node* node : targets.subspan(1)) {
        if (node->complex() != complex0) {
            throw LogicError("Cannot raise/lower nodes: One of the nodes is "
                             "from a different complex than the others.");
        }
        if (node->parentGroup() != group0) {
            throw LogicError("Cannot raise/lower nodes: One of the nodes is "
                             "from a different group than the others.");
        }
    }

    return group0;
}

core::Array<geometry::Rect2d> computeBoundingBoxes(
    core::ConstSpan<Node*> nodes,
    core::Array<Node*>& visibleNodes,
    core::AnimTime t) {

    core::Array<geometry::Rect2d> res;
    for (Node* node : nodes) {
        geometry::Rect2d bbox = node->boundingBoxAt(t);
        if (!bbox.isEmpty()) {
            visibleNodes.append(node);
            res.append(bbox);
        }
    }
    return res;
}

bool overlapsWith(
    const core::Array<geometry::Rect2d>& bboxes,
    Node* node,
    core::AnimTime t) {

    geometry::Rect2d nodeBbox = node->boundingBoxAt(t);
    for (const geometry::Rect2d& bbox : bboxes) {
        if (nodeBbox.intersects(bbox)) {
            return true;
        }
    }
    return false;
}

} // namespace

// Assumes same group
void raise(core::ConstSpan<Node*> targets, core::AnimTime t) {

    // Check pre-condition and get group that contains all target nodes.
    //
    Group* group = checkRaiseLowerPreConditions(targets);
    if (!group) {
        return;
    }

    // Compute bounding boxes of target nodes.
    //
    core::Array<Node*> visibleTargets;
    core::Array<geometry::Rect2d> targetBboxes =
        computeBoundingBoxes(targets, visibleTargets, t);

    // Iterate from bottom to collect all target nodes and their boundary until
    // we found all n targets.
    //
    // Note that we do not compute the boundary of targets in advance, since we
    // only want to start collecting the boundary of a given target node once
    // that target node is itself collected.
    //
    core::Array<Node*> collected;
    core::Array<Node*> collectedBoundary;
    Int n = visibleTargets.length();
    Int i = 0;
    Node* node = group->firstChild();
    while (node && i < n) {
        if (visibleTargets.contains(node)) {
            ++i;
            collected.append(node);
            if (Cell* cell = node->toCell()) {
                arrayUniteWith(collectedBoundary, cell->boundary());
            }
        }
        else if (collectedBoundary.contains(node)) {
            collected.append(node);
        }
        node = node->nextSibling();
    }

    // Continue iterating and collecting the boundary of target nodes until we
    // find a node which is not in this boundary and that overlaps with one of
    // the target nodes.
    //
    // Once such overlapping node is found, compute the destination node (i.e.,
    // where to move the collected nodes):
    // - If the overlapping node is a group, the destination node is the group.
    // - If the overlapping node is a cell, the destination node is the
    //   top-most node in the closure of the overlapping node, excluding the
    //   collected boundary.
    //
    Node* destinationNode = nullptr;
    while (node) {
        if (collectedBoundary.contains(node)) {
            collected.append(node);
        }
        else {
            if (overlapsWith(targetBboxes, node, t)) {
                destinationNode = node;
                if (Cell* cell = node->toCell()) {
                    core::Array<Node*> cellBoundary(cell->boundary());
                    arrayDifferenceWith(cellBoundary, collectedBoundary);
                    Node* topMost = topMostInGroupAbove(node, cellBoundary);
                    if (topMost) {
                        destinationNode = topMost;
                    }
                }
                break;
            }
        }
        node = node->nextSibling();
    }

    // Continue iterating and collecting the boundary of target nodes until we
    // reach the destination node. If there is no destination node (i.e., there
    // was no overlapping node above the targets), then we move the collected
    // nodes to the top of the group.
    //
    while (node && node != destinationNode) {
        if (collectedBoundary.contains(node)) {
            collected.append(node);
        }
        node = node->nextSibling();
    }
    if (!node || !destinationNode) {
        destinationNode = group->lastChild();
    }

    // Move the collected nodes.
    //
    detail::Operations ops(group->complex());
    for (Node* cn : collected) {
        ops.moveToGroup(cn, group, destinationNode->nextSibling());
        destinationNode = cn;
    }
}

void lower(core::ConstSpan<Node*> targets, core::AnimTime t) {

    // Check pre-condition and get group that contains all target nodes.
    //
    Group* group = checkRaiseLowerPreConditions(targets);
    if (!group) {
        return;
    }

    // Compute bounding boxes of target nodes.
    //
    core::Array<Node*> visibleTargets;
    core::Array<geometry::Rect2d> targetBboxes =
        computeBoundingBoxes(targets, visibleTargets, t);

    // Iterate from top to collect all target nodes and their star until we
    // found all n targets.
    //
    core::Array<Node*> collected;
    core::Array<Node*> collectedStar;
    Int n = visibleTargets.length();
    Int i = 0;
    Node* node = group->lastChild();
    while (node && i < n) {
        if (visibleTargets.contains(node)) {
            ++i;
            collected.append(node);
            if (Cell* cell = node->toCell()) {
                arrayUniteWith(collectedStar, cell->star());
            }
        }
        else if (collectedStar.contains(node)) {
            collected.append(node);
        }
        node = node->previousSibling();
    }

    // Continue iterating and collecting the star of target nodes until we
    // find a node which is not in this star and that overlaps with one of
    // the target nodes.
    //
    // Once such overlapping node is found, compute the destination node (i.e.,
    // where to move the collected nodes):
    // - If the overlapping node is a group, the destination node is the group.
    // - If the overlapping node is a cell, the destination node is the
    //   bottom-most node in the opening of the overlapping node, excluding the
    //   collected star.
    //
    Node* destinationNode = nullptr;
    while (node) {
        if (collectedStar.contains(node)) {
            collected.append(node);
        }
        else {
            if (overlapsWith(targetBboxes, node, t)) {
                destinationNode = node;
                if (Cell* cell = node->toCell()) {
                    core::Array<Node*> cellStar(cell->star());
                    arrayDifferenceWith(cellStar, collectedStar);
                    Node* bottomMost = bottomMostInGroupBelow(node, cellStar);
                    if (bottomMost) {
                        destinationNode = bottomMost;
                    }
                }
                break;
            }
        }
        node = node->previousSibling();
    }

    // Continue iterating and collecting the star of target nodes until we
    // reach the destination node. If there is no destination node (i.e., there
    // was no overlapping node above the targets), then we move the collected
    // nodes to the bottom of the group.
    //
    while (node && node != destinationNode) {
        if (collectedStar.contains(node)) {
            collected.append(node);
        }
        node = node->previousSibling();
    }
    if (!node || !destinationNode) {
        destinationNode = group->firstChild();
    }

    // Move the collected nodes.
    //
    detail::Operations ops(group->complex());
    for (Node* cn : collected) {
        ops.moveToGroup(cn, group, destinationNode);
        destinationNode = cn;
    }
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
