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

#ifndef VGC_STYLE_STYLE_H
#define VGC_STYLE_STYLE_H

#include <any>
#include <memory>
#include <string_view>
#include <unordered_map>

#include <vgc/core/array.h>
#include <vgc/core/color.h>
#include <vgc/core/innercore.h>
#include <vgc/core/stringid.h>

#include <vgc/style/api.h>
#include <vgc/style/strings.h>
#include <vgc/style/token.h>

namespace vgc::style {

namespace internal {
class StyleParser;
}

VGC_DECLARE_OBJECT(StylableObject);

VGC_DECLARE_OBJECT(StyleSheet);
VGC_DECLARE_OBJECT(StyleRuleSet);
VGC_DECLARE_OBJECT(StyleSelector);
VGC_DECLARE_OBJECT(StyleDeclaration);

using StyleRuleSetArray = core::Array<StyleRuleSet*>;
using StyleSelectorArray = core::Array<StyleSelector*>;
using StyleDeclarationArray = core::Array<StyleDeclaration*>;

/// \enum vgc::style::StyleValueType
/// \brief The type of a StyleValue
///
enum class StyleValueType : Int8 {
    None,          ///< There is no value at all
    Invalid,       ///< The value is invalid (e.g., parse error)
    Inherit,       ///< The value should inherit from a parent StylableObject
    Identifier,    ///< The value is an identifier
    Number,        ///< The value is a number
    String,        ///< The value is a string
    Custom         ///< The value is a custom type
};

/// \enum vgc::style::StyleValue
/// \brief Stores the value of a style attribute.
///
class VGC_STYLE_API StyleValue {
private:
    /// Creates a StyleValue of the given type
    ///
    StyleValue(StyleValueType type)
        : type_(type) {
    }

    /// Creates a StyleValue of the given type and value
    ///
    template<typename TValue>
    StyleValue(StyleValueType type, TValue value)
        : type_(type)
        , value_(value) {
    }

public:
    /// Creates a StyleValue of type None.
    ///
    StyleValue()
        : StyleValue(StyleValueType::None) {
    }

    /// Creates a StyleValue of type None
    ///
    static StyleValue none() {
        return StyleValue(StyleValueType::None);
    }

    /// Creates a StyleValue of type Invalid
    ///
    static StyleValue invalid() {
        return StyleValue(StyleValueType::Invalid);
    }

    /// Creates a StyleValue of type Inherit
    ///
    static StyleValue inherit() {
        return StyleValue(StyleValueType::Inherit);
    }

    /// Creates a StyleValue of type Identifier
    ///
    static StyleValue identifier(const std::string& string) {
        return identifier(core::StringId(string));
    }

    /// Creates a StyleValue of type Identifier
    ///
    static StyleValue identifier(core::StringId stringId) {
        return StyleValue(StyleValueType::Identifier, stringId);
    }

    /// Creates a StyleValue of type Number
    ///
    static StyleValue number(double x) {
        return StyleValue(StyleValueType::Number, x);
    }

    /// Creates a StyleValue of type String
    ///
    static StyleValue string(const std::string& str) {
        return string(core::StringId(str));
    }

    /// Creates a StyleValue of type String
    ///
    static StyleValue string(core::StringId stringId) {
        return StyleValue(StyleValueType::String, stringId);
    }

    /// Creates a StyleValue of type Custom
    ///
    template<typename TValue>
    static StyleValue custom(const TValue& value) {
        return StyleValue(StyleValueType::Custom, value);
    }

    /// Returns the type of the StyleValue
    ///
    StyleValueType type() const {
        return type_;
    }

    /// Returns the StyleValue as a `float`. The behavior is undefined
    /// if the type isn't Number.
    ///
    float toFloat() const {
        return static_cast<float>(std::any_cast<double>(value_));
    }

    /// Returns the StyleValue as a `double`. The behavior is undefined
    /// if the type isn't Number.
    ///
    double toDouble() const {
        return std::any_cast<double>(value_);
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
        return (type() == StyleValueType::Identifier ||
                type() == StyleValueType::String) &&
               std::any_cast<core::StringId>(value_) == other;
    }

    /// Returns whether this StyleValue is of type Identifier or String and
    /// whose string value is equal the given string.
    ///
    bool operator==(const core::StringId& other) const {
        return (type() == StyleValueType::Identifier ||
                type() == StyleValueType::String) &&
               std::any_cast<core::StringId>(value_) == other;
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

private:
    StyleValueType type_;
    std::any value_;
};

/// \typedef vgc::style::StylePropertyParser
/// \brief The type of a function that takes as input a token range
///        and outputs a StyleValue.
///
using StylePropertyParser = StyleValue(*)(StyleTokenIterator begin, StyleTokenIterator end);

/// This is the default function used for parsing properties when no
/// StylePropertySpec exists for the given property.
///
/// If the property value is made of a single Identifier token, then
/// it returns a StyleValue of type `Identifier`.
///
/// TODO: other simple cases, such as Number, Dimension, String, etc.
///
VGC_STYLE_API
StyleValue parseStyleDefault(StyleTokenIterator begin, StyleTokenIterator end);

namespace internal {
class StylePropertySpecMaker;
}

/// \class vgc::style::StylePropertySpec
/// \brief Specifies the name, initial value, and inheritability of a given
///        style property.
///
/// \sa StylePropertyTable
///
/// https://www.w3.org/TR/CSS2/propidx.html
///
class VGC_STYLE_API StylePropertySpec {
public:    
    /// Creates a StylePropertySpec.
    ///
    StylePropertySpec(core::StringId name,
                      const StyleValue& initialValue,
                      bool isInherited,
                      StylePropertyParser parser)
        : name_(name)
        , initialValue_(initialValue)
        , isInherited_(isInherited)
        , parser_(parser) {
    }

    /// Creates a StylePropertySpec.
    ///
    StylePropertySpec(const char* name,
                      const StyleValue& initialValue,
                      bool isInherited,
                      StylePropertyParser parser)
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
    const StyleValue& initialValue() const {
        return initialValue_;
    }

    /// Returns whether this property is inherited.
    ///
    bool isInherited() const {
        return isInherited_;
    }

    StylePropertyParser parser() const {
        return parser_;
    }

private:
    core::StringId name_;
    StyleValue initialValue_;
    bool isInherited_;
    StylePropertyParser parser_;
};

/// \class vgc::style::StylePropertySpecTable
/// \brief Stores a table of multiple StylePropertySpec.
///
class VGC_STYLE_API StylePropertySpecTable
        : public std::enable_shared_from_this<StylePropertySpecTable> {
public:
    StylePropertySpecTable() {}

    void insert(const char* name,
                const StyleValue& initialValue,
                bool isInherited,
                StylePropertyParser parser) {
        insert(core::StringId(name), initialValue, isInherited, parser);
    }

    void insert(core::StringId name,
                const StyleValue& initialValue,
                bool isInherited,
                StylePropertyParser parser) {
        StylePropertySpec spec = StylePropertySpec(name, initialValue, isInherited, parser);
        map_.insert({name, spec});
    }

    const StylePropertySpec* get(core::StringId name) {
        auto search = map_.find(name);
        if (search == map_.end()) {
            return nullptr;
        }
        else {
            return &(search->second);
        }
    }

private:
    std::unordered_map<core::StringId, StylePropertySpec> map_;
};

using StylePropertySpecTablePtr = std::shared_ptr<StylePropertySpecTable>;

/// \class vgc::style::StyleSheet
/// \brief Parses and stores a VGCSS stylesheet.
///
class VGC_STYLE_API StyleSheet : public core::Object {
private:
    VGC_OBJECT(StyleSheet, core::Object)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

public:

    /// Creates a stylesheet from the given specs and string.
    ///
    static StyleSheetPtr create(const StylePropertySpecTablePtr& specs,
                                std::string_view s);

    /// Returns all the rule sets of this stylesheet.
    ///
    const StyleRuleSetArray& ruleSets() const {
        return ruleSets_;
    }

    const StylePropertySpecTablePtr& propertySpecs() const {
        return propertySpecs_;
    }

private:
    StylePropertySpecTablePtr propertySpecs_;
    StyleRuleSetArray ruleSets_;

    friend class internal::StyleParser;
    StyleSheet();
    static StyleSheetPtr create();
};

/// \class vgc::style::StyleRuleSet
/// \brief One rule set of a stylesheet.
///
class VGC_STYLE_API StyleRuleSet : public core::Object {
private:
    VGC_OBJECT(StyleRuleSet, core::Object)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

public:
    const StyleSelectorArray& selectors() const {
        return selectors_;
    }

    const StyleDeclarationArray& declarations() const {
        return declarations_;
    }

private:
    StyleSelectorArray selectors_;
    StyleDeclarationArray declarations_;

    friend class internal::StyleParser;
    StyleRuleSet();
    static StyleRuleSetPtr create();
};

/// \enum vgc::style::StyleSelectorItemType
/// \brief The type of a StyleSelectorItem
///
enum class StyleSelectorItemType : Int8 {
    ClassSelector
};

/// \class vgc::style::StyleSelectorItem
/// \brief One item of a StyleSelector.
///
/// A style selector consists of a sequence of "items". For now, the only
/// available item is "class selector".
///
/// In the future, more items should be supported, such as the universal
/// selector, combinators, pseudo classes, and perhaps attribute selectors and
/// pseudo-elements. See:
///
/// https://www.w3.org/TR/selectors-3/#selector-syntax
///
class VGC_STYLE_API StyleSelectorItem {
public:
    /// Creates a StyleSelectorItem of the given type and given name.
    ///
    StyleSelectorItem(StyleSelectorItemType type, core::StringId name) :
        type_(type), name_(name) {}

    /// Returns the type of this StyleSelectorItem.
    ///
    StyleSelectorItemType type() const { return type_; }

    /// Returns the name of this StyleSelectorItem. What this names represents
    /// depends on the type of this item. In the case of a ClassSelector, this
    /// represent the class name.
    ///
    core::StringId name() const { return name_; }

private:
    StyleSelectorItemType type_;
    core::StringId name_;
};

using StyleSpecificity = UInt64;

/// \class vgc::style::StyleSelector
/// \brief One selector of a rule set of a stylesheet.
///
class VGC_STYLE_API StyleSelector : public core::Object {
private:
    VGC_OBJECT(StyleSelector, core::Object)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

public:
    /// Returns whether the given StylableObject matches this selector.
    ///
    bool matches(StylableObject* node);

    /// Returns the specificy of the selector
    ///
    StyleSpecificity specificity() const {
        // TODO: compute actual specificity based on id/class/etc.
        // It could be computed as:
        //    2^48 * numIDs
        //  + 2^32 * numClasses
        //  + numTypes
        // (note: inline styles are treated as separate layers and thus don't need
        //        to be given higher specificity)
        return core::int_cast<StyleSpecificity>(items_.size());
    }

private:
    core::Array<StyleSelectorItem> items_;

    friend class internal::StyleParser;
    StyleSelector();
    static StyleSelectorPtr create();
};

/// \class vgc::style::StyleDeclaration
/// \brief One declaration of a rule set of a stylesheet.
///
class VGC_STYLE_API StyleDeclaration : public core::Object {
private:
    VGC_OBJECT(StyleDeclaration, core::Object)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

public:
    /// Returns the property name of this declaration.
    ///
    const core::StringId& property() {
        return property_;
    }

    /// Returns the string representation of the value of this declaration.
    ///
    const std::string& text() {
        return text_;
    }

    /// Returns the value of this declaration.
    ///
    const StyleValue& value() {
        return value_;
    }

private:
    core::StringId property_;
    std::string text_;
    StyleValue value_;

    friend class internal::StyleParser;
    StyleDeclaration();
    static StyleDeclarationPtr create();
};

} // namespace vgc::style

#endif // VGC_STYLE_STYLE_H
