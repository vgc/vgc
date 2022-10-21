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

#include <vgc/style/logcategories.h>

namespace vgc::style {

StylableObject::StylableObject() {

    // Create the spec table. Note that we do not populate it here since
    // calling the populateStyleSpecTableVirtual() would be pointless: it would
    // not be dispatched to the subclass implementation. We populate instead in
    // updateStyle_().
    //
    styleSpecTable_ = std::make_shared<style::SpecTable>();
}

StylableObjectPtr StylableObject::create() {
    return StylableObjectPtr(new StylableObject);
}

void StylableObject::setStyleSheet(StyleSheetPtr styleSheet) {
    styleSheet_ = styleSheet;
    updateStyle_();
}

void StylableObject::setStyleSheet(std::string_view string) {
    setStyleSheet(StyleSheet::create(string));
}

void StylableObject::addStyleClass(core::StringId class_) {
    if (isAlive() && !styleClasses_.contains(class_)) {
        styleClasses_.add(class_);
        updateStyle_();
    }
}

void StylableObject::removeStyleClass(core::StringId class_) {
    if (isAlive() && styleClasses_.contains(class_)) {
        styleClasses_.remove(class_);
        updateStyle_();
    }
}

void StylableObject::toggleStyleClass(core::StringId class_) {
    if (isAlive()) {
        styleClasses_.toggle(class_);
        updateStyle_();
    }
}

void StylableObject::replaceStyleClass(core::StringId oldClass, core::StringId newClass) {
    if (oldClass == newClass) {
        return;
    }
    if (isAlive()) {
        bool changed = false;
        if (styleClasses_.contains(oldClass)) {
            styleClasses_.remove(oldClass);
            changed = true;
        }
        if (!styleClasses_.contains(newClass)) {
            styleClasses_.add(newClass);
            changed = true;
        }
        if (changed) {
            updateStyle_();
        }
    }
}

StyleValue StylableObject::style(core::StringId property) const {
    return getStyleComputedValue_(property);
}

void StylableObject::appendChildStylableObject(StylableObject* child) {

    StylableObject* oldParent = child->parentStylableObject();
    StylableObject* newParent = this;

    // Remove from previous parent if any
    if (oldParent) {
        oldParent->removeChildStylableObject(child);
    }

    // Update hierarchy
    childStylableObjects_.append(child);
    child->parentStylableObject_ = newParent;

    // Update style (which will indirectly merge the spec tables)
    child->updateStyle_();
}

void StylableObject::removeChildStylableObject(StylableObject* child) {
    if (child->parentStylableObject_ != this) {
        VGC_WARNING(
            LogVgcStyle,
            "Cannot remove child StylableObject {}: it isn't a child of {}",
            ptr(child),
            ptr(this));
        return;
    }
    childStylableObjects_.removeOne(child);
    child->parentStylableObject_ = nullptr;
    child->updateStyle_();
}

void StylableObject::onStyleChanged() {
}

void StylableObject::populateStyleSpecTable(SpecTable*) {
    // nothing
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

    // Ensure that we use the same spec table as our parent,
    // and that the object is properly registered in the spec table.
    //
    // Note that it's best to do this is and not sooner (e.g., in the
    // constructor of `StylableObject`), because here it's more likely that the
    // object is fully constructed, and therefore the virtuall call to
    // populateStyleSpecTable() is properly dispatched to the subclass.
    //
    if (parentStylableObject()
        && parentStylableObject()->styleSpecTable() != styleSpecTable()) {

        styleSpecTable_ = parentStylableObject()->styleSpecTable_;
    }
    populateStyleSpecTableVirtual(styleSpecTable_.get());

    // Get all non-null stylesheets from this node to the root nodes
    for (StylableObject* node = this; //
         node != nullptr;             //
         node = node->parentStylableObject()) {

        const StyleSheet* styleSheet = node->styleSheet();
        if (styleSheet) {
            styleCachedData_.ruleSetSpans.append({styleSheet, 0, 0});
        }
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
                &declaration->value();
        }
    }

    // Recursily update childen
    for (StylableObject* child : childStylableObjects_) {
        child->updateStyle_();
    }

    // Notify the object of the change of style
    onStyleChanged();
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
const StyleValue* StylableObject::getStyleCascadedValue_(core::StringId property) const {

    // Defines a `None` value that we can return as const pointer
    static StyleValue noneValue = StyleValue::none();

    auto search = styleCachedData_.cascadedValues.find(property);
    if (search != styleCachedData_.cascadedValues.end()) {
        return search->second;
    }
    else {
        return &noneValue;
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
const StyleValue& StylableObject::getStyleComputedValue_(core::StringId property) const {

    // Defines values that we can return as const ref
    static StyleValue noneValue = StyleValue::none();
    static StyleValue inheritValue = StyleValue::inherit();

    // Get the cascaded value
    const StyleValue* res = getStyleCascadedValue_(property);

    // Parse the value if not yet parsed, becomes `None` if parsing fails
    if (res->type() == StyleValueType::Unparsed) {
        StyleValue* mutableValue = const_cast<StyleValue*>(res);
        mutableValue->parse_(styleSpecTable()->get(property));
    }

    // If there is no cascaded value, try to see if we should inherit
    if (res->type() == StyleValueType::None) {
        const StylePropertySpec* spec = styleSpecTable()->get(property);
        if (spec) {
            if (spec->isInherited()) {
                res = &inheritValue;
            }
            else {
                res = &spec->initialValue();
            }
        }
        else {
            return *res;
        }
    }

    // Get value from ancestors if inherited
    if (res->type() == StyleValueType::Inherit) {
        StylableObject* parent = parentStylableObject();
        if (parent) {
            res = &parent->getStyleComputedValue_(property);
        }
        else {
            const StylePropertySpec* spec = styleSpecTable()->get(property);
            if (spec) {
                res = &spec->initialValue();
            }
            else {
                res = &noneValue;
            }
        }
    }

    return *res;
}

} // namespace vgc::style
