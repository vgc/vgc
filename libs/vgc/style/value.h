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

class StylePropertySpec;

/// \enum vgc::style::ValueType
/// \brief The type of a StyleValue
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

/// \enum vgc::style::StyleValue
/// \brief Stores the value of a style attribute.
///
class VGC_STYLE_API StyleValue {
private:
    /// Creates a StyleValue of the given type
    ///
    StyleValue(ValueType type)
        : type_(type) {
    }

    /// Creates a StyleValue of the given type and value
    ///
    template<typename TValue>
    StyleValue(ValueType type, TValue value)
        : type_(type)
        , value_(value) {
    }

public:
    /// Creates a StyleValue of type None.
    ///
    StyleValue()
        : StyleValue(ValueType::None) {
    }

    /// Creates a StyleValue of type None
    ///
    static StyleValue none() {
        return StyleValue(ValueType::None);
    }

    /// Creates a StyleValue of type Unparsed. This allows to defer parsing the
    /// value until the `SpecTable` of the tree is properly populated.
    ///
    static StyleValue unparsed(StyleTokenIterator begin, StyleTokenIterator end);

    /// Creates a StyleValue of type Invalid
    ///
    static StyleValue invalid() {
        return StyleValue(ValueType::Invalid);
    }

    /// Creates a StyleValue of type Inherit
    ///
    static StyleValue inherit() {
        return StyleValue(ValueType::Inherit);
    }

    /// Creates a StyleValue of type Identifier
    ///
    static StyleValue identifier(std::string_view string) {
        return identifier(core::StringId(string));
    }

    /// Creates a StyleValue of type Identifier
    ///
    static StyleValue identifier(core::StringId stringId) {
        return StyleValue(ValueType::Identifier, stringId);
    }

    /// Creates a StyleValue of type Number
    ///
    static StyleValue number(float x) {
        return StyleValue(ValueType::Number, x);
    }

    /// Creates a StyleValue of type String
    ///
    static StyleValue string(const std::string& str) {
        return string(core::StringId(str));
    }

    /// Creates a StyleValue of type String
    ///
    static StyleValue string(core::StringId stringId) {
        return StyleValue(ValueType::String, stringId);
    }

    /// Creates a StyleValue of type Custom
    ///
    template<typename TValue>
    static StyleValue custom(const TValue& value) {
        return StyleValue(ValueType::Custom, value);
    }

    /// Returns the type of the StyleValue
    ///
    ValueType type() const {
        return type_;
    }

    /// Returns whether the value is valid.
    ///
    bool isValid() const {
        return type_ != ValueType::Invalid;
    }

    /// Returns the StyleValue as a `float`. The behavior is undefined
    /// if the type isn't Number.
    ///
    float toFloat() const {
        return std::any_cast<float>(value_);
    }

    /// Returns the StyleValue as an `std::string`.
    /// The behavior is undefined if the type isn't Identifier or String.
    ///
    const std::string& toString() const {
        return std::any_cast<core::StringId>(value_).string();
    }

    /// Returns the StyleValue as a `vgc::core::StringId`.
    /// The behavior is undefined if the type isn't Identifier or String.
    ///
    core::StringId toStringId() const {
        return std::any_cast<core::StringId>(value_);
    }

    /// Returns whether this StyleValue is of type Identifier or String and
    /// whose string value is equal the given string.
    ///
    bool operator==(const std::string& other) const {
        return (type() == ValueType::Identifier || type() == ValueType::String)
               && std::any_cast<core::StringId>(value_) == other;
    }

    /// Returns whether this StyleValue is of type Identifier or String and
    /// whose string value is equal the given string.
    ///
    bool operator==(const core::StringId& other) const {
        return (type() == ValueType::Identifier || type() == ValueType::String)
               && std::any_cast<core::StringId>(value_) == other;
    }

    /// Returns whether this `StyleValue` stores a value of type `TValue`.
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
    void parse_(const StylePropertySpec* spec);
};

namespace detail {

// Stores the unparsed string of the value as well its tokenized version.
//
class UnparsedValue {
public:
    UnparsedValue(StyleTokenIterator begin, StyleTokenIterator end);
    UnparsedValue(const UnparsedValue& other);
    UnparsedValue(UnparsedValue&& other);
    UnparsedValue& operator=(const UnparsedValue& other);
    UnparsedValue& operator=(UnparsedValue&& other);

    const std::string& rawString() const {
        return rawString_;
    }

    const StyleTokenArray& tokens() const {
        return tokens_;
    }

private:
    std::string rawString_;
    StyleTokenArray tokens_;

    // Note: tokens_ contains pointers to characters in the strings. These must
    // be properly updated whenever the string is copied. See remapPointers_().

    static std::string initRawString(StyleTokenIterator begin, StyleTokenIterator end);
    void remapPointers_();
};

} // namespace detail

} // namespace vgc::style

#endif // VGC_STYLE_VALUE_H
