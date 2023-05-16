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

// clang-format off
enum class ModifiedNodeFlag {
    Created                  = 0x01,
    Removed                  = 0x02,
    Reparented               = 0x04,
    ChildrenChanged          = 0x08,
    TransformChanged         = 0x10,
    GeometryChanged          = 0x20,
    MeshChanged              = 0x40,
    // do we need BoundaryMeshChanged ?
    BoundaryChanged          = 0x80,
    StarChanged              = 0x100,
};
VGC_DEFINE_FLAGS(ModifiedNodeFlags, ModifiedNodeFlag)
// clang-format on

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

class VGC_VACOMPLEX_API ModifiedNodeInfo {
public:
    ModifiedNodeInfo(Node* node)
        : nodeId_(node->id())
        , node_(node) {
    }

    core::Id nodeId() const {
        return nodeId_;
    }

    Node* node() const {
        return node_;
    }

    ModifiedNodeFlags flags() const {
        return flags_;
    }

    void setFlags(ModifiedNodeFlags flags) {
        flags_ = flags;
    }

private:
    core::Id nodeId_;
    Node* node_;
    ModifiedNodeFlags flags_ = {};
};

class VGC_VACOMPLEX_API ComplexDiff {
public:
    ComplexDiff() = default;

    void clear() {
        modifiedNodes_.clear();
    }

    bool isEmpty() const {
        return modifiedNodes_.empty();
    }

    const core::Array<CreatedNodeInfo>& createdNodes() const {
        return createdNodes_;
    }

    const core::Array<ModifiedNodeInfo>& modifiedNodes() const {
        return modifiedNodes_;
    }

    const core::Array<core::Id>& removedNodes() const {
        return removedNodes_;
    }

    void merge(const ComplexDiff& other);

private:
    friend class detail::Operations;
    friend class Complex;

    core::Array<CreatedNodeInfo> createdNodes_;
    core::Array<ModifiedNodeInfo> modifiedNodes_;
    core::Array<core::Id> removedNodes_;

    // ops helpers

    void onNodeCreated(Node* node, NodeSourceOperation sourceOperation) {
        createdNodes_.emplaceLast(node, std::move(sourceOperation));
    }

    void onModifiedNode(Node* node, ModifiedNodeFlags diffFlags) {
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

    void onNodeRemoved(core::Id id) {
        for (Int i = 0; i < createdNodes_.length(); ++i) {
            if (createdNodes_[i].nodeId() == id) {
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
        removedNodes_.append(id);
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

    //const ComplexDiff& pendingDiff() {
    //    return diff_;
    //}

    //bool emitPendingDiff();

    VGC_SIGNAL(nodeCreated, (Node*, node), (const NodeSourceOperation&, sourceOperation))
    VGC_SIGNAL(nodeAboutToBeRemoved, (Node*, node))
    VGC_SIGNAL(nodeMoved, (Node*, node))
    VGC_SIGNAL(nodeModified, (Node*, node), (ModifiedNodeFlags, flags))

    //VGC_SIGNAL(changed, (const ComplexDiff&, diff))

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
    ComplexDiff diff_ = {};
    bool isDiffEnabled_ = false;

    // Guard against recursion when calling clear() / resetRoot()
    bool isBeingCleared_ = false;
    bool isOperationInProgress_ = false;
};

} // namespace vgc::vacomplex

#endif // VGC_VACOMPLEX_VAC_H
