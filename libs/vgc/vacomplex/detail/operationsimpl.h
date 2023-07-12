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
#include <vgc/vacomplex/edgegeometry.h>

namespace vgc::vacomplex::detail {

class VGC_VACOMPLEX_API Operations {
    friend vacomplex::KeyEdgeGeometry;
    using GroupChildrenIterator = Group;
    //using GroupChildrenConstIterator = decltype(Group::children_)::const_iterator;

public:
    // Creates an instance of `Operations` for operating on the given `complex`.
    //
    // Throws a `LogicError` if `complex` is nullptr.
    //
    Operations(Complex* complex);

    ~Operations();

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
        NodeSourceOperation sourceOperation = {},
        core::AnimTime t = {});

    // Assumes `nextSibling` is either `nullptr` or a child of `parentGroup`.
    // Assumes `startVertex` is from the same `Complex` as `parentGroup`.
    // Assumes `endVertex` is from the same `Complex` as `parentGroup`.
    //
    KeyEdge* createKeyOpenEdge(
        KeyVertex* startVertex,
        KeyVertex* endVertex,
        const std::shared_ptr<KeyEdgeGeometry>& geometry,
        Group* parentGroup,
        Node* nextSibling = nullptr,
        NodeSourceOperation sourceOperation = {});

    // Assumes `nextSibling` is either `nullptr` or a child of `parentGroup`.
    //
    KeyEdge* createKeyClosedEdge(
        const std::shared_ptr<KeyEdgeGeometry>& geometry,
        Group* parentGroup,
        Node* nextSibling = nullptr,
        NodeSourceOperation sourceOperation = {},
        core::AnimTime t = {});

    // Assumes `cycles` are valid.
    // Assumes `nextSibling` is either `nullptr` or a child of `parentGroup`.
    //
    KeyFace* createKeyFace(
        core::Array<KeyCycle> cycles,
        Group* parentGroup,
        Node* nextSibling = nullptr,
        NodeSourceOperation sourceOperation = {},
        core::AnimTime t = {});

    void hardDelete(Node* node, bool deleteIsolatedVertices);
    void softDelete(Node* node, bool deleteIsolatedVertices);

    KeyVertex*
    glueKeyVertices(core::Span<KeyVertex*> kvs, const geometry::Vec2d& position);

    // Assumes `khes` does not contain more than one halfedge for any edge.
    //
    KeyEdge* glueKeyOpenEdges(
        core::Span<KeyHalfedge> khes,
        std::shared_ptr<KeyEdgeGeometry> geometry,
        const geometry::Vec2d& startPosition,
        const geometry::Vec2d& endPosition);

    // Assumes `khes` does not contain more than one halfedge for any edge.
    //
    KeyEdge* glueKeyClosedEdges( //
        core::Span<KeyHalfedge> khes,
        std::shared_ptr<KeyEdgeGeometry> geometry);

    core::Array<KeyEdge*> unglueKeyEdges(KeyEdge* ke);
    core::Array<KeyVertex*> unglueKeyVertices(
        KeyVertex* kv,
        core::Array<std::pair<core::Id, core::Array<KeyEdge*>>>& ungluedKeyEdges);

    void moveToGroup(Node* node, Group* parentGroup, Node* nextSibling = nullptr);
    void moveBelowBoundary(Node* node);

    void setKeyVertexPosition(KeyVertex* kv, const geometry::Vec2d& pos);

    void
    setKeyEdgeGeometry(KeyEdge* ke, const std::shared_ptr<KeyEdgeGeometry>& geometry);

    void setKeyEdgeSamplingQuality(KeyEdge* ke, geometry::CurveSamplingQuality quality);

private:
    Complex* complex_ = nullptr;

    void onNodeCreated_(Node* node, NodeSourceOperation sourceOperation);
    void onNodeInserted_(Node* node, Node* oldParent, NodeInsertionType insertionType);
    void onNodeModified_(Node* node, NodeModificationFlags diffFlags);

    // Creates a new node and inserts it to the complex.
    //
    // Note: we cannot use make_unique due to the non-public constructor,
    // which is why we have to use `new` here.
    //
    template<class T, typename... Args>
    T* createNode_(NodeSourceOperation sourceOperation, Args&&... args) {
        core::Id id = core::genId();
        T* node = new T(id, std::forward<Args>(args)...);
        std::unique_ptr<T> nodePtr(node);
        Complex::NodePtrMap& nodes = complex()->nodes_;
        bool emplaced = nodes.try_emplace(id, std::move(nodePtr)).second;
        if (!emplaced) {
            throw LogicError("Id collision error.");
        }
        onNodeCreated_(node, std::move(sourceOperation));
        return node;
    }

    // Creates a new node at the given location.
    //
    template<class T, typename... Args>
    T* createNodeAt_(
        Group* parentGroup,
        Node* nextSibling,
        NodeSourceOperation sourceOperation,
        Args&&... args) {

        T* node = createNode_<T>(std::move(sourceOperation), std::forward<Args>(args)...);
        moveToGroup(node, parentGroup, nextSibling);
        return node;
    }

    void insertNodeBeforeSibling_(Node* node, Node* nextSibling);
    void insertNodeAfterSibling_(Node* node, Node* previousSibling);
    void insertNodeAsFirstChild_(Node* node, Group* parent);
    void insertNodeAsLastChild_(Node* node, Group* parent);

    static Node* findTopMost(core::Span<Node*> nodes);

    // Assumes node has no children.
    void destroyNode_(Node* node);

    // Assumes that all descendants of all `nodes` are also in `nodes`.
    void destroyNodes_(const std::unordered_set<Node*>& nodes);

    void onBoundaryChanged_(Cell* cell);
    void onGeometryChanged_(Cell* cell);
    void onBoundaryMeshChanged_(Cell* cell);
    void dirtyMesh_(Cell* cell);

    // Adds the `boundingCell` to the boundary of the `boundedCell`.
    //
    // This updates both boundingCell->star_ and boundedCell->boundary_ and
    // sets the appropriate ModifiedNode flags.
    //
    // Throw a LogicError if either `boundingCell` or `boundedCell` is null.
    //
    void addToBoundary_(Cell* boundedCell, Cell* boundingCell);

    // Adds all the cells in the given `cycle` to the boundary of the `face`.
    //
    void addToBoundary_(FaceCell* face, const KeyCycle& cycle);

    void substitute_(KeyVertex* oldVertex, KeyVertex* newVertex);
    void substitute_(const KeyHalfedge& oldHalfedge, const KeyHalfedge& newHalfedge);

    // Other helper methods
    void collectDependentNodes_(Node* node, std::unordered_set<Node*>& dependentNodes);

    Int countUses_(KeyVertex* kv);
    Int countUses_(KeyEdge* ke);
};

} // namespace vgc::vacomplex::detail

#endif // VGC_VACOMPLEX_DETAIL_OPERATIONS_H
