// Copyright 2017 The VGC Developers
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

#include <vgc/core/algorithm.h>
#include <vgc/core/logging.h>
#include <vgc/dom/document.h>

namespace vgc {
namespace dom {

Node::Node(NodeType type) :
    nodeType_(type),
    parent_(nullptr),
    document_(nullptr)
{
    if (type == NodeType::Document) {
        document_ = Document::cast(this);
    }
}

bool Node::canAppendChild(Node* node, std::string* reason)
{
    if (node->nodeType() == NodeType::Document) {
        if (reason) {
            *reason += "nodes can't have Document children";
        }
        return false;
    }
    else if (this->nodeType() == NodeType::Document &&
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
}

Node* Node::appendChild(NodeSharedPtr node)
{
    std::string reason;
    if (!canAppendChild(node.get(), &reason)) {
        core::warning() << "Can't append child: " << reason << std::endl;
        return nullptr;
    }

    // Remove from existing parent if any, but don't nullify node->document_
    // since we override this just after anyway.
    if (node->parent_) {
        bool nullifyDocument = false;
        node->parent_->removeChild_(node.get(), nullifyDocument);
    }

    children_.push_back(node);
    node->parent_ = this;
    node->setDocument_(document());

    return node.get();
}

NodeSharedPtr Node::removeChild(Node* node)
{
    return removeChild_(node);
}

NodeSharedPtr Node::removeChild_(Node* node, bool nullifyDocument)
{
    NodeSharedPtr nodePtr = node->sharedPtr();
    bool removed = core::removeOne(children_, nodePtr);

    if (!removed) {
        core::warning() << "Can't remove child: the given node is not a child of this node" << std::endl;
        return NodeSharedPtr();
    };

    // The node doesn't have any owner document anymore, so we should set
    // document_ to nullptr. As an optimization, we omit this operation if the
    // caller knows that document_ will be overidden just after anyway.
    if (nullifyDocument) {
        node->setDocument_(nullptr);
    }

    return nodePtr;
}

void Node::setDocument_(Document* document)
{
    // Nothing to do if it's already done
    if (document_ == document) {
        return;
    }

    document_ = document;
    for (const NodeSharedPtr& node : children_) {
        node->setDocument_(document);
    }
}

} // namespace dom
} // namespace vgc
