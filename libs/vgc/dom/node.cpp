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
bool checkCanAppendChild_(Node* parent, Node* child, bool simulate = false)
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

    if (parent->nodeType() == NodeType::Document &&
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

bool Node::canAppendChild(Node* node)
{
    const bool simulate = true;
    return checkCanAppendChild_(this, node, simulate);
}

void Node::appendChild(Node* node)
{
    checkCanAppendChild_(this, node);
    NodeSharedPtr nodePtr = node->detachFromParent_();
    appendChild_(std::move(nodePtr));
}

bool Node::replaceChild(Node* newChild, Node* oldChild)
{
    checkAlive();

    if (oldChild->parent() != this) {
        core::warning() << "Can't replace child: oldChild is not a child of this Node" << std::endl;
        return false;
    }

    if (this->document() != newChild->document()) {
        core::warning() << "Can't replace child: newChild is owned by another Document" << std::endl;
        return false;
    }

    if (newChild->nodeType() == NodeType::Document) {
        core::warning() << "Can't replace child: newChild is a Document node" << std::endl;
        return false;
    }

    if (nodeType() == NodeType::Document &&
        newChild->nodeType() == NodeType::Element &&
        newChild->nodeType() != NodeType::Element)
    {
        core::warning() << "Can't replace child: would add a second root element of this Document" << std::endl;
        return false;
    }

    // XXX TODO: raise warning if newChild is an ancestor of this Node (including this Node itself).

    if (oldChild == newChild) {
        // nothing to do
        return true;
    }

    // Detach newChild from current parent.
    // Note: After this, sh points to newChild.
    NodeSharedPtr sh = newChild->detachFromParent_();

    // Update owning pointers.
    // Note: After this, sh points to oldChild.
    NodeSharedPtr& owning = oldChild->previousSibling_ ? oldChild->previousSibling_->nextSibling_ : this->firstChild_;
    std::swap(owning, sh);
    std::swap(oldChild->nextSibling_, newChild->nextSibling_);

    // Update non-owning pointers.
    Node*& nonowning = newChild->nextSibling_ ? newChild->nextSibling_->previousSibling_ : this->lastChild_;
    nonowning = newChild;
    std::swap(oldChild->previousSibling_, newChild->previousSibling_);
    std::swap(oldChild->parent_, newChild->parent_);

    // Destroy oldChild
    oldChild->destroy();

    // Destruct oldChild now, unless other shared pointers point to oldChild.
    // This line of code is redundant with closing the scope, but it is kept
    // for clarifying intent.
    sh.reset();

    return true;
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

Node* Node::appendChild_(NodeSharedPtr nodePtr)
{
    Node* node = nodePtr.get();

    // Append to the end of the doubly linked-list of siblings.
    if (this->lastChild_) {
        this->lastChild_->nextSibling_ = std::move(nodePtr);
        node->previousSibling_ = this->lastChild_;
    }
    else {
        this->firstChild_ = std::move(nodePtr);
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
