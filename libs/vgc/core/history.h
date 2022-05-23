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
    VGC_OBJECT(HistoryNode, Object);

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

    bool commit();

    bool isCommitted() const {
        return isCommitted_;
    }

    bool isUndone() const {
        return isUndone_;
    }

    HistoryNode* prevNode() const {
        return static_cast<HistoryNode*>(this->parentObject());
    }

    HistoryNode* nextNodeInMainBranch() const {
        return static_cast<HistoryNode*>(this->lastChildObject());
    }

    HistoryNode* nextNodeInSecondaryBranch() const {
        auto last = this->lastChildObject();
        return static_cast<HistoryNode*>(last ? last->previousSiblingObject() : nullptr);
    }

private:
    friend History;

    core::Array<std::unique_ptr<Operation>> operations_;
    core::StringId name_;
    HistoryNodeIndex index_;
    History* history_;
    bool isCommitted_ = false;
    bool isUndone_ = false;

    static HistoryNodePtr create(core::StringId name, History* history) {
        return HistoryNodePtr(new HistoryNode(name, history));
    }

    void undo_();
    void redo_();
};

class VGC_CORE_API History : public Object {
private:
    VGC_OBJECT(History, Object);

protected:
    friend HistoryNode;

    History(core::StringId entrypointName);

public:
    void setMinLevelsCount(Int count);
    void setMaxLevelsCount(Int count);

    Int getMinLevelsCount() const {
        return minLevels_;
    }

    Int getMaxLevelsCount() const {
        return maxLevels_;
    }

    Int getLevelsCount() const;
    Int getNodesCount() const {
        return nodesCounts_;
    }

    bool isEnabled() const {
        return (maxLevels_ > 0) || (getLevelsCount() > 0);
    }

    bool cancelOne();
    bool undoOne();
    bool redoOne();

    bool gotoNode(HistoryNode* node);

    void createNode(core::StringId name);

    //template<typename TOperation, typename... Args,
    //    core::Requires<std::is_base_of_v<Operation, TOperation>> = true>
    //void doAtomicOperation_(Args&&... args) {
    //    // AtomicOperation constructor is private, cannot emplace with args.
    //    atomicOperations_.emplaceLast(TAtomicOperation(std::forward<Args>(args)...));
    //}

private:
    Int minLevels_ = 0;
    Int maxLevels_ = 0;

    // When inserting into a node we don't know yet if it requires an automatic sub-node.
    core::Array<std::unique_ptr<Operation>> pendingOperations_;
    HistoryNode* root_;
    HistoryNode* cursor_ = nullptr;
    Int nodesCounts_ = 0;

    bool commitNode_(HistoryNode* node);
    void prune_();

    // need custom iterator ?
    //bool gotoState(OperationIndex idx);
};

} // namespace vgc::core

#endif // VGC_CORE_HISTORY_H
