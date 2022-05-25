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
    return history_->finalizeNode_(this);
}

void HistoryNode::undo_(bool isAbort)
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
    undone().emit(this, isAbort);
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
    HistoryNode* ptr = new HistoryNode(entrypointName, this);
    this->appendChildObject_(ptr);
    root_ = ptr;
    head_ = ptr;
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

bool History::abort()
{
    if (head_->isOngoing()) {
        undoOne_(true);
        return true;
    }
    return false;
}

bool History::undo()
{
    if (head_ != root_) {
        do {
            undoOne_();
        } while (head_->isOngoing());
        headChanged().emit(head_);
        return true;
    }
    return false;
}

bool History::redo()
{
    HistoryNode* child = head_->mainChild();
    if (child) {
        child->redo_();
        head_ = child;
        headChanged().emit(head_);
        return true;
    }
    return false;
}

void History::goTo(HistoryNode* node)
{
    // The common ancestor of node and head_ is the first node that is not
    // undone in the path from node to root.
    // It always exists and it can be head_ itself.

    if (head_ == node) {
        return;
    }

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
    while (head_ != a) {
        undoOne_();
    }

    // Then redo all from common ancestor to node (included).
    HistoryNode* x = head_;
    while (x != node) {
        x = x->mainChild();
        x->redo_();
    }

    headChanged().emit(head_);
}

HistoryNode* History::createNode(core::StringId name)
{
    // destroy first ongoing in main redos if present.
    HistoryNode* child = head_->mainChild();
    while (child) {
        if (child->isOngoing()) {
            child->destroyObject_();
            break;
        }
        child = child->mainChild();
    }

    HistoryNodePtr ptr = HistoryNode::create(name, this);
    HistoryNode* raw = ptr.get();
    //raw->squashNode_ = head_->squashNode_;
    raw->squashNode_ = raw;
    head_->appendChildObject_(raw);
    head_ = raw;
    headChanged().emit(head_);
    return raw;
}

void History::undoOne_(bool forceAbort)
{
    HistoryNode* parent = head_->parent();
    bool abort = forceAbort || head_->isOngoingLeaf_();
    head_->undo_(abort);
    if (abort) {
        head_->destroyObject_();
    }
    head_ = parent;
}

bool History::finalizeNode_(HistoryNode* node)
{
    // Requirements:
    // - `node` is ongoing
    // - `node` is not undone (implies node is in main branch)
    // - `node` is the first ongoing node in the path from head_ to root

    if (!node->isOngoing()) {
        throw LogicError("Cannot finalize a HistoryNode which is already finalized.");
    }

    if (node->isUndone()) {
        throw LogicError("Cannot finalize a HistoryNode that is currently undone.");
    }

    // visit nodes between `node` and head_ to check that there is no active
    // nested ongoing node.
    for (HistoryNode* x = head_; x != node; x = x->parent()) {
        if (x->isOngoing()) {
            throw LogicError("Cannot finalize a HistoryNode before its nested ones are finalized.");
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

    headChanged().emit(head_);
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
