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

#ifndef VGC_STYLE_STYLESHEET_H
#define VGC_STYLE_STYLESHEET_H

#include <vgc/core/array.h>
#include <vgc/core/object.h>

#include <vgc/style/api.h>
#include <vgc/style/value.h>

namespace vgc::style {

namespace detail {

class StyleParser;

} // namespace detail

VGC_DECLARE_OBJECT(StylableObject);
VGC_DECLARE_OBJECT(StyleSheet);
VGC_DECLARE_OBJECT(StyleRuleSet);
VGC_DECLARE_OBJECT(StyleSelector);
VGC_DECLARE_OBJECT(StyleDeclaration);

using StyleRuleSetArray = core::Array<StyleRuleSet*>;
using StyleSelectorArray = core::Array<StyleSelector*>;
using StyleDeclarationArray = core::Array<StyleDeclaration*>;

/// \class vgc::style::StyleSheet
/// \brief Parses and stores a VGC stylesheet.
///
class VGC_STYLE_API StyleSheet : public core::Object {
private:
    VGC_OBJECT(StyleSheet, core::Object)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

public:
    /// Creates a stylesheet from the given specs and string.
    ///
    static StyleSheetPtr create(std::string_view s);

    /// Returns all the rule sets of this stylesheet.
    ///
    const StyleRuleSetArray& ruleSets() const {
        return ruleSets_;
    }

private:
    StyleRuleSetArray ruleSets_;

    friend class detail::StyleParser;
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

    friend class detail::StyleParser;
    StyleRuleSet();
    static StyleRuleSetPtr create();
};

/// \enum vgc::style::StyleSelectorItemType
/// \brief The type of a StyleSelectorItem
///
enum class StyleSelectorItemType : Int8 {
    // Non-combinator items don't have the 0x10 bit set
    ClassSelector = 0x01,

    // Combinator items have the 0x10 bit set
    DescendantCombinator = 0x10,
    ChildCombinator = 0x11
};

VGC_STYLE_API
VGC_DECLARE_ENUM(StyleSelectorItemType)

/// \class vgc::style::StyleSelectorItem
/// \brief One item of a StyleSelector.
///
/// A style selector consists of a sequence of "items", such as class selectors and combinators.
///
/// Note for now, we do not support the universal selector, the adjacent or
/// siblings combinators, pseudo-classes, pseudo-elements, and attribute
/// selectors, but this could be added in the future.
///
/// https://www.w3.org/TR/selectors-3/#selector-syntax
///
class VGC_STYLE_API StyleSelectorItem {
public:
    /// Creates a StyleSelectorItem of the given type and an empty name.
    ///
    StyleSelectorItem(StyleSelectorItemType type)
        : type_(type)
        , name_() {
    }

    /// Creates a StyleSelectorItem of the given type and given name.
    ///
    StyleSelectorItem(StyleSelectorItemType type, core::StringId name)
        : type_(type)
        , name_(name) {
    }

    /// Returns the type of this StyleSelectorItem.
    ///
    StyleSelectorItemType type() const {
        return type_;
    }

    /// Returns the name of this StyleSelectorItem. What this names represents
    /// depends on the type of this item. In the case of a ClassSelector, this
    /// represent the class name.
    ///
    core::StringId name() const {
        return name_;
    }

    /// Returns whether this item is a combinator selector item
    ///
    bool isCombinator() const {
        return core::toUnderlying(type_) & 0x10;
    }

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

    /// Returns the specificy of the selector.
    ///
    StyleSpecificity specificity() const {
        return specificity_;
    }

private:
    core::Array<StyleSelectorItem> items_;
    StyleSpecificity specificity_;

    friend class detail::StyleParser;
    StyleSelector(core::Array<StyleSelectorItem>&& items);
    static StyleSelectorPtr create(core::Array<StyleSelectorItem>&& items);
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

    friend class detail::StyleParser;
    StyleDeclaration();
    static StyleDeclarationPtr create();
};

} // namespace vgc::style

#endif // VGC_STYLE_STYLESHEET_H
