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
    GeometryChanged         = 0x08,
    MeshChanged             = 0x10,
    // do we need BoundaryMeshChanged ?
    BoundaryChanged         = 0x20,
    StarChanged             = 0x40,
    All                     = 0x7F,
    // clang-format on
};
VGC_DEFINE_FLAGS(NodeModificationFlags, NodeModificationFlag)

class VGC_VACOMPLEX_API NodeSourceOperation {
    // TODO
    // This is intended to help workspace rebuild styles on different operations.
    // For instance cutting an edge should not change the final appearance,
    // and thus also split an eventual color gradient or adjust a pattern
    // phase parameter.
};

class VGC_VACOMPLEX_API CreatedNodeInfo {
public:
    CreatedNodeInfo(Node* node, NodeSourceOperation sourceOperation)
        : nodeId_(node->id())
        , node_(node)
        , sourceOperation_(std::move(sourceOperation)) {
    }

    core::Id nodeId() const {
        return nodeId_;
    }

    Node* node() const {
        return node_;
    }

    const NodeSourceOperation& sourceOperation() const {
        return sourceOperation_;
    }

private:
    core::Id nodeId_;
    Node* node_;
    NodeSourceOperation sourceOperation_;
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
    TransientNodeInfo(core::Id nodeId, NodeSourceOperation sourceOperation)
        : nodeId_(nodeId)
        , sourceOperation_(std::move(sourceOperation)) {
    }

    core::Id nodeId() const {
        return nodeId_;
    }

    const NodeSourceOperation& sourceOperation() const {
        return sourceOperation_;
    }

private:
    core::Id nodeId_;
    NodeSourceOperation sourceOperation_;
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

private:
    core::Id nodeId_;
    Node* node_;
    NodeModificationFlags flags_ = {};
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
        modifiedNodes_.clear();
        insertions_.clear();
    }

    bool isEmpty() const {
        return createdNodes_.isEmpty() && modifiedNodes_.isEmpty()
               && destroyedNodes_.isEmpty();
    }

    const core::Array<CreatedNodeInfo>& createdNodes() const {
        return createdNodes_;
    }

    const core::Array<DestroyedNodeInfo>& destroyedNodes() const {
        return destroyedNodes_;
    }

    const core::Array<TransientNodeInfo>& transientNodes() const {
        return transientNodes_;
    }

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

    void onNodeCreated(Node* node, NodeSourceOperation sourceOperation) {
        createdNodes_.emplaceLast(node, std::move(sourceOperation));
    }

    void onNodeDestroyed(core::Id id) {
        for (Int i = 0; i < createdNodes_.length(); ++i) {
            const CreatedNodeInfo& info = createdNodes_[i];
            if (info.nodeId() == id) {
                transientNodes_.emplaceLast(id, info.sourceOperation());
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
    Complex() {
        resetRoot();
    }

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

    // An increasing version seems enough, we don't need it to match document version.

    Int64 version() const {
        return version_;
    }

    VGC_SIGNAL(nodesChanged, (const ComplexDiff&, diff))

private:
    friend detail::Operations;
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
    bool isOperationInProgress_ = false;
};

} // namespace vgc::vacomplex

#endif // VGC_VACOMPLEX_VAC_H
