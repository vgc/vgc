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

#ifndef VGC_DOM_NODE_H
#define VGC_DOM_NODE_H

#include <vgc/core/object.h>
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

/// Converts the given NodeType to a string.
///
VGC_CORE_API
inline std::string toString(NodeType type) {
    switch (type) {
    case NodeType::Element: return "Element";
    case NodeType::Document: return "Document";
    }
}

VGC_CORE_DECLARE_PTRS(Node);

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
class VGC_DOM_API NodeIterator
{
public:
    typedef Node* value_type;
    typedef Node*& reference;
    typedef Node** pointer;
    typedef std::input_iterator_tag iterator_category;

    /// Constructs an iterator pointing to the given Node.
    ///
    NodeIterator(Node* node) : node_(node) {}

    /// Prefix-increments this iterator.
    ///
    NodeIterator& operator++();

    /// Postfix-increments this iterator.
    ///
    NodeIterator operator++(int);

    /// Dereferences this iterator with the star operator.
    ///
    const value_type& operator*() const { return node_; }

    /// Dereferences this iterator with the arrow operator.
    ///
    const value_type* operator->() const { return &node_; }

    /// Returns whether the two iterators are equals.
    ///
    friend bool operator==(const NodeIterator&, const NodeIterator&);

    /// Returns whether the two iterators are differents.
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
class VGC_DOM_API NodeList
{
public:
    /// Constructs a range of sibling nodes from \p begin to \p end. The
    /// range includes \p begin but excludes \p end. The behavior is undefined
    /// if \p begin and \p end do not have the same parent. If \p end is null,
    /// then the range includes all siblings after \p begin. If both \p start
    /// and \p end are null, then the range is empty. The behavior is undefined
    /// if \p begin is null but \p end is not.
    ///
    NodeList(Node* begin, Node* end) : begin_(begin), end_(end) {}

    /// Returns the begin of the range.
    ///
    const NodeIterator& begin() const { return begin_; }

    /// Returns the end of the range.
    ///
    const NodeIterator& end() const { return end_; }

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
class VGC_DOM_API Node: public core::Object
{
    VGC_CORE_OBJECT(Node)

protected:
    /// Constructs a parent-less Node of the given \p nodeType, owned by the
    /// given \p document. This constructor is an implementation detail only
    /// available to derived classes. In order to create nodes, please use one
    /// of the following:
    ///
    /// \code
    /// DocumentSharedPtr document = Document::create();
    /// ElementSharedPtr element = Element::create(parent, name);
    /// \endcode
    ///
    Node(Document* document, NodeType nodeType);

public:
    /// Destructs the Node. Never call this manually, and instead let the
    /// shared pointers do the work for you.
    ///
    /// This ought to be a protected method to avoid accidental misuse, but it
    /// is currently kept public due to a limitation of pybind11 (see
    /// https://github.com/pybind/pybind11/issues/114), and possibly other
    /// related issues.
    ///
    virtual ~Node();

    /// Returns the owner Document of this Node. This is always guaranteed to
    /// be a non-null valid Document.
    ///
    Document* document() const {
        checkAlive();
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

    /// Returns whether this Node is "alive".
    ///
    /// More precisely, the lifetime of a Node goes through three stages:
    ///
    /// 1. This node is created via Document::create() or
    ///    Element::create(parent, name): the node is alive, and Node::isAlive()
    ///    returns true.
    ///
    /// 2. Node::destroy() has been called: the node is not alive anymore, but
    ///    all pointers to the node are still valid non-dangling pointers.
    ///    Dereferencing the pointer is safe. Calling node->isAlive() is safe
    ///    and returns false. But calling most other member methods will cause
    ///    NotAliveError to be thrown.
    ///
    /// 3. Node::~Node() has been called: all raw pointers to the node are now
    ///    dangling pointers, and all weak pointers to the node are now expired.
    ///
    /// Quite obviously, this function is safe to call even when the node is
    /// not alive.
    ///
    /// \sa destroy().
    ///
    bool isAlive() const noexcept {
        // Note: don't be tempted to use document() here instead of document_,
        // since the former throws when the node is not alive.
        return document_ != nullptr;
    }

    /// Throws a NotAliveError if this Node is not alive. Does nothing
    /// otherwise.
    ///
    void checkAlive() const {
        if (!isAlive()) {
            throw NotAliveError(this);
        }
    }

    /// Destroys this Node.
    ///
    /// \sa isAlive().
    ///
    void destroy();

    /// Returns the parent Node of this Node. This is always nullptr for
    /// Document nodes, and always a non-null valid Node otherwise.
    ///
    /// \sa firstChild(), lastChild(), previousSibling(), and nextSibling().
    ///
    Node* parent() const {
        checkAlive();
        return parent_;
    }

    /// Returns the first child Node of this Node. Returns nullptr if this Node
    /// has no children.
    ///
    /// \sa lastChild(), previousSibling(), nextSibling(), and parent().
    ///
    Node* firstChild() const {
        checkAlive();
        return firstChild_.get();
    }

    /// Returns the last child Node of this Node. Returns nullptr if this Node
    /// has no children.
    ///
    /// \sa firstChild(), previousSibling(), nextSibling(), and parent().
    ///
    Node* lastChild() const {
        checkAlive();
        return lastChild_;
    }

    /// Returns the previous sibling of this Node. Returns nullptr if this Node
    /// is a Document node, or if it is the first child of its parent.
    ///
    /// \sa nextSibling(), parent(), firstChild(), and lastChild().
    ///
    Node* previousSibling() const {
        checkAlive();
        return previousSibling_;
    }

    /// Returns the next sibling of this Node. Returns nullptr if this Node
    /// is a Document node, or if it is the last child of its parent.
    ///
    /// \sa previousSibling(), parent(), firstChild(), and lastChild().
    ///
    Node* nextSibling() const {
        checkAlive();
        return nextSibling_.get();
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
    NodeList children() const {
        checkAlive();
        return NodeList(firstChild(), nullptr);
    }

    /// Returns whether the given \p node can be inserted as the last child of
    /// this Node. See appendChild() for details.
    ///
    bool canAppendChild(Node* node);

    /// Appends the given \p node as the last child of this Node. The \p node
    /// is first removed from the children of its current parent. If this Node
    /// is already the parent of \p node, then \p node is moved as the last
    /// child of this Node.
    ///
    /// An exception is raised in the following cases:
    ///
    /// 1. WrongDocumentError: You're trying to append a Node which is owned by
    ///    another Document.
    ///
    /// 2. WrongChildTypeError: You're trying to append a Node whose type is
    ///    not compatible with its parent type.
    ///
    /// 3. SecondRootElementError: You're trying to append a second child
    ///    Element node to a Document node.
    ///
    /// 4. ChildCycleError: You're trying to append a Node as a child of itself
    ///    or one of its descendants.
    ///
    /// If several exceptions apply, the one appearing first in the list above is
    /// raised.
    ///
    void appendChild(Node* node);

    /// Replaces the child node \p oldChild with \p newChild. Does nothing if
    /// newChild == oldChild.
    ///
    /// The operation is not performed and a warning is raised in all of the
    /// following cases:
    ///
    /// 1. oldChild is not a child of this Node
    ///
    /// 2. newChild is owned by another Document
    ///
    /// 3. newChild is a Document node
    ///
    /// 4. this Node is a Document node and replacing oldChild with newChild
    ///    would add a second root element.
    ///
    /// XXX Throw an exception instead?
    ///
    /// Returns whether the child was successfully replaced.
    ///
    bool replaceChild(Node* newChild, Node* oldChild);

    /// Returns whether this node is a descendant of the given \p other node.
    /// Returns true if this node is equal to the \p other node.
    ///
    bool isDescendant(const Node* other) const;

protected:
    // Helper method for Derived::create_. Assumes the node is alive, is fully
    // constructed, has no parent, and can indeed be appended.
    //
    template <typename T>
    static T* init_(std::shared_ptr<T> node, Node* parent) {
        Node* res = parent->appendChild_(std::move(node));
        return T::cast(res);
    }

private:
    // Owner document (also used as an 'isAlive_' flag)
    Document* document_;

    // Node type
    NodeType nodeType_;

    // parent-child relationship
    Node* parent_;
    NodeSharedPtr firstChild_;
    Node* lastChild_;
    Node* previousSibling_;
    NodeSharedPtr nextSibling_;

    // Helper method for appendChild() and init_(). Assumes that the node is
    // alive, is fully constructed, has no parent, and can indeed be appended.
    //
    Node* appendChild_(NodeSharedPtr node);

    // Removes this Node from its parent's children, but without destroying it,
    // and returns the now parent-less Node as a shared pointer. Returns a null
    // pointer if the node had no parent already.
    //
    NodeSharedPtr detachFromParent_();

    // Helper method for ~Node and destroy()
    void destroy_();
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
