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

#include <vgc/vacomplex/complex.h>

#include <vgc/vacomplex/detail/operationsimpl.h>

namespace vgc::vacomplex {

void ComplexDiff::merge(const ComplexDiff& other) {

    // todo: optimize, keep sorted ?

    for (const CreatedNodeInfo& info : other.createdNodes_) {
        onNodeCreated(info.node(), info.sourceOperation());
    }
    for (const ModifiedNodeInfo& info : other.modifiedNodes_) {
        onNodeModified(info.node(), info.flags());
    }
    for (const DestroyedNodeInfo& info : other.destroyedNodes_) {
        onNodeDestroyed(info.nodeId());
    }
}

void Complex::onDestroyed() {
    clear();
    isDiffEnabled_ = false;
}

ComplexPtr Complex::create() {
    return ComplexPtr(new Complex());
}

// TODO: Move to Operations
//
void Complex::clear() {

    isBeingCleared_ = true;

    // Emit about to be removed for all the nodes
    for (const auto& node : nodes_) {
        nodeDestroyed().emit(node.first);
    }

    // Remove all the nodes
    NodePtrMap nodesCopy = std::move(nodes_);
    nodes_ = NodePtrMap();

    // Add the removal of all the nodes to the diff
    if (isDiffEnabled_) {
        for (const auto& node : nodesCopy) {
            diff_.onNodeDestroyed(node.first);
        }
    }

    isBeingCleared_ = false;
    root_ = nullptr;
    ++version_;
}

Group* Complex::resetRoot() {
    if (isBeingCleared_) {
        return nullptr;
    }
    clear(); // should be an operation
    detail::Operations ops(this);
    root_ = ops.createRootGroup();
    return root_;
}

Node* Complex::find(core::Id id) const {
    auto it = nodes_.find(id);
    return it != nodes_.end() ? it->second.get() : nullptr;
}

Cell* Complex::findCell(core::Id id) const {
    Node* n = find(id);
    return (n && n->isCell()) ? static_cast<Cell*>(n) : nullptr;
}

Group* Complex::findGroup(core::Id id) const {
    Node* n = find(id);
    return (n && n->isGroup()) ? static_cast<Group*>(n) : nullptr;
}

bool Complex::containsNode(core::Id id) const {
    return find(id) != nullptr;
}

//bool Complex::emitPendingDiff() {
//    if (!diff_.isEmpty()) {
//        changed().emit(diff_);
//        diff_.clear();
//        return true;
//    }
//    return false;
//}

} // namespace vgc::vacomplex
