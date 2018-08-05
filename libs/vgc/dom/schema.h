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

#ifndef VGC_DOM_SCHEMA_H
#define VGC_DOM_SCHEMA_H

#include <map>
#include <vgc/core/stringid.h>
#include <vgc/dom/api.h>
#include <vgc/dom/attribute.h>

namespace vgc {
namespace dom {

/// \class vgc::dom::Schema
/// \brief Specifies built-in attributes for a give Element type.
///
/// In the DOM, each attribute has a "type" (example: Vec2dArray). When parsing
/// a VGC file, the parser must know in advance the expected type of any
/// attribute, in order to convert the XML attribute value (which is a string,
/// example: "[(1.0, 2.0), (3.0, 4.0)]") into the correct vgc::dom::Value.
///
/// This is done by defining a Schema for each Element type. The schema of a
/// given Element type defines the list of all of its built-in attribute names,
/// together with their types and default values. When the parser encounters an
/// XML element, it fetches its corresponding schema in the global list of all
/// registered Element types, then when it encounters an XML attribute for this
/// element, it does a lookup in the schema to know what is the expected type
/// of this attribute.
///
/// \sa BuiltInAttribute
///
/// Note: as of now, only built-in attributes are supported. In the future, we
/// are also planning to support "custom attributes", which would be generic
/// user-defined attributes not part of the Element's schema. For these, we are
/// planning to encode the type of the attribute as part of the attribute name
/// (e.g., data-vec2darray-mypositions="[]").
///
class Schema
{
public:
    /// Creates a Schema for the given Element \p name, with the given built-in
    /// \p attributes.
    ///
    Schema(core::StringId name,
           const std::map<core::StringId, BuiltInAttribute>& attributes);

    /// Returns the default value of the built-in attribute given by its \p name.
    ///
    const Value& defaultValue(core::StringId name) const;

    /// Returns the ValueType of the built-in attribute given by its \p name.
    ///
    ValueType valueType(core::StringId name) const;

private:
    core::StringId name_;
    std::map<core::StringId, BuiltInAttribute> attributes_;

};

} // namespace dom
} // namespace vgc

#endif // VGC_DOM_SCHEMA_H
