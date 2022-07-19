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

#ifndef VGC_STYLE_STYLABLEOBJECT_H
#define VGC_STYLE_STYLABLEOBJECT_H

#include <vgc/core/array.h>
#include <vgc/core/object.h>
#include <vgc/core/stringid.h>

#include <vgc/style/api.h>
#include <vgc/style/style.h>

namespace vgc::style {

VGC_DECLARE_OBJECT(StylableObject);

/// \typedef vgc::style::ClassSet
/// \brief Stores a set of style classes.
///
/// Each StylableObject is assigned a set of classes (e.g., "Button", "on") which can
/// be used to apply different styles to different widgets, or select a subset
/// of widgets in the application.
///
/// ```cpp
/// for (core::StringId class_ : widget->styleClasses()) {
///     if (class_ == "someclass") {
///         // ...
///     }
/// }
/// ```
///
/// Note that ClassSet guarantees that the same class cannot be added
/// twice, that is, if you call add() twice with the same class, then it is
/// added only once. Therefore, you always get a sequence of unique class names
/// when iterating over a ClassSet.
///
class VGC_STYLE_API ClassSet {
public:
    using const_iterator = typename core::Array<core::StringId>::const_iterator;

    /// Returns an iterator to the first class.
    ///
    const_iterator begin() const {
        return a_.begin();
    }

    /// Returns an iterator to the past-the-last class.
    ///
    const_iterator end() const {
        return a_.end();
    }

    /// Returns whether this set of classes contains the
    /// given class.
    ///
    bool contains(core::StringId class_) const {
        return a_.contains(class_);
    }

    /// Adds a class.
    ///
    void add(core::StringId class_) {
        if(!contains(class_)) {
            a_.append(class_);
        }
    }

    /// Removes a class.
    ///
    void remove(core::StringId class_) {
        a_.removeOne(class_);
    }

    /// Adds the class to the list if it's not already there,
    /// otherwise removes the class.
    ///
    void toggle(core::StringId class_) {
        auto it = std::find(begin(), end(), class_);
        if (it == end()) {
            a_.append(class_);
        }
        else {
            a_.erase(it);
        }
    }

private:
    // The array storing all the strings.
    //
    // Note 1: we use StringId instead of std::string because there is
    // typically only a fixed number of class names, which are reused by many
    // objects. This makes comparing between strings faster, and reduce memory
    // usage.
    //
    // Note 2: we use an array rather than an std::set or std::unordered_set
    // because it's typically very small, so an array is most likely faster
    // even for searching. In fact, it might even be a good idea to use a
    // SmallArray<n, T> instead, that is, an array that directly stores `n`
    // value in the stack, similar to the small-string optimization. But at the
    // time of writing, we haven't implemented such class yet.
    //
    core::Array<core::StringId> a_;
};

namespace internal {

using RuleSetArray = core::Array<std::pair<StyleRuleSet*, StyleSpecificity>>;

struct RuleSetSpan {
    const StyleSheet* styleSheet;
    Int begin;
    Int end;
};

using RuleSetSpans = core::Array<RuleSetSpan>;

struct StyleCachedData {

    // Buffer to compute and store which RuleSets from which StyleSheets
    // matches a given StylableObject. The StyleSheets are stored in
    // ruleSetSpans from higher precedence to lower precedence, and
    // the RuleSets are stored in ruleSetArray from lower specificity
    // to higher specificity.
    RuleSetArray ruleSetArray;
    RuleSetSpans ruleSetSpans;

    // Stores all cascaded values for a given StylableObject
    // TODO: share this data across all StylableObject that
    // have the same ruleSetArray and ruleSetSpans.
    std::unordered_map<core::StringId, StyleValue> cascadedValues;

    void clear() {
        ruleSetArray.clear();
        ruleSetSpans.clear();
        cascadedValues.clear();
    }
};

} // namespace internal

/// \typedef vgc::style::StylableObject
/// \brief Base abstract that must be implemented to use the style engine.
///
class VGC_STYLE_API StylableObject : public core::Object {
private:
    VGC_OBJECT(StylableObject, core::Object)

public:
    /// Sets the `StyleSheet` of this `StylableObject`.
    ///
    /// This style sheet affects both this object and all its descendants.
    ///
    /// This style sheet has a higher priority than the style sheets of ancestor
    /// objects. In other words, rules from this style sheet always win over
    /// rules from any ancestor style sheet, regardless of the selectors
    /// specificity.
    ///
    /// This behavior is similar to CSS layers, although in our case the style
    /// sheet is "scoped" (that is, it only applies to this objects and its
    /// descendants), while CSS does not support scoped style sheet.
    ///
    void setStyleSheet(StyleSheetPtr styleSheet);

    /// Overload of `setStyleSheet(StyleSheetPtr)` that creates and sets a style
    /// sheet from the given string and uses the same `StylePropertySpecTable`
    /// as the `defaultStyleSheet()`.
    ///
    void setStyleSheet(std::string_view string);

    /// Returns the style sheet of this StylableObject.
    ///
    const StyleSheet* styleSheet() const {
        return styleSheet_.get();
    }

    /// Returns the style classes of this object.
    ///
    ClassSet styleClasses() {
        return styleClasses_;
    }

    /// Returns whether this widget is assigned the given class.
    ///
    bool hasStyleClass(core::StringId class_) const {
        return styleClasses_.contains(class_);
    }

    /// Adds the given class to this widget.
    ///
    void addStyleClass(core::StringId class_);

    /// Removes the given class to this widget.
    ///
    void removeStyleClass(core::StringId class_) ;

    /// Toggles the given class to this widget.
    ///
    void toggleStyleClass(core::StringId class_);

    /// Returns the computed value of a given style property of this widget.
    ///
    StyleValue style(core::StringId property) const;

    virtual StylableObject* parentStylableObject() const = 0;
    virtual StylableObject* firstChildStylableObject() const = 0;
    virtual StylableObject* lastChildStylableObject() const = 0;
    virtual StylableObject* previousSiblingStylableObject() const = 0;
    virtual StylableObject* nextSiblingStylableObject() const = 0;

    /// A stylesheet that is used by default if no other stylesheet provides
    /// a value for the given property.
    ///
    virtual const StyleSheet* defaultStyleSheet() const = 0;

protected :
    StylableObject();

private:
    StyleSheetPtr styleSheet_;
    ClassSet styleClasses_;
    internal::StyleCachedData styleCachedData_;

    void updateStyle_();
    const StylePropertySpec* getStylePropertySpec_(core::StringId property) const;
    StyleValue getStyleCascadedValue_(core::StringId property) const;
    StyleValue getStyleComputedValue_(core::StringId property) const;
};

} // namespace vgc::style

#endif // VGC_STYLE_STYLABLEOBJECT_H
