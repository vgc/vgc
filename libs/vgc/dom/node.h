// Copyright 2021 The VGC Developers
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

#include <vgc/core/format.h>
#include <vgc/core/object.h>
#include <vgc/dom/api.h>
#include <vgc/dom/exceptions.h>
#include <vgc/dom/value.h>

namespace vgc::dom {

VGC_DECLARE_OBJECT(Element);
class Path;

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

/// Writes the given NodeType to the output stream.
///
template<typename OStream>
void write(OStream& out, NodeType type) {
    switch (type) {
    case NodeType::Element:
        write(out, "Element");
        break;
    case NodeType::Document:
        write(out, "Document");
        break;
    }
}

VGC_DECLARE_OBJECT(Node);
VGC_DECLARE_OBJECT(Document);

namespace detail {
void destroyNode(Node* node);
} // namespace detail

/// \class vgc::dom::Node
/// \brief Represents a node of the document Node tree.
///
/// See Document for details.
///
/// Owner Document
/// --------------
///
/// Every alive node is owned by exactly one Document which you can query via
/// document(). This method is always guaranteed to return a valid non-null
/// Document*. This document is determined when the node is created, and never
/// changes in the lifetime of the node.
///
/// In particular, we intentionally disallow to move a node from one Document
/// to another. This is a design decision which was taken in order to make many
/// other more common operations faster, and to remove a lot of burden on
/// client code. For example, if a GUI widget stores a pointer to a Node, then
/// it never has to worry about the node moving from one Document to another,
/// and how to handle this appropriately.
///
/// Constness
/// ---------
///
/// XXX Give details on what "constness" mean for nodes.
///
///
class VGC_DOM_API Node : public core::Object {
private:
    VGC_OBJECT(Node, core::Object)

protected:
    /// Constructs a parent-less Node of the given \p nodeType, owned by the
    /// given \p document. This constructor is an implementation detail only
    /// available to derived classes. In order to create nodes, please use one
    /// of the following:
    ///
    /// ```cpp
    /// DocumentPtr document = Document::create();
    /// Element* element = Element::create(parent, name);
    /// ```
    ///
    Node(Document* document, NodeType nodeType);

    void onDestroyed() override;

public:
    /// Returns the owner Document of this Node. This is always guaranteed to
    /// be a non-null valid Document.
    ///
    Document* document() const {
        return document_;
    }

    /// Returns the NodeType of this Node.
    ///
    /// This function is safe to call even when the node is not alive.
    ///
    NodeType nodeType() const {
        return nodeType_;
    }

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
    /// This function is safe to call even when the node is not alive.
    ///
    static Node* cast(Node* node) {
        return node;
    }

    /// Remove this node from its document (undoable).
    ///
    void remove();

    /// Returns the parent `Node` of this `Node`. This is always nullptr for
    /// `Document` nodes, and always a non-null valid `Node` otherwise.
    ///
    /// \sa firstChild(), lastChild(), previousSibling(), and nextSibling().
    ///
    Node* parent() const {
        return static_cast<Node*>(parentObject());
    }

    /// Returns the first child `Node` of this `Node`. Returns nullptr if this
    /// `Node` has no child `Node`.
    ///
    /// \sa lastChild(), previousSibling(), nextSibling(), and parent().
    ///
    Node* firstChild() const {
        return static_cast<Node*>(firstChildObject());
    }

    /// Returns the last child `Node` of this `Node`. Returns nullptr if this
    /// `Node` has no child `Node`.
    ///
    /// \sa firstChild(), previousSibling(), nextSibling(), and parent().
    ///
    Node* lastChild() const {
        return static_cast<Node*>(lastChildObject());
    }

    /// Returns the previous sibling of this `Node`. Returns nullptr if this
    /// `Node` is a `Document` node, or if this `Node` is the first child of its
    /// parent.
    ///
    /// \sa nextSibling(), parent(), firstChild(), and lastChild().
    ///
    Node* previousSibling() const {
        return static_cast<Node*>(previousSiblingObject());
    }

    /// Returns the next sibling of this `Node`. Returns nullptr if this `Node`
    /// is a `Document` node, or if this `Node` is the last child of its parent.
    ///
    /// \sa previousSibling(), parent(), firstChild(), and lastChild().
    ///
    Node* nextSibling() const {
        return static_cast<Node*>(nextSiblingObject());
    }

    /// Returns all children of this `Node` as an iterable range.
    ///
    /// Example:
    ///
    /// \code
    /// for (Node* child : node->children()) {
    ///     // ...
    /// }
    /// \endcode
    ///
    NodeListView children() const {
        // TODO: store children in a NodeList (see ui::Widget for an example)
        return NodeListView(firstChild(), nullptr);
    }

    /// Adds the given `child` to this node children before `nextSibling`.
    ///
    void insertChild(Node* nextSibling, Node* child);

    /// Returns whether this Node can be reparented with the given \p newParent.
    /// See reparent() for details.
    ///
    bool canReparent(Node* newParent);

    /// Moves this Node from its current position in the DOM tree to the last
    /// child of the given \p newParent. If \p newParent is already the current
    /// parent of this node, then this Node is simply moved last without
    /// changing its parent.
    ///
    /// An exception is raised in the following cases:
    ///
    /// 1. WrongDocumentError: this Node and \p newParent belong to different
    ///    documents.
    ///
    /// 2. WrongChildTypeError: the type of this Node is not allowed as a child
    ///    of \p newParent. Here is the list of allowed child types:
    ///    - Document: allowed child types are Element (at most one)
    ///    - Element: allowed child types are Element
    ///
    /// 3. SecondRootElementError: this Node is an Element node, \p newParent
    ///    is the Document node, and reparenting would result in a second root
    ///    element.
    ///
    /// 4. ChildCycleError: \p newParent is this Node itself or one of its
    ///    descendant.
    ///
    /// If several exceptions apply, the one appearing first in the list above is
    /// raised.
    ///
    void reparent(Node* newParent);

    /// Returns whether \p oldNode can be replaced by this Node. See replace()
    /// for details.
    ///
    bool canReplace(Node* oldNode);

    /// Replaces the given \p oldNode with this Node. This destroys \p oldNode
    /// and all its descendants, except this Node and all its descendants. Does
    /// nothing if \p oldNode is this Node itself.
    ///
    /// An exception is raised in the following cases:
    ///
    /// 1. ReplaceDocumentError: oldNode is the Document node and is not this
    ///    Node itself.
    ///
    /// 2. WrongDocumentError: oldNode is owned by another Document
    ///
    /// 3. WrongChildTypeError: the type of this Node is not allowed as a child
    ///    of \p oldNode's parent. Here is the list of allowed child types:
    ///    - Document: allowed child types are Element (at most one)
    ///    - Element: allowed child types are Element
    ///
    /// 4. SecondRootElementError: this Node is an Element node, the parent of
    ///    \p oldNode is the Document node, and replacing would result in a
    ///    second root element.
    ///
    /// 5. ChildCycleError: \p oldNode is a (strict) descendant this Node
    ///
    /// If several exceptions apply, the one appearing first in the list above is
    /// raised.
    ///
    void replace(Node* oldNode);

    /// Returns whether this node is a descendant of the given \p other node.
    /// Returns true if this node is equal to the \p other node.
    ///
    bool isDescendant(const Node* other) const {
        return isDescendantObject(other);
    }

    /// Returns the `Element` that the given `path` refers to.
    ///
    /// If the path refers to an attribute, this returns the element that owns
    /// the attribute.
    /// If the path is empty, invalid, or does refer to an element that does not
    /// exist, this returns `nullptr`.
    ///
    Element* elementFromPath(const Path& path) const;

    /// Returns the `Value` of the attribute that the given `path` refers to.
    ///
    /// If the path is empty, invalid, does not refer to an attribute, or one of
    /// its segment cannot not be resolved, this returns `nullptr`.
    ///
    // XXX Later, consider returning a ValuePtr or ValueRef.
    Value valueFromPath(const Path& path) const;

private:
    // Operations
    friend class RemoveNodeOperation;
    friend class MoveNodeOperation;

    Document* document_;
    NodeType nodeType_;

    friend void detail::destroyNode(Node* node);
};

} // namespace vgc::dom

#endif // VGC_DOM_NODE_H
