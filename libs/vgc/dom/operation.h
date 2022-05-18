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

#ifndef VGC_DOM_OPERATION_H
#define VGC_DOM_OPERATION_H

#include <set>
#include <string>
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

VGC_DECLARE_OBJECT(Node);
VGC_DECLARE_OBJECT(Document);
VGC_DECLARE_OBJECT(Element);

enum class AtomicOperationKind {
    CreateAuthoredAttribute,
    RemoveAuthoredAttribute,
    ChangeAuthoredAttribute,
    CreateNode, // and its children
    RemoveNode, // and its children
    MoveNode,
};

class VGC_DOM_API CreateAuthoredAttributeOperands {
public:
    Element* element()const  {
        return element_;
    }

    const core::StringId& name() const {
        return name_;
    }

    const Value& value() const {
        return value_;
    }

private:
    friend Document;

    Element* element_;
    core::StringId name_;
    Value value_;

    CreateAuthoredAttributeOperands(
        Element* element,
        core::StringId name,
        Value value) :
        element_(element),
        name_(name),
        value_(value) {}

};

class VGC_DOM_API RemoveAuthoredAttributeOperands {
public:
    Element* element()const  {
        return element_;
    }

    const core::StringId& name() const {
        return name_;
    }

    const Value& value() const {
        return value_;
    }

private:
    friend Document;

    Element* element_;
    core::StringId name_;
    Value value_;

    RemoveAuthoredAttributeOperands(
        Element* element,
        core::StringId name,
        Value value) :
        element_(element),
        name_(name),
        value_(value) {}
};

class VGC_DOM_API ChangeAuthoredAttributeOperands {
public:
    Element* element()const  {
        return element_;
    }

    const core::StringId& name() const {
        return name_;
    }

    const Value& oldValue() const {
        return oldValue_;
    }

    const Value& newValue() const {
        return newValue_;
    }

private:
    friend Document;

    Element* element_;
    core::StringId name_;
    Value oldValue_;
    Value newValue_;

    ChangeAuthoredAttributeOperands(
        Element* element,
        core::StringId name,
        Value oldValue,
        Value newValue) :
        element_(element),
        name_(name),
        oldValue_(oldValue),
        newValue_(newValue) {}
};

class VGC_DOM_API NodeLinks {
public:
    NodeLinks(Node* node) :
        NodeLinks(
            node->parent(),
            node->previousSibling(),
            node->nextSibling()) {}

    Node* parent()const  {
        return parent_;
    }

    Node* previousSibling() const {
        return previousSibling_;
    }

    Node* nextSibling() const {
        return nextSibling_;
    }

private:
    friend Document;

    Node* parent_;
    Node* previousSibling_;
    Node* nextSibling_;

    NodeLinks(
        Node* parent,
        Node* previousSibling,
        Node* nextSibling) :
        parent_(parent),
        previousSibling_(previousSibling),
        nextSibling_(nextSibling) {}
};

class VGC_DOM_API CreateNodeOperands {
public:
    Node* node()const  {
        return node_;
    }

    const NodeLinks& links() const {
        return links_;
    }

private:
    friend Document;

    Node* node_;
    NodeLinks links_;

    CreateNodeOperands(
        Node* node,
        const NodeLinks& links) :
        node_(node),
        links_(links) {}
};

class VGC_DOM_API RemoveNodeOperands {
public:
    Node* node()const  {
        return node_;
    }

    const NodeLinks& links() const {
        return links_;
    }

private:
    friend Document;

    Node* node_;
    NodeLinks links_;

    RemoveNodeOperands(
        Node* node,
        const NodeLinks& links) :
        node_(node),
        links_(links) {}
};

class VGC_DOM_API MoveNodeOperands {
public:
    Node* node()const  {
        return node_;
    }

    const NodeLinks& oldLinks() const {
        return oldLinks_;
    }

    const NodeLinks& newLinks() const {
        return newLinks_;
    }

private:
    friend Document;

    Node* node_;
    NodeLinks oldLinks_;
    NodeLinks newLinks_;

    MoveNodeOperands(
        Node* node,
        const NodeLinks& oldLinks,
        const NodeLinks& newLinks) :
        node_(node),
        oldLinks_(oldLinks),
        newLinks_(newLinks) {}
};

// wip: possible refactor of atomic op to use polymorphism
//      it'd be better to have the operation impl in a single place.
//      on the other end the onVicinityChanged maybe only needed on the
//      first run since we can store the diff in Operation.

class VGC_DOM_API AtomicOperation2 {
public:
    virtual ~AtomicOperation2() = default;
    virtual void undo(class Operation* parentOp) = 0;
    virtual void redo(class Operation* parentOp) = 0;

private:
    AtomicOperationKind kind_;
};

class VGC_DOM_API AtomicOperation {
public:
    const CreateAuthoredAttributeOperands&
    getCreateAuthoredAttributeOperands() const {
        return std::get<CreateAuthoredAttributeOperands>(operands_);
    }

    const RemoveAuthoredAttributeOperands&
    getRemoveAuthoredAttributeOperands() const {
        return std::get<RemoveAuthoredAttributeOperands>(operands_);
    }

    const ChangeAuthoredAttributeOperands&
        getChangeAuthoredAttributeOperands() const {
        return std::get<ChangeAuthoredAttributeOperands>(operands_);
    }

    const CreateNodeOperands& getCreateNodeOperands() const {
        return std::get<CreateNodeOperands>(operands_);
    }

    const RemoveNodeOperands& getRemoveNodeOperands() const {
        return std::get<RemoveNodeOperands>(operands_);
    }

    const MoveNodeOperands& getMoveNodeOperands() const {
        return std::get<MoveNodeOperands>(operands_);
    }

    void get(CreateAuthoredAttributeOperands&
        createAuthoredAttributeOperands) const {
        createAuthoredAttributeOperands =
            getCreateAuthoredAttributeOperands();
    }

    void get(RemoveAuthoredAttributeOperands&
        removeAuthoredAttributeOperands) const {
        removeAuthoredAttributeOperands =
            getRemoveAuthoredAttributeOperands();
    }

    void get(ChangeAuthoredAttributeOperands&
        changeAuthoredAttributeOperands) const {
        changeAuthoredAttributeOperands =
            getChangeAuthoredAttributeOperands();
    }

    void get(CreateNodeOperands& createNodeOperands) const {
        createNodeOperands = getCreateNodeOperands();
    }

    void get(RemoveNodeOperands& removeNodeOperands) const {
        removeNodeOperands = getRemoveNodeOperands();
    }

    void get(MoveNodeOperands& moveNodeOperands) const {
        moveNodeOperands = getMoveNodeOperands();
    }

private:
    friend Document;
    friend class Operation;

    // XXX remove it, make it via conversion from operands_.index()
    AtomicOperationKind kind_;
    std::variant<
        CreateAuthoredAttributeOperands,
        RemoveAuthoredAttributeOperands,
        ChangeAuthoredAttributeOperands,
        CreateNodeOperands,
        RemoveNodeOperands,
        MoveNodeOperands>
        operands_;

    explicit AtomicOperation(
        const CreateAuthoredAttributeOperands&
        createAuthoredAttributeOperands) :
        kind_(AtomicOperationKind::CreateAuthoredAttribute),
        operands_(createAuthoredAttributeOperands) {}

    explicit AtomicOperation(
        const RemoveAuthoredAttributeOperands&
        removeAuthoredAttributeOperands) :
        kind_(AtomicOperationKind::RemoveAuthoredAttribute),
        operands_(removeAuthoredAttributeOperands) {}

    explicit AtomicOperation(
        const ChangeAuthoredAttributeOperands&
        changeAuthoredAttributeOperands) :
        kind_(AtomicOperationKind::ChangeAuthoredAttribute),
        operands_(changeAuthoredAttributeOperands) {}

    explicit AtomicOperation(const CreateNodeOperands& createNodeOperands) :
        kind_(AtomicOperationKind::CreateNode),
        operands_(createNodeOperands) {}

    explicit AtomicOperation(const RemoveNodeOperands& removeNodeOperands) :
        kind_(AtomicOperationKind::RemoveNode),
        operands_(removeNodeOperands) {}

    explicit AtomicOperation(const MoveNodeOperands& moveNodeOperands) :
        kind_(AtomicOperationKind::MoveNode),
        operands_(moveNodeOperands) {}
};

using OperationIndex = UInt32;

OperationIndex genOperationIndex();

class VGC_DOM_API Operation {
public:
    ~Operation() {
        for (auto& node : removedNodes_) {
            internal::destroyNode(node.get());
        }
    }

    // non-copyable
    Operation(const Operation&) = delete;
    Operation& operator=(const Operation&) = delete;

    Operation(Operation&& other) noexcept :
        name_(other.name_),
        index_(other.index_),
        isReverted_(other.isReverted_) {

        atomicOperations_.swap(other.atomicOperations_);
        removedNodes_.swap(other.removedNodes_);
    }

    Operation& operator=(Operation&& other) noexcept {
        std::swap(name_, other.name_);
        std::swap(index_, other.index_);
        std::swap(isReverted_, other.isReverted_);
        atomicOperations_.swap(other.atomicOperations_);
        removedNodes_.swap(other.removedNodes_);
    }

    const core::StringId& name() const {
        return name_;
    }

    const core::Array<AtomicOperation>& atomicOperations() const {
        return atomicOperations_;
    }

    OperationIndex index() const {
        return index_;
    }

    bool isReverted() const {
        return isReverted_;
    }

private:
    friend Node;
    friend Document;

    core::StringId name_;
    core::Array<AtomicOperation> atomicOperations_;
    core::Array<NodePtr> removedNodes_;
    OperationIndex index_;
    bool isReverted_ = false;

    explicit Operation(
        core::StringId name) :
        name_(name),
        atomicOperations_(),
        index_(genOperationIndex()) {}

    template<typename... Args>
    void emplaceLastAtomicOperation(Args&&... args) {
        // AtomicOperation constructor is private, cannot emplace with args.
        atomicOperations_.emplaceLast(AtomicOperation(std::forward<Args>(args)...));
    }
};


//template<typename AtomicOperationIter>
//Diff(AtomicOperationIter first, AtomicOperationIter last) {
//    while (first != last) {
//        // xxx move to cpp
//
//    }
//}

class VGC_DOM_API Diff {
public:
    const core::Array<Node*>& createdNodes() const {
        return createdNodes_;
    }

    const core::Array<Node*>& removedNodes() const {
        return removedNodes_;
    }

    const std::set<Node*>& reparentedNodes() const {
        return reparentedNodes_;
    }

    /// Nodes with at least one different sibling but same parent.
    const std::set<Node*>& reorderedNodes() const {
        return reorderedNodes_;
    }

    const std::set<Node*>& childrenChangedNodes() const {
        return childrenChangedNodes_;
    }

    const std::unordered_map<Element*, std::set<core::StringId>>&
    modifiedElements() const {
        return modifiedElements_;
    }

private:
    friend Document;

    core::Array<Node*> createdNodes_;
    core::Array<Node*> removedNodes_;

    std::set<Node*> reparentedNodes_;
    std::set<Node*> reorderedNodes_;
    std::set<Node*> childrenChangedNodes_;

    std::unordered_map<Element*, std::set<core::StringId>>
        modifiedElements_;

    Diff() = default;

    void reset() {
        createdNodes_.clear();
        removedNodes_.clear();
        reparentedNodes_.clear();
        reorderedNodes_.clear();
        childrenChangedNodes_.clear();
        modifiedElements_.clear();
    }

    bool isEmpty() const {
        return (
            createdNodes_.empty() &&
            removedNodes_.empty() &&
            reparentedNodes_.empty() &&
            reorderedNodes_.empty() &&
            childrenChangedNodes_.empty() &&
            modifiedElements_.empty());
    }
};

} // namespace vgc::dom

#endif // VGC_DOM_OPERATION_H
