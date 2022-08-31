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

#include <vgc/style/stylableobject.h>

namespace vgc::style {

StylableObject::StylableObject() {
}

void StylableObject::setStyleSheet(StyleSheetPtr styleSheet) {
    styleSheet_ = styleSheet;
    updateStyle_();
}

void StylableObject::setStyleSheet(std::string_view string) {
    StyleSheetPtr styleSheet =
        StyleSheet::create(defaultStyleSheet()->propertySpecs(), string);
    setStyleSheet(styleSheet);
}

void StylableObject::addStyleClass(core::StringId class_) {
    if (!styleClasses_.contains(class_)) {
        styleClasses_.add(class_);
        updateStyle_();
    }
}

void StylableObject::removeStyleClass(core::StringId class_) {
    if (styleClasses_.contains(class_)) {
        styleClasses_.remove(class_);
        updateStyle_();
    }
}

void StylableObject::toggleStyleClass(core::StringId class_) {
    styleClasses_.toggle(class_);
    updateStyle_();
}

StyleValue StylableObject::style(core::StringId property) const {
    return getStyleComputedValue_(property);
}

void StylableObject::onStyleChanged() {
}

void StylableObject::updateStyle_() {
    // In this function, we precompute which rule sets match this node and
    // precompute all "cascaded values". Note that "computed values" are
    // computed on the fly based on "cascaded values".
    //
    // We currently iterate over all rule sets of all stylesheets to find which
    // rule sets are matching. In the future, we may want to do some
    // precomputation per stylesheet to make this faster (e.g., the stylesheets
    // could return a list of candidate rule sets based on a given node's
    // id/styles/type).

    // Clear previous data
    styleCachedData_.clear();

    // Get all non-null stylesheets from this node to the root nodes
    StylableObject* root = this;
    for (StylableObject* node = this; //
         node != nullptr;             //
         node = node->parentStylableObject()) {

        const StyleSheet* styleSheet = node->styleSheet();
        if (styleSheet) {
            styleCachedData_.ruleSetSpans.append({styleSheet, 0, 0});
        }
        root = node;
    }
    const StyleSheet* defaultStyleSheet = root->defaultStyleSheet();
    if (defaultStyleSheet) {
        styleCachedData_.ruleSetSpans.append({defaultStyleSheet, 0, 0});
    }

    // Iterate over all stylesheets from the root node to this node, that is,
    // from lower precedence to higher precedence.
    //
    // Then, for each stylesheet, we insert all matching rule sets from lower
    // specificity to higher specificity, preserving order in case of equal
    // specificity.
    //
    auto itBegin = styleCachedData_.ruleSetSpans.rbegin();
    auto itEnd = styleCachedData_.ruleSetSpans.rend();
    for (auto& it = itBegin; it < itEnd; ++it) {
        const StyleSheet* styleSheet = it->styleSheet;
        it->begin = styleCachedData_.ruleSetArray.length();
        for (StyleRuleSet* rule : styleSheet->ruleSets()) {
            bool matches = false;
            StyleSpecificity maxSpecificity{}; // zero-init
            for (StyleSelector* selector : rule->selectors()) {
                if (selector->matches(this)) {
                    matches = true;
                    maxSpecificity = (std::max)(maxSpecificity, selector->specificity());
                }
            }
            if (matches) {
                styleCachedData_.ruleSetArray.append({rule, maxSpecificity});
            }
        }
        it->end = styleCachedData_.ruleSetArray.length();
        std::stable_sort(
            styleCachedData_.ruleSetArray.begin() + it->begin,
            styleCachedData_.ruleSetArray.begin() + it->end,
            [](const auto& p1, const auto& p2) { return p1.second < p2.second; });
    }

    // Compute cascaded values.
    //
    for (const auto& r : styleCachedData_.ruleSetArray) {
        StyleRuleSet* ruleSet = r.first;
        for (StyleDeclaration* declaration : ruleSet->declarations()) {
            styleCachedData_.cascadedValues[declaration->property()] =
                declaration->value();
        }
    }

    // Recursily update childen
    StylableObject* childBegin = firstChildStylableObject();
    StylableObject* childEnd = nullptr;
    for (StylableObject* child = childBegin; //
         child != childEnd;                  //
         child = child->nextSiblingStylableObject()) {
        child->updateStyle_();
    }

    // Notify the object of the change of style
    onStyleChanged();
}

const StylePropertySpec*
StylableObject::getStylePropertySpec_(core::StringId property) const {
    for (const detail::RuleSetSpan& span : styleCachedData_.ruleSetSpans) {
        const StylePropertySpec* res = span.styleSheet->propertySpecs()->get(property);
        if (res) {
            return res;
        }
    }
    return nullptr;
}

// Returns the cascaded value of the given property, that is,
// the value "winning the cascade". See:
//
// https://www.w3.org/TR/css-cascade-4/#cascaded
//
// This takes into account stylesheet precedence (nested stylesheets have
// higher precedence, as if it was a stronger CSS layer), as well as selector
// specificity, and finally order of appearance in a given stylesheet.
//
// This does NOT take into account StylableObject inheritance (i.e., properties
// set of the parent StylableObject are ignored) and does not take into account
// default values.
//
// If there is no declared value for the given property, then
// a value of type StyleValueType::None is returned.
//
StyleValue StylableObject::getStyleCascadedValue_(core::StringId property) const {
    auto search = styleCachedData_.cascadedValues.find(property);
    if (search != styleCachedData_.cascadedValues.end()) {
        return search->second;
    }
    else {
        return StyleValue::none();
    }
}

// Returns the computed value of the given property. See:
//
// https://www.w3.org/TR/css-cascade-4/#computed
//
// This resolves StylableObject inheritance and default values. In other words,
// the returned StyleValue is never of type StyleValueType::Inherit.
// However, the type could be StyleValueType::None if there is no known
// default value for the given property (this can be the case for custom
// properties which are missing from the stylesheet).
//
StyleValue StylableObject::getStyleComputedValue_(core::StringId property) const {
    StyleValue v = getStyleCascadedValue_(property);
    if (v.type() == StyleValueType::None) {
        const StylePropertySpec* spec = getStylePropertySpec_(property);
        if (spec) {
            if (spec->isInherited()) {
                v = StyleValue::inherit();
            }
            else {
                v = spec->initialValue();
            }
        }
        else {
            return v;
        }
    }
    if (v.type() == StyleValueType::Inherit) { // Not a `else if` on purpose
        StylableObject* parent = parentStylableObject();
        if (parent) {
            v = parent->getStyleComputedValue_(property);
        }
        else {
            const StylePropertySpec* spec = getStylePropertySpec_(property);
            if (spec) {
                v = spec->initialValue();
            }
            else {
                v = StyleValue::none();
            }
        }
    }
    return v;
}

} // namespace vgc::style
