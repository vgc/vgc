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

#ifndef VGC_DOM_ATTRIBUTE_H
#define VGC_DOM_ATTRIBUTE_H

#include <vgc/core/object.h>
#include <vgc/core/stringid.h>
#include <vgc/dom/api.h>
#include <vgc/dom/value.h>

namespace vgc {
namespace dom {

class Element;

/// \class vgc::dom::BuiltInAttribute
/// \brief Holds the name and default value of a built-in attribute.
///
/// This tiny immutable class is nothing else but a pair consisting of a name()
/// and a defaultValue(), representing the specifications of a built-in
/// attribute. The ValueType of the built-in attribute can be retrieved via
/// valueType(), which is a short-hand for defaultValue().valueType().
///
/// This class is mostly relevant for programmers implementing new Element
/// types. If you are simply manipulating a Document by creating, modifying, or
/// deleting elements, you should rarely need to use this, apart maybe for
/// inspecting the list of all built-in attributes of a given Element, which
/// you can do via Element::builtInAttributes().
///
/// Below, we provide more technical info for implementers of new Element types.
///
/// We recall that each type of Element (e.g., "path") have built-in attributes
/// (e.g., "positions"). These built-in attributes are globally registered to
/// the type system as a list of BuiltInAttributes, allocated only once per
/// Element type (that is, not once per Element instance). We do this since the
/// number of built-in attributes of each element may be potentially large,
/// while in practice only very few, if any, are actually authored in the
/// document (that is, appear in the XML file). In other words, in a given
/// document, a large majority of built-in attributes keep their default value,
/// and it would be extremely wasteful to allocate memory for each built-in
/// attribute of each element instance.
///
/// Therefore, only authored attributes are actually allocated in memory on a
/// per-element basis, as an instance of an internal class
/// vgc::dom::detail::AuthoredAttribute, managed by their owner Element. This
/// internal object not exposed in the public API is the one actually holding
/// the value of an authored attribute. The public class Attribute does not
/// actually holds the attribute value. Instead, it holds a pointer to its
/// owner Element, a pointer to its corresponding global BuiltInAttribute
/// (unless it's a custom attribute), and if authored, a pointer to the
/// corresponding AuthoredAttribute. Modifying the attribute value via
/// Attribute's public API automatically takes care of creating a new
/// AuthoredAttribute instance when the attribute is authored for the first
/// time.
///
class VGC_DOM_API BuiltInAttribute {
public:
    /// Creates a built-in attribute.
    ///
    BuiltInAttribute(core::StringId name, const Value& defaultValue) :
        name_(name),
        defaultValue_(defaultValue) {

    }

    /// Returns the name of this built-in attribute.
    ///
    core::StringId name() const {
        return name_;
    }

    /// Returns the default value of this built-in attribute.
    ///
    const Value& defaultValue() const {
        return defaultValue_;
    }

    /// Returns the ValueType of this built-in attribute.
    ///
    ValueType valueType() const {
        return defaultValue_.type();
    }

private:
    core::StringId name_;
    Value defaultValue_;
};

namespace detail {

VGC_CORE_DECLARE_PTRS(AuthoredAttribute);

// Holds the current authored value of a given attribute of a given element.
// See documentation of BuiltInAttribute and Element for more details.
//
class AuthoredAttribute: public core::Object
{
public:
    VGC_CORE_OBJECT(AuthoredAttribute)

    Element*          element() const { return element_; }
    core::StringId    name()    const { return name_; }
    const Value&      value()   const { return value_; }
    BuiltInAttribute* builtIn() const { return builtIn_; }

private:
    friend class Element;

    AuthoredAttribute(Element* element,
                      core::StringId name,
                      const Value& value = Value(),
                      BuiltInAttribute* builtIn = nullptr) :
        element_(element),
        name_(name),
        value_(value),
        builtIn_(builtIn) {

    }

    Element* element_;
    core::StringId name_;
    Value value_;
    BuiltInAttribute* builtIn_; //< nullptr if custom attribute
};

} // namespace detail

/// \class vgc::dom::Attribute
/// \brief Represents an attribute of a given Element.
///
class VGC_DOM_API Attribute
{
public:
    /// Returns the value hold by this Attribute.
    ///
    const Value& value() const {
        return isAuthored() ? authored_->value() : builtIn_->defaultValue();
    }

    /// Sets the value hold by this Attribute.
    ///
    void setValue(const Value& value);

    /// Returns the name of this Attribute.
    ///
    core::StringId name() const {
        return name_;
    }

    /// Returns the Element this Attribute belongs to.
    ///
    Element* element() const {
        return element_;
    }

    /// Returns whether this Attribute has an authored value.
    ///
    bool isAuthored() const {
        return static_cast<bool>(authored_);
    }

    /// If this attribute is a built-in attribute, returns its corresponding
    /// BuiltInAttribute*. Otherwise, returns nullptr.
    ///
    /// Use this method whenever you need to check whether this attribute is a built-in
    /// attribute or a custom attribute, even if you don't need access to the
    /// underlying BuiltInAttribute object.
    ///
    /// Example:
    ///
    /// \code
    /// Attribute attr = element->attribute(attrName);
    /// if (attr.builtIn()) {
    ///     // attr is a built-in attribute
    /// }
    /// else {
    ///     // attr is a custom attribute
    /// }
    /// \endcode
    ///
    BuiltInAttribute* builtIn() const {
        return builtIn_;
    }

private:
    friend class Element;

    Attribute(Element* element,
              core::StringId name,
              BuiltInAttribute* builtIn,
              detail::AuthoredAttribute* authored) :
        element_(element),
        name_(name),
        builtIn_(builtIn),
        authored_(authored) {

    }

    Element* element_;
    core::StringId name_;
    BuiltInAttribute* builtIn_;
    detail::AuthoredAttribute* authored_;
};

} // namespace dom
} // namespace vgc

#endif // VGC_DOM_ATTRIBUTE_H
