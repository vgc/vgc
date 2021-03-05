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

#ifndef VGC_UI_STYLE_H
#define VGC_UI_STYLE_H

#include <unordered_map>

#include <vgc/core/array.h>
#include <vgc/core/color.h>
#include <vgc/core/innercore.h>
#include <vgc/core/stringid.h>
#include <vgc/ui/api.h>
#include <vgc/ui/styletoken.h>

namespace vgc {
namespace ui {

namespace internal {
class StyleParser;
}

VGC_DECLARE_OBJECT(StyleSheet);
VGC_DECLARE_OBJECT(StyleRuleSet);
VGC_DECLARE_OBJECT(StyleSelector);
VGC_DECLARE_OBJECT(StyleDeclaration);
VGC_DECLARE_OBJECT(Widget);

using StyleRuleSetArray = core::Array<StyleRuleSet*>;
using StyleSelectorArray = core::Array<StyleSelector*>;
using StyleDeclarationArray = core::Array<StyleDeclaration*>;

/// \enum vgc::ui::StyleValueType
/// \brief The type of a StyleValue
///
enum class StyleValueType : Int8 {
    None,     ///< There is no value at all
    Invalid,  ///< The value is invalid (e.g., parse error)
    Inherit,  ///< The value should inherit from a parent widget
    Auto,     ///< The value is auto
    Length,   ///< The value is a length
    String,   ///< The value is a string
    Color,    ///< The value is a color
};

/// \enum vgc::ui::StyleValue
/// \brief Stores the value of a style attribute.
///
class VGC_UI_API StyleValue {
private:
    /// Creates a StyleValue of the given type
    ///
    StyleValue(StyleValueType type) : type_(type) {}

public:
    /// Returns the type of the StyleValue
    ///
    StyleValueType type() const { return type_; }

    /// Creates a StyleValue of type None.
    ///
    StyleValue() : StyleValue(StyleValueType::None) {}

    /// Creates a StyleValue of type None
    ///
    static StyleValue none() { return StyleValue(StyleValueType::None); }

    /// Creates a StyleValue of type Invalid
    ///
    static StyleValue invalid() { return StyleValue(StyleValueType::Invalid); }

    /// Creates a StyleValue of type Inherit
    ///
    static StyleValue inherit() { return StyleValue(StyleValueType::Inherit); }

    /// Creates a StyleValue of type Length
    ///
    StyleValue(float length) :
        type_(StyleValueType::Length),
        length_(length)
    {

    }

    /// Returns the length of the StyleValue.
    /// The behavior is undefined if the type isn't Length.
    ///
    float length() const { return length_; }

    /// Creates a StyleValue of type String
    ///
    StyleValue(const std::string& string) :
        type_(StyleValueType::String),
        string_(string)
    {

    }

    /// Returns the string of the StyleValue.
    /// The behavior is undefined if the type isn't String.
    ///
    const std::string& string() const { return string_; }

    /// Creates a StyleValue of type Color
    ///
    StyleValue(const core::Color color) :
        type_(StyleValueType::Color),
        color_(color)
    {

    }

    /// Returns the color of the StyleValue.
    /// The behavior is undefined if the type isn't Color.
    ///
    core::Color color() const { return color_; }

private:
    StyleValueType type_;
    // TODO: Encapsulate following member variables in std::variant (C++17)
    float length_;
    std::string string_;
    core::Color color_;
};

/// \typedef vgc::ui::StylePropertyParser
/// \brief The type of a function that takes as input a token range
///        and outputs a StyleValue.
///
using StylePropertyParser = StyleValue(*)(StyleTokenIterator begin, StyleTokenIterator end);

/// This is the default function used for parsing properties when no
/// StylePropertySpec exists for the given property. It returns a StyleValue of
/// type String, where the string is made of all the characters in the given
/// token range (including quotes and double quotes characters, if any).
///
StyleValue parseStyleDefault(StyleTokenIterator begin, StyleTokenIterator end);

/// Parses the given style tokens as a color.
///
StyleValue parseStyleColor(StyleTokenIterator begin, StyleTokenIterator end);

/// Parses the given style tokens as a length.
///
StyleValue parseStyleLength(StyleTokenIterator begin, StyleTokenIterator end);

namespace internal {
class StylePropertySpecMaker;
}

/// \class vgc::ui::StylePropertySpec
/// \brief Specifies the name, initial value, and inheritability of a given
///        style property.
///
/// \sa StylePropertyTable
///
/// https://www.w3.org/TR/CSS2/propidx.html
///
class VGC_UI_API StylePropertySpec {
public:
    /// Returns the StylePropertySpec corresponding to the given property name.
    /// Returns nullptr if the property has no known StylePropertySpec.
    ///
    static StylePropertySpec* get(core::StringId property);

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

    friend class internal::StylePropertySpecMaker;
    using Table = std::unordered_map<core::StringId, StylePropertySpec>;
    static Table map_;
    static void init();
    StylePropertySpec(core::StringId name, const StyleValue& initialValue, bool isInherited, StylePropertyParser parser) :
        name_(name), initialValue_(initialValue), isInherited_(isInherited), parser_(parser) {}
};

namespace internal {
class StylePropertySpecMaker {
public:
    static StylePropertySpec::Table::value_type
    make(const char* name, const StyleValue& initialValue, bool isInherited, StylePropertyParser parser) {
        core::StringId n(name);
        return std::make_pair(n, StylePropertySpec(n, initialValue, isInherited, parser));
    }
};
}

/// \class vgc::ui::Style
/// \brief Stores a given style.
///
class VGC_UI_API Style {
public:
    /// Constructs an empty style.
    ///
    Style();

    /// Returns the cascaded value of the given property, that is,
    /// the value "winning the cascade". See:
    ///
    /// https://www.w3.org/TR/css-cascade-4/#cascaded
    ///
    /// This takes into account the selector specificity and the order of
    /// appearance in the stylesheet.
    ///
    /// This does NOT take into account widget inheritance (i.e., properties
    /// set of the parent widget are ignored) and does not take into account
    /// default values.
    ///
    /// If there is no declared value for the given property, then
    /// a value of type StyleValueType::None is returned.
    ///
    StyleValue cascadedValue(core::StringId property) const;

    /// Returns the computed value of the given property for the given
    /// widget, see:
    ///
    /// https://www.w3.org/TR/css-cascade-4/#computed
    ///
    /// This resolves widget inheritance and default values. In other words,
    /// the returned StyleValue is never of type StyleValueType::Inherit.
    /// However, the type could be StyleValueType::None if there is no known
    /// default value for the given property (this can be the case for custom
    /// properties which are missing from the stylesheet).
    ///
    StyleValue computedValue(core::StringId property, const Widget* widget) const;

private:
    friend class StyleSheet;
    // TODO: Share style data across similar widgets by encapsulating
    // all the member variables in a new class internal::StyleData,
    // and only storing here an std::shared_ptr<StyleData>.
    core::Array<StyleRuleSet*> ruleSets_;
    std::unordered_map<core::StringId, StyleValue> map_;
    StyleValue computedValue_(core::StringId property, const Widget* widget, StylePropertySpec* spec) const;
};

/// Get global stylesheet.
///
StyleSheet* styleSheet();

/// \class vgc::ui::StyleSheet
/// \brief Parses and stores a VGCSS stylesheet.
///
class VGC_UI_API StyleSheet : public core::Object {
private:
    VGC_OBJECT(StyleSheet)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

public:
    /// Creates an empty stylesheet.
    ///
    static StyleSheetPtr create();

    /// Creates a stylesheet from the given string.
    ///
    static StyleSheetPtr create(const std::string& s);

    /// Returns all the rule sets of this stylesheet.
    ///
    const StyleRuleSetArray& ruleSets() const {
        return ruleSets_;
    }

    /// Returns the style that applies to the given widget,
    /// by computing which ruleSets matches the widget.
    ///
    Style computeStyle(Widget* widget) const;

private:
    StyleRuleSetArray ruleSets_;

    friend class internal::StyleParser;
    StyleSheet();
};

/// \class vgc::ui::StyleRuleSet
/// \brief One rule set of a stylesheet.
///
class VGC_UI_API StyleRuleSet : public core::Object {
private:
    VGC_OBJECT(StyleRuleSet)
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

/// \enum vgc::ui::StyleSelectorItemType
/// \brief The type of a StyleSelectorItem
///
enum class StyleSelectorItemType : Int8 {
    ClassSelector
};

/// \class vgc::ui::StyleSelectorItem
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
class VGC_UI_API StyleSelectorItem {
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

/// \class vgc::ui::StyleSelector
/// \brief One selector of a rule set of a stylesheet.
///
class VGC_UI_API StyleSelector : public core::Object {
private:
    VGC_OBJECT(StyleSelector)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

public:
    /// Returns whether the given widget matches this selector.
    ///
    bool matches(Widget* widget);

private:
    core::Array<StyleSelectorItem> items_;

    friend class internal::StyleParser;
    StyleSelector();
    static StyleSelectorPtr create();
};

/// \class vgc::ui::StyleDeclaration
/// \brief One declaration of a rule set of a stylesheet.
///
class VGC_UI_API StyleDeclaration : public core::Object {
private:
    VGC_OBJECT(StyleDeclaration)
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

} // namespace ui
} // namespace vgc

#endif // VGC_UI_STYLE_H
