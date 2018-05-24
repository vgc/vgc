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

#include <vgc/core/assert.h>
#include <vgc/core/logging.h>
#include <vgc/dom/document.h>
#include <vgc/dom/element.h>

namespace vgc {
namespace dom {

Node::Node(NodeType type) :
    nodeType_(type),
    parent_(nullptr),
    firstChild_(),
    lastChild_(nullptr),
    previousSibling_(nullptr),
    nextSibling_(),
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

    // XXX how about if this == node?
    // or node is an ancestor of this?

    // XXX how about if node == Document::cast(this)->rootElement() ? This
    // should just move the root element as the last child and be allowed. For
    // example, one should be able to call doc->appendChild(doc->rootElement())
    // with no warnings.
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

    // Append to the end of the doubly linked-list of siblings.
    // This takes ownership of the node.
    if (this->lastChild_) {
        this->lastChild_->nextSibling_ = node;
        node->previousSibling_ = this->lastChild_;
    }
    else {
        this->firstChild_ = node;
    }

    // Set parent-child relationship
    this->lastChild_ = node.get();
    node->parent_ = this;
    node->setDocument_(document());

    return node.get();
}

NodeSharedPtr Node::removeChild(Node* node)
{
    if (node->parent_ != this) {
        core::warning() << "Can't remove child: the given node is not a child of this node" << std::endl;
        return NodeSharedPtr();
    };

    return removeChild_(node);
}

NodeSharedPtr Node::removeChild_(Node* node, bool nullifyDocument)
{
    VGC_CORE_ASSERT(node->parent_);

    // Take ownership of node
    NodeSharedPtr nodePtr = node->sharedPtr();

    // Update who points to node->nextSibling_
    // (this would delete the node if we hadn't taken ownership)
    if (node->previousSibling_) {
        node->previousSibling_->nextSibling_ = node->nextSibling_;
    }
    else {
        node->parent_->firstChild_ = node->nextSibling_;
    }

    // Update who points to node->previousSibling_
    if (node->nextSibling_) {
        node->nextSibling_->previousSibling_ = node->previousSibling_;
    }
    else {
        node->parent_->lastChild_ = node->previousSibling_;
    }

    // Set as root of new Node tree
    node->parent_ = nullptr;
    node->previousSibling_ = nullptr;
    node->nextSibling_.reset();

    // The node doesn't have any owner document anymore, so we should set
    // document_ to nullptr. As an optimization, we omit this operation if the
    // caller knows that document_ will be overidden just after anyway.
    // Note: node can't be a Document node since it had a parent.
    if (nullifyDocument) {
        node->setDocument_(nullptr);
    }

    // Pass ownership
    return nodePtr;
}

void Node::setDocument_(Document* document)
{
    // Nothing to do if it's already done
    if (document_ == document) {
        return;
    }

    document_ = document;
    for (Node* child = firstChild(); child != nullptr; child = child->nextSibling()) {
        child->setDocument_(document);
    }
}

namespace {

void writeIndent_(std::ofstream& out,
                  const XmlFormattingStyle& style,
                  int indentLevel)
{
    char c = (style.indentStyle == XmlIndentStyle::Spaces) ? ' ' : '\t';
    out << std::string(indentLevel * style.indentSize, c);
}

void writeAttributeIndent_(std::ofstream& out,
                           const XmlFormattingStyle& style,
                           int indentLevel)
{
    char c = (style.indentStyle == XmlIndentStyle::Spaces) ? ' ' : '\t';
    out << std::string(indentLevel * style.indentSize + style.attributeIndentSize, c);
}

}

void Node::writeChildren_(std::ofstream& out,
                          const XmlFormattingStyle& style,
                          int indentLevel) const
{
    /*
    for (Node* node = firstChild(); node != nullptr; node = node->nextSibling()) {
        if (Element* element = Element::cast(node)) {
            writeIndent_(out, style, indentLevel);
            out << '<' << element->name();
            for (const AuthoredAttribute& a : element->authoredAttributes()) {
                out << '\n';
                writeAttributeIndent_(out, style, indentLevel);
                out << a.name() << "=\"" << toString(a.value()) << "\"";
            }
            out << ">\n";
            writeChildren_(out, style, indentLevel + 1);
            writeIndent_(out, style, indentLevel);
            out << '<' << element->name() << "/>\n";
        }
    }
    */
}

} // namespace dom
} // namespace vgc
