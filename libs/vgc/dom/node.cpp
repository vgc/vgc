// Copyright 2018 The VGC Developers
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

#include <vgc/dom/node.h>

#include <vgc/core/assert.h>
#include <vgc/core/logging.h>
#include <vgc/core/stringutil.h>
#include <vgc/dom/document.h>
#include <vgc/dom/element.h>

namespace vgc {
namespace dom {

Node::Node(Document* document, NodeType nodeType) :
    Object(core::Object::ConstructorKey()),
    document_(document),
    nodeType_(nodeType),
    parent_(nullptr),
    firstChild_(),
    lastChild_(nullptr),
    previousSibling_(nullptr),
    nextSibling_()
{

}

Node::~Node()
{
    if (isAlive()) { // => this Node is a Document, or an exception was thrown
        destroy_();
    }
}

void Node::destroy()
{
    checkAlive();
    destroy_();
}

namespace {
bool checkCanReparent_(Node* parent, Node* child, bool simulate = false, bool checkSecondRootElement = true)
{
    parent->checkAlive();
    child->checkAlive();

    if (parent->document() != child->document()) {
        if (simulate) return false;
        else throw WrongDocumentError(parent, child);
    }

    if (child->nodeType() == NodeType::Document) {
        if (simulate) return false;
        else throw WrongChildTypeError(parent, child);
    }

    if (checkSecondRootElement &&
        parent->nodeType() == NodeType::Document &&
        child->nodeType() == NodeType::Element)
    {
        Element* rootElement = Document::cast(parent)->rootElement();
        if (rootElement && rootElement != child) {
            if (simulate) return false;
            else throw SecondRootElementError(Document::cast(parent));
        }
    }

    if (parent->isDescendant(child)) {
        if (simulate) return false;
        else throw ChildCycleError(parent, child);
    }

    return true;
}
} // namespace

bool Node::canReparent(Node* newParent)
{
    const bool simulate = true;
    return checkCanReparent_(newParent, this, simulate);
}

void Node::reparent(Node* newParent)
{
    checkCanReparent_(newParent, this);
    NodeSharedPtr nodePtr = detachFromParent_();
    newParent->attachChild_(std::move(nodePtr));
}

namespace {
bool checkCanReplace_(Node* oldNode, Node* newNode, bool simulate = false)
{
    oldNode->checkAlive();
    newNode->checkAlive();

    // Avoid raising ReplaceDocumentError if oldNode == newNode (= Document node)
    if (oldNode == newNode) {
        return true;
    }

    if (oldNode->nodeType() == NodeType::Document) {
        if (simulate) return false;
        else throw ReplaceDocumentError(Document::cast(oldNode), newNode);
    }
    Node* oldNodeParent = oldNode->parent(); // guaranteed non-null

    // Avoid raising SecondRootElementError if oldNode is the root element
    bool checkSecondRootElement = (oldNode->nodeType() != NodeType::Element);

    // All other checks are the same as for reparent(), so we delate.
    return checkCanReparent_(oldNodeParent, newNode, simulate, checkSecondRootElement);
}
} // namespace

bool Node::canReplace(Node* oldNode)
{
    const bool simulate = true;
    return checkCanReplace_(oldNode, this, simulate);
}

void Node::replace(Node* oldNode)
{
    checkCanReplace_(oldNode, this);

    if (this == oldNode) {
        // nothing to do
        return;
    }

    // Detach from current parent.
    // Note: After this line of code, sh points to this.
    NodeSharedPtr sh = detachFromParent_();

    // Update owning pointers.
    // Note: After these lines of code, sh points to oldNode.
    NodeSharedPtr& owning = oldNode->previousSibling_ ? oldNode->previousSibling_->nextSibling_ : oldNode->parent_->firstChild_;
    std::swap(owning, sh);
    std::swap(oldNode->nextSibling_, nextSibling_);

    // Update non-owning pointers.
    Node*& nonowning = nextSibling_ ? nextSibling_->previousSibling_ : oldNode->parent_->lastChild_;
    nonowning = this;
    std::swap(oldNode->previousSibling_, previousSibling_);
    std::swap(oldNode->parent_, parent_);

    // Destroy oldNode
    oldNode->destroy();

    // Destruct oldNode now, unless other shared pointers point to oldNode.
    // This line of code is redundant with closing the scope, but it is kept
    // for clarifying intent.
    sh.reset();
}

bool Node::isDescendant(const Node* other) const
{
    // Go up from this node to the root; check if we encounter the other node
    const Node* node = this;
    while (node != nullptr) {
        if (node == other) {
            return true;
        }
        node = node->parent();
    }
    return false;
}

Node* Node::attachChild_(NodeSharedPtr child)
{
    Node* node = child.get();

    // Append to the end of the doubly linked-list of siblings.
    if (this->lastChild_) {
        this->lastChild_->nextSibling_ = std::move(child);
        node->previousSibling_ = this->lastChild_;
    }
    else {
        this->firstChild_ = std::move(child);
    }

    // Set parent-child relationship
    this->lastChild_ = node;
    node->parent_ = this;

    return node;
}

NodeSharedPtr Node::detachFromParent_()
{
    NodeSharedPtr res;

    if (parent_) {
        // Update owning pointers
        NodeSharedPtr& owning = previousSibling_ ? previousSibling_->nextSibling_ : parent_->firstChild_;
        std::swap(res, owning);
        std::swap(owning, nextSibling_);

        // Update non-owning pointers
        Node*& nonowning = owning ? owning->previousSibling_ : parent_->lastChild_;
        nonowning = previousSibling_;
        parent_ = nullptr;
        previousSibling_ = nullptr;
    }

    return res;
}

void Node::destroy_()
{
    // Recursively destroy descendants. Note: we can't use a range loop here
    // since we're modifying the range while iterating.
    while (Node* child = lastChild()) {
        child->destroy();
    }

    // Detach this Node from current parent if any. Note: if we are already
    // part of this Node's destructor call stack, then this node is guaranteed
    // to have no parent.
    NodeSharedPtr thisPtr = detachFromParent_();

    // Clear data
    //
    // Note: we intentionally don't clear nodeType_ since it is useful
    // debug info and clearing it wouldn't save memory
    //
    // XXX TODO: call virtual method overriden by Element and Document
    // which clear() and shrink_to_fit() the authored attributes
    //

    // Set this Node as not alive.
    //
    // Note: the raw pointer document_ might be dangling after this node is
    // destroyed, which is why we must set it to nullptr, and thus we use it as
    // an otherwise redundant "isAlive_" member variable.
    //
    document_ = nullptr;

    // Destruct this Node now, unless:
    // - other shared pointers also point to this Node (= observers), or
    // - we are already part of this Node's destructor call stack.
    //
    // This line of code is redundant with closing the scope, but it is kept
    // for clarifying intent.
    //
    thisPtr.reset();
}

} // namespace dom
} // namespace vgc
