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

#include <iterator>
#include <memory>
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

VGC_DECLARE_OBJECT(Node);
VGC_DECLARE_OBJECT(Document);
VGC_DECLARE_OBJECT(Element);

class AtomicOperation;
class Operation;
class Diff;

class VGC_DOM_API NodeRelatives {
public:
    NodeRelatives(Node* node) :
        NodeRelatives(
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

    NodeRelatives(
        Node* parent,
        Node* previousSibling,
        Node* nextSibling) :
        parent_(parent),
        previousSibling_(previousSibling),
        nextSibling_(nextSibling) {}
};

class VGC_DOM_API AtomicOperation {
protected:
    AtomicOperation() = default;

public:
    virtual ~AtomicOperation() = default;

protected:
    friend Operation;
    virtual void undo(Document* doc) = 0;
    virtual void redo(Document* doc) = 0;
};

class VGC_DOM_API CreateNodeOperation : public AtomicOperation {
protected:
    friend Operation;

    CreateNodeOperation(
        Node* node,
        const NodeRelatives& relatives) :
        node_(node),
        savedRelatives_(relatives) {}

    ~CreateNodeOperation() {
        if (keepAlive_) {
            internal::destroyNode(node_.get());
        }
    }

public:
    Node* node()const  {
        return node_.get();
    }

    const NodeRelatives& savedRelatives() const {
        return savedRelatives_;
    }

protected:
    void undo(Document* doc) override;
    void redo(Document* doc) override;

private:
    NodePtr node_;
    bool keepAlive_ = false;
    NodeRelatives savedRelatives_;
};

class VGC_DOM_API RemoveNodeOperation : public AtomicOperation {
protected:
    friend Operation;

    RemoveNodeOperation(
        Node* node,
        const NodeRelatives& relatives) :
        node_(node),
        savedRelatives_(relatives) {}

    ~RemoveNodeOperation() {
        if (keepAlive_) {
            internal::destroyNode(node_.get());
        }
    }

public:
    Node* node()const  {
        return node_.get();
    }

    const NodeRelatives& savedRelatives() const {
        return savedRelatives_;
    }

protected:
    void undo(Document* doc) override;
    void redo(Document* doc) override;

private:
    NodePtr node_;
    bool keepAlive_ = true;
    NodeRelatives savedRelatives_;
};

class VGC_DOM_API MoveNodeOperation : public AtomicOperation {
protected:
    friend Operation;

    MoveNodeOperation(
        Node* node,
        const NodeRelatives& oldRelatives,
        const NodeRelatives& newRelatives) :
        node_(node),
        oldRelatives_(oldRelatives),
        newRelatives_(newRelatives) {}

public:
    Node* node()const  {
        return node_;
    }

    const NodeRelatives& oldRelatives() const {
        return oldRelatives_;
    }

    const NodeRelatives& newRelatives() const {
        return newRelatives_;
    }

protected:
    void undo(Document* doc) override;
    void redo(Document* doc) override;

private:
    Node* node_;
    NodeRelatives oldRelatives_;
    NodeRelatives newRelatives_;
};

class VGC_DOM_API CreateAuthoredAttributeOperation : public AtomicOperation {
protected:
    friend Operation;

    CreateAuthoredAttributeOperation(
        Element* element,
        core::StringId name,
        const Value& value) :
        element_(element),
        name_(name),
        value_(value) {}

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

protected:
    void undo(Document* doc) override;
    void redo(Document* doc) override;

private:
    Element* element_;
    core::StringId name_;
    Value value_;
};

class VGC_DOM_API RemoveAuthoredAttributeOperation : public AtomicOperation {
protected:
    friend Operation;

    RemoveAuthoredAttributeOperation(
        Element* element,
        core::StringId name,
        Value value) :
        element_(element),
        name_(name),
        value_(value) {}

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

protected:
    void undo(Document* doc) override;
    void redo(Document* doc) override;

private:
    Element* element_;
    core::StringId name_;
    Value value_;
};

class VGC_DOM_API ChangeAuthoredAttributeOperation : public AtomicOperation {
protected:
    friend Operation;

    ChangeAuthoredAttributeOperation(
        Element* element,
        core::StringId name,
        Value oldValue,
        Value newValue) :
        element_(element),
        name_(name),
        oldValue_(oldValue),
        newValue_(newValue) {}

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

protected:
    void undo(Document* doc) override;
    void redo(Document* doc) override;

private:
    Element* element_;
    core::StringId name_;
    Value oldValue_;
    Value newValue_;
};

using OperationIndex = UInt32;
OperationIndex genOperationIndex();

class VGC_DOM_API Operation {
protected:
    explicit Operation(core::StringId name) :
        name_(name), index_(genOperationIndex()) {}

public:
    // non-copyable
    Operation(const Operation&) = delete;
    Operation& operator=(const Operation&) = delete;

    Operation(Operation&& other) noexcept :
        name_(other.name_),
        index_(other.index_),
        isReverted_(other.isReverted_) {

        atomicOperations_.swap(other.atomicOperations_);
    }

    Operation& operator=(Operation&& other) noexcept {
        std::swap(name_, other.name_);
        std::swap(index_, other.index_);
        std::swap(isReverted_, other.isReverted_);
        atomicOperations_.swap(other.atomicOperations_);
    }

    const core::StringId& name() const {
        return name_;
    }

    OperationIndex index() const {
        return index_;
    }

    bool isReverted() const {
        return isReverted_;
    }

private:
    friend Document;

    core::Array<std::unique_ptr<AtomicOperation>> atomicOperations_;
    core::StringId name_;
    OperationIndex index_;
    bool isReverted_ = false;

    const auto& atomicOperations_() const {
        return atomicOperations_;
    }

    template<typename TAtomicOperation, typename... Args,
        core::Requires<std::is_base_of_v<AtomicOperation, TAtomicOperation>> = true>
    void emplaceLastAtomicOperation_(Args&&... args) {
        // AtomicOperation constructor is private, cannot emplace with args.
        atomicOperations_.emplaceLast(TAtomicOperation(std::forward<Args>(args)...));
    }

    // XXX pointer to document could be a member..

    void undo_(Document* doc);
    void redo_(Document* doc);
};


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
    std::set<Node*> childrenChangedNodes_;

    std::unordered_map<Element*, std::set<core::StringId>>
        modifiedElements_;

    Diff() = default;

    void reset() {
        createdNodes_.clear();
        removedNodes_.clear();
        reparentedNodes_.clear();
        childrenChangedNodes_.clear();
        modifiedElements_.clear();
    }

    bool isEmpty() const {
        return (
            createdNodes_.empty() &&
            removedNodes_.empty() &&
            reparentedNodes_.empty() &&
            childrenChangedNodes_.empty() &&
            modifiedElements_.empty());
    }
};

} // namespace vgc::dom

#endif // VGC_DOM_OPERATION_H
