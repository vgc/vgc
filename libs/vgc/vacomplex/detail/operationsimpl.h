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
    // Creates an instance of `Operations` for operating on the given `complex`.
    //
    // Throws a `LogicError` if `complex` is nullptr.
    //
    Operations(Complex* complex);

    // Returns the `Complex` that this `Operations` operates on.
    //
    // This function never returns nullptr.
    //
    Complex* complex() const {
        return complex_;
    }

    Group* createRootGroup();

    // Assumes `nextSibling` is either `nullptr` or a child of `parentGroup`.
    //
    Group* createGroup(Group* parentGroup, Node* nextSibling = nullptr);

    // Assumes `nextSibling` is either `nullptr` or a child of `parentGroup`.
    //
    KeyVertex* createKeyVertex(
        const geometry::Vec2d& position,
        Group* parentGroup,
        Node* nextSibling = nullptr,
        core::Span<Node*> operationSourceNodes = {},
        core::AnimTime t = {});

    // Assumes `nextSibling` is either `nullptr` or a child of `parentGroup`.
    // Assumes `startVertex` is from the same `Complex` as `parentGroup`.
    // Assumes `endVertex` is from the same `Complex` as `parentGroup`.
    //
    KeyEdge* createKeyOpenEdge(
        KeyVertex* startVertex,
        KeyVertex* endVertex,
        const geometry::SharedConstVec2dArray& points,
        const core::SharedConstDoubleArray& widths,
        Group* parentGroup,
        Node* nextSibling = nullptr,
        core::Span<Node*> operationSourceNodes = {},
        core::AnimTime t = {});

    // Assumes `nextSibling` is either `nullptr` or a child of `parentGroup`.
    //
    KeyEdge* createKeyClosedEdge(
        const geometry::SharedConstVec2dArray& points,
        const core::SharedConstDoubleArray& widths,
        Group* parentGroup,
        Node* nextSibling = nullptr,
        core::Span<Node*> operationSourceNodes = {},
        core::AnimTime t = {});

    // Assumes `cycles` are valid.
    // Assumes `nextSibling` is either `nullptr` or a child of `parentGroup`.
    //
    KeyFace* createKeyFace(
        core::Array<KeyCycle> cycles,
        Group* parentGroup,
        Node* nextSibling = nullptr,
        core::Span<Node*> operationSourceNodes = {},
        core::AnimTime t = {});

    void removeNode(Node* node, bool removeFreeVertices);
    void removeNodeSmart(Node* node, bool removeFreeVertices);

    void moveToGroup(Node* node, Group* parentGroup, Node* nextSibling = nullptr);

    void setKeyVertexPosition(KeyVertex* kv, const geometry::Vec2d& pos);

    void
    setKeyEdgeCurvePoints(KeyEdge* ke, const geometry::SharedConstVec2dArray& points);

    void setKeyEdgeCurveWidths(KeyEdge* ke, const core::SharedConstDoubleArray& widths);

    void setKeyEdgeSamplingParameters(
        KeyEdge* ke,
        const geometry::CurveSamplingParameters& parameters);

private:
    Complex* complex_;

    // Creates a new node and inserts it to the complex.
    //
    // Note: we cannot use make_unique due to the non-public constructor,
    // which is why we have to use `new` here.
    //
    template<class T, typename... Args>
    T* createNode_(Args&&... args) {
        T* node = new T(args...);
        std::unique_ptr<T> nodePtr(node);
        Complex::NodePtrMap& nodes = complex()->nodes_;
        core::Id id = node->id();
        bool emplaced = nodes.try_emplace(id, std::move(nodePtr)).second;
        if (!emplaced) {
            throw LogicError("Id collision error.");
        }
        return node;
    }

    // Other helper methods
    void collectDependentNodes_(Node* node, std::unordered_set<Node*>& dependentNodes);
    void dirtyGeometry_(Cell* cell);
};

} // namespace vgc::vacomplex::detail

#endif // VGC_VACOMPLEX_DETAIL_OPERATIONS_H
