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
enum class NodeDiffFlag {
    Created                 = 0x01,
    Removed                 = 0x02,
    Reparented              = 0x04,
    ChildrenChanged         = 0x08,
    TransformChanged        = 0x10,
    GeometryChanged         = 0x20,
    BoundaryChanged         = 0x40,
    StarChanged             = 0x80,
};
VGC_DEFINE_FLAGS(NodeDiffFlags, NodeDiffFlag)
// clang-format on

class VGC_VACOMPLEX_API NodeDiff {
public:
    Node* node() const {
        return node_;
    }

    NodeDiffFlags flags() const {
        return flags_;
    }

    void setNode(Node* node) {
        node_ = node;
    }

    void setFlags(NodeDiffFlags flags) {
        flags_ = flags;
    }

private:
    Node* node_ = nullptr;
    NodeDiffFlags flags_ = {};
};

class VGC_VACOMPLEX_API ComplexDiff {
public:
    ComplexDiff() = default;

    void clear() {
        nodeDiffs_.clear();
    }

    bool isEmpty() const {
        return nodeDiffs_.empty();
    }

    const std::unordered_map<core::Id, NodeDiff>& nodeDiffs() const {
        return nodeDiffs_;
    }

    void merge(const ComplexDiff& other);

private:
    friend class detail::Operations;
    friend class Complex;

    std::unordered_map<core::Id, NodeDiff> nodeDiffs_;

    // ops helpers

    void onNodeRemoved(Node* node) {
        auto it = nodeDiffs_.try_emplace(node->id()).first;
        NodeDiff& nodeDiff = it->second;
        if (nodeDiff.flags().has(NodeDiffFlag::Created)) {
            // If this node was created then removed as part of the same diff,
            // we simply consider that it was never created in the first place.
            nodeDiffs_.erase(it);
        }
        else {
            nodeDiff.setNode(node);
            nodeDiff.setFlags(nodeDiff.flags() | NodeDiffFlag::Removed);
        }
    }

    void onNodeDiff(Node* node, NodeDiffFlags diffFlags) {
        NodeDiff& nodeDiff = nodeDiffs_[node->id()];
        nodeDiff.setNode(node);
        nodeDiff.setFlags(nodeDiff.flags() | diffFlags);
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

    VGC_SIGNAL(nodeCreated, (Node*, node), (core::Span<Node*>, operationSourceNodes))
    VGC_SIGNAL(nodeAboutToBeRemoved, (Node*, node))
    VGC_SIGNAL(nodeMoved, (Node*, node))
    VGC_SIGNAL(nodeModified, (Node*, node), (NodeDiffFlags, diffs))

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
};

} // namespace vgc::vacomplex

#endif // VGC_VACOMPLEX_VAC_H
