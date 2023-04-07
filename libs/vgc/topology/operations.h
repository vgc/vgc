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
/// Throws `LogicError` if `parentGroup` is nullptr.
///
VGC_TOPOLOGY_API
VacGroup* createVacGroup(VacGroup* parentGroup, VacNode* nextSibling = nullptr);

/// Throws `NotAChildError` if `nextSibling` is not a child of `parentGroup` or `nullptr`.
/// Throws `LogicError` if `parentGroup` is nullptr.
///
VGC_TOPOLOGY_API
KeyVertex* createKeyVertex(
    const geometry::Vec2d& position,
    VacGroup* parentGroup,
    VacNode* nextSibling = nullptr,
    core::AnimTime t = {});

/// Throws `NotAChildError` if `nextSibling` is not a child of `parentGroup` or `nullptr`.
/// Throws `LogicError` if `parentGroup`, `startVertex`, or `endVertex` is nullptr.
///
VGC_TOPOLOGY_API
KeyEdge* createKeyOpenEdge(
    KeyVertex* startVertex,
    KeyVertex* endVertex,
    const geometry::SharedConstVec2dArray& points,
    const core::SharedConstDoubleArray& widths,
    VacGroup* parentGroup,
    VacNode* nextSibling = nullptr,
    core::AnimTime t = {});

/// Throws `NotAChildError` if `nextSibling` is not a child of `parentGroup` or `nullptr`.
/// Throws `LogicError` if `parentGroup` is nullptr.
///
VGC_TOPOLOGY_API
KeyEdge* createKeyClosedEdge(
    const geometry::SharedConstVec2dArray& points,
    const core::SharedConstDoubleArray& widths,
    VacGroup* parentGroup,
    VacNode* nextSibling = nullptr,
    core::AnimTime t = {});

/// Throws `NotAChildError` if `nextSibling` is not a child of `parentGroup` or `nullptr`.
/// Throws `LogicError` if `parentGroup` is nullptr or one of the given `cycles` is not valid.
///
VGC_TOPOLOGY_API
KeyFace* createKeyFace(
    core::Array<KeyCycle> cycles,
    VacGroup* parentGroup,
    VacNode* nextSibling = nullptr,
    core::AnimTime t = {});

/// Throws `NotAChildError` if `nextSibling` is not a child of `parentGroup` or `nullptr`.
/// Throws `LogicError` if `parentGroup` is nullptr or one of the given `cycles` is not valid.
///
VGC_TOPOLOGY_API
KeyFace* createKeyFace(
    KeyCycle cycle,
    VacGroup* parentGroup,
    VacNode* nextSibling = nullptr,
    core::AnimTime t = {});

VGC_TOPOLOGY_API
void removeNode(VacNode* node, bool removeFreeVertices);

VGC_TOPOLOGY_API
void removeNodeSmart(VacNode* node, bool removeFreeVertices);

/// Throws `NotAChildError` if `nextSibling` is not a child of `parentGroup` or `nullptr`.
// XXX should check if node belongs to same vac.
VGC_TOPOLOGY_API
void moveToGroup(VacNode* node, VacGroup* parentGroup, VacNode* nextSibling = nullptr);

VGC_TOPOLOGY_API
void setKeyVertexPosition(KeyVertex* vertex, const geometry::Vec2d& pos);

VGC_TOPOLOGY_API
void setKeyEdgeCurvePoints(KeyEdge* edge, const geometry::SharedConstVec2dArray& points);

VGC_TOPOLOGY_API
void setKeyEdgeCurveWidths(KeyEdge* edge, const core::SharedConstDoubleArray& widths);

} // namespace vgc::topology::ops

#endif // VGC_TOPOLOGY_OPERATIONS_H
