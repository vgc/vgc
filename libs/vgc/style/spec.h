// Copyright 2022 The VGC Developers
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

#ifndef VGC_STYLE_SPEC_H
#define VGC_STYLE_SPEC_H

#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include <vgc/core/objecttype.h>
#include <vgc/core/stringid.h>

#include <vgc/style/api.h>
#include <vgc/style/token.h>
#include <vgc/style/value.h>

namespace vgc::style {

namespace detail {

class Parser;

} // namespace detail

/// \typedef vgc::style::PropertyParser
/// \brief The type of a function that takes as input a token range
///        and outputs a Value.
///
using PropertyParser = Value (*)(TokenIterator begin, TokenIterator end);

/// This is the default function used for parsing properties when no
/// PropertySpec exists for the given property.
///
/// If the property value is made of a single Identifier token, then
/// it returns a Value of type `Identifier`.
///
/// TODO: other simple cases, such as Number, Dimension, String, etc.
///
VGC_STYLE_API
Value parseStyleDefault(TokenIterator begin, TokenIterator end);

/// \class vgc::style::PropertySpec
/// \brief Specifies the name, initial value, and inheritability of a given
///        style property.
///
/// \sa StylePropertyTable
///
/// https://www.w3.org/TR/CSS2/propidx.html
///
class VGC_STYLE_API PropertySpec {
public:
    /// Creates a PropertySpec.
    ///
    PropertySpec(
        core::StringId name,
        const Value& initialValue,
        bool isInherited,
        PropertyParser parser)

        : name_(name)
        , initialValue_(initialValue)
        , isInherited_(isInherited)
        , parser_(parser) {
    }

    /// Creates a PropertySpec.
    ///
    PropertySpec(
        const char* name,
        const Value& initialValue,
        bool isInherited,
        PropertyParser parser)

        : name_(name)
        , initialValue_(initialValue)
        , isInherited_(isInherited)
        , parser_(parser) {
    }

    /// Returns the name of this property.
    ///
    core::StringId name() const {
        return name_;
    }

    /// Returns the initial value of this property.
    ///
    const Value& initialValue() const {
        return initialValue_;
    }

    /// Returns whether this property is inherited.
    ///
    bool isInherited() const {
        return isInherited_;
    }

    PropertyParser parser() const {
        return parser_;
    }

private:
    core::StringId name_;
    Value initialValue_;
    bool isInherited_;
    PropertyParser parser_;
};

/// \class vgc::style::SpecTable
/// \brief Stores a table of multiple PropertySpec.
///
class VGC_STYLE_API SpecTable : public std::enable_shared_from_this<SpecTable> {

public:
    /// Creates an empty `SpecTable`.
    ///
    SpecTable() {
    }

    /// Inserts a `PropertySpec` with the given values to this table.
    ///
    /// Emits a warning and does not perform the insertion if there is already
    /// a spec for the given `attributeName`.
    ///
    void insert(
        core::StringId attributeName,
        const Value& initialValue,
        bool isInherited,
        PropertyParser parser);

    /// \overload
    void insert(
        std::string_view attributeName,
        const Value& initialValue,
        bool isInherited,
        PropertyParser parser) {

        insert(core::StringId(attributeName), initialValue, isInherited, parser);
    }

    /// Returns the `PropertySpec` associated with the given `attributeName`.
    ///
    /// Returns `nullptr` if the table does not contain a spec for the given
    /// `attributeName`.
    ///
    const PropertySpec* get(core::StringId attributeName) const {
        auto search = map_.find(attributeName);
        if (search == map_.end()) {
            return nullptr;
        }
        else {
            return &(search->second);
        }
    }

    /// Returns whether the given `objectType` is in the set of types already
    /// registered in this `SpecTable`.
    ///
    bool isRegistered(const core::ObjectType& objectType) const {
        return registeredObjectTypes_.find(objectType) != registeredObjectTypes_.end();
    }
    /// \overload
    template<typename T>
    bool isRegistered() const {
        return isRegistered(T::staticObjectType());
    }

    /// Attempts to insert the given `objectType` in the set of types
    /// registered in this `SpecTable`. Returns true if the type was actually
    /// inserted, that is, if the type wasn't already registered.
    ///
    bool setRegistered(const core::ObjectType& objectType);
    /// \overload
    template<typename T>
    bool setRegistered() {
        return setRegistered(T::staticObjectType());
    }

private:
    std::unordered_set<core::ObjectType> registeredObjectTypes_;
    std::unordered_map<core::StringId, PropertySpec> map_;
};

using SpecTablePtr = std::shared_ptr<SpecTable>;

} // namespace vgc::style

#endif // VGC_STYLE_SPEC_H
