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

#ifndef VGC_STYLE_VALUE_H
#define VGC_STYLE_VALUE_H

#include <any>
#include <string_view>

#include <vgc/core/arithmetic.h>
#include <vgc/core/enum.h>
#include <vgc/core/stringid.h>

#include <vgc/style/api.h>
#include <vgc/style/token.h>

namespace vgc::style {

class PropertySpec;

/// \enum vgc::style::ValueType
/// \brief The type of a Value
///
enum class ValueType : Int8 {
    None,       ///< There is no value at all
    Unparsed,   ///< The value hasn't been parsed yet
    Invalid,    ///< The value is invalid (e.g., parse error)
    Inherit,    ///< The value should inherit from a parent StylableObject
    Identifier, ///< The value is an identifier
    Number,     ///< The value is a number
    String,     ///< The value is a string
    Custom      ///< The value is a custom type
};

VGC_STYLE_API
VGC_DECLARE_ENUM(ValueType)

/// \enum vgc::style::Value
/// \brief Stores the value of a style attribute.
///
class VGC_STYLE_API Value {
private:
    /// Creates a Value of the given type
    ///
    Value(ValueType type)
        : type_(type) {
    }

    /// Creates a Value of the given type and value
    ///
    template<typename TValue>
    Value(ValueType type, TValue value)
        : type_(type)
        , value_(value) {
    }

public:
    /// Creates a Value of type None.
    ///
    Value()
        : Value(ValueType::None) {
    }

    /// Creates a Value of type None
    ///
    static Value none() {
        return Value(ValueType::None);
    }

    /// Creates a Value of type Unparsed. This allows to defer parsing the
    /// value until the `SpecTable` of the tree is properly populated.
    ///
    static Value unparsed(TokenIterator begin, TokenIterator end);

    /// Creates a Value of type Invalid
    ///
    static Value invalid() {
        return Value(ValueType::Invalid);
    }

    /// Creates a Value of type Inherit
    ///
    static Value inherit() {
        return Value(ValueType::Inherit);
    }

    /// Creates a Value of type Identifier
    ///
    static Value identifier(std::string_view string) {
        return identifier(core::StringId(string));
    }

    /// Creates a Value of type Identifier
    ///
    static Value identifier(core::StringId stringId) {
        return Value(ValueType::Identifier, stringId);
    }

    /// Creates a Value of type Number
    ///
    static Value number(float x) {
        return Value(ValueType::Number, x);
    }

    /// Creates a Value of type String
    ///
    static Value string(const std::string& str) {
        return string(core::StringId(str));
    }

    /// Creates a Value of type String
    ///
    static Value string(core::StringId stringId) {
        return Value(ValueType::String, stringId);
    }

    /// Creates a Value of type Custom
    ///
    template<typename TValue>
    static Value custom(const TValue& value) {
        return Value(ValueType::Custom, value);
    }

    /// Returns the type of the Value
    ///
    ValueType type() const {
        return type_;
    }

    /// Returns whether the value is valid.
    ///
    bool isValid() const {
        return type_ != ValueType::Invalid;
    }

    /// Returns the Value as a `float`. The behavior is undefined
    /// if the type isn't Number.
    ///
    float toFloat() const {
        return std::any_cast<float>(value_);
    }

    /// Returns the Value as an `std::string`.
    /// The behavior is undefined if the type isn't Identifier or String.
    ///
    const std::string& toString() const {
        return std::any_cast<core::StringId>(value_).string();
    }

    /// Returns the Value as a `vgc::core::StringId`.
    /// The behavior is undefined if the type isn't Identifier or String.
    ///
    core::StringId toStringId() const {
        return std::any_cast<core::StringId>(value_);
    }

    /// Returns whether this Value is of type Identifier or String and
    /// whose string value is equal the given string.
    ///
    bool operator==(const std::string& other) const {
        return (type() == ValueType::Identifier || type() == ValueType::String)
               && std::any_cast<core::StringId>(value_) == other;
    }

    /// Returns whether this Value is of type Identifier or String and
    /// whose string value is equal the given string.
    ///
    bool operator==(const core::StringId& other) const {
        return (type() == ValueType::Identifier || type() == ValueType::String)
               && std::any_cast<core::StringId>(value_) == other;
    }

    /// Returns whether this `Value` stores a value of type `TValue`.
    ///
    template<typename TValue>
    bool has() const {
        return typeid(TValue) == value_.type();
    }

    /// Returns the stored value as a `TValue`. An exception is raised if the
    /// stored value is not of type `TValue`.
    ///
    /// Note that an `Identifier` and `String` is
    /// stored as a `StringId`, and a `Number` is stored as a double (for now).
    ///
    template<typename TValue>
    TValue to() const {
        return std::any_cast<TValue>(value_);
    }

    /// Returns the value stored as a `TValue`. An default constructed value is
    /// returned if the stored value is not of type `TValue`.
    ///
    /// Note that an `Identifier` and `String` is
    /// stored as a `StringId`, and a `Number` is stored as a double (for now).
    ///
    template<typename TValue>
    TValue valueOrDefault(const TValue& defaultValue = TValue{}) const {
        const TValue* v = std::any_cast<TValue>(&value_);
        return v ? *v : defaultValue;
    }

private:
    ValueType type_;
    std::any value_;

    friend class StylableObject;
    void parse_(const PropertySpec* spec);
};

namespace detail {

// Stores the unparsed string of the value as well its tokenized version.
//
class UnparsedValue {
public:
    UnparsedValue(TokenIterator begin, TokenIterator end);
    UnparsedValue(const UnparsedValue& other);
    UnparsedValue(UnparsedValue&& other);
    UnparsedValue& operator=(const UnparsedValue& other);
    UnparsedValue& operator=(UnparsedValue&& other);

    const std::string& rawString() const {
        return rawString_;
    }

    const TokenArray& tokens() const {
        return tokens_;
    }

private:
    std::string rawString_;
    TokenArray tokens_;

    // Note: tokens_ contains pointers to characters in the strings. These must
    // be properly updated whenever the string is copied. See remapPointers_().

    static std::string initRawString(TokenIterator begin, TokenIterator end);
    void remapPointers_();
};

} // namespace detail

} // namespace vgc::style

#endif // VGC_STYLE_VALUE_H
