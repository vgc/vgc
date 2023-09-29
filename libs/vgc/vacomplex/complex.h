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

enum class NodeModificationFlag {
    // clang-format off
    Reparented              = 0x01,
    ChildrenChanged         = 0x02,
    TransformChanged        = 0x04,
    // edge model, vertex position, ...
    GeometryChanged         = 0x08,
    PropertyChanged         = 0x10,
    // edge sampling, ...
    MeshChanged             = 0x20,
    BoundaryMeshChanged     = 0x40,
    BoundaryChanged         = 0x80,
    StarChanged             = 0x100,
    All                     = 0x1FF,
    // clang-format on
};
VGC_DEFINE_FLAGS(NodeModificationFlags, NodeModificationFlag)

class VGC_VACOMPLEX_API CreatedNodeInfo {
public:
    CreatedNodeInfo(Node* node)
        : nodeId_(node->id())
        , node_(node) {
    }

    core::Id nodeId() const {
        return nodeId_;
    }

    Node* node() const {
        return node_;
    }

private:
    core::Id nodeId_;
    Node* node_;
};

class VGC_VACOMPLEX_API DestroyedNodeInfo {
public:
    explicit DestroyedNodeInfo(core::Id id)
        : nodeId_(id) {
    }

    core::Id nodeId() const {
        return nodeId_;
    }

private:
    core::Id nodeId_;
};

class VGC_VACOMPLEX_API TransientNodeInfo {
public:
    TransientNodeInfo(core::Id nodeId)
        : nodeId_(nodeId) {
    }

    core::Id nodeId() const {
        return nodeId_;
    }

private:
    core::Id nodeId_;
};

class VGC_VACOMPLEX_API ModifiedNodeInfo {
public:
    explicit ModifiedNodeInfo(Node* node)
        : nodeId_(node->id())
        , node_(node) {
    }

    core::Id nodeId() const {
        return nodeId_;
    }

    Node* node() const {
        return node_;
    }

    NodeModificationFlags flags() const {
        return flags_;
    }

    void setFlags(NodeModificationFlags flags) {
        flags_ = flags;
    }

    const core::Array<core::StringId>& modifiedProperties() {
        return modifiedProperties_;
    }

    void insertModifiedProperty(core::StringId name) {
        flags_.set(NodeModificationFlag::PropertyChanged);
        if (!modifiedProperties_.contains(name)) {
            modifiedProperties_.append(name);
        }
    }

private:
    core::Id nodeId_ = {};
    Node* node_ = nullptr;
    NodeModificationFlags flags_ = {};
    core::Array<core::StringId> modifiedProperties_;
};

enum class NodeInsertionType {
    BeforeSibling,
    AfterSibling,
    FirstChild,
    LastChild,
};

class VGC_VACOMPLEX_API NodeInsertionInfo {
public:
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

    core::Id nodeId() const {
        return nodeId_;
    }

    core::Id newParentId() const {
        return newParentId_;
    }

    core::Id newSiblingId() const {
        return newSiblingId_;
    }

    NodeInsertionType type() const {
        return type_;
    }

private:
    core::Id nodeId_;
    core::Id newParentId_;
    core::Id newSiblingId_;
    NodeInsertionType type_;
};

class VGC_VACOMPLEX_API ComplexDiff {
public:
    ComplexDiff() = default;

    void clear() {
        createdNodes_.clear();
        destroyedNodes_.clear();
        transientNodes_.clear();
        modifiedNodes_.clear();
        insertions_.clear();
    }

    bool isEmpty() const {
        return createdNodes_.isEmpty() && modifiedNodes_.isEmpty()
               && destroyedNodes_.isEmpty();
    }

    /// Does not contain transient nodes.
    ///
    const core::Array<CreatedNodeInfo>& createdNodes() const {
        return createdNodes_;
    }

    /// Contains both transient nodes and previously existing nodes that
    /// have been destroyed.
    ///
    const core::Array<DestroyedNodeInfo>& destroyedNodes() const {
        return destroyedNodes_;
    }

    /// Nodes that have been both created and destroyed during
    /// the operation. Knowing about these nodes is useful since
    /// their ID can be referred in `NodeInsertionInfo`, to know
    /// that some node has been moved next to a transient node.
    ///
    const core::Array<TransientNodeInfo>& transientNodes() const {
        return transientNodes_;
    }

    /// Does not contain transient nodes or destroyed nodes. May contain
    /// created nodes.
    ///
    const core::Array<ModifiedNodeInfo>& modifiedNodes() const {
        return modifiedNodes_;
    }

    /// History of node insertions. Cannot be compressed.
    ///
    const core::Array<NodeInsertionInfo>& insertions() const {
        return insertions_;
    }

    void merge(const ComplexDiff& other);

private:
    friend class detail::Operations;
    friend class Complex;

    core::Array<CreatedNodeInfo> createdNodes_;
    core::Array<DestroyedNodeInfo> destroyedNodes_;
    core::Array<TransientNodeInfo> transientNodes_;
    core::Array<ModifiedNodeInfo> modifiedNodes_;
    core::Array<NodeInsertionInfo> insertions_;

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

    void onNodeInserted(Node* node, Node* oldParent, NodeInsertionType insertionType) {
        Node* parent = node->parentGroup();
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
