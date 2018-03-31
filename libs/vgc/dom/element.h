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

#ifndef VGC_DOM_ELEMENT_H
#define VGC_DOM_ELEMENT_H

#include <vgc/core/stringid.h>
#include <vgc/dom/api.h>
#include <vgc/dom/attribute.h>
#include <vgc/dom/node.h>
#include <vgc/dom/value.h>

namespace vgc {
namespace dom {

VGC_CORE_DECLARE_PTRS(Element);

/// \class vgc::dom::Element
/// \brief Represents an element of the DOM.
///
class VGC_DOM_API Element: public Node
{
public:
    VGC_CORE_OBJECT(Element)

    /// Creates a new Element with the given \p name.
    ///
    Element(core::StringId name);

    /// Casts the given \p node to an Element. Returns nullptr if node is
    /// nullptr or if node->nodeType() != NodeType::Element.
    ///
    /// This is functionaly equivalent to dynamic_cast<Element*>, while being
    /// as fast as static_cast<Element*>. Therefore, always prefer using this
    /// method over static_cast<Element*> or dynamic_cast<Element*>.
    ///
    static Element* cast(Node* node) {
        return (node && node->nodeType() == NodeType::Element) ?
               static_cast<Element*>(node) :
               nullptr;
    }

    /// Returns the name of the element. This is equivalent to tagName()
    /// in the W3C DOM Specification.
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

    /// Returns all built-in attributes of this element. Note that this
    /// function does not let you access the current values of these
    /// attributes, only their names and their default value.
    ///
    virtual const std::vector<BuiltInAttribute>& builtInAttributes() const;

    /// Returns the attribute given by its \p name.
    ///
    Attribute attr(core::StringId name);

private:
    friend class Attribute;

    core::StringId name_;
    std::vector<detail::AuthoredAttributeSharedPtr> authoredAttributes_;
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
#define VGC_DOM_ELEMENT_DEFINE_NAME(key, name)                   \
    static vgc::core::StringId VGC_DOM_ELEMENT_NAME_##key##_() { \
        static vgc::core::StringId s(name);                      \
        return s;                                                \
    }

/// Retrieves the element name defined via
/// VGC_DOM_ELEMENT_DEFINE_NAME(key, name)
///
#define VGC_DOM_ELEMENT_GET_NAME(key) VGC_DOM_ELEMENT_NAME_##key##_()

} // namespace dom
} // namespace vgc

#endif // VGC_DOM_ELEMENT_H
