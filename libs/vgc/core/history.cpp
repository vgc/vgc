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

bool HistoryNode::commit()
{
    history_->commitNode_(this);
}

void HistoryNode::undo_()
{

}

void HistoryNode::redo_()
{

}

History::History(core::StringId entrypointName)
    : nodesCounts_(0) // root doesn't count
{
    auto ptr = HistoryNode::create(entrypointName, this);
    this->appendChildObject_(ptr.get());
    root_ = ptr.get();
}

void History::setMinLevelsCount(Int count)
{
    minLevels_ = count;
    maxLevels_ = std::max(minLevels_, maxLevels_);
}

void History::setMaxLevelsCount(Int count)
{
    maxLevels_ = count;
    // XXX prune now
}

Int History::getLevelsCount() const
{
}

bool History::cancelOne()
{
}

bool History::undoOne()
{
    if (cursor_ != root_) {
        cursor_->undo_();
        cursor_ = cursor_->prevNode();
        return true;
    }
    return false;
}

bool History::redoOne()
{
    auto next = cursor_->nextNodeInMainBranch();
    if (next) {
        next->redo_();
        cursor_ = next;
        return true;
    }
    return false;
}

bool History::gotoNode(HistoryNode* node)
{
}

void History::createNode(core::StringId name)
{
    auto next = cursor_;
    while (next = cursor_->nextNodeInMainBranch(), next) {
        if (!next->isCommitted()) {
            next->destroyObject_();
        }
    }

    auto ptr = HistoryNode::create(name, this);
    cursor_->appendChildObject_(ptr.get());
    cursor_ = ptr.get();
}

bool History::commitNode_(HistoryNode* node)
{


    // logic error if out
    /*{
        if (operationStack_.isEmpty()) {
            throw LogicError("Trying to end an operation when the operation stack is empty.");
        }
        if (!ongoingOperation_.has_value()) {
            throw LogicError("Trying to end an operation when there is no ongoing operation.");
        }
        if (currentOperationIterator_ != operationHistory_.end()) {
            throw LogicError("Operation history iterator has moved during ongoing operation.");
        }
    }

    operationStack_.pop();
    if (operationStack_.isEmpty()) {
        emitPendingDiff();

        if (hasHistoryEnabled()) {
            operationHistory_.emplace_back(std::move(ongoingOperation_.value()));
            if (operationHistory_.size() > operationHistorySizeMax_) {
                operationHistory_.pop_front();
            }
        }

        ongoingOperation_.reset();
    }*/
    return false;
}

void History::prune_()
{
}

//void History::setHistoryLimit(Int size)
//{
//    if (size == operationHistorySizeMax_) {
//        return;
//    }
//
//    operationHistorySizeMax_ = size;




    // XXXXXXXXXX Boris wants an iterator in an operation tree for the ongoing operation.

    // notes:
    // need stack of OperationGroup { undoOne() -> returns true if if was the last.. }
    // Diff must be filled by atomic ops themselves (doIt())
    // we need last iteration direction, cuz we can diff only one way..


    /*

    auto doAtomicOp<TAtomicOp>(document, diff, args) {
    TAtomicOp& op = static_cast<TAtomicOp&>(opStack[-1].emplaceOp(TAtomicOp(args)));
    return TAtomicOp::doIt(document, diff, op);
    return op.doIt(document, diff);

    Element::CreateOperation
    }

    */



    /*if (operationHistory_.size() > operationHistorySizeMax_) {
    operationHistory_.removeRange(0, operationHistory_.size() - operationHistorySizeMax_);
    }*/
//}

//bool History::gotoState(UndoGroupIndex idx)
//{
//    if (it == currentOperationIterator_) {
//        return true;
//    }
//
//    if (pos < operationHistoryPosition_) {
//        // undo
//        for (Int i = operationHistoryPosition_ - 1; i >= pos; --i) {
//            auto& op = operationHistory_[i];
//            for (auto it = op.atomicOperations_.rbegin(), end = op.atomicOperations_.rend(); it != end; ++it) {
//            }
//
//            // emit the diff too..
//        }
//    }
//    else {
//        // redo
//        for (Int i = operationHistoryPosition_; i < pos; ++i) {
//            auto& op = operationHistory_[i];
//            for (auto it = op.atomicOperations_.begin(), end = op.atomicOperations_.end(); it != end; ++it) {
//            }
//
//            // do..
//            // XXX maybe we should have the impl in the op..
//
//
//            // weird case: an operation containing setAttr + removeElem
//            // if diff contains the attribute change then we update resources for possibly nothing..
//            // and if we don't, we do it on Undo..
//
//
//            // emit the diff too..
//        }
//    }
//}





} // namespace vgc::core
