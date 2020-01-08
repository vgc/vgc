// Copyright 2020 The VGC Developers
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

#ifndef VGC_DOM_SCHEMA_H
#define VGC_DOM_SCHEMA_H

#include <map>
#include <vgc/core/stringid.h>
#include <vgc/dom/api.h>
#include <vgc/dom/value.h>

namespace vgc {
namespace dom {

/// \class vgc::dom::AttributeSpec
/// \brief Specifies the name and default value of a built-in attribute.
///
/// This immutable class is nothing else but a pair consisting of a name() and
/// a defaultValue(), representing the specifications of a built-in attribute.
/// The ValueType of the built-in attribute can be retrieved via valueType(),
/// which is a short-hand for defaultValue().valueType().
///
/// This is one of the building blocks that define a Schema.
///
class VGC_DOM_API AttributeSpec {
public:
    /// Creates a built-in attribute.
    ///
    AttributeSpec(const std::string& name, const Value& defaultValue) :
        name_(core::StringId(name)),
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

/// \class vgc::dom::ElementSpec
/// \brief Specifies all built-in attributes for a given Element type.
///
/// This immutable class is essentially a dictionary of ElementSpec, specifying
/// the name, type, and default value of all built-in attributes of a given
/// Element type.
///
/// This is one of the building blocks that define a Schema.
///
class ElementSpec
{
public:
    /// Creates an ElementSpec for the given Element \p name, with the given
    /// built-in \p attributes.
    ///
    ElementSpec(const std::string& name,
                const std::vector<AttributeSpec>& attributes);

    /// Returns the name of the Element specified by this ElementSpec.
    ///
    core::StringId name() const {
        return name_;
    }

    /// Finds the AttributeSpec for the given attribute \p name. Returns
    /// nullptr if the given \p name is not a built-in attribute of this
    /// Element type.
    ///
    const AttributeSpec* findAttributeSpec(core::StringId name) const;
    /// \overload
    const AttributeSpec* findAttributeSpec(const std::string& name) const {
        return findAttributeSpec(core::StringId(name));
    }

    /// Returns the default value of the built-in attribute given by its \p
    /// name. Returns an invalid value if the given \p name is not a built-in
    /// attribute of this Element type.
    ///
    const Value& defaultValue(core::StringId name) const;
    /// \overload
    const Value& defaultValue(const std::string& name) const {
        return defaultValue(core::StringId(name));
    }

    /// Returns the ValueType of the built-in attribute given by its \p name.
    /// Returns ValueType::Invalid if the given \p name is not a built-in
    /// attribute of this Element type.
    ///
    ValueType valueType(core::StringId name) const;
    /// \overload
    ValueType valueType(const std::string& name) const {
        return valueType(core::StringId(name));
    }

private:
    core::StringId name_;
    std::map<core::StringId, AttributeSpec> attributes_;
};

/// \class vgc::dom::Schema
/// \brief Specifies the structure and built-in attributes of a VGC document.
///
/// A VGC document is made of elements, and these elements are made of
/// attributes with a given type. For example, the Element named "path" has a
/// built-in Attribute named "positions" of type ValueType::Vec2dArray. When
/// reading a VGC document, the parser must know in advance the type of any
/// attribute, in order to be able to convert its string representation
/// (example: "[(1.0, 2.0), (3.0, 4.0)]") into the correct vgc::dom::Value.
/// Also, the parser must know the default values of built-in attributes in
/// case the attribute is omitted.
///
/// The VGC Schema, an immutable global object accessible via the method
/// vgc::dom::schema() is where this type information and default values are
/// stored.
///
/// A Schema is essentially a dictionary of ElementSpec, where each ElementSpec
/// is a dictionary of AttributeSpec, where each AttributeSpec specifies the
/// name, type, and default value of a built-in attribute.
///
/// Built-in Attributes vs. Custom Attributes
/// -----------------------------------------
///
/// Currently, only built-in attributes are supported, that is, all Element
/// types have a well defined list of attributes, and no other attributes are
/// allowed. However, in the future, we are also planning to support "custom
/// attributes", which would be generic user-defined attributes not part of the
/// ElementSpec. In order to inform the parser of the type of these attributes,
/// we are planning to encode the type of the attribute as part of the
/// attribute name itself, for example:
///
/// \code
/// <path data-vec2darray-mypositions="[]"/>
/// \endcode
///
class Schema
{
public:
    /// Creates a Schema with the given \p element specifications.
    ///
    Schema(const std::vector<ElementSpec>& elements);

    /// Finds the ElementSpec for the given Element \p name. Returns nullptr
    /// if the given \p name is not defined in the Schema.
    ///
    const ElementSpec* findElementSpec(core::StringId name) const;
    /// \overload
    const ElementSpec* findElementSpec(const std::string& name) const {
        return findElementSpec(core::StringId(name));
    }

private:
    std::map<core::StringId, ElementSpec> elements_;
};

/// Returns the VGC Schema singleton.
///
const Schema& schema();

} // namespace dom
} // namespace vgc

#endif // VGC_DOM_SCHEMA_H
