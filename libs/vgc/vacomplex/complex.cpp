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
    for (auto& p : other.nodeDiffs_) {
        const NodeDiff& nextDiff = p.second;
        NodeDiff& nodeDiff = nodeDiffs_[p.first];
        nodeDiff.setNode(nextDiff.node());
        if (nextDiff.flags().has(NodeDiffFlag::Removed)) {
            nodeDiff.setFlags(NodeDiffFlag::Removed);
        }
        else if (nextDiff.flags().has(NodeDiffFlag::Removed)) {
            nodeDiff.setFlags(nextDiff.flags());
        }
        else {
            nodeDiff.setFlags(nodeDiff.flags() | nextDiff.flags());
        }
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
        nodeAboutToBeRemoved().emit(node.second.get());
    }

    // Remove all the nodes
    auto nodesCopy = std::move(nodes_);
    nodes_ = NodePtrMap();

    // Add the removal of all the nodes to the diff
    if (isDiffEnabled_) {
        for (const auto& node : nodesCopy) {
            diff_.onNodeRemoved(node.second.get());
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

bool Complex::insertNode(std::unique_ptr<Node>&& node) {
    if (!nodes_.try_emplace(node->id(), std::move(node)).second) {
        throw LogicError("Id collision error.");
    }
    return true;
}

} // namespace vgc::vacomplex
