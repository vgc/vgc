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

#ifndef VGC_DOM_HISTORY_H
#define VGC_DOM_HISTORY_H

#include <iterator>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <variant>

#include <vgc/core/arithmetic.h>
#include <vgc/core/array.h>
#include <vgc/core/object.h>
#include <vgc/core/stringid.h>
#include <vgc/dom/api.h>
#include <vgc/dom/node.h>
#include <vgc/dom/value.h>

namespace vgc::dom {

class History;
class Operation;
class UndoGroup;

using UndoGroupIndex = UInt32;
VGC_DOM_API UndoGroupIndex genUndoGroupIndex();

class VGC_DOM_API Operation {
protected:
    friend UndoGroup;
    friend History;

    Operation() = default;

public:
    virtual ~Operation() = default;

protected:
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

class VGC_DOM_API UndoGroup {
protected:
    explicit UndoGroup(core::StringId name) :
        name_(name),
        index_(genUndoGroupIndex()) {}

public:
    // non-copyable
    UndoGroup(const UndoGroup&) = delete;
    UndoGroup& operator=(const UndoGroup&) = delete;

    UndoGroup(UndoGroup&& other) noexcept :
        name_(other.name_),
        index_(other.index_),
        isUndone_(other.isUndone_) {
        operations_.swap(other.operations_);
    }

    UndoGroup& operator=(UndoGroup&& other) noexcept {
        std::swap(name_, other.name_);
        std::swap(index_, other.index_);
        std::swap(isUndone_, other.isUndone_);
        operations_.swap(other.operations_);
    }

    core::StringId name() const {
        return name_;
    }

    UndoGroupIndex index() const {
        return index_;
    }

    bool isUndone() const {
        return isUndone_;
    }

private:
    friend History;

    core::Array<std::unique_ptr<Operation>> operations_;
    std::any userState_;
    core::StringId name_;
    UndoGroupIndex index_;
    bool isUndone_ = false;

    void undo_();
    void redo_();
};

using SubGroupsList = std::list<UndoGroup>;
using SubGroupsIteraror = SubGroupsList::iterator;

class VGC_DOM_API OpenGroup {
public:
    /// Creates an empty OpenGroup.
    ///
    OpenGroup(core::StringId name) :
        name_(name) {
        firstRedo_ = subGroups_.end();
    }

    const SubGroupsList& subGroups() const {
        return subGroups_;
    }

    SubGroupsIteraror firstRedo() const {
        return firstRedo_;
    }

    core::StringId name() const {
        return name_;
    }

private:
    SubGroupsList subGroups_;
    SubGroupsIteraror firstRedo_;
    core::StringId name_;
};


class VGC_DOM_API History {
public:
    void setHistoryLimit(Int size);

    Int getLengthLimit() const {
        return lengthLimit_;
    }

    Int getLength() const {
        return core::int_cast<Int>(openGroupsStack_[0].subGroups().size());
    }

    bool isEnabled() const {
        return (lengthLimit_ > 0) || (getLength() > 0);
    }

    bool cancelGroup();
    bool undoOne();
    bool redoOne();

    void beginUndoGroup(core::StringId name, std::any&& toolState);
    bool endUndoGroup(std::any&& toolState);

    //template<typename TOperation, typename... Args,
    //    core::Requires<std::is_base_of_v<Operation, TOperation>> = true>
    //void doAtomicOperation_(Args&&... args) {
    //    // AtomicOperation constructor is private, cannot emplace with args.
    //    atomicOperations_.emplaceLast(TAtomicOperation(std::forward<Args>(args)...));
    //}

private:

    // need custom iterator ?
    bool gotoState(OperationIndex idx);

    Int lengthLimit_ = 0;

    // When inserting into a group we don't know yet if it requires an automatic subgroup.
    core::Array<std::unique_ptr<Operation>> pendingOperations_;
    core::Array<OpenGroup> openGroupsStack_;

    Int insertionStackIndex_ = 0;
    SubGroupsIteraror insertionIt_;
};

} // namespace vgc::dom

#endif // VGC_DOM_HISTORY_H
