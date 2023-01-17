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

#ifndef VGC_TOPOLOGY_DETAIL_OPERATIONS_H
#define VGC_TOPOLOGY_DETAIL_OPERATIONS_H

#include <unordered_set>

#include <vgc/topology/vac.h>

namespace vgc::topology::detail {

class VGC_TOPOLOGY_API Operations {
    using VacGroupChildrenIterator = VacGroup;
    //using VacGroupChildrenConstIterator = decltype(VacGroup::children_)::const_iterator;

public:
    /// Assumes `nextSibling` is either `nullptr` or a child of `parentGroup`.
    static VacGroup*
    createVacGroup(core::Id id, VacGroup* parentGroup, VacNode* nextSibling = nullptr);

    /// Assumes `nextSibling` is either `nullptr` or a child of `parentGroup`.
    static KeyVertex* createKeyVertex(
        core::Id id,
        VacGroup* parentGroup,
        VacNode* nextSibling = nullptr,
        core::AnimTime t = {});

    /// Assumes `nextSibling` is either `nullptr` or a child of `parentGroup`.
    /// Assumes `startVertex` is from the same `Vac` as `parentGroup`.
    /// Assumes `endVertex` is from the same `Vac` as `parentGroup`.
    ///
    static KeyEdge* createKeyEdge(
        core::Id id,
        VacGroup* parentGroup,
        KeyVertex* startVertex,
        KeyVertex* endVertex,
        VacNode* nextSibling = nullptr,
        core::AnimTime t = {});

    /// Assumes `nextSibling` is either `nullptr` or a child of `parentGroup`.
    static KeyEdge* createKeyClosedEdge(
        core::Id id,
        VacGroup* parentGroup,
        VacNode* nextSibling = nullptr,
        core::AnimTime t = {});

    static void removeNode(VacNode* node, bool removeFreeVertices);
    static void removeNodeSmart(VacNode* node, bool removeFreeVertices);

    static void
    moveToGroup(VacNode* node, VacGroup* parentGroup, VacNode* nextSibling = nullptr);

    static void setKeyVertexPosition(KeyVertex* v, const geometry::Vec2d& pos);

    static void
    setKeyEdgeCurvePoints(KeyEdge* e, const geometry::SharedConstVec2dArray& points);
    static void
    setKeyEdgeCurveWidths(KeyEdge* e, const core::SharedConstDoubleArray& widths);

private:
    static void
    collectDependentNodes_(VacNode* node, std::unordered_set<VacNode*>& dependentNodes);
};

} // namespace vgc::topology::detail

#endif // VGC_TOPOLOGY_DETAIL_OPERATIONS_H
