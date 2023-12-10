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

#include <vgc/vacomplex/complexdiff.h>

namespace vgc::vacomplex {

void ComplexDiff::clear() {
    createdNodes_.clear();
    destroyedNodes_.clear();
    transientNodes_.clear();
    modifiedNodes_.clear();
    insertions_.clear();
}

void ComplexDiff::onNodeCreated_(Node* node) {
    createdNodes_.append(CreatedNodeInfo(node));
}

void ComplexDiff::onNodeDestroyed_(core::Id nodeId) {
    for (Int i = 0; i < createdNodes_.length(); ++i) {
        const CreatedNodeInfo& info = createdNodes_[i];
        if (info.nodeId() == nodeId) {
            transientNodes_.append(TransientNodeInfo(nodeId));
            createdNodes_.removeAt(i);
            break;
        }
    }
    for (Int i = 0; i < modifiedNodes_.length(); ++i) {
        if (modifiedNodes_[i].nodeId() == nodeId) {
            modifiedNodes_.removeAt(i);
            break;
        }
    }
    destroyedNodes_.append(DestroyedNodeInfo(nodeId));
}

void ComplexDiff::onNodeModified_(Node* node, NodeModificationFlags flags) {
    for (Int i = 0; i < createdNodes_.length(); ++i) {
        if (createdNodes_[i].node() == node) {
            // swallow node diffs when node is new
            return;
        }
    }
    for (ModifiedNodeInfo& modifiedNodeInfo : modifiedNodes_) {
        if (modifiedNodeInfo.node() == node) {
            modifiedNodeInfo.addFlags(flags);
            return;
        }
    }
    modifiedNodes_.append(ModifiedNodeInfo(node, flags));
}

void ComplexDiff::onNodePropertyModified_(Node* node, core::StringId name) {
    for (Int i = 0; i < createdNodes_.length(); ++i) {
        if (createdNodes_[i].node() == node) {
            // swallow node diffs when node is new
            return;
        }
    }
    for (ModifiedNodeInfo& modifiedNodeInfo : modifiedNodes_) {
        if (modifiedNodeInfo.node() == node) {
            modifiedNodeInfo.addModifiedProperty(name);
            return;
        }
    }
    ModifiedNodeInfo modifiedNodeInfo(node);
    modifiedNodeInfo.addModifiedProperty(name);
    modifiedNodes_.append(std::move(modifiedNodeInfo));
}

void ComplexDiff::onNodeInserted_(
    Node* node,
    Node* oldParent,
    NodeInsertionType insertionType) {

    Node* parent = node->parentGroup();
    VGC_ASSERT(parent);

    Node* newSibling = nullptr;
    if (insertionType == NodeInsertionType::BeforeSibling) {
        newSibling = node->nextSibling();
    }
    else if (insertionType == NodeInsertionType::AfterSibling) {
        newSibling = node->previousSibling();
    }

    insertions_.append(NodeInsertionInfo(
        node->id(),
        parent->id(),
        newSibling ? newSibling->id() : core::Id(),
        insertionType));

    onNodeModified_(parent, NodeModificationFlag::ChildrenChanged);
    if (oldParent != parent) {
        if (oldParent) {
            onNodeModified_(oldParent, NodeModificationFlag::ChildrenChanged);
        }
        onNodeModified_(node, NodeModificationFlag::Reparented);
    }
}
} // namespace vgc::vacomplex
