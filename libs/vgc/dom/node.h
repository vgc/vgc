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

#ifndef VGC_DOM_NODE_H
#define VGC_DOM_NODE_H

#include <fstream>
#include <vector>
#include <vgc/core/object.h>
#include <vgc/dom/api.h>
#include <vgc/dom/xmlformattingstyle.h>

namespace vgc {
namespace dom {

class Document;

/// \enum vgc::dom::NodeType
/// \brief Specifies the type of a Node.
///
/// Only a subset of XML is currently supported. Full support
/// will be implemented later.
///
/// Note that the enum value of each NodeType is not arbitrary, but follows the
/// W3C DOM Specification (e.g., NodeType::Document == 9). However, please be
/// aware that not all W3C node types have a corresponding vgc::dom::NodeType,
/// for example, in vgc::dom, an attribute is not considered to be a Node, and
/// therefore NodeType::Attribute is not defined.
///
enum class NodeType {
    Element = 1,  ///< An Element node.
    Document = 9, ///< A Document node.

    /* Other W3C DOM node types, for reference:
     *
     * Attribute = 2,       // Note: we won't add this in VGC DOM (attributes aren't nodes in the VGC DOM)
     * Text = 3,            // Note: rename to TextNode in VGC DOM? We may want to reserve "Text" for <text> elements
     * CDATA = 4,           // Note: rename to CharacterData in VGC DOM?
     * EntityReference = 5,
     * Entity = 6,
     * ProcessingInstruction = 7,
     * Comment = 8,
     * DocumentType = 10,
     * DocumentFragment = 11,
     * Notation = 12
     */
};

VGC_CORE_DECLARE_PTRS(Node);

/// \class vgc::dom::Node
/// \brief Represents a node of the DOM. Most dom classes derive from Node.
///
/// This is the base class for most classes in the DOM tree. For example, the
/// Document is a node and each Element is a Node. As a notable exception, note
/// that an Attribute is not a Node.
///
/// Each Node has childNodes() and is responsible for the lifetime of
/// these children. Child nodes are ordered, which correspond to the order of
/// appearance in the XML file.
///
class VGC_DOM_API Node: public core::Object
{
public:
    VGC_CORE_OBJECT(Node)

    /// Returns the given \p node. This no-op function is provided for use in
    /// generic templated code where T might be Node or one of its direct
    /// subclass:
    ///
    /// \code
    /// T* castedNode = T::cast(node);
    /// \endcode
    ///
    /// Obviously, don't use this function in non-templated code, that would be
    /// silly. However, do use the cast() functions provided by all direct
    /// subclasses, say, Element::cast().
    ///
    static Node* cast(Node* node) {
        return node;
    }

    /// Returns the NodeType of this Node.
    ///
    NodeType nodeType() const {
        return nodeType_;
    }

    /// Returns the parent Node of this Node.
    ///
    /// Note that a Node may have a null parent Node for the following reasons:
    ///
    /// 1. This Node is a Document.
    ///
    /// 2. This Node is the root of a Node tree which does not belong to
    ///    any Document (for instance, as a temporary state while moving nodes from
    ///    one Document to another).
    ///
    /// In all other cases, this function returns a non-null pointer.
    ///
    /// \sa firstChild(), lastChild(), previousSibling(), and nextSibling().
    ///
    Node* parent() const {
        return parent_;
    }

    /// Returns the first child Node of this Node. Returns nullptr if this Node
    /// has no children.
    ///
    /// \sa lastChild(), previousSibling(), nextSibling(), and parent().
    ///
    Node* firstChild() const {
        return firstChild_.get();
    }

    /// Returns the last child Node of this Node. Returns nullptr if this Node
    /// has no children.
    ///
    /// \sa firstChild(), previousSibling(), nextSibling(), and parent().
    ///
    Node* lastChild() const {
        return lastChild_;
    }

    /// Returns the previous sibling of this Node. Returns nullptr if this Node
    /// has a null parent(), or if it is the first child of its parent.
    ///
    /// \sa nextSibling(), parent(), firstChild(), and lastChild().
    ///
    Node* previousSibling() const {
        return previousSibling_;
    }

    /// Returns the next sibling of this Node. Returns nullptr if this Node
    /// has a null parent(), or if it is the last child of its parent.
    ///
    /// \sa previousSibling(), parent(), firstChild(), and lastChild().
    ///
    Node* nextSibling() const {
        return nextSibling_.get();
    }

    /// Returns the owner Document of this Node.
    ///
    /// If this Node is a Document, then this function always returns this
    /// Node.
    ///
    /// This function may return a null pointer if this node is not part of any
    /// Document, for instance, as a temporary state while moving nodes from
    /// one Document to another.
    ///
    Document* document() const {
        return document_;
    }

    /// Returns whether the given \p node can be appended as the last child of
    /// this Node. Here is the exhaustive list of reasons why a node might not
    /// be appendable:
    ///
    /// 1. You're trying to append a Document node.
    ///
    /// 2. You're trying to append an Element node to a Document node that
    ///    already has a rootElement.
    ///
    /// If \p reason is not null, then the reason preventing insertion (if
    /// any), is appended to the string.
    ///
    bool canAppendChild(Node* node, std::string* reason = nullptr);

    /// Appends the given \p node as the last child of this Node. If \p node
    /// already has a parent, then it is first removed from the children of
    /// this anterior parent. If this Node is already the parent of \p node,
    /// then it is moved as the last child of this Node.
    ///
    /// If the node cannot be appended (see canAppendChild()), a warning is
    /// emitted and the node is not appended.
    ///
    /// Returns a pointer to the appended node. This is equal to node.get() if
    /// the node was appended; nullptr otherwise.
    ///
    /// \sa canAppendChild().
    ///
    Node* appendChild(NodeSharedPtr node);

    /// Removes the given \p node from the children of this Node.
    ///
    /// If the node is not a child of this Node (which can be checked via
    /// `node->parent() == this`), a warning is emitted and the node is not
    /// removed.
    ///
    /// Returns a shared pointer to the removed node, or a null shared pointer
    /// if no node was removed. The node will be deleted unless you hold on to
    /// the return shared pointer, or another object already hold on to it.
    ///
    /// \sa appendChild(), children(), parent().
    ///
    NodeSharedPtr removeChild(Node* node);

protected:
    /// Creates a new Node with no parent. You cannot call this function
    /// directly, instead, please create a Document or an Element.
    ///
    Node(NodeType type);

private:
    // Node type
    NodeType nodeType_;

    // parent-child relationship
    Node* parent_;
    NodeSharedPtr firstChild_;
    Node* lastChild_;
    Node* previousSibling_;
    NodeSharedPtr nextSibling_;

    // Helper method for removeChild() and appendChild()
    NodeSharedPtr removeChild_(Node* node, bool nullifyDocument = true);

    // Owner document
    void setDocument_(Document* document);
    Document* document_;

    // Helper method for Document::save()
    void writeChildren_(std::ofstream& out,
                        const XmlFormattingStyle& style,
                        int indentLevel) const;
};

} // namespace dom
} // namespace vgc

#endif // VGC_DOM_NODE_H
