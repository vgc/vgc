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

#include <vgc/core/history.h>

namespace vgc::core {

namespace {

HistoryNodeIndex lastId = 0;

} // namespace

HistoryNodeIndex genHistoryNodeIndex()
{
    // XXX make this thread-safe ?
    return ++lastId;
}

bool HistoryNode::finalize()
{
    history_->finalizeNode_(this);
}

void HistoryNode::undo_(bool forceCancel)
{
#ifdef VGC_DEBUG
    if (isUndone_) {
        throw LogicError("Already undone.");
    }
#endif

    for (auto it = operations_.rbegin(); it != operations_.rend(); ++it) {
        (*it)->callUndo_();
    }

    isUndone_ = true;
    bool cancel = forceCancel || (isOngoing() && !firstChildObject());
    undone().emit(this, cancel);
    if (cancel) {
        destroyObject_();
    }
}

void HistoryNode::redo_()
{
#ifdef VGC_DEBUG
    if (!isUndone_) {
        throw LogicError("Cannot redo if not undone first.");
    }
#endif

    for (auto it = operations_.begin(); it != operations_.end(); ++it) {
        (*it)->callRedo_();
    }

    isUndone_ = false;
    redone().emit(this);
}

History::History(core::StringId entrypointName)
    : nodesCount_(0) // root doesn't count
{
    auto ptr = HistoryNode::create(entrypointName, this);
    auto raw = ptr.get();
    this->appendChildObject_(raw);
    root_ = raw;
}

void History::setMinLevelsCount(Int count)
{
    minLevels_ = count;
    maxLevels_ = std::max(minLevels_, maxLevels_);
}

void History::setMaxLevelsCount(Int count)
{
    maxLevels_ = count;
    prune_();
}

bool History::cancelOne()
{
    if (head_->isOngoing()) {
        HistoryNode* prev = head_->parent();
        head_->undo_(true);
        // XXX count ?
        head_ = prev;
        return true;
    }
    return false;
}

bool History::undoOne()
{
    if (head_ != root_) {
        do {
            head_->undo_();
            head_ = head_->parent();
        } while (head_->isOngoing());
        return true;
    }
    return false;
}

bool History::redoOne()
{
    HistoryNode* child = head_->mainChild();
    if (child) {
        child->redo_();
        head_ = child;
        return true;
    }
    return false;
}

void History::gotoNode(HistoryNode* node)
{
    // The common ancestor of node and head_ is the first node that is not
    // undone in the path from node to root.
    // It always exists and it can be head_ itself.

    // While searching for the common ancestor we have to reorder the branches
    // of visited nodes to setup the new main path.
    //
    HistoryNode* a = node;
    while (!a->isUndone()) {
        HistoryNode* prev = a->parent();
        prev->appendChildObject_(a);
        a = prev;
    }

    // First undo all between head_ and common ancestor.
    HistoryNode* x = head_;
    while (x != a) {
        x->undo_();
        x = x->parent();
    }

    // Then redo all from common ancestor to node (included).
    while (x != node) {
        x = x->mainChild();
        x->redo_();
    }
}

HistoryNode* History::createNode(core::StringId name)
{
    // destroy first ongoing in main redos if present.
    auto child = head_;
    while (child = child->mainChild(), child) {
        if (child->isOngoing()) {
            child->destroyObject_();
            break;
        }
    }

    auto ptr = HistoryNode::create(name, this);
    auto raw = ptr.get();
    raw->squashNode_ = head_->squashNode_;
    head_->appendChildObject_(raw);
    head_ = raw;
    return raw;
}

bool History::finalizeNode_(HistoryNode* node)
{
    // Requirements:
    // - `node` is ongoing
    // - `node` is not undone (implies node is in main branch)
    // - `node` is the first ongoing node in the path from head_ to root

    if (!node->isOngoing() || node->isUndone()) {
        return false;
    }

    // visit nodes between `node` and head_ to check that there is no active
    // nested ongoing node.
    for (HistoryNode* x = head_; x != node; x = x->parent()) {
        if (node->isOngoing()) {
            return false;
        }
    }

    // Collect all operations in descendant nodes to form a single one.
    for (HistoryNode* x = node; x != head_;) {
        x = x->mainChild(); // iterate after test
        for (auto& op : x->operations_) {
            node->operations_.emplaceLast(std::move(op));
        }
        x->operations_.clear();
    }

    // Remove descendants and set node as head.
    node->destroyAllChildObjects_();
    HistoryNode* prev = node->parent();
    node->squashNode_ = prev->squashNode_;
    head_ = node;

    if (!node->squashNode_) {
        ++levelsCount_;
        ++nodesCount_;
        prune_();
    }

    return true;
}

void History::prune_()
{
    if ((levelsCount_ < maxLevels_) && (nodesCount_ < 4 * maxLevels_)) {
        return;
    }

    // XXX todo: trim oldest
}

} // namespace vgc::core
