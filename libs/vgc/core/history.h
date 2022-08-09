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

    Operation() = default;

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

class VGC_CORE_API UndoGroup : public Object {
private:
    VGC_OBJECT(UndoGroup, Object)

protected:
    friend History;

    explicit UndoGroup(core::StringId name, History* history) :
        name_(name),
        history_(history),
        index_(genUndoGroupIndex()) {}

public:
    // non-copyable
    UndoGroup(const UndoGroup&) = delete;
    UndoGroup& operator=(const UndoGroup&) = delete;

    core::StringId name() const {
        return name_;
    }

    UndoGroupIndex index() const {
        return index_;
    }

    bool close();

    bool isOpen() const {
        return openAncestor_ == this;
    }

    bool isPartOfAnOpenGroup() const {
        return openAncestor_ != nullptr;
    }

    bool isUndone() const {
        return isUndone_;
    }

    Int numOperations() const {
        return operations_.size();
    }

    UndoGroup* parent() const {
        return static_cast<UndoGroup*>(this->parentObject());
    }

    UndoGroup* mainChild() const {
        return lastChild();
    }

    UndoGroup* secondaryChild() const {
        auto last = lastChild();
        return last ? last->previousAlternative() : nullptr;
    }

    UndoGroup* lastChild() const {
        return static_cast<UndoGroup*>(this->lastChildObject());
    }

    UndoGroup* firstChild() const {
        return static_cast<UndoGroup*>(this->firstChildObject());
    }

    UndoGroup* previousAlternative() const {
        return static_cast<UndoGroup*>(this->previousSiblingObject());
    }

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

    static UndoGroupPtr create(core::StringId name, History* history) {
        return UndoGroupPtr(new UndoGroup(name, history));
    }

    // Undo the contained operations.
    // isAbort is passed to the undone signal.
    //
    // Assumes isUndone_ is false.
    void undo_(bool isAbort = false);

    // Assumes isUndone_ is true.
    void redo_();
};


class VGC_CORE_API History : public Object {
private:
    VGC_OBJECT(History, Object)

protected:
    friend UndoGroup;

    History(core::StringId entrypointName);

    template<typename T>
    struct EnableConstruct : T {
        template<typename... UArgs>
        EnableConstruct(UArgs&&... args) : T(std::forward<UArgs>(args)...) {}
    };

public:
    static HistoryPtr create(core::StringId entrypointName) {
        HistoryPtr p(new History(entrypointName));
        return p;
    }

    UndoGroup* root() const {
        return root_;
    }

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

    bool abort();
    bool undo();
    bool redo();

    bool canUndo() const {
        return head_ != root_;
    }

    bool canRedo() const {
        return head_->mainChild() != nullptr;
    }

    void goTo(UndoGroup* node);

    UndoGroup* createUndoGroup(core::StringId name);

    template<typename TOperation, typename... Args,
             VGC_REQUIRES(std::is_base_of_v<Operation, TOperation>)>
    static void do_(History* history, Args&&... args) {
        if (history) {
            if (!history->head_->isOpen()) {
                throw LogicError("Cannot perform the requested operation without an open undo group.");
            }
            if (history->head_->firstChild() != nullptr) {
                throw LogicError("Cannot perform the requested operation since the current undo group has nested groups.");

            }
            const std::unique_ptr<Operation>& op =
                history->head_->operations_.emplaceLast(new EnableConstruct<TOperation>(std::forward<Args>(args)...));
            op->do_();
        }
        else {
            EnableConstruct<TOperation> op(std::forward<Args>(args)...);
            static_cast<Operation&>(op).do_();
        }
    }

    VGC_SIGNAL(headChanged, (UndoGroup*, newNode))

private:
    Int maxLevels_ = 1000;

    UndoGroup* root_ = nullptr;
    UndoGroup* head_ = nullptr;
    Int numNodes_ = 0;
    Int numLevels_ = 0;

    // Assumes head_ is undoable.
    void undoOne_(bool forceAbort = false);

    // Assumes head_->mainChild() exists.
    void redoOne_();


    bool closeUndoGroup_(UndoGroup* node);
    void prune_();
};

} // namespace vgc::core

#endif // VGC_CORE_HISTORY_H
