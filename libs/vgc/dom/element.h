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

#include <vgc/core/stringid.h>
#include <vgc/dom/api.h>
#include <vgc/dom/attribute.h>
#include <vgc/dom/node.h>
#include <vgc/dom/value.h>

namespace vgc::dom {

VGC_DECLARE_OBJECT(Document);
VGC_DECLARE_OBJECT(Element);

/// \class vgc::dom::Element
/// \brief Represents an element of the DOM.
///
class VGC_DOM_API Element : public Node {
private:
    VGC_OBJECT(Element, Node)

protected:
    /// Constructs a parent-less Element with the given \p name, owned by the
    /// given \p document. This constructor is an implementation detail only
    /// available to derived classes. In order to create an Element, please use
    /// the following:
    ///
    /// ```cpp
    /// ElementPtr element = Element::create(parent, name);
    /// ```
    ///
    Element(Document* document, core::StringId name);

public:
    /// Creates an element with the given \p name as the root element of the
    /// given \p parent Document. Returns a valid non-null Element.
    ///
    /// A SecondRootElementError exception is raised if the given \p parent
    /// Document already has a root element.
    ///
    static Element* create(Document* parent, core::StringId name);
    /// \overload
    static Element* create(Document* parent, const std::string& name) {
        return create(parent, core::StringId(name));
    }

    /// Creates an element with the given \p name as the last child of the
    /// given \p parent Element. Always returns a valid non-null Element.
    ///
    static Element* create(Element* parent, core::StringId name);
    /// \overload
    static Element* create(Element* parent, const std::string& name) {
        return create(parent, core::StringId(name));
    }

    /// Casts the given \p node to an Element. Returns nullptr if node is
    /// nullptr or if node->nodeType() != NodeType::Element.
    ///
    /// This is functionaly equivalent to dynamic_cast<Element*>, while being
    /// as fast as static_cast<Element*>. Therefore, always prefer using this
    /// method over static_cast<Element*> or dynamic_cast<Element*>.
    ///
    static Element* cast(Node* node) {
        return (node && node->nodeType() == NodeType::Element)
                   ? static_cast<Element*>(node)
                   : nullptr;
    }

    /// Returns the name of the element. This is equivalent to tagName()
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
    core::StringId name() const {
        return name_;
    }

    /// Returns the authored attributes of this element.
    ///
    const core::Array<AuthoredAttribute>& authoredAttributes() const {
        return authoredAttributes_;
    }

    /// Gets the value of the given attribute. Emits a warning and returns an
    /// invalid value if the attribute does not exist.
    ///
    const Value& getAttribute(core::StringId name) const;

    /// Sets the value of the given attribute.
    ///
    void setAttribute(core::StringId name, const Value& value);

    /// Clears the authored value of the given attribute.
    ///
    void clearAttribute(core::StringId name);

    /// Returns the first child `Element` of this `Element`.
    /// Returns nullptr if this `Element` has no child `Element`.
    ///
    /// \sa lastChildElement(), previousSiblingElement(), and nextSiblingElement().
    ///
    Element* firstChildElement() const {
        return static_cast<Element*>(firstChildObject());
    }

    /// Returns the last child `Element` of this `Element`.
    /// Returns nullptr if this `Element` has no child `Element`.
    ///
    /// \sa firstChildElement(), previousSiblingElement(), and nextSiblingElement().
    ///
    Element* lastChildElement() const {
        return static_cast<Element*>(lastChildObject());
    }

    /// Returns the previous sibling of this `Element`.
    /// Returns nullptr if this `Element` is the last child `Element` of its parent.
    ///
    /// \sa nextSiblingElement(), firstChildElement(), and lastChildElement().
    ///
    Element* previousSiblingElement() const {
        return static_cast<Element*>(previousSiblingObject());
    }

    /// Returns the next sibling of this `Element`.
    /// Returns nullptr if this `Element` is the last child `Element` of its parent.
    ///
    /// \sa previousSiblingElement(), firstChildElement(), and lastChildElement().
    ///
    Element* nextSiblingElement() const {
        return static_cast<Element*>(nextSiblingObject());
    }

private:
    // Operations
    friend class CreateElementOperation;
    friend class SetAttributeOperation;
    friend class RemoveAuthoredAttributeOperation;

    // Name of this element.
    core::StringId name_;

    // Helper method for create(). Assumes that a new Element can indeed be
    // appended to parent.
    //
    static Element* create_(Node* parent, core::StringId name);

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
};

/// Defines the name of an element, retrievable via the
/// VGC_DOM_ELEMENT_GET_NAME(key) macro. This must only be used in .cpp files
/// where subclasses of Element are defined. Never use this in header files.
/// Also, the corresponding VGC_DOM_ELEMENT_GET_NAME(key) can only be used in
/// the same .cpp file where the name has been defined.
///
/// Example:
///
/// \code
/// VGC_DOM_ELEMENT_DEFINE_NAME(foo, "foo")
///
/// Foo::Foo() : Element(VGC_DOM_ELEMENT_GET_NAME(foo)) { }
/// \endcode
///
#define VGC_DOM_ELEMENT_DEFINE_NAME(key, name)                                           \
    static vgc::core::StringId VGC_DOM_ELEMENT_NAME_##key##_() {                         \
        static vgc::core::StringId s(name);                                              \
        return s;                                                                        \
    }

/// Retrieves the element name defined via
/// VGC_DOM_ELEMENT_DEFINE_NAME(key, name)
///
#define VGC_DOM_ELEMENT_GET_NAME(key) VGC_DOM_ELEMENT_NAME_##key##_()

} // namespace vgc::dom

#endif // VGC_DOM_ELEMENT_H
