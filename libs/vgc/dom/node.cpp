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
#include <vgc/dom/document.h>

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
    if (isAlive()) {
        // We can be here if `this` is a Document, or if an exception was
        // thrown in one of Node's methods.
        const bool calledFromDestructor = true;
        destroy_(calledFromDestructor);
    }
}

void Node::destroy()
{
    checkAlive();
    const bool calledFromDestructor = false;
    destroy_(calledFromDestructor);
}

bool Node::canAppendChild(Node* node, std::string* reason)
{
    checkAlive();

    // Warning! Be careful when modifying this function: it is possible that
    // node is an Element with no parent. Normally, elements are guaranteed to
    // have a parent, but this function is also called within Element::create()
    // to set its initial parent.

    if (this->document() != node->document())
    {
        if (reason) {
            *reason += "The node to append must belong to the same document as this node";
        }
        return false;
    }

    if (node->nodeType() == NodeType::Document) {
        if (reason) {
            *reason += "nodes can't have Document children";
        }
        return false;
    }

    if (this->nodeType() == NodeType::Document &&
             node->nodeType() == NodeType::Element)
    {
        if (Document::cast(this)->rootElement()) {
            if (reason) {
                *reason += "Document nodes can't have more than one Element child";
            }
            return false;
        }
    }

    return true;

    // XXX how about if this == node?
    // or node is an ancestor of this?

    // XXX how about if node == Document::cast(this)->rootElement() ? This
    // should just move the root element as the last child and be allowed. For
    // example, one should be able to call doc->appendChild(doc->rootElement())
    // with no warnings.
}

bool Node::appendChild(Node* node)
{
    checkAlive();

    // Warning! Be careful when modifying this function: it is possible that
    // node is an Element with no parent. Normally, elements are guaranteed to
    // have a parent, but this function is also called within Element::create()
    // to set its initial parent.

    std::string reason;
    if (!canAppendChild(node, &reason)) {
        core::warning() << "Can't append child: " << reason << std::endl;
        return false;
    }

    // Detach node from current parent
    NodeSharedPtr nodePtr = node->detachFromParent_();

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

    return true;
}

bool Node::removeChild(Node* node)
{
    checkAlive();

    if (node->parent_ != this) {
        core::warning() << "Can't remove child: the given node is not a child of this node" << std::endl;
        return false;
    };

    node->destroy();

    return true;
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

    // Detach newChild from current parent
    NodeSharedPtr newChildPtr = newChild->detachFromParent_();

    // Keep oldChild alive
    NodeSharedPtr oldChildPtr = oldChild->sharedPtr();

    // Update pointers
    if (oldChild->previousSibling_) {
        oldChild->previousSibling_->nextSibling_ = newChildPtr;
    }
    else {
        firstChild_ = newChildPtr;
    }
    if (oldChild->nextSibling_) {
        oldChild->nextSibling_->previousSibling_ = newChild;
    }
    else {
        lastChild_ = newChild;
    }
    std::swap(oldChild->previousSibling_, newChild->previousSibling_);
    std::swap(oldChild->nextSibling_, newChild->nextSibling_);
    std::swap(oldChild->parent_, newChild->parent_);

    // Destroy oldChild
    oldChild->destroy();

    // Destruct oldChild now, unless other shared pointers point to oldChild.
    // This line of code is redundant with closing the scope, but it is kept
    // for clarifying intent.
    oldChildPtr.reset();

    return true;
}

NodeSharedPtr Node::detachFromParent_(bool calledFromNodeDestructor)
{
    // Keep alive
    NodeSharedPtr res;

    if (!calledFromNodeDestructor) {
        res = sharedPtr();
    }

    // Update pointers
    if (parent()) {
        if (previousSibling()) {
            previousSibling()->nextSibling_ = nextSibling_;
        }
        else {
            parent()->firstChild_ = nextSibling_;
        }
        if (nextSibling()) {
            nextSibling()->previousSibling_ = previousSibling_;
        }
        else {
            parent()->lastChild_ = previousSibling_;
        }
        parent_ = nullptr;
        previousSibling_ = nullptr;
        nextSibling_.reset();
    }

    return res;
}

void Node::destroy_(bool calledFromDestructor)
{
    // Recursively destroy descendants. Note: we can't use a range loop here
    // since we're modifying the range while iterating.
    while (Node* child = lastChild()) {
        child->destroy();
    }

    // Detach this Node from current parent
    NodeSharedPtr thisPtr = detachFromParent_(calledFromDestructor);

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

    // Destruct this Node now, unless other shared pointers point to this Node.
    // This line of code is redundant with closing the scope, but it is kept
    // for clarifying intent.
    thisPtr.reset();
}

} // namespace dom
} // namespace vgc
