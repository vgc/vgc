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

#ifndef VGC_TOPOLOGY_OPERATIONS_H
#define VGC_TOPOLOGY_OPERATIONS_H

#include <vgc/core/id.h>
#include <vgc/topology/api.h>
#include <vgc/topology/detail/operationsimpl.h>
#include <vgc/topology/exceptions.h>
#include <vgc/topology/vac.h>

namespace vgc::topology::ops {

// TODO: impl the exceptions mentioned in comments, verify preconditions checks and throws.
// Indeed functions in detail::Operations do not throw and are unchecked.

/// Throws `NotAChildError` if `nextSibling` is not a child of `parentGroup` or `nullptr`.
/// Throws `IdCollisionError` if `id` is already used by a node of the `Vac` of `parentGroup`.
///
inline VacGroup*
createVacGroup(core::Id id, VacGroup* parentGroup, VacNode* nextSibling = nullptr) {
    if (!parentGroup) {
        throw LogicError("createVacGroup: parentGroup is nullptr.");
    }
    return detail::Operations::createVacGroup(id, parentGroup, nextSibling);
}

/// Throws `NotAChildError` if `nextSibling` is not a child of `parentGroup` or `nullptr`.
inline VacGroup* createVacGroup(VacGroup* parentGroup, VacNode* nextSibling = nullptr) {
    return createVacGroup(core::genId(), parentGroup, nextSibling);
}

/// Throws `NotAChildError` if `nextSibling` is not a child of `parentGroup` or `nullptr`.
/// Throws `IdCollisionError` if `id` is already used by a node of the `Vac` of `parentGroup`.
///
inline KeyVertex* createKeyVertex(
    core::Id id,
    VacGroup* parentGroup,
    VacNode* nextSibling = nullptr,
    core::AnimTime t = {}) {

    if (!parentGroup) {
        throw LogicError("createKeyVertex: parentGroup is nullptr.");
    }
    return detail::Operations::createKeyVertex(id, parentGroup, nextSibling, t);
}

/// Throws `NotAChildError` if `nextSibling` is not a child of `parentGroup` or `nullptr`.
inline KeyVertex* createKeyVertex(
    VacGroup* parentGroup,
    VacNode* nextSibling = nullptr,
    core::AnimTime t = {}) {

    return createKeyVertex(core::genId(), parentGroup, nextSibling, t);
}

/// Throws `NotAChildError` if `nextSibling` is not a child of `parentGroup` or `nullptr`.
/// Throws `IdCollisionError` if `id` is already used by a node of the `Vac` of `parentGroup`.
///
VGC_TOPOLOGY_API
KeyEdge* createKeyOpenEdge(
    core::Id id,
    VacGroup* parentGroup,
    KeyVertex* startVertex,
    KeyVertex* endVertex,
    VacNode* nextSibling = nullptr,
    core::AnimTime t = {});

/// Throws `NotAChildError` if `nextSibling` is not a child of `parentGroup` or `nullptr`.
inline KeyEdge* createKeyOpenEdge(
    VacGroup* parentGroup,
    KeyVertex* startVertex,
    KeyVertex* endVertex,
    VacNode* nextSibling = nullptr,
    core::AnimTime t = {}) {

    return createKeyOpenEdge(
        core::genId(), parentGroup, startVertex, endVertex, nextSibling, t);
}

/// Throws `NotAChildError` if `nextSibling` is not a child of `parentGroup` or `nullptr`.
/// Throws `IdCollisionError` if `id` is already used by a node of the `Vac` of `parentGroup`.
///
inline KeyEdge* createKeyClosedEdge(
    core::Id id,
    VacGroup* parentGroup,
    VacNode* nextSibling = nullptr,
    core::AnimTime t = {}) {

    if (!parentGroup) {
        throw LogicError("createKeyClosedEdge: parentGroup is nullptr.");
    }
    return detail::Operations::createKeyClosedEdge(id, parentGroup, nextSibling, t);
}

/// Throws `NotAChildError` if `nextSibling` is not a child of `parentGroup` or `nullptr`.
inline KeyEdge* createKeyClosedEdge(
    VacGroup* parentGroup,
    VacNode* nextSibling = nullptr,
    core::AnimTime t = {}) {

    return createKeyClosedEdge(core::genId(), parentGroup, nextSibling, t);
}

inline void removeNode(VacNode* node, bool removeFreeVertices) {
    if (!node) {
        throw LogicError("removeNode: node is nullptr.");
    }
    detail::Operations::removeNode(node, removeFreeVertices);
}

inline void removeNodeSmart(VacNode* node, bool removeFreeVertices) {
    if (!node) {
        throw LogicError("removeNodeSmart: node is nullptr.");
    }
    detail::Operations::removeNodeSmart(node, removeFreeVertices);
}

/// Throws `NotAChildError` if `nextSibling` is not a child of `parentGroup` or `nullptr`.
// XXX should check if node belongs to same vac.
inline void
moveToGroup(VacNode* node, VacGroup* parentGroup, VacNode* nextSibling = nullptr) {
    if (!node) {
        throw LogicError("moveToGroup: node is nullptr.");
    }
    if (!parentGroup) {
        throw LogicError("moveToGroup: parentGroup is nullptr.");
    }
    return detail::Operations::moveToGroup(node, parentGroup, nextSibling);
}

inline void setKeyVertexPosition(KeyVertex* vertex, const geometry::Vec2d& pos) {
    if (!vertex) {
        throw LogicError("setKeyVertexPosition: vertex is nullptr.");
    }
    return detail::Operations::setKeyVertexPosition(vertex, pos);
}

inline void
setKeyEdgeCurvePoints(KeyEdge* edge, const geometry::SharedConstVec2dArray& points) {
    if (!edge) {
        throw LogicError("setKeyEdgeCurvePoints: edge is nullptr.");
    }
    return detail::Operations::setKeyEdgeCurvePoints(edge, points);
}

inline void
setKeyEdgeCurveWidths(KeyEdge* edge, const core::SharedConstDoubleArray& widths) {
    if (!edge) {
        throw LogicError("setKeyEdgeCurveWidths: edge is nullptr.");
    }
    return detail::Operations::setKeyEdgeCurveWidths(edge, widths);
}

} // namespace vgc::topology::ops

#endif // VGC_TOPOLOGY_OPERATIONS_H
