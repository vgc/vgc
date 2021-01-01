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

#include <vgc/core/object.h>
#include <vgc/core/format.h>
#include <vgc/dom/api.h>
#include <vgc/dom/exceptions.h>

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

/// Writes the given NodeType to the output stream.
///
template<typename OStream>
void write(OStream& out, NodeType type)
{
    switch (type) {
    case NodeType::Element: write(out, "Element"); break;
    case NodeType::Document: write(out, "Document"); break;
    }
}

VGC_DECLARE_OBJECT(Node);

/// \class vgc::dom::NodeIterator
/// \brief Iterates over a range of Node siblings.
///
/// This iterator class is a thin wrapper around Node*, where `operator++` is
/// implemented as `node->nextSibling()`. Together with NodeList, this
/// class enables range-based loops like the following:
///
/// \code
/// for (Node* child : node->children()) {
///     // ...
/// }
/// \endcode
///
/// Typically, you never have to manipulate NodeIterators directly, but
/// keep them as an implementation detail for range-based loops. If you need
/// more fine-grain traversal, we recommend to directly use `Node*` as your
/// "iterator type", and use the methods Node::previousSibling() and
/// Node::nextSibling() to access neighbor nodes.
///
/// \sa NodeList and Node.
///
/// Past-last-child Iterator
/// ------------------------
///
/// There is a special sentinel value which we call the "past-last-child"
/// iterator, which is constructed by passing nullptr to the constructor of
/// NodeIterator.
///
/// Be aware that all node->children() share the same past-last-child iterator.
/// In other words:
///
///     node1->children().end() == node2->children().end()
///
/// Intuitively, this is because this past-last-child iterator is simply
/// implemented by storing a nullptr, that is, the iterator does not know what
/// is its "parent". This allows for a more intuitive API, stronger guarantees
/// on iterator validity (see next section), and slightly more efficient
/// implementation.
///
/// However, we note that this design choice is why our NodeIterator is not
/// bidirectional: we wouldn't be able to decrement the past-last-child
/// iterator. We decided that this was a minor concern since in the rare cases
/// where you'd need bidirectionality, you can simply use the Node API
/// directly.
///
/// Iterator Validity
/// -----------------
///
/// NodeIterators are always valid as long as the underlying Node* stays
/// valid. If the iterator is a past-last-child iterator, then it is always
/// valid.
///
/// In particular, deleting or inserting siblings of a given iterator keeps
/// this iterator valid. Reparenting or moving the location of a Node among its
/// sibling keeps the iterator valid. However, be careful to keep the end of a
/// range reachable! This is never a problem if the end of the range is the
/// past-last-child iterator, but can be in other cases.
///
/// Despite all of these guarantees, we recommend that whenever you're doing
/// anything else than a simple iteration over all children, you should simply
/// use the Node API (node->nextSibling(), etc.) instead of iterators. This is
/// even safer and more readable anyway.
///
class VGC_DOM_API NodeIterator {
public:
    typedef Node* value_type;
    typedef Node*& reference;
    typedef Node** pointer;
    typedef std::input_iterator_tag iterator_category;

    /// Constructs an iterator pointing to the given Node.
    ///
    NodeIterator(Node* node) : node_(node)
    {

    }

    /// Prefix-increments this iterator.
    ///
    NodeIterator& operator++();

    /// Postfix-increments this iterator.
    ///
    NodeIterator operator++(int);

    /// Dereferences this iterator with the star operator.
    ///
    const value_type& operator*() const
    {
        return node_;
    }

    /// Dereferences this iterator with the arrow operator.
    ///
    const value_type* operator->() const
    {
        return &node_;
    }

    /// Returns whether the two iterators are equal.
    ///
    friend bool operator==(const NodeIterator&, const NodeIterator&);

    /// Returns whether the two iterators are different.
    ///
    friend bool operator!=(const NodeIterator&, const NodeIterator&);

private:
    Node* node_;
};

/// \class vgc::dom::NodeList
/// \brief Enable range-based loops for sibling Nodes.
///
/// This range class is used together with NodeIterator to
/// enable range-based loops like the following:
///
/// \code
/// for (Node* child : node->children()) {
///     // ...
/// }
/// \endcode
///
/// \sa NodeIterator and Node.
///
class VGC_DOM_API NodeList {
public:
    /// Constructs a range of sibling nodes from \p begin to \p end. The
    /// range includes \p begin but excludes \p end. The behavior is undefined
    /// if \p begin and \p end do not have the same parent. If \p end is null,
    /// then the range includes all siblings after \p begin. If both \p start
    /// and \p end are null, then the range is empty. The behavior is undefined
    /// if \p begin is null but \p end is not.
    ///
    NodeList(Node* begin, Node* end) : begin_(begin), end_(end)
    {

    }

    /// Returns the begin of the range.
    ///
    const NodeIterator& begin() const
    {
        return begin_;
    }

    /// Returns the end of the range.
    ///
    const NodeIterator& end() const
    {
        return end_;
    }

private:
    NodeIterator begin_;
    NodeIterator end_;
};

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
    VGC_OBJECT(Node)

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

public:
    /// Returns the owner Document of this Node. This is always guaranteed to
    /// be a non-null valid Document.
    ///
    Document* document() const
    {
        return document_;
    }

    /// Returns the NodeType of this Node.
    ///
    /// This function is safe to call even when the node is not alive.
    ///
    NodeType nodeType() const
    {
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
    static Node* cast(Node* node)
    {
        return node;
    }

    /// Destroys this Node.
    ///
    /// \sa vgc::core::Object::isAlive().
    ///
    // XXX Should we forbid to destroy the Document?
    //
    void destroy()
    {
        destroyObject_();
    }

    /// Returns the parent Node of this Node. This is always nullptr for
    /// Document nodes, and always a non-null valid Node otherwise.
    ///
    /// \sa firstChild(), lastChild(), previousSibling(), and nextSibling().
    ///
    Node* parent() const
    {
        return static_cast<Node*>(parentObject());
    }

    /// Returns the first child Node of this Node. Returns nullptr if this Node
    /// has no children.
    ///
    /// \sa lastChild(), previousSibling(), nextSibling(), and parent().
    ///
    Node* firstChild() const
    {
        return static_cast<Node*>(firstChildObject());
    }

    /// Returns the last child Node of this Node. Returns nullptr if this Node
    /// has no children.
    ///
    /// \sa firstChild(), previousSibling(), nextSibling(), and parent().
    ///
    Node* lastChild() const
    {
        return static_cast<Node*>(lastChildObject());
    }

    /// Returns the previous sibling of this Node. Returns nullptr if this Node
    /// is a Document node, or if it is the first child of its parent.
    ///
    /// \sa nextSibling(), parent(), firstChild(), and lastChild().
    ///
    Node* previousSibling() const
    {
        return static_cast<Node*>(previousSiblingObject());
    }

    /// Returns the next sibling of this Node. Returns nullptr if this Node
    /// is a Document node, or if it is the last child of its parent.
    ///
    /// \sa previousSibling(), parent(), firstChild(), and lastChild().
    ///
    Node* nextSibling() const
    {
        return static_cast<Node*>(nextSiblingObject());
    }

    /// Returns all children of this Node as an iterable range.
    ///
    /// Example:
    ///
    /// \code
    /// for (Node* child : node->children()) {
    ///     // ...
    /// }
    /// \endcode
    ///
    NodeList children() const
    {
        return NodeList(firstChild(), nullptr);
    }

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
    bool isDescendant(const Node* other) const
    {
        return isDescendantObject(other);
    }

private:
    Document* document_;
    NodeType nodeType_;
};

inline NodeIterator& NodeIterator::operator++() {
    node_ = node_->nextSibling();
    return *this;
}

inline NodeIterator NodeIterator::operator++(int) {
    NodeIterator res(*this);
    operator++();
    return res;
}

inline bool operator==(const NodeIterator& it1, const NodeIterator& it2) {
    return it1.node_ == it2.node_;
}

inline bool operator!=(const NodeIterator& it1, const NodeIterator& it2) {
    return !(it1 == it2);
}

} // namespace dom
} // namespace vgc

#endif // VGC_DOM_NODE_H
