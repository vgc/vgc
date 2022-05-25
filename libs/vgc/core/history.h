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
    ~: ongoing
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
VGC_DECLARE_OBJECT(HistoryNode);

class Operation;

using HistoryNodeIndex = Int;
VGC_CORE_API HistoryNodeIndex genHistoryNodeIndex();

class VGC_CORE_API Operation {
protected:
    friend HistoryNode;
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

class VGC_CORE_API HistoryNode : public Object {
private:
    VGC_OBJECT(HistoryNode, Object)

protected:
    friend History;

    explicit HistoryNode(core::StringId name, History* history) :
        name_(name),
        history_(history),
        index_(genHistoryNodeIndex()) {}

public:
    // non-copyable
    HistoryNode(const HistoryNode&) = delete;
    HistoryNode& operator=(const HistoryNode&) = delete;

    core::StringId name() const {
        return name_;
    }

    HistoryNodeIndex index() const {
        return index_;
    }

    bool finalize();

    bool isOngoing() const {
        return squashNode_ == this;
    }

    bool isFinalized() const {
        return !isOngoing();
    }

    bool isUndone() const {
        return isUndone_;
    }

    HistoryNode* parent() const {
        return static_cast<HistoryNode*>(this->parentObject());
    }

    HistoryNode* mainChild() const {
        return lastChild();
    }

    HistoryNode* secondaryChild() const {
        auto last = lastChild();
        return last ? last->previousAlternative() : nullptr;
    }

    HistoryNode* lastChild() const {
        return static_cast<HistoryNode*>(this->lastChildObject());
    }

    HistoryNode* firstChild() const {
        return static_cast<HistoryNode*>(this->firstChildObject());
    }

    HistoryNode* previousAlternative() const {
        return static_cast<HistoryNode*>(this->previousSiblingObject());
    }

    HistoryNode* nextAlternative() const {
        return static_cast<HistoryNode*>(this->nextSiblingObject());
    }

    VGC_SIGNAL(undone, (HistoryNode*, node), (bool, isAbort))
    VGC_SIGNAL(redone, (HistoryNode*, node))

private:
    friend History;

    core::Array<std::unique_ptr<Operation>> operations_;
    core::StringId name_;
    History* history_ = nullptr;
    HistoryNode* squashNode_ = nullptr;
    HistoryNodeIndex index_ = -1;
    bool isUndone_ = false;

    static HistoryNodePtr create(core::StringId name, History* history) {
        return HistoryNodePtr(new HistoryNode(name, history));
    }

    bool isOngoingLeaf_() const {
        return isOngoing() && !firstChildObject();
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
    friend HistoryNode;

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

    HistoryNode* root() const {
        return root_;
    }

    HistoryNode* head() const {
        return head_;
    }

    // XXX setting max levels to 0 should disable and
    //     thus remove all history directly for safety !!

    void setMinLevelsCount(Int count);
    void setMaxLevelsCount(Int count);

    Int getMinLevelsCount() const {
        return minLevels_;
    }

    Int getMaxLevelsCount() const {
        return maxLevels_;
    }

    Int getLevelsCount() const {
        return levelsCount_;
    }

    Int getNodesCount() const {
        return nodesCount_;
    }

    /*bool isEnabled() const {
        return (maxLevels_ > 0) || (getLevelsCount() > 0);
    }*/

    // XXX todos:
    // - forbid createNode if head already has ops !
    // - design a coalescing system

    bool abort();
    bool undo();
    bool redo();

    void goTo(HistoryNode* node);

    HistoryNode* createNode(core::StringId name);

    template<typename TOperation, typename... Args,
        core::Requires<std::is_base_of_v<Operation, TOperation>> = true>
    static void do_(History* history, Args&&... args) {
        if (history) {
            if (!history->head_->isOngoingLeaf_()) {
                throw LogicError("Cannot perform the requested operation without a non-finalized HistoryNode.");
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

    VGC_SIGNAL(headChanged, (HistoryNode*, newNode))

private:
    Int minLevels_ = 0;
    Int maxLevels_ = 0;

    HistoryNode* root_;
    HistoryNode* head_ = nullptr;
    Int nodesCount_ = 0;
    Int levelsCount_ = 0;

    // Assumes head_ is undoable.
    void undoOne_(bool forceAbort = false);

    bool finalizeNode_(HistoryNode* node);
    void prune_();
};

} // namespace vgc::core

#endif // VGC_CORE_HISTORY_H
