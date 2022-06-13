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

UndoGroupIndex lastId = 0;

} // namespace

UndoGroupIndex genUndoGroupIndex()
{
    // XXX make this thread-safe ?
    return ++lastId;
}

bool UndoGroup::close()
{
    return history_->closeUndoGroup_(this);
}

void UndoGroup::undo_(bool isAbort)
{
#ifdef VGC_DEBUG_BUILD
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

void UndoGroup::redo_()
{
#ifdef VGC_DEBUG_BUILD
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
    UndoGroup* ptr = new UndoGroup(entrypointName, this);
    this->appendChildObject_(ptr);
    root_ = ptr;
    head_ = ptr;
}

void History::setMaxLevelsCount(Int count)
{
    maxLevels_ = std::max(count, Int(1));
    prune_();
}

bool History::abort()
{
    if (head_->isOpen()) {
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
        } while (head_->isOpen());
        headChanged().emit(head_);
        return true;
    }
    return false;
}

bool History::redo()
{
    UndoGroup* child = head_->mainChild();
    if (child) {
        do {
            redoOne_();
        } while (head_->isOpen() && head_->mainChild());
        headChanged().emit(head_);
        return true;
    }
    return false;
}

void History::goTo(UndoGroup* node)
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
    UndoGroup* a = node;
    while (!a->isUndone()) {
        UndoGroup* prev = a->parent();
        prev->appendChildObject_(a);
        a = prev;
    }

    // First undo all between head_ and common ancestor.
    while (head_ != a) {
        undoOne_();
    }

    // Then redo all from common ancestor to node (included).
    while (head_ != node) {
        redoOne_();
    }

    headChanged().emit(head_);
}

UndoGroup* History::createUndoGroup(core::StringId name)
{
    // Check current ongoing node (if any) doesn't have recorded operations.
    if (head_->isOpen() && head_->numOperations()) {
      throw LogicError(
          "Cannot nest an undo group under another if the latter already contains operations."
      );
    }

    // Destroy first ongoing in main redos if present.
    UndoGroup* child = head_->mainChild();
    while (child) {
        if (child->isOpen()) {
            child->destroyObject_();
            break;
        }
        child = child->mainChild();
    }

    UndoGroupPtr ptr = UndoGroup::create(name, this);
    UndoGroup* raw = ptr.get();
    raw->openAncestor_ = raw;
    head_->appendChildObject_(raw);
    head_ = raw;
    headChanged().emit(head_);
    return raw;
}

void History::undoOne_(bool forceAbort)
{
    UndoGroup* parent = head_->parent();
    bool abort = forceAbort;

    if (head_->isOpen() && head_->firstChild() == nullptr) {
        abort = true;
    }

    head_->undo_(abort);
    if (!head_->openAncestor_) {
        --levelsCount_;
    }
    if (abort) {
        head_->destroyObject_();
    }
    head_ = parent;
}


void History::redoOne_()
{
    UndoGroup* child = head_->mainChild();

    child->redo_();
    if (!head_->openAncestor_) {
        ++levelsCount_;
    }
    head_ = child;
}

bool History::closeUndoGroup_(UndoGroup* node)
{
    // Requirements:
    // - `node` is ongoing
    // - `node` is not undone (implies node is in main branch)
    // - `node` is the first ongoing node in the path from head_ to root

    if (!node->isOpen()) {
        throw LogicError("Cannot close an undo group which is already closed.");
    }

    if (node->isUndone()) {
        throw LogicError("Cannot close an undo group that is currently undone.");
    }

    // Visit nodes between `node` and head_ to check that there is no active
    // nested ongoing node.
    for (UndoGroup* x = head_; x != node; x = x->parent()) {
        if (x->isOpen()) {
            throw LogicError("Cannot close an undo group before its nested ones are closed.");
        }
    }

    // Collect all operations in descendant nodes to form a single one.
    for (UndoGroup* x = node; x != head_;) {
        x = x->mainChild(); // iterate after test
        for (auto& op : x->operations_) {
            node->operations_.emplaceLast(std::move(op));
        }
        x->operations_.clear();
    }

    // Remove descendants and set node as head.
    node->destroyAllChildObjects_();
    UndoGroup* prev = node->parent();
    node->openAncestor_ = prev->openAncestor_;
    head_ = node;

    if (!node->openAncestor_) {
        ++levelsCount_;
        ++nodesCount_;
        prune_();
    }

    headChanged().emit(head_);

    if (!head_->openAncestor_) {
        prune_();
    }

    return true;
}

void History::prune_()
{
    // requires maxLevels_ >= 1
    Int extraLevelsCount = levelsCount_ - maxLevels_;
    for (Int i = 0; i < extraLevelsCount; ++i) {
        Int size = root_->branchSize();
        UndoGroup* newRoot = root_->mainChild();
        size -= newRoot->branchSize();
        this->appendChildObject_(newRoot);
        root_->destroyObject_();
        nodesCount_ -= size;
        --levelsCount_;
        root_ = newRoot;
    }

    Int maxNodesCount = 4 * maxLevels_;
    // keeps the main branch intact, only able to destroy redos from the oldest branches.
    while (nodesCount_ > maxNodesCount) {
        UndoGroup* p = root_;
        while (UndoGroup* n = p->firstChild()) {
            p = n;
        }
        if (p->isPartOfAnOpenGroup() || !p->isUndone()) {
            break;
        }
        p->destroyObject_();
        --nodesCount_;
    }
}

} // namespace vgc::core
