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

#ifndef VGC_VACOMPLEX_DETAIL_OPERATIONS_H
#define VGC_VACOMPLEX_DETAIL_OPERATIONS_H

#include <unordered_set>

#include <vgc/vacomplex/complex.h>

namespace vgc::vacomplex::detail {

class VGC_VACOMPLEX_API Operations {
    using GroupChildrenIterator = Group;
    //using GroupChildrenConstIterator = decltype(Group::children_)::const_iterator;

public:
    static Group* createRootGroup(Complex* complex);

    /// Assumes `nextSibling` is either `nullptr` or a child of `parentGroup`.
    static Group* createGroup(Group* parentGroup, Node* nextSibling = nullptr);

    /// Assumes `nextSibling` is either `nullptr` or a child of `parentGroup`.
    static KeyVertex* createKeyVertex(
        const geometry::Vec2d& position,
        Group* parentGroup,
        Node* nextSibling = nullptr,
        core::Span<Node*> operationSourceNodes = {},
        core::AnimTime t = {});

    /// Assumes `nextSibling` is either `nullptr` or a child of `parentGroup`.
    /// Assumes `startVertex` is from the same `Complex` as `parentGroup`.
    /// Assumes `endVertex` is from the same `Complex` as `parentGroup`.
    ///
    static KeyEdge* createKeyOpenEdge(
        KeyVertex* startVertex,
        KeyVertex* endVertex,
        const geometry::SharedConstVec2dArray& points,
        const core::SharedConstDoubleArray& widths,
        Group* parentGroup,
        Node* nextSibling = nullptr,
        core::Span<Node*> operationSourceNodes = {},
        core::AnimTime t = {});

    /// Assumes `nextSibling` is either `nullptr` or a child of `parentGroup`.
    static KeyEdge* createKeyClosedEdge(
        const geometry::SharedConstVec2dArray& points,
        const core::SharedConstDoubleArray& widths,
        Group* parentGroup,
        Node* nextSibling = nullptr,
        core::Span<Node*> operationSourceNodes = {},
        core::AnimTime t = {});

    /// Assumes `cycles` are valid.
    /// Assumes `nextSibling` is either `nullptr` or a child of `parentGroup`.
    ///
    static KeyFace* createKeyFace(
        core::Array<KeyCycle> cycles,
        Group* parentGroup,
        Node* nextSibling = nullptr,
        core::Span<Node*> operationSourceNodes = {},
        core::AnimTime t = {});

    static void removeNode(Node* node, bool removeFreeVertices);
    static void removeNodeSmart(Node* node, bool removeFreeVertices);

    static void moveToGroup(Node* node, Group* parentGroup, Node* nextSibling = nullptr);

    static void setKeyVertexPosition(KeyVertex* kv, const geometry::Vec2d& pos);

    static void
    setKeyEdgeCurvePoints(KeyEdge* ke, const geometry::SharedConstVec2dArray& points);
    static void
    setKeyEdgeCurveWidths(KeyEdge* ke, const core::SharedConstDoubleArray& widths);

    static void setKeyEdgeSamplingParameters(
        KeyEdge* ke,
        const geometry::CurveSamplingParameters& parameters);

private:
    static void
    collectDependentNodes_(Node* node, std::unordered_set<Node*>& dependentNodes);

    static void dirtyGeometry_(Cell* cell);
};

} // namespace vgc::vacomplex::detail

#endif // VGC_VACOMPLEX_DETAIL_OPERATIONS_H
