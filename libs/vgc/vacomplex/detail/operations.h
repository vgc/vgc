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
#include <vgc/vacomplex/operations.h>

namespace vgc::vacomplex::detail {

// Note: this struct cannot be publicized as is, as we first need to generalize
// it in the presence of inbetween vertices.
//
struct UncutAtKeyVertexResult {
    core::Id removedKeId1 = 0;
    core::Id removedKeId2 = 0;
    KeyEdge* resultKe = nullptr;
    KeyFace* resultKf = nullptr;
    bool success = false; // whether an uncut actually happened
};

// Note: this struct cannot be publicized as is, as we first need to generalize
// it in the presence of inbetween edges.
//
struct UncutAtKeyEdgeResult {
    core::Id removedKfId1 = 0;
    core::Id removedKfId2 = 0;
    KeyFace* resultKf = nullptr;
    bool success = false; // whether an uncut actually happened
};

class VGC_VACOMPLEX_API Operations {
private:
    Complex* complex_ = nullptr;

    friend vacomplex::CellProperties;
    friend vacomplex::CellData;
    friend vacomplex::KeyEdgeData;

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

    //-------------------------------------------------------------------------
    //                Methods implemented in notify.cpp
    //-------------------------------------------------------------------------

private:
    void onNodeCreated_(Node* node);
    void onNodeDestroyed_(core::Id nodeId);
    void onNodeInserted_(Node* node, Node* oldParent, NodeInsertionType insertionType);
    void onNodeModified_(Node* node, NodeModificationFlags diffFlags);
    void onNodePropertyModified_(Node* node, core::StringId name);
    void onBoundaryChanged_(Cell* boundedCell, Cell* boundingCell);
    void onGeometryChanged_(Cell* cell);

    //-------------------------------------------------------------------------
    //                Methods implemented in boundary.cpp
    //-------------------------------------------------------------------------

private:
    // Adds or remove the `boundingCell` to the boundary of the `boundedCell`.
    //
    // This updates both boundingCell->star_ and boundedCell->boundary_ and
    // sets the appropriate ModifiedNode flags.
    //
    // Throw a LogicError if either `boundingCell` or `boundedCell` is null.
    //
    void addToBoundary_(Cell* boundedCell, Cell* boundingCell);
    void removeFromBoundary_(Cell* boundedCell, Cell* boundingCell);

    // Convenient helper that calls addToBoundary_(face, cell)
    // for all the cells in the given `cycle`.
    //
    void addToBoundary_(FaceCell* face, const KeyCycle& cycle);

    // Convenient helper that calls addToBoundary_(face, cell)
    // for all the cells in the given `path`.
    //
    void addToBoundary_(FaceCell* face, const KeyPath& path);

    //-------------------------------------------------------------------------
    //                Methods implemented in create.cpp
    //-------------------------------------------------------------------------

private:
    // Creates a new node and inserts it to the complex.
    template<class T, typename... Args>
    T* createNode_(Args&&... args);

    // Creates a new node at the given location.
    template<class T, typename... Args>
    T* createNodeAt_(Group* parentGroup, Node* nextSibling, Args&&... args);

public:
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
        KeyEdgeData&& data,
        Group* parentGroup,
        Node* nextSibling = nullptr);

    // Assumes `nextSibling` is either `nullptr` or a child of `parentGroup`.
    //
    KeyEdge* createKeyClosedEdge(
        KeyEdgeData&& data,
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

    // Assumes `kf` is not null.
    // Assumes `cycle` is valid and matches kf's complex and time.
    //
    void addCycleToFace(KeyFace* kf, KeyCycle cycle);

    void setKeyVertexPosition(KeyVertex* kv, const geometry::Vec2d& pos);

    //-------------------------------------------------------------------------
    //                Methods implemented in delete.cpp
    //-------------------------------------------------------------------------

private:
    template<typename TNodeRange>
    void deleteWithDependents_(
        TNodeRange nodes,
        bool deleteIsolatedVertices = false,
        bool tryRepairingStarCells = true);

    void delete_(
        std::unordered_set<Node*> descendants,
        bool deleteIsolatedVertices = false,
        bool tryRepairingStarCells = true);

    // Assumes node has no children.
    void destroyChildlessNode_(Node* node);

    // Assumes that all descendants of all `nodes` are also in `nodes`.
    void destroyNodes_(core::ConstSpan<Node*> nodes);

public:
    void hardDelete(core::ConstSpan<Node*> nodes, bool deleteIsolatedVertices = false);

    void hardDelete(Node* node, bool deleteIsolatedVertices = false);

    void softDelete(core::ConstSpan<Node*> nodes, bool deleteIsolatedVertices = false);

    //-------------------------------------------------------------------------
    //                Methods implemented in simplify.cpp
    //-------------------------------------------------------------------------

public:
    core::Array<KeyCell*>
    simplify(core::Span<KeyVertex*> kvs, core::Span<KeyEdge*> kes, bool smoothJoins);

    //-------------------------------------------------------------------------
    //                Methods implemented in glue.cpp
    //-------------------------------------------------------------------------

private:
    // Substitutes open with open or closed with closed.
    void substituteVertex_(KeyVertex* oldVertex, KeyVertex* newVertex);
    void substituteEdge_(const KeyHalfedge& oldHalfedge, const KeyHalfedge& newHalfedge);
    KeyEdge* glueKeyOpenEdges_(core::ConstSpan<KeyHalfedge> khes);
    KeyEdge* glueKeyClosedEdges_(
        core::ConstSpan<KeyHalfedge> khes,
        core::ConstSpan<double> uOffsets);

public:
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

    //-------------------------------------------------------------------------
    //                Methods implemented in unglue.cpp
    //-------------------------------------------------------------------------

private:
    Int countUses_(KeyVertex* kv);
    Int countUses_(KeyEdge* ke);

public:
    core::Array<KeyEdge*> unglueKeyEdges(KeyEdge* ke);

    core::Array<KeyVertex*> unglueKeyVertices(
        KeyVertex* kv,
        core::Array<std::pair<core::Id, core::Array<KeyEdge*>>>& ungluedKeyEdges);

    //-------------------------------------------------------------------------
    //                Methods implemented in cut.cpp
    //-------------------------------------------------------------------------

public:
    CutEdgeResult
    cutEdge(KeyEdge* ke, core::ConstSpan<geometry::CurveParameter> parameters);

    CutFaceResult cutGlueFace(
        KeyFace* kf,
        const KeyEdge* ke,
        OneCycleCutPolicy oneCycleCutPolicy = OneCycleCutPolicy::Auto,
        TwoCycleCutPolicy twoCycleCutPolicy = TwoCycleCutPolicy::Auto);

    CutFaceResult cutGlueFace(
        KeyFace* kf,
        const KeyHalfedge& khe,
        KeyFaceVertexUsageIndex startIndex,
        KeyFaceVertexUsageIndex endIndex,
        OneCycleCutPolicy oneCycleCutPolicy = OneCycleCutPolicy::Auto,
        TwoCycleCutPolicy twoCycleCutPolicy = TwoCycleCutPolicy::Auto);

    CutFaceResult cutFaceWithClosedEdge(
        KeyFace* kf,
        KeyEdgeData&& geometry,
        OneCycleCutPolicy oneCycleCutPolicy = OneCycleCutPolicy::Auto);

    CutFaceResult cutFaceWithOpenEdge(
        KeyFace* kf,
        KeyEdgeData&& geometry,
        KeyFaceVertexUsageIndex startIndex,
        KeyFaceVertexUsageIndex endIndex,
        OneCycleCutPolicy oneCycleCutPolicy = OneCycleCutPolicy::Auto,
        TwoCycleCutPolicy twoCycleCutPolicy = TwoCycleCutPolicy::Auto);

    CutFaceResult cutFaceWithOpenEdge(
        KeyFace* kf,
        KeyEdgeData&& geometry,
        KeyVertex* startVertex,
        KeyVertex* endVertex,
        OneCycleCutPolicy oneCycleCutPolicy = OneCycleCutPolicy::Auto,
        TwoCycleCutPolicy twoCycleCutPolicy = TwoCycleCutPolicy::Auto);

    void cutGlueFaceWithVertex(KeyFace* kf, KeyVertex* kv);

    KeyVertex* cutFaceWithVertex(KeyFace* kf, const geometry::Vec2d& position);

    //-------------------------------------------------------------------------
    //                Methods implemented in uncut.cpp
    //-------------------------------------------------------------------------

private:
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

public:
    UncutAtKeyVertexResult uncutAtKeyVertex(KeyVertex* kv, bool smoothJoin);

    UncutAtKeyEdgeResult uncutAtKeyEdge(KeyEdge* ke);

    //-------------------------------------------------------------------------
    //                Methods implemented in intersect.cpp
    //-------------------------------------------------------------------------

public:
    // Assumes group is non-null, all edges are non-null, and no edge appears twice.
    IntersectResult intersectWithGroup(
        core::ConstSpan<KeyEdge*> edges,
        Group* group,
        const IntersectSettings& settings);

    //-------------------------------------------------------------------------
    //                Methods implemented in move.cpp
    //-------------------------------------------------------------------------

private:
    void insertNodeBeforeSibling_(Node* node, Node* nextSibling);
    void insertNodeAfterSibling_(Node* node, Node* previousSibling);
    void insertNodeAsFirstChild_(Node* node, Group* parent);
    void insertNodeAsLastChild_(Node* node, Group* parent);

public:
    void moveToGroup(Node* node, Group* parentGroup, Node* nextSibling = nullptr);
    void moveBelowBoundary(Node* node);

    static Node* findTopMost(core::ConstSpan<Node*> nodes);
    static Node* findBottomMost(core::ConstSpan<Node*> nodes);
};

} // namespace vgc::vacomplex::detail

#endif // VGC_VACOMPLEX_DETAIL_OPERATIONS_H
