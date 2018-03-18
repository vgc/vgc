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

#include <vector>
#include <vgc/core/object.h>
#include <vgc/dom/api.h>

namespace vgc {
namespace dom {

/// \enum vgc::dom::NodeType
/// \brief Specifies the type of a Node.
///
/// Only a subset of XML is currently supported. Full support
/// will be implemented later.
///
/// Note that the value corresponding to each node type is conformant with
/// the W3C DOM Specification. However, not all W3C node types have a
/// corresponding vgc::dom::Node::Type, for example one reason is that our
/// Attributes are not nodes.
///
enum class NodeType {
    /// An Element node.
    ///
    Element = 1, ///< An Element node.

    /* // Attribute = 2, // Note: we are not planning to implement this (attributes aren't nodes in our design)
     * Text = 3, // Or TextNode? We reserve the class name "Text" for a <text> element, and plan to use TextNode for text nodes.
     * CDATA = 4, // Or CharacterData?
     * EntityReference = 5,
     * Entity = 6,
     * ProcessingInstruction = 7,
     * Comment */

     /// A Document node.
     ///
     Document = 9,

     /* DocumentType = 10,
      * DocumentFragment = 11,
      * Notation = 12 */
};

VGC_CORE_DECLARE_PTRS(Document);
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

    /// Returns the Type of this Node.
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
    /// \sa childNodes()
    ///
    Node* parentNode() const {
        return parentNode_;
    }

    /// Returns the child nodes of this Node.
    ///
    /// If this Node is a Document, then its childNodes include:
    /// - XmlDeclaration nodes (at most one, appears first)
    /// - Comment nodes
    /// - Element nodes (at most one, called the
    /// Child nodes of a Do
    ///
    /// \sa parentNode().
    ///
    const std::vector<NodeSharedPtr>& childNodes() const {
        // XXX use iterators instead.
        return childNodes_;
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

protected:
    /// Creates a new Node with no parent. You cannot call this function
    /// directly, instead, please create a Document or an Element.
    ///
    Node(NodeType type);

private:
    // Node type
    NodeType nodeType_;

    // parent-child relationship
    Node* parentNode_;
    std::vector<NodeSharedPtr> childNodes_;
    void addChild_(NodeSharedPtr node);
    void removeAllChildren_();

    // Owner document
    Document* document_;

    // Allow each type of node to set children/parent, since they have
    // different constraints that Node does not know about (for example, an
    // Attribute cannot have a child Element). However, note that subclasses of
    // these are not befriended, that is, a PathElement cannot directly
    // manipulate its children_.
    friend class Document;
    friend class Element;
};

} // namespace dom
} // namespace vgc

#endif // VGC_DOM_NODE_H
