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

#ifndef VGC_DOM_ELEMENT_H
#define VGC_DOM_ELEMENT_H

#include <vgc/core/id.h>
#include <vgc/core/stringid.h>
#include <vgc/dom/api.h>
#include <vgc/dom/attribute.h>
#include <vgc/dom/document.h>
#include <vgc/dom/node.h>
#include <vgc/dom/path.h>
#include <vgc/dom/strings.h>
#include <vgc/dom/value.h>

namespace vgc::dom {

VGC_DECLARE_OBJECT(Document);
VGC_DECLARE_OBJECT(Element);

/// \class vgc::dom::NamedElementIterator
/// \brief Iterates over sibling elements with a given tag name.
///
class VGC_DOM_API NamedElementIterator {
public:
    typedef Element* value_type;
    typedef Int difference_type;
    typedef const value_type& reference; // 'const' => non-mutable forward iterator
    typedef const value_type* pointer;   // 'const' => non-mutable forward iterator
    typedef std::forward_iterator_tag iterator_category;

    /// Constructs an invalid `ElementIterator`.
    ///
    NamedElementIterator()
        : p_(nullptr) {
    }

    /// Constructs `NamedElementIterator` starting at the given element, and
    /// iterating over its siblings with the given `tagName`.
    ///
    explicit NamedElementIterator(Element* element, core::StringId tagName)
        : p_(element)
        , tagName_(tagName) {
    }

    /// Prefix-increments this iterator.
    ///
    NamedElementIterator& operator++();

    /// Postfix-increments this iterator.
    ///
    NamedElementIterator operator++(int) {
        NamedElementIterator res(*this);
        operator++();
        return res;
    }

    /// Dereferences this iterator with the star operator.
    ///
    const value_type& operator*() const {
        return p_;
    }

    /// Dereferences this iterator with the arrow operator.
    ///
    const value_type* operator->() const {
        return &p_;
    }

    /// Returns whether the two iterators are equal.
    ///
    bool operator==(const NamedElementIterator& other) const {
        return p_ == other.p_;
    }

    /// Returns whether the two iterators are different.
    ///
    bool operator!=(const NamedElementIterator& other) const {
        return p_ != other.p_;
    }

private:
    Element* p_;
    core::StringId tagName_;
};

/// \class vgc::dom::NamedElementRange
/// \brief A range of sibling elements with a given tag name.
///
class VGC_DOM_API NamedElementRange {
public:
    /// Constructs a `NamedElementRange` iterating forward over elements
    /// between `begin` (included) and `end` (excluded) with the given `tagName`.
    ///
    NamedElementRange(Element* begin, Element* end, core::StringId tagName)
        : begin_(begin, tagName)
        , end_(end, tagName) {
    }

    /// Returns the begin of the range.
    ///
    const NamedElementIterator& begin() const {
        return begin_;
    }

    /// Returns the end of the range.
    ///
    const NamedElementIterator& end() const {
        return end_;
    }

    /// Returns the number of objects in the range.
    ///
    /// Note that this function is slow (linear complexity), because it has to
    /// iterate over all objects in the range.
    ///
    Int length() const {
        return std::distance(begin_, end_);
    }

private:
    NamedElementIterator begin_;
    NamedElementIterator end_;
};

/// \class vgc::dom::Element
/// \brief Represents an element of the DOM.
///
class VGC_DOM_API Element : public Node {
private:
    VGC_OBJECT(Element, Node)

protected:
    struct PrivateKey {};

    /// Constructs a parent-less `Element` with the given `tagName`, owned by the
    /// given `document`. This constructor is an implementation detail only
    /// available to derived classes. In order to create an `Element`, please use
    /// the following:
    ///
    /// ```cpp
    /// ElementPtr element = Element::create(parent, tagName);
    /// ```
    ///
    Element(CreateKey, PrivateKey, Document* document, core::StringId tagName);

    void onDestroyed() override;

public:
    /// Creates an `Element` with the given `tagName` as the root element of
    /// the given `parent` `Document`. Returns a valid non-null `Element`.
    ///
    /// A `SecondRootElementError` exception is raised if the given `parent`
    /// `Document` already has a root element.
    ///
    static Element* create(Document* parent, core::StringId tagName);
    /// \overload
    static Element* create(Document* parent, std::string_view tagName) {
        return create(parent, core::StringId(tagName));
    }

    /// Creates an `Element` with the given `tagName` as a child of the
    /// given `parent` `Element` before `nextSibling` if it is non-null.
    /// Returns a valid non-null `Element`.
    ///
    static Element*
    create(Element* parent, core::StringId tagName, Element* nextSibling = nullptr);
    /// \overload
    static Element*
    create(Element* parent, std::string_view tagName, Element* nextSibling = nullptr) {
        return create(parent, core::StringId(tagName), nextSibling);
    }

    /// Creates a copy of the given `source` `Element` as the root element of
    /// the given `parent` `Document`. Returns a valid non-null `Element`.
    ///
    /// A `SecondRootElementError` exception is raised if the given `parent`
    /// `Document` already has a root element.
    ///
    static Element* createCopy(Document* parent, const Element* source);

    /// Creates a copy of the given `source` `Element` as a child of the
    /// given `parent` `Element` before `nextSibling` if it is non-null.
    /// Returns a valid non-null `Element`.
    ///
    static Element*
    createCopy(Element* parent, const Element* source, Element* nextSibling = nullptr);

    /// Casts the given `node` to an `Element`. Returns `nullptr` if `node` is
    /// `nullptr` or if `node->nodeType() != NodeType::Element`.
    ///
    /// This is functionaly equivalent to `dynamic_cast<Element*>`, while being
    /// as fast as `static_cast<Element*>`. Therefore, always prefer using this
    /// method over `static_cast<Element*>` or `dynamic_cast<Element*>`.
    ///
    static Element* cast(Node* node) {
        return (node && node->nodeType() == NodeType::Element)
                   ? static_cast<Element*>(node)
                   : nullptr;
    }

    /// Returns the tag name of the element. This is equivalent to tagName()
    /// in the W3C DOM Specification.
    ///
    /// This function is safe to call even when the node is not alive.
    ///
    /// XXX Should this be elementType() instead? or typeName()? or simply
    /// type() ? We've chosen name() for now for consistency with W3C DOM, but
    /// I personally don't like this terminology. For example, it makes more
    /// sense to say "let's define a new element type" than "let's define a new
    /// element name". I'd rather use "name" as being a built-in attribute name
    /// of all Elements, similar to "id" but without requiring them to be
    /// unique as long as they have different parent, like filenames, or
    /// Pixar's USD prim names.
    ///
    core::StringId tagName() const {
        return tagName_;
    }

    /// Returns the custom name of this element.
    ///
    core::StringId name() const {
        return name_;
    }

    /// Sets the custom name of this element.
    ///
    void setName(core::StringId name) {
        setAttribute(strings::name, name);
    }

    /// Sets the custom name of this element.
    ///
    void setName(std::string_view name) {
        setName(core::StringId(name));
    }

    /// Returns the unique identifier of this element if it has one.
    /// It is only unique document-wise and is not guaranteed to
    /// remain the same when transferring an element to another
    /// document.
    ///
    core::StringId id() const {
        return id_;
    }

    core::Id internalId() const {
        return internalId_;
    }

    /// Returns or creates the unique identifier of this element.
    /// It is only unique document-wise and is not guaranteed to
    /// remain the same when transferring an element to another
    /// document.
    ///
    core::StringId getOrCreateId() const;

    Path getPathFromId() const {
        return Path::fromId(getOrCreateId());
    }

    /// Returns the authored attributes of this element.
    ///
    const core::Array<AuthoredAttribute>& authoredAttributes() const {
        return authoredAttributes_;
    }

    /// Gets the authored value of the attribute named `name`.
    /// Returns an invalid value if the attribute does not exist.
    ///
    const Value& getAuthoredAttribute(core::StringId name) const;

    /// Gets the value of the attribute named `name`. Emits a warning and returns an
    /// invalid value if the attribute neither is authored nor has a default value.
    ///
    const Value& getAttribute(core::StringId name) const;

    /// Gets the element referred to by the path attribute named `name`.
    ///
    /// If the path cannot be resolved, this emits a warning and returns `nullptr`.
    ///
    /// If `tagNameFilter` is not empty and does not compare equal to the found
    /// element tag name, this emits a warning and returns `nullptr`.
    ///
    /// Returns `std::nullopt` if the attribute is optional and not set.
    ///
    std::optional<Element*> getElementFromPathAttribute(
        core::StringId name,
        core::StringId tagNameFilter = {}) const;

    /// Sets the value of the given attribute.
    ///
    void setAttribute(core::StringId name, const Value& value);
    /// \overload
    void setAttribute(core::StringId name, Value&& value);

    /// Clears the authored value of the given attribute.
    ///
    void clearAttribute(core::StringId name);

    /// Returns the parent `Element` of this `Element`.
    /// Returns nullptr if the parent of this `Element` is not an `Element`.
    ///
    /// \sa lastChildElement(), previousSiblingElement(), and nextSiblingElement().
    ///
    Element* parentElement() const {
        Node* const p = parent();
        return (p && p->nodeType() == NodeType::Element) ? static_cast<Element*>(p)
                                                         : nullptr;
    }

    /// Returns the first child `Element` of this `Element`.
    /// Returns nullptr if this `Element` has no child `Element`.
    ///
    /// \sa lastChildElement(), previousSiblingElement(), and nextSiblingElement().
    ///
    Element* firstChildElement() const {
        return findElementNext_(firstChild());
    }

    /// Returns the first child `Element` of this `Element` that has the given
    /// `tagName`. Returns nullptr if this `Element` has no child `Element` with
    /// the given `tagName`.
    ///
    /// \sa lastChildElement(), previousSiblingElement(), and nextSiblingElement().
    ///
    Element* firstChildElement(core::StringId tagName) const {
        return findElementNext_(firstChild(), tagName);
    }

    /// Returns the last child `Element` of this `Element`.
    /// Returns nullptr if this `Element` has no child `Element`.
    ///
    /// \sa firstChildElement(), previousSiblingElement(), and nextSiblingElement().
    ///
    Element* lastChildElement() const {
        return findElementPrevious_(lastChild());
    }

    /// Returns the last child `Element` of this `Element` that has the given
    /// `tagName`. Returns nullptr if this `Element` has no child `Element` with
    /// the given `tagName`.
    ///
    /// \sa lastChildElement(), previousSiblingElement(), and nextSiblingElement().
    ///
    Element* lastChildElement(core::StringId tagName) const {
        return findElementPrevious_(lastChild(), tagName);
    }

    /// Returns the previous sibling of this `Element`.
    /// Returns nullptr if this `Element` is the first child `Element` of its parent.
    ///
    /// \sa nextSiblingElement(), firstChildElement(), and lastChildElement().
    ///
    Element* previousSiblingElement() const {
        return findElementPrevious_(previousSibling());
    }

    /// Returns the previous sibling of this `Element` that has the given
    /// `tagName`. Returns nullptr if this `Element` is the first child `Element`
    /// of its parent with the given `tagName`.
    ///
    /// \sa nextSiblingElement(), firstChildElement(), and lastChildElement().
    ///
    Element* previousSiblingElement(core::StringId tagName) const {
        return findElementPrevious_(previousSibling(), tagName);
    }

    /// Returns the next sibling of this `Element`.
    /// Returns nullptr if this `Element` is the last child `Element` of its parent.
    ///
    /// \sa previousSiblingElement(), firstChildElement(), and lastChildElement().
    ///
    Element* nextSiblingElement() const {
        return findElementNext_(nextSibling());
    }

    /// Returns the next sibling of this `Element` that has the given `tagName`.
    /// Returns nullptr if this `Element` is the last child `Element` of its
    /// parent with the given `tagName`.
    ///
    /// \sa previousSiblingElement(), firstChildElement(), and lastChildElement().
    ///
    Element* nextSiblingElement(core::StringId tagName) const {
        return findElementNext_(nextSibling(), tagName);
    }

    /// Iterates over all child elements with the given `tagName`.
    ///
    NamedElementRange childElements(core::StringId tagName) {
        return NamedElementRange(firstChildElement(tagName), nullptr, tagName);
    }

    VGC_SIGNAL(
        attributeChanged,
        (core::StringId, name),
        (const Value&, oldValue),
        (const Value&, newValue))

private:
    // Operations
    friend class CreateElementOperation;
    friend class SetAttributeOperation;
    friend class RemoveAuthoredAttributeOperation;

    // Tag name of this element.
    core::StringId tagName_;

    // Name of this element. (cache)
    core::StringId name_;

    // Unique identifier of this element. (cache)
    core::StringId id_;

    // Unique internal Id of this element.
    core::Id internalId_;

    // Helper methods for create().
    // Assumes that a new Element can indeed be appended to parent.
    //
    static Element* create_(Node* parent, core::StringId tagName, Element* nextSibling);

    static Element*
    createCopy_(Node* parent, const Element* source, Element* nextSibling);

    friend Document;

    static Element* createCopy_(
        Node* parent,
        const Element* source,
        Element* nextSibling,
        PathUpdateData& pud);

    static Element* createCopyRec_(
        Node* parent,
        const Element* source,
        Element* nextSibling,
        PathUpdateData& pud);

    // Authored attributes of this element. Note: copying AuthoredAttribute
    // instances is expensive, but fortunately there shouldn't be any copy with
    // the implementation below, even when the vector grows. Indeed, on modern
    // compilers, move semantics should be used instead, since
    // AuthoredAttribute has a non-throwing move constructor and destructor.
    //
    core::Array<AuthoredAttribute> authoredAttributes_;

    // Helper functions to find attributes. Return nullptr if not found.
    AuthoredAttribute* findAuthoredAttribute_(core::StringId name);
    const AuthoredAttribute* findAuthoredAttribute_(core::StringId name) const;

    Element* findElementNext_(Node* node) const {
        while (node && node->nodeType() != NodeType::Element) {
            node = node->nextSibling();
        }
        return static_cast<Element*>(node);
    }

    Element* findElementPrevious_(Node* node) const {
        while (node && node->nodeType() != NodeType::Element) {
            node = node->previousSibling();
        }
        return static_cast<Element*>(node);
    }

    Element* findElementNext_(Node* node, core::StringId tagName) const {
        while (node
               && (node->nodeType() != NodeType::Element
                   || static_cast<Element*>(node)->tagName() != tagName)) {
            node = node->nextSibling();
        }
        return static_cast<Element*>(node);
    }

    Element* findElementPrevious_(Node* node, core::StringId tagName) const {
        while (node
               && (node->nodeType() != NodeType::Element
                   || static_cast<Element*>(node)->tagName() != tagName)) {
            node = node->previousSibling();
        }
        return static_cast<Element*>(node);
    }

    void onAttributeChanged_(
        core::StringId name,
        const Value& oldValue,
        const Value& newValue);

    friend void detail::prepareInternalPathsForUpdate(const Node* workingNode);
    void prepareInternalPathsForUpdate_() const;

    friend void
    detail::updateInternalPaths(const Node* workingNode, const PathUpdateData& data);
    void updateInternalPaths_(const PathUpdateData& data);
};

inline NamedElementIterator& NamedElementIterator::operator++() {
    p_ = p_->nextSiblingElement(tagName_);
    return *this;
}

/// Defines the tag name of an element, retrievable via the
/// VGC_DOM_ELEMENT_GET_TAGNAME(key) macro. This must only be used in .cpp files
/// where subclasses of Element are defined. Never use this in header files.
/// Also, the corresponding VGC_DOM_ELEMENT_GET_TAGNAME(key) can only be used in
/// the same .cpp file where the tag name has been defined.
///
/// Example:
///
/// \code
/// VGC_DOM_ELEMENT_DEFINE_NAME(foo, "foo")
///
/// Foo::Foo() : Element(VGC_DOM_ELEMENT_GET_NAME(foo)) { }
/// \endcode
///
#define VGC_DOM_ELEMENT_DEFINE_TAGNAME(key, tagName)                                     \
    static vgc::core::StringId VGC_DOM_ELEMENT_TAGNAME_##key##_() {                      \
        static vgc::core::StringId s(tagName);                                           \
        return s;                                                                        \
    }

/// Retrieves the element tag name defined via
/// VGC_DOM_ELEMENT_DEFINE_TAGNAME(key, tagName)
///
#define VGC_DOM_ELEMENT_GET_TAGNAME(key) VGC_DOM_ELEMENT_TAGNAME_##key##_()

} // namespace vgc::dom

#endif // VGC_DOM_ELEMENT_H
