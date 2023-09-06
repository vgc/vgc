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

#include <vgc/core/arithmetic.h>
#include <vgc/vacomplex/celldata.h>
#include <vgc/vacomplex/complex.h>
#include <vgc/vacomplex/keyedgedata.h>

namespace vgc::vacomplex {

namespace detail {

struct UncutAtKeyVertexResult {
    core::Id removedKeId1 = 0;
    core::Id removedKeId2 = 0;
    KeyEdge* resultKe = nullptr;
    bool success = false;
};

struct UncutAtKeyEdgeResult {
    core::Id removedKfId1 = 0;
    core::Id removedKfId2 = 0;
    KeyFace* resultKf = nullptr;
    bool success = false;
};

class VGC_VACOMPLEX_API Operations {
private:
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
        core::AnimTime t = {});

    // Assumes `nextSibling` is either `nullptr` or a child of `parentGroup`.
    // Assumes `startVertex` is from the same `Complex` as `parentGroup`.
    // Assumes `endVertex` is from the same `Complex` as `parentGroup`.
    //
    KeyEdge* createKeyOpenEdge(
        KeyVertex* startVertex,
        KeyVertex* endVertex,
        std::unique_ptr<KeyEdgeData>&& geometry,
        Group* parentGroup,
        Node* nextSibling = nullptr);

    // Assumes `nextSibling` is either `nullptr` or a child of `parentGroup`.
    //
    KeyEdge* createKeyClosedEdge(
        std::unique_ptr<KeyEdgeData>&& geometry,
        Group* parentGroup,
        Node* nextSibling = nullptr,
        core::AnimTime t = {});

    // Assumes `cycles` are valid.
    // Assumes `nextSibling` is either `nullptr` or a child of `parentGroup`.
    //
    KeyFace* createKeyFace(
        core::Array<KeyCycle> cycles,
        Group* parentGroup,
        Node* nextSibling = nullptr,
        core::AnimTime t = {});

    void hardDelete(Node* node, bool deleteIsolatedVertices = false);

    void softDelete(core::ConstSpan<Node*> nodes, bool deleteIsolatedVertices = false);

    core::Array<KeyCell*>
    simplify(core::Span<KeyVertex*> kvs, core::Span<KeyEdge*> kes, bool smoothJoins);

    KeyVertex*
    glueKeyVertices(core::Span<KeyVertex*> kvs, const geometry::Vec2d& position);

    // Assumes `khes` does not contain more than one halfedge for any edge.
    //
    KeyEdge* glueKeyOpenEdges(core::ConstSpan<KeyHalfedge> khes);

    // Assumes `kes` does not contain any edge more than once.
    //
    KeyEdge* glueKeyOpenEdges(core::ConstSpan<KeyEdge*> kes);

    // Assumes `khes` does not contain more than one halfedge for any edge.
    //
    KeyEdge* glueKeyClosedEdges( //
        core::ConstSpan<KeyHalfedge> khes);

    // Assumes `kes` does not contain any edge more than once.
    //
    KeyEdge* glueKeyClosedEdges(core::ConstSpan<KeyEdge*> kes);

    core::Array<KeyEdge*> unglueKeyEdges(KeyEdge* ke);
    core::Array<KeyVertex*> unglueKeyVertices(
        KeyVertex* kv,
        core::Array<std::pair<core::Id, core::Array<KeyEdge*>>>& ungluedKeyEdges);

    UncutAtKeyVertexResult uncutAtKeyVertex(KeyVertex* kv, bool smoothJoin);

    UncutAtKeyEdgeResult uncutAtKeyEdge(KeyEdge* ke);

    void moveToGroup(Node* node, Group* parentGroup, Node* nextSibling = nullptr);
    void moveBelowBoundary(Node* node);

    void setKeyVertexPosition(KeyVertex* kv, const geometry::Vec2d& pos);

    void setKeyEdgeData(KeyEdge* ke, std::unique_ptr<KeyEdgeData>&& geometry);

    void setKeyEdgeSamplingQuality(KeyEdge* ke, geometry::CurveSamplingQuality quality);

private:
    Complex* complex_ = nullptr;

    void onNodeCreated_(Node* node);
    void onNodeInserted_(Node* node, Node* oldParent, NodeInsertionType insertionType);
    void onNodeModified_(Node* node, NodeModificationFlags diffFlags);
    void onNodePropertyModified_(Node* node, core::StringId name);

    // Creates a new node and inserts it to the complex.
    //
    // Note: we cannot use make_unique due to the non-public constructor,
    // which is why we have to use `new` here.
    //
    template<class T, typename... Args>
    T* createNode_(Args&&... args) {
        core::Id id = core::genId();
        T* node = new T(id, std::forward<Args>(args)...);
        std::unique_ptr<T> nodePtr(node);
        Complex::NodePtrMap& nodes = complex()->nodes_;
        bool emplaced = nodes.try_emplace(id, std::move(nodePtr)).second;
        if (!emplaced) {
            throw LogicError("Id collision error.");
        }
        onNodeCreated_(node);
        return node;
    }

    // Creates a new node at the given location.
    //
    template<class T, typename... Args>
    T* createNodeAt_(Group* parentGroup, Node* nextSibling, Args&&... args) {

        T* node = createNode_<T>(std::forward<Args>(args)...);
        moveToGroup(node, parentGroup, nextSibling);
        return node;
    }

    void insertNodeBeforeSibling_(Node* node, Node* nextSibling);
    void insertNodeAfterSibling_(Node* node, Node* previousSibling);
    void insertNodeAsFirstChild_(Node* node, Group* parent);
    void insertNodeAsLastChild_(Node* node, Group* parent);

    static Node* findTopMost(core::ConstSpan<Node*> nodes);
    static Node* findBottomMost(core::ConstSpan<Node*> nodes);

    // Assumes node has no children.
    void destroyChildlessNode_(Node* node);

    // Assumes that all descendants of all `nodes` are also in `nodes`.
    void destroyNodes_(core::ConstSpan<Node*> nodes);

    friend vacomplex::CellProperties;
    friend vacomplex::CellData;
    friend vacomplex::KeyEdgeData;

    void onGeometryChanged_(Cell* cell);
    void onPropertyChanged_(Cell* cell, core::StringId name);
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

    void removeFromBoundary_(Cell* boundedCell, Cell* boundingCell);

    void substitute_(KeyVertex* oldVertex, KeyVertex* newVertex);

    // Substitutes open with open or closed with closed.
    //
    void substitute_(const KeyHalfedge& oldHalfedge, const KeyHalfedge& newHalfedge);

    // Other helper methods
    void collectDependentNodes_(Node* node, std::unordered_set<Node*>& dependentNodes);

    KeyEdge* glueKeyOpenEdges_(core::ConstSpan<KeyHalfedge> khes);

    KeyEdge* glueKeyClosedEdges_(
        core::ConstSpan<KeyHalfedge> khes,
        core::ConstSpan<double> uOffsets);

    static KeyPath subPath(const KeyCycle& cycle, Int first, Int last);
    static KeyPath concatPath(const KeyPath& p1, const KeyPath& p2);

    struct UncutAtKeyVertexInfo_ {
        KeyFace* kf = nullptr;
        Int cycleIndex = 0;
        KeyHalfedge khe1 = {};
        KeyHalfedge khe2 = {};
        bool isValid = false;
    };

    UncutAtKeyVertexInfo_ prepareUncutAtKeyVertex_(KeyVertex* kv);

    struct UncutAtKeyEdgeInfo_ {
        KeyFace* kf1 = nullptr;
        Int cycleIndex1 = 0;
        Int componentIndex1 = 0;
        KeyFace* kf2 = nullptr;
        Int cycleIndex2 = 0;
        Int componentIndex2 = 0;
        bool isValid = false;
    };

    UncutAtKeyEdgeInfo_ prepareUncutAtKeyEdge_(KeyEdge* ke);

    Int countSteinerUses_(KeyVertex* kv);

    Int countUses_(KeyVertex* kv);
    Int countUses_(KeyEdge* ke);
};

} // namespace detail

} // namespace vgc::vacomplex

#endif // VGC_VACOMPLEX_DETAIL_OPERATIONS_H
