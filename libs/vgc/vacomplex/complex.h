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

#ifndef VGC_VACOMPLEX_VAC_H
#define VGC_VACOMPLEX_VAC_H

#include <functional>
#include <unordered_map>
#include <unordered_set>

#include <vgc/core/array.h>
#include <vgc/core/flags.h>
#include <vgc/core/object.h>
#include <vgc/core/span.h>
#include <vgc/core/stringid.h>
#include <vgc/vacomplex/api.h>
#include <vgc/vacomplex/cell.h>
#include <vgc/vacomplex/exceptions.h>
#include <vgc/vacomplex/inbetweenedge.h>
#include <vgc/vacomplex/inbetweenface.h>
#include <vgc/vacomplex/inbetweenvertex.h>
#include <vgc/vacomplex/keycycle.h>
#include <vgc/vacomplex/keyedge.h>
#include <vgc/vacomplex/keyface.h>
#include <vgc/vacomplex/keyvertex.h>

namespace vgc::vacomplex {

VGC_DECLARE_OBJECT(Complex);

class ComplexDiff;

namespace detail {

class Operations;

} // namespace detail

/// \enum vgc::vacomplex::NodeModificationFlag
/// \brief Specifies the nature of a node modification.
///
/// This is used as part of `ComplexDiff`, the mechanism used to notify about
/// changes of a `Complex`.
///
/// \sa `Complex::nodesChanged`, `ComplexDiff::modifiedNodes()`,
///     `ModifiedNodeInfo::flags()`.
///
enum class NodeModificationFlag : UInt32 {

    /// This flag is set whenever the node's parent has changed.
    ///
    Reparented = 0x01,

    /// This flag is set whenever the node's children has changed, that is, a
    /// child has been added, removed, or its position in the list of children
    /// has changed.
    ///
    ChildrenChanged = 0x02,

    /// This flag is set whenever the topological boundary of the cell is
    /// changed, that is, whenever a cell has been added or removed to the
    /// `Cell::boundary()`.
    ///
    BoundaryChanged = 0x04,

    /// This flag is set whenever the topological star of the cell is changed,
    /// that is, whenever a cell has been added or removed to the
    /// `Cell::star()`.
    ///
    StarChanged = 0x08,

    /// This flag is set whenever the node's "authored geometry" has changed.
    ///
    /// For a KeyVertex, this means when its `position()` has changed.
    ///
    /// For a KeyEdge, this means when its `stroke()` or
    /// `strokeSamplingQuality()` has changed.
    ///
    /// For a KeyFace, this is currently never set since its geometry is fully
    /// implicitly defined by the geometry of its boundary. In the future, if
    /// we add a `WindingRule` attribute to faces, or ways to explicitly define
    /// the geometry of faces, then this flag would be set when changing it.
    ///
    GeometryChanged = 0x10,

    /// This flag is set whenever:
    ///
    /// 1. The "mesh" of a node has changed, and
    ///
    /// 2. The "mesh" of a node had been queried since the last time
    ///    this flag was set for this node.
    ///
    /// By "mesh", we mean the position of a vertex, the sampling of an edge,
    /// or the triangulation of a face.
    ///
    /// Note that it is possible that the `GeometryChanged` flag is set, while
    /// the `MeshChanged` flag is not set. This happens when the geometry of
    /// the node changed (and therefore, its mesh also changed), but had not b
    ///
    /// However, if the `MeshChanged` flag is set, then the `GeometryChanged`
    /// flag is not necessarily set, for example the triangulation of a face
    /// changes when the geometry of the boundary of the face changes, while
    /// the geometry of the face itself hasn't necessarily changed.
    ///
    // TODO: rename `CachedGeometryChanged`? This would more explicitly conveys
    // the idea that whether the flag is set or not depends on whether getter
    // functions where called (there is a mutable state / cache mechanism
    // involved).
    //
    // XXX: Do we actually still need this at all? It would make the API less
    // confusing and/or error prone with just GeometryChanged (and
    // BoundaryGeometryChanged). This flag was initially implemented before the
    // flag BoundaryGeometryChanged existed, and a lot of the architecture has
    // changed since. In particular, note that the virtual function
    // `Cell::dirtyMesh()`, always called just before setting this flag,
    // currently does nothing at all.
    //
    // However, we should keep in mind that this flag may still be useful for
    // the following reasons:
    //
    // 1. The mechanism `hasMeshBeenQueriedSinceLastDirtyEvent_` may avoid
    // recomputing data several times between repaints. But such optimization
    // should also be achievable (arguable more cleanly) by using dirty flags
    // in Workspace when receiving GeometryChanged.
    //
    // 2. In theory, it prevents writing attributes to the DOM when the
    // "authored geometry" hasn't changed. But currently, this has no practical
    // use. Indeed, when the boundary of a face changes, the fact that the face
    // does not receive GeometryChanged isn't very useful, since nothing would
    // be written to the DOM anyway (a face has no geometry).
    //
    // Note that currently, changing the `strokeSamplingQuality()` is
    // considered a change of the geometry. If this was considered as a change
    // of the mesh only, then maybe there would be more benefits to keep the
    // distinction between geometry change and mesh change.
    //
    // In conclusion, I think it would be worth investigating in more details
    // and try to remove this with benchmarks and see if there is any benefits
    // in keeping this. There is cleary a huge maintainance benefit in removing
    // it: the simpler the better.
    //
    MeshChanged = 0x20,

    /// This flag is set whenever at least one of the node's properties
    /// has changed, that is, its `cell->data().properties()`
    ///
    /// \sa `CellData`, `CellProperties`.
    ///
    PropertyChanged = 0x40,

    // This flag is set whenever the `transform` attribute of the node has
    // changed. This is not implemented yet.
    //
    //TransformChanged        = 0x80,

    /// This flag is set whenever:
    /// - `BoundaryChanged` is set on the cell, or
    /// - `GeometryChanged` is set on at least one cell in the boundary of the
    ///    cell.
    ///
    BoundaryGeometryChanged = 0x100,

    /// This flag is set whenever `MeshChanged` is set on at least one cell in
    /// the boundary of the cell.
    ///
    BoundaryMeshChanged = 0x200,

    /// Convenient enum value with all flags set.
    ///
    All = 0xFFFFFFFF,
};
VGC_DEFINE_FLAGS(NodeModificationFlags, NodeModificationFlag)

/// \class vgc::vacomplex::CreatedNodeInfo
/// \brief Provides information about nodes that have been created.
///
/// This is used as part of `ComplexDiff`, the mechanism used to notify about
/// changes of a `Complex`.
///
/// \sa `Complex::nodesChanged`, `ComplexDiff::createdNodes()`.
///
class VGC_VACOMPLEX_API CreatedNodeInfo {
public:
    /// Returns the ID of the created node.
    ///
    core::Id nodeId() const {
        return nodeId_;
    }

    /// Returns the created node.
    ///
    /// This may be a dangling pointer if the node has been destroyed since the
    /// `ComplexDiff` was emitted.
    ///
    // XXX NodeWeakPtr?
    //
    Node* node() const {
        return node_;
    }

private:
    friend ComplexDiff;

    core::Id nodeId_;
    Node* node_;

    CreatedNodeInfo(Node* node)
        : nodeId_(node->id())
        , node_(node) {
    }
};

/// \class vgc::vacomplex::DestroyedNodeInfo
/// \brief Provides information about nodes that have been destroyed.
///
/// This is used as part of `ComplexDiff`, the mechanism used to notify about
/// changes of a `Complex`.
///
/// \sa `Complex::nodesChanged`, `ComplexDiff::destroyedNodes()`.
///
class VGC_VACOMPLEX_API DestroyedNodeInfo {
public:
    /// Returns the ID of the destroyed node.
    ///
    core::Id nodeId() const {
        return nodeId_;
    }

private:
    friend ComplexDiff;

    core::Id nodeId_;

    explicit DestroyedNodeInfo(core::Id id)
        : nodeId_(id) {
    }
};

/// \class vgc::vacomplex::TransientNodeInfo
/// \brief Provides information about nodes that have been created then destroyed.
///
/// This is used as part of `ComplexDiff`, the mechanism used to notify about
/// changes of a `Complex`.
///
/// \sa `Complex::nodesChanged`, `ComplexDiff::transientNodes()`.
///
class VGC_VACOMPLEX_API TransientNodeInfo {
public:
    /// Returns the ID of the transient node.
    ///
    core::Id nodeId() const {
        return nodeId_;
    }

private:
    friend ComplexDiff;

    core::Id nodeId_;

    explicit TransientNodeInfo(core::Id nodeId)
        : nodeId_(nodeId) {
    }
};

/// \class vgc::vacomplex::ModifiedNodeInfo
/// \brief Provides information about nodes that have been modified.
///
/// This is used as part of `ComplexDiff`, the mechanism used to notify about
/// changes of a `Complex`.
///
/// \sa `Complex::nodesChanged`, `ComplexDiff::modifiedNodes()`.
///
class VGC_VACOMPLEX_API ModifiedNodeInfo {
public:
    /// Returns the ID of the modified node.
    ///
    core::Id nodeId() const {
        return nodeId_;
    }

    /// Returns the modified node.
    ///
    /// This may be a dangling pointer if the node has been destroyed since the
    /// `ComplexDiff` was emitted.
    ///
    // XXX NodeWeakPtr?
    //
    Node* node() const {
        return node_;
    }

    /// Returns which types of modification have occured on the node.
    ///
    NodeModificationFlags flags() const {
        return flags_;
    }

    /// Returns which node properties have been modified, if any.
    ///
    /// \sa `NodeModificationFlag::PropertyChanged`.
    ///
    const core::Array<core::StringId>& modifiedProperties() const {
        return modifiedProperties_;
    }

private:
    friend ComplexDiff;

    core::Id nodeId_ = {};
    Node* node_ = nullptr;
    NodeModificationFlags flags_ = {};
    core::Array<core::StringId> modifiedProperties_;

    explicit ModifiedNodeInfo(Node* node)
        : nodeId_(node->id())
        , node_(node) {
    }

    void setFlags(NodeModificationFlags flags) {
        flags_ = flags;
    }

    void insertModifiedProperty(core::StringId name) {
        flags_.set(NodeModificationFlag::PropertyChanged);
        if (!modifiedProperties_.contains(name)) {
            modifiedProperties_.append(name);
        }
    }
};

/// \enum vgc::vacomplex::NodeInsertionType
/// \brief Specifies the nature of a node insertion.
///
/// This is used as part of `ComplexDiff`, the mechanism used to notify about
/// changes of a `Complex`.
///
/// \sa `Complex::nodesChanged`, `ComplexDiff::insertions()`,
///     `NodeInsertionInfo::type()`.
///
enum class NodeInsertionType {
    BeforeSibling, ///< The node has been inserted just before a sibling node.
    AfterSibling,  ///< The node has been inserted just after a sibling node.
    FirstChild,    ///< The node has been inserted as first child of its parent node.
    LastChild,     ///< The node has been inserted as last child of its parent node.
};

/// \class vgc::vacomplex::NodeInsertionInfo
/// \brief Provides information about a node insertion.
///
/// This is used as part of `ComplexDiff`, the mechanism used to notify about
/// changes of a `Complex`.
///
/// \sa `Complex::nodesChanged`, `ComplexDiff::insertions()`.
///
class VGC_VACOMPLEX_API NodeInsertionInfo {
public:
    /// Returns the ID of the inserted node.
    ///
    core::Id nodeId() const {
        return nodeId_;
    }

    /// Returns the ID of the parent of the node just after the insertion
    /// happened.
    ///
    /// Note that this can be used regardless of the insertion `type()`, that
    /// is, it always returns the ID of the parent, even when the insertion
    /// type is `BeforeSibling` or `AfterSibling`.
    ///
    core::Id newParentId() const {
        return newParentId_;
    }

    /// If `type() == BeforeSibling`, returns the ID of the sibling before
    /// which the node has been inserted.
    ///
    /// If `type() == AfterSibling`, returns the ID of the sibling after
    /// which the node has been inserted.
    ///
    /// Otherwise, returns 0.
    ///
    core::Id newSiblingId() const {
        return newSiblingId_;
    }

    /// Returns the nature of the insertion, that is, whether the node has been
    /// inserted as first/last child of its parent node, or whether it has been
    /// inserted just before/after a sibling node.
    ///
    /// Note that as far as the `Complex` is concerned, inserting a node as
    /// `FirstChild` (resp. `LastChild`) can be equivalently expressed as
    /// inserting it as `BeforeSibling` (resp. `AfterSibling`), as long as the
    /// node is not an only child.
    ///
    /// However, there is a difference in intent that can be useful for
    /// synchronization purposes. For example, consider the following DOM:
    ///
    /// ```
    /// <group>
    ///   <vertex id="v1"/>
    ///   <text/>
    /// </group>
    /// ```
    ///
    /// which is kept in sync with a `Complex`:
    ///
    /// ```
    /// group
    ///   └ v1
    /// ```
    ///
    /// Note how the `text` element is only part of the DOM, but is not part
    /// of the complex.
    ///
    /// If you insert a new vertex v2 to the complex "as last child of the
    /// group", you get the same complex as if you insert it "just after v1":
    ///
    /// ```
    /// group
    ///   ├ v1
    ///   └ v2
    /// ```
    ///
    /// However, in the first case, you want the DOM to be updated to:
    ///
    /// ```
    /// <group>
    ///   <vertex id="v1"/>
    ///   <text/>
    ///   <vertex id="v2"/>
    /// </group>
    /// ```
    ///
    /// While in the second case, you want the DOM to be updated to:
    ///
    /// ```
    /// <group>
    ///   <vertex id="v1"/>
    ///   <vertex id="v2"/>
    ///   <text/>
    /// </group>
    /// ```
    ///
    /// This is why preserving this semantic difference is useful.
    ///
    NodeInsertionType type() const {
        return type_;
    }

private:
    friend ComplexDiff;

    core::Id nodeId_;
    core::Id newParentId_;
    core::Id newSiblingId_;
    NodeInsertionType type_;

    NodeInsertionInfo(
        core::Id nodeId,
        core::Id newParentId,
        core::Id newSiblingId,
        NodeInsertionType type)

        : nodeId_(nodeId)
        , newParentId_(newParentId)
        , newSiblingId_(newSiblingId)
        , type_(type) {
    }
};

/// \class vgc::vacomplex::ComplexDiff
/// \brief Stores information about what changed in a `Complex`.
///
/// This is provided as argument to the signal `Complex::nodesChanged()`, so
/// that observers can be made aware of what has changed in the complex, and
/// update themselves accordingly.
///
class VGC_VACOMPLEX_API ComplexDiff {
public:
    /// Returns all the nodes that have been created during the operation and
    /// that are still alive at the end of the operation.
    ///
    /// This does not include `transientNodes()`.
    ///
    const core::Array<CreatedNodeInfo>& createdNodes() const {
        return createdNodes_;
    }

    /// Returns all the nodes that have been destroyed during the operation.
    ///
    /// This includes both `transientNodes()` and previously existing nodes that
    /// have been destroyed.
    ///
    const core::Array<DestroyedNodeInfo>& destroyedNodes() const {
        return destroyedNodes_;
    }

    /// Returns all the nodes that have been both created and destroyed during
    /// the operation.
    ///
    /// Information about these nodes is useful since their ID can be referred
    /// in `NodeInsertionInfo`, for example when a node has been moved next to
    /// a transient node.
    ///
    const core::Array<TransientNodeInfo>& transientNodes() const {
        return transientNodes_;
    }

    /// Returns all the nodes that have been modified during the operation and
    /// that are still alive at the end of the operation.
    ///
    /// This does not include `transientNodes()` or `destroyedNodes()`, but may
    /// include `createdNodes()`.
    ///
    const core::Array<ModifiedNodeInfo>& modifiedNodes() const {
        return modifiedNodes_;
    }

    /// Returns the history of all node insertions that happened during the
    /// operation, in chronological order.
    ///
    /// A node insertion occurs either when a node is created, or when an
    /// existing node is moved to a different location in the node hierarchy.
    ///
    /// Unlike most other functions in `ComplexDiff` (e.g., `createdNode()`),
    /// the same node may appear several times in the returned array, that is,
    /// the history is not "compressed". Having access to this uncompressed history is important
    /// for code that requires to synchronize the `Complex` node tree with
    /// a parallel tree containing more objects than the `Complex` is aware of,
    /// so that they can reliably move them to an appropriate location, including
    /// in the presence of `transientNodes()`.
    ///
    const core::Array<NodeInsertionInfo>& insertions() const {
        return insertions_;
    }

private:
    friend detail::Operations;
    friend Complex;

    core::Array<CreatedNodeInfo> createdNodes_;
    core::Array<DestroyedNodeInfo> destroyedNodes_;
    core::Array<TransientNodeInfo> transientNodes_;
    core::Array<ModifiedNodeInfo> modifiedNodes_;
    core::Array<NodeInsertionInfo> insertions_;

    ComplexDiff() = default;

    void clear() {
        createdNodes_.clear();
        destroyedNodes_.clear();
        transientNodes_.clear();
        modifiedNodes_.clear();
        insertions_.clear();
    }

    // ops helpers

    void onNodeCreated(Node* node) {
        createdNodes_.emplaceLast(node);
    }

    void onNodeDestroyed(core::Id id) {
        for (Int i = 0; i < createdNodes_.length(); ++i) {
            const CreatedNodeInfo& info = createdNodes_[i];
            if (info.nodeId() == id) {
                transientNodes_.emplaceLast(id);
                createdNodes_.removeAt(i);
                break;
            }
        }
        for (Int i = 0; i < modifiedNodes_.length(); ++i) {
            if (modifiedNodes_[i].nodeId() == id) {
                modifiedNodes_.removeAt(i);
                break;
            }
        }
        destroyedNodes_.emplaceLast(id);
    }

    void onNodeModified(Node* node, NodeModificationFlags diffFlags) {
        for (Int i = 0; i < createdNodes_.length(); ++i) {
            if (createdNodes_[i].node() == node) {
                // swallow node diffs when node is new
                return;
            }
        }
        for (ModifiedNodeInfo& modifiedNodeInfo : modifiedNodes_) {
            if (modifiedNodeInfo.node() == node) {
                modifiedNodeInfo.setFlags(modifiedNodeInfo.flags() | diffFlags);
                return;
            }
        }
        ModifiedNodeInfo& modifiedNodeInfo = modifiedNodes_.emplaceLast(node);
        modifiedNodeInfo.setFlags(modifiedNodeInfo.flags() | diffFlags);
    }

    void onNodePropertyModified(Node* node, core::StringId name) {
        for (Int i = 0; i < createdNodes_.length(); ++i) {
            if (createdNodes_[i].node() == node) {
                // swallow node diffs when node is new
                return;
            }
        }
        for (ModifiedNodeInfo& modifiedNodeInfo : modifiedNodes_) {
            if (modifiedNodeInfo.node() == node) {
                modifiedNodeInfo.insertModifiedProperty(name);
                return;
            }
        }
        ModifiedNodeInfo& modifiedNodeInfo = modifiedNodes_.emplaceLast(node);
        modifiedNodeInfo.insertModifiedProperty(name);
    }

    // Preconditions:
    // - node is non-null
    // - oldParent may be null
    // - node->parentGroup() is non-null, that is,
    //   onNodeInserted() shouldn't be called for the root group
    //
    void onNodeInserted(Node* node, Node* oldParent, NodeInsertionType insertionType) {

        Node* parent = node->parentGroup();
        VGC_ASSERT(parent);

        Node* newSibling = nullptr;
        if (insertionType == NodeInsertionType::BeforeSibling) {
            newSibling = node->nextSibling();
        }
        else if (insertionType == NodeInsertionType::AfterSibling) {
            newSibling = node->previousSibling();
        }

        insertions_.emplaceLast(
            node->id(),
            parent->id(),
            newSibling ? newSibling->id() : core::Id(),
            insertionType);

        onNodeModified(parent, NodeModificationFlag::ChildrenChanged);
        if (oldParent != parent) {
            if (oldParent) {
                onNodeModified(oldParent, NodeModificationFlag::ChildrenChanged);
            }
            onNodeModified(node, NodeModificationFlag::Reparented);
        }
    }
};

/// \class vgc::vacomplex::Complex
/// \brief Represents a VAC.
///
class VGC_VACOMPLEX_API Complex : public core::Object {
private:
    VGC_OBJECT(Complex, core::Object)

protected:
    Complex(CreateKey);

    void onDestroyed() override;

public:
    static ComplexPtr create();

    void clear();

    Group* resetRoot();

    Group* rootGroup() const {
        return root_;
    }

    Node* find(core::Id id) const;

    Cell* findCell(core::Id id) const;

    Group* findGroup(core::Id id) const;

    bool containsNode(core::Id id) const;

    /// Actual type is implementation detail. Only assume forward range.
    using VertexRange = core::Array<VertexCell*>;

    /// Actual type is implementation detail. Only assume forward range.
    using EdgeRange = core::Array<EdgeCell*>;

    /// Actual type is implementation detail. Only assume forward range.
    using FaceRange = core::Array<FaceCell*>;

    VertexRange vertices() const;
    EdgeRange edges() const;
    FaceRange faces() const;

    VertexRange vertices(core::AnimTime t) const;
    EdgeRange edges(core::AnimTime t) const;
    FaceRange faces(core::AnimTime t) const;

    // An increasing version seems enough, we don't need it to match document version.

    Int64 version() const {
        return version_;
    }

    /// Returns whether someone is currently modifying this complex.
    ///
    bool isOperationInProgress() const {
        return numOperationsInProgress_ > 0;
    }

    /// Prints the tree of nodes of the Complex for debug purposes.
    ///
    void debugPrint();

    VGC_SIGNAL(nodesChanged, (const ComplexDiff&, diff))

private:
    friend detail::Operations;
    friend class KeyEdgeData;
    using NodePtrMap = std::unordered_map<core::Id, std::unique_ptr<Node>>;

    // Container storing and owning all the nodes in the Complex.
    NodePtrMap nodes_;

    // Non-owning pointer to the root Group.
    // Note that the root Group is also in nodes_.
    Group* root_;

    // Version control
    Int64 version_ = 0;

    // Guard against recursion when calling clear() / resetRoot()
    bool isBeingCleared_ = false;
    Int numOperationsInProgress_ = 0;

    // Stores the diff of operations that have taken place
    // and not yet emitted.
    ComplexDiff opDiff_ = {};

    // This set is used in the implementation of some operations to check later
    // whether a given cell is still alive. Any cell added to this set will
    // be automatically removed from the set when the cell is deleted.
    //
    core::Array<Cell*> temporaryCellSet_;
};

/// Returns which node among the given `nodes`, if any, is the top-most among
/// the children of `group`. Top-most means that it appears last in the list of
/// children, and is therefore drawn last, potentially occluding previous
/// siblings.
///
/// Returns `nullptr` if the `group` does not contain any of the nodes in
/// `nodes`.
///
/// \sa `bottomMostInGroup()`,
///     `topMostInGroupAbove()`, `bottomMostInGroupBelow()`.
///
VGC_VACOMPLEX_API
Node* topMostInGroup(Group* group, core::ConstSpan<Node*> nodes);

/// Returns which node among the given `nodes`, if any, is the bottom-most
/// among the children of `group`. Bottom-most means that it appears first in
/// the list of children, and is therefore drawn first, potentially occluded
/// by next siblings.
///
/// Returns `nullptr` if the `group` does not contain any of the nodes in
/// `nodes`.
///
/// \sa `topMostInGroup()`,
///     `topMostInGroupAbove()`, `bottomMostInGroupBelow()`.
///
VGC_VACOMPLEX_API
Node* bottomMostInGroup(Group* group, core::ConstSpan<Node*> nodes);

/// Returns which node among the given `nodes`, if any, is the top-most among
/// the next siblings of `node`. Top-most means that it appears last in the
/// list of children, and is therefore drawn last, potentially occluding
/// previous siblings.
///
/// Returns `nullptr` if the next siblings of `node` do not contain any of the
/// nodes in `nodes`.
///
/// \sa `topMostInGroup()`, `bottomMostInGroup()`,
///     `bottomMostInGroupBelow()`.
///
VGC_VACOMPLEX_API
Node* topMostInGroupAbove(Node* node, core::ConstSpan<Node*> nodes);

/// Returns which node among the given `nodes`, if any, is the bottom-most
/// among the previous siblings of `node`. Bottom-most means that it appears
/// first in the list of children, and is therefore drawn first, potentially
/// occluded by next siblings.
///
/// Returns `nullptr` if the previous siblings of `node` do not contain any of
/// the nodes in `nodes`.
///
/// \sa `topMostInGroup()`, `bottomMostInGroup()`,
///     `topMostInGroupAbove()`.
///
VGC_VACOMPLEX_API
Node* bottomMostInGroupBelow(Node* node, core::ConstSpan<Node*> nodes);

} // namespace vgc::vacomplex

#endif // VGC_VACOMPLEX_VAC_H
