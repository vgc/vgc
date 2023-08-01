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

#ifndef VGC_CORE_HISTORY_H
#define VGC_CORE_HISTORY_H

#include <iterator>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <variant>

#include <vgc/core/api.h>
#include <vgc/core/arithmetic.h>
#include <vgc/core/array.h>
#include <vgc/core/object.h>
#include <vgc/core/stringid.h>

/*
    [x]: main branch node
    (x): dangling branch node
    U: undoable
    R: redoable
    C: cancellable
    ~: open
    <: head

      [0]
       |
      [1] U
     _/ \_____
     |       |
    (2) R   [3] C~
         ___/ \___
         |       |
        (4) R   [6] U<
         |       |
        (5) R   [7] R~
             ___/ \___
             |       |
            (8) R   [9] R
*/

namespace vgc::core {

VGC_DECLARE_OBJECT(History);
VGC_DECLARE_OBJECT(UndoGroup);

class Operation;

using UndoGroupIndex = Int;
VGC_CORE_API UndoGroupIndex genUndoGroupIndex();

class VGC_CORE_API Operation {
protected:
    friend UndoGroup;
    friend History;

    Operation() noexcept = default;

public:
    virtual ~Operation() = default;

protected:
    // must be called only once
    virtual void do_() = 0;

    virtual void undo_() = 0;
    virtual void redo_() = 0;

private:
    // For friends to not have to upcast to Operation*
    // (since undo_ and redo_ are protected in child classes).
    void callUndo_() {
        undo_();
    }

    // For friends to not have to upcast to Operation*
    // (since undo_ and redo_ are protected in child classes).
    void callRedo_() {
        redo_();
    }
};

/// \class vgc::core::UndoGroup
/// \brief Represents a group of undoable operations.
///
class VGC_CORE_API UndoGroup : public Object {
private:
    VGC_OBJECT(UndoGroup, Object)

protected:
    friend History;

    UndoGroup(CreateKey key, StringId name, History* history);

public:
    // non-copyable
    UndoGroup(const UndoGroup&) = delete;
    UndoGroup& operator=(const UndoGroup&) = delete;

    /// Returns the name of this group, given at construction.
    ///
    StringId name() const {
        return name_;
    }

    /// Returns the index of this group. It is unique per program execution.
    ///
    UndoGroupIndex index() const {
        return index_;
    }

    /// Closes this undo group.
    ///
    /// If this node contains no operations, it is simply destroyed.
    ///
    /// If `tryAmendParent` is true then:
    /// - If the parent node of this node is
    /// both closed and has a single child, then the parent node
    /// is amended with the operations of this node and this node is
    /// destroyed.
    /// - Otherwise, this node is simply closed as if `tryAmendParent` was false.
    ///
    /// Throws `LogicError` if:
    ///  - this node is not open, or
    ///  - this node is undone, or
    ///  - this node is not the first open node in the path
    ///    from head to root in the history.
    ///
    /// Throws `LogicError` if the history is undergoing an undo or redo operation.
    ///
    bool close(bool tryAmendParent = false);

    /// Returns whether this group is still open.
    ///
    /// An open group that is the head of the history is appended with new operations.
    /// An open group without a closed child group is considered aborted when undone.
    ///
    bool isOpen() const {
        return openAncestor_ == this;
    }

    /// Returns whether this group has an open ancestor.
    ///
    bool isPartOfAnOpenGroup() const {
        return openAncestor_ != nullptr;
    }

    /// Returns whether this group is undone, that is, all its operations have been undone.
    ///
    bool isUndone() const {
        return isUndone_;
    }

    /// Returns the number of operations stored in this group.
    ///
    /// If this node is open and has children nodes it always returns 0.
    ///
    Int numOperations() const {
        return operations_.size();
    }

    /// Returns the first ancestor node that is open.
    ///
    /// When the ancestor is going to be closed, this node's operations will
    /// be merged in and this node will be removed.
    ///
    UndoGroup* openAncestor() const {
        return openAncestor_;
    }

    /// Returns the parent group of this group.
    ///
    UndoGroup* parent() const {
        return static_cast<UndoGroup*>(this->parentObject());
    }

    /// Returns the child group of this group which is the preferred
    /// child to redo next.
    ///
    UndoGroup* mainChild() const {
        return lastChild();
    }

    /// Returns the child group of this group which is the preferred alternative
    /// child to redo next.
    ///
    UndoGroup* secondaryChild() const {
        auto last = lastChild();
        return last ? last->previousAlternative() : nullptr;
    }

    /// Returns the number of child groups this group has.
    ///
    /// Child groups are alternative futures in the context of the History.
    ///
    Int numChildren() const {
        return this->numChildObjects();
    }

    /// Returns the last child group of this group.
    ///
    UndoGroup* lastChild() const {
        return static_cast<UndoGroup*>(this->lastChildObject());
    }

    /// Returns the first child group of this group.
    ///
    UndoGroup* firstChild() const {
        return static_cast<UndoGroup*>(this->firstChildObject());
    }

    /// Returns the previous sibling group of this group.
    ///
    /// A sibling group is an alternative future of its parent
    /// in the context of the History.
    ///
    UndoGroup* previousAlternative() const {
        return static_cast<UndoGroup*>(this->previousSiblingObject());
    }

    /// Returns the next sibling group of this group.
    ///
    /// A sibling group is an alternative future of its parent
    /// in the context of the History.
    ///
    UndoGroup* nextAlternative() const {
        return static_cast<UndoGroup*>(this->nextSiblingObject());
    }

    VGC_SIGNAL(undone, (UndoGroup*, node), (bool, isAbort))
    VGC_SIGNAL(redone, (UndoGroup*, node))

private:
    friend History;

    core::Array<std::unique_ptr<Operation>> operations_;
    core::StringId name_;
    History* history_ = nullptr;
    UndoGroup* openAncestor_ = nullptr;
    UndoGroupIndex index_ = -1;
    bool isUndone_ = false;

    static UndoGroupPtr create(StringId name, History* history);

    // Undo the contained operations.
    // isAbort is passed to the undone signal.
    //
    // Assumes isUndone_ is false.
    void undo_(bool isAbort = false);

    // Assumes isUndone_ is true.
    void redo_();
};

/// \class vgc::core::History
/// \brief Represents a tree of grouped operations for undo/redo purposes.
///
class VGC_CORE_API History : public Object {
private:
    VGC_OBJECT(History, Object)

protected:
    friend UndoGroup;

    History(CreateKey, core::StringId entrypointName);

    template<typename T>
    struct EnableConstruct : T {
        template<typename... UArgs>
        EnableConstruct(UArgs&&... args)
            : T(std::forward<UArgs>(args)...) {
        }
    };

public:
    static HistoryPtr create(StringId entrypointName);

    /// Returns the root undo group of this history.
    ///
    UndoGroup* root() const {
        return root_;
    }

    /// Returns the head undo group of this history.
    ///
    /// It is the group that, if open, will be appended with
    /// new operations, or new sub-groups.
    ///
    UndoGroup* head() const {
        return head_;
    }

    // XXX setting max levels to 0 should disable and
    //     thus remove all history directly for safety !!

    void setMinLevels(Int n);
    void setMaxLevels(Int n);

    Int maxLevels() const {
        return maxLevels_;
    }

    /*Int numLevels() const {
        return numLevels_;
    }*/

    /*Int numNodes() const {
        return numNodes_;
    }*/

    // XXX todos:
    // - design a coalescing system

    /// Reverts the operations of the main open undo group and its sub-groups, resets the
    /// head to its parent group, then destroys it and its sub-groups.
    ///
    /// Throws `LogicError` if the history is undergoing an undo or redo operation.
    ///
    bool abort();

    /// Reverts the operations of the head group and its parent group becomes the new head
    /// group.
    ///
    /// Returns false if there is nothing to undo, that is, the head group is the root group.
    /// Otherwise returns true.
    ///
    /// If it is open and has no closed sub-group then the reverted group is also
    /// destroyed (similar to abort).
    ///
    /// Throws `LogicError` if the history is undergoing an undo or redo operation.
    ///
    bool undo();

    /// Redoes the operations of the main child of the head group and this child becomes
    /// the new head group.
    ///
    /// Returns false if there is nothing to redo, that is, the head group has no children.
    /// Otherwise returns true.
    ///
    /// Throws `LogicError` if the history is undergoing an undo or redo operation.
    ///
    bool redo();

    bool isUndoingOrRedoing() const {
        return isUndoingOrRedoing_;
    }

    /// Returns whether it is possible to undo the head group.
    ///
    bool canUndo() const {
        return (head_ != root_) && !isUndoingOrRedoing_;
    }

    /// Returns whether it is possible to redo the main child of the head group.
    ///
    bool canRedo() const {
        return (head_->mainChild() != nullptr) && !isUndoingOrRedoing_;
    }

    /// Undoes all groups between the head group and its common ancestor with `node`, ancestor
    /// excluded. Then redoes all groups between this common ancestor and `node`, `node`
    /// included but ancestor excluded.
    ///
    /// Throws `LogicError` if the history is undergoing an undo or redo operation.
    ///
    void goTo(UndoGroup* node);

    /// Opens a new undo group, child of the current head group.
    ///
    /// Throws `LogicError` if the current head group is open and already has operations.
    ///
    /// Throws `LogicError` if the history is undergoing an undo or redo operation.
    ///
    UndoGroup* createUndoGroup(core::StringId name);

    template<
        typename TOperation,
        typename... Args,
        VGC_REQUIRES(std::is_base_of_v<Operation, TOperation>)>
    static void do_(History* history, Args&&... args) {
        if (history) {
            if (!history->head_->isOpen()) {
                throw LogicError(
                    "Cannot perform the requested operation without an open undo group.");
            }
            if (history->head_->firstChild() != nullptr) {
                throw LogicError("Cannot perform the requested operation since the "
                                 "current undo group has nested groups.");
            }
            const std::unique_ptr<Operation>& op =
                history->head_->operations_.emplaceLast(
                    new EnableConstruct<TOperation>(std::forward<Args>(args)...));
            op->do_();
        }
        else {
            EnableConstruct<TOperation> op(std::forward<Args>(args)...);
            static_cast<Operation&>(op).do_();
        }
    }

    VGC_SIGNAL(headChanged, (UndoGroup*, newNode))
    VGC_SIGNAL(aboutToUndo)
    VGC_SIGNAL(undone)
    VGC_SIGNAL(aboutToRedo)
    VGC_SIGNAL(redone)

private:
    Int maxLevels_ = 1000;

    UndoGroup* root_ = nullptr;
    UndoGroup* head_ = nullptr;
    Int numNodes_ = 0;
    Int numLevels_ = 0;
    bool isUndoingOrRedoing_ = false;

    // Assumes head_ is undoable. Does not emit headChanged().
    void undoOne_(bool forceAbort = false);

    // Assumes head_->mainChild() exists. Does not emit headChanged().
    void redoOne_();

    // Returns true if the group was not empty.
    bool closeUndoGroup_(UndoGroup* node, bool tryAmendParent);

    // Returns true if the group was not empty.
    bool closeUndoGroupUnchecked_(UndoGroup* node);

    // Returns true if the group was not empty.
    bool amendUndoGroupUnchecked_(UndoGroup* amendNode);

    void prune_();
};

} // namespace vgc::core

#endif // VGC_CORE_HISTORY_H
