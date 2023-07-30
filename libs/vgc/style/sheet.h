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

#ifndef VGC_STYLE_SHEET_H
#define VGC_STYLE_SHEET_H

#include <vgc/core/array.h>
#include <vgc/core/object.h>

#include <vgc/style/api.h>
#include <vgc/style/value.h>

namespace vgc::style {

namespace detail {

class Parser;

} // namespace detail

VGC_DECLARE_OBJECT(StylableObject);
VGC_DECLARE_OBJECT(Sheet);
VGC_DECLARE_OBJECT(RuleSet);
VGC_DECLARE_OBJECT(Selector);
VGC_DECLARE_OBJECT(Declaration);

using RuleSetArray = core::Array<RuleSet*>;
using SelectorArray = core::Array<Selector*>;
using DeclarationArray = core::Array<Declaration*>;

/// \class vgc::style::Sheet
/// \brief Parses and stores a VGC style sheet.
///
class VGC_STYLE_API Sheet : public core::Object {
private:
    VGC_OBJECT(Sheet, core::Object)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

public:
    /// Creates a style sheet from the given specs and string.
    ///
    static SheetPtr create(std::string_view s);

    /// Returns all the rule sets of this style sheet.
    ///
    const RuleSetArray& ruleSets() const {
        return ruleSets_;
    }

private:
    RuleSetArray ruleSets_;

    friend class detail::Parser;
    Sheet(CreateKey);
    static SheetPtr create();
};

/// \class vgc::style::RuleSet
/// \brief One rule set of a style sheet.
///
class VGC_STYLE_API RuleSet : public core::Object {
private:
    VGC_OBJECT(RuleSet, core::Object)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

public:
    const SelectorArray& selectors() const {
        return selectors_;
    }

    const DeclarationArray& declarations() const {
        return declarations_;
    }

private:
    SelectorArray selectors_;
    DeclarationArray declarations_;

    friend class detail::Parser;
    RuleSet(CreateKey);
    static RuleSetPtr create();
};

/// \enum vgc::style::SelectorItemType
/// \brief The type of a SelectorItem
///
enum class SelectorItemType : Int8 {
    // Non-combinator items don't have the 0x10 bit set
    ClassSelector = 0x01,

    // Combinator items have the 0x10 bit set
    DescendantCombinator = 0x10,
    ChildCombinator = 0x11
};

VGC_STYLE_API
VGC_DECLARE_ENUM(SelectorItemType)

/// \class vgc::style::SelectorItem
/// \brief One item of a Selector.
///
/// A style selector consists of a sequence of "items", such as class selectors and combinators.
///
/// Note for now, we do not support the universal selector, the adjacent or
/// siblings combinators, pseudo-classes, pseudo-elements, and attribute
/// selectors, but this could be added in the future.
///
/// https://www.w3.org/TR/selectors-3/#selector-syntax
///
class VGC_STYLE_API SelectorItem {
public:
    /// Creates a SelectorItem of the given type and an empty name.
    ///
    SelectorItem(SelectorItemType type)
        : type_(type)
        , name_() {
    }

    /// Creates a SelectorItem of the given type and given name.
    ///
    SelectorItem(SelectorItemType type, core::StringId name)
        : type_(type)
        , name_(name) {
    }

    /// Returns the type of this SelectorItem.
    ///
    SelectorItemType type() const {
        return type_;
    }

    /// Returns the name of this SelectorItem. What this names represents
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
    SelectorItemType type_;
    core::StringId name_;
};

using Specificity = UInt64;

/// \class vgc::style::Selector
/// \brief One selector of a rule set of a style sheet.
///
class VGC_STYLE_API Selector : public core::Object {
private:
    VGC_OBJECT(Selector, core::Object)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

public:
    /// Returns whether the given StylableObject matches this selector.
    ///
    bool matches(StylableObject* node);

    /// Returns the specificy of the selector.
    ///
    Specificity specificity() const {
        return specificity_;
    }

private:
    core::Array<SelectorItem> items_;
    Specificity specificity_;

    friend class detail::Parser;
    Selector(CreateKey, core::Array<SelectorItem>&& items);
    static SelectorPtr create(core::Array<SelectorItem>&& items);
};

/// \class vgc::style::Declaration
/// \brief One declaration of a rule set of a style sheet.
///
class VGC_STYLE_API Declaration : public core::Object {
private:
    VGC_OBJECT(Declaration, core::Object)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

public:
    /// Returns the property name of this declaration.
    ///
    core::StringId property() {
        return property_;
    }

    /// Returns the string representation of the value of this declaration.
    ///
    const std::string& text() {
        return text_;
    }

    /// Returns the value of this declaration.
    ///
    const Value& value() {
        return value_;
    }

private:
    core::StringId property_;
    std::string text_;
    Value value_;

    friend class detail::Parser;
    Declaration(CreateKey);
    static DeclarationPtr create();
};

} // namespace vgc::style

#endif // VGC_STYLE_SHEET_H
