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
#include <vgc/style/types.h>

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

void StylableObject::setStyleSheet(SheetPtr styleSheet) {
    styleSheet_ = styleSheet;
    updateStyle_();
}

void StylableObject::setStyleSheet(std::string_view string) {
    setStyleSheet(Sheet::create(string));
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

Value StylableObject::style(core::StringId property) const {

    Value res = getStyleComputedValue_(property);

    constexpr bool compactMode = false;
    if constexpr (compactMode) {
        std::string_view p(property.string());
        if (p.substr(0, 8) == "padding-" || p.substr(p.size() - 4) == "-gap") {
            LengthOrPercentage lp = res.to<LengthOrPercentage>();
            float newLengthInDp = 3.0f;
            if (lp.isLength()) {
                Length length(lp.value(), lp.unit());
                float lengthInPx = length.toPx(styleMetrics());
                float lengthInDp = lengthInPx / styleMetrics().scaleFactor();
                newLengthInDp = (std::min)(newLengthInDp, lengthInDp);
            }
            else {
                if (lp.value() == 0) {
                    newLengthInDp = 0;
                }
            }
            res = Value::custom(LengthOrPercentage(newLengthInDp, LengthUnit::Dp));
        }
    }

    return res;
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

void StylableObject::setStyleMetrics(const Metrics& metrics) {
    styleMetrics_ = metrics;
    updateStyle_();
};

void StylableObject::updateStyle_() {
    // In this function, we precompute which rule sets match this node and
    // precompute all "cascaded values". Note that "computed values" are
    // computed on the fly based on "cascaded values".
    //
    // We currently iterate over all rule sets of all style sheets to find which
    // rule sets are matching. In the future, we may want to do some
    // precomputation per style sheet to make this faster (e.g., the style
    // sheets could return a list of candidate rule sets based on a given node's
    // id/styles/type).

    // Clear previous data
    styleCache_.clear();

    StylableObject* parent = parentStylableObject();

    // Ensure that we use the same spec table as our parent,
    // and that the object is properly registered in the spec table.
    //
    // Note that it's best to do this is and not sooner (e.g., in the
    // constructor of `StylableObject`), because here it's more likely that the
    // object is fully constructed, and therefore the virtuall call to
    // populateStyleSpecTable() is properly dispatched to the subclass.
    //
    if (parent && parent->styleSpecTable() != styleSpecTable()) {
        styleSpecTable_ = parentStylableObject()->styleSpecTable_;
    }
    populateStyleSpecTableVirtual(styleSpecTable_.get());

    // Update style metrics
    if (parent) {
        styleMetrics_ = parent->styleMetrics();
        // Note: don't call setStyleMetrics() to avoid recursion
    }

    // Get all non-null style sheets from this node to the root nodes
    for (StylableObject* node = this; //
         node != nullptr;             //
         node = node->parentStylableObject()) {

        const Sheet* styleSheet = node->styleSheet();
        if (styleSheet) {
            styleCache_.ruleSetSpans.append({styleSheet, 0, 0});
        }
    }

    // Iterate over all style sheets from the root node to this node, that is,
    // from lower precedence to higher precedence.
    //
    // Then, for each style sheet, we insert all matching rule sets from lower
    // specificity to higher specificity, preserving order in case of equal
    // specificity.
    //
    auto itBegin = styleCache_.ruleSetSpans.rbegin();
    auto itEnd = styleCache_.ruleSetSpans.rend();
    for (auto& it = itBegin; it < itEnd; ++it) {
        const Sheet* styleSheet = it->styleSheet;
        it->begin = styleCache_.ruleSetArray.length();
        for (RuleSet* rule : styleSheet->ruleSets()) {
            bool matches = false;
            Specificity maxSpecificity = 0;
            for (Selector* selector : rule->selectors()) {
                if (selector->matches(this)) {
                    matches = true;
                    maxSpecificity = (std::max)(maxSpecificity, selector->specificity());
                }
            }
            if (matches) {
                styleCache_.ruleSetArray.append({rule, maxSpecificity});
            }
        }
        it->end = styleCache_.ruleSetArray.length();
        std::stable_sort(
            styleCache_.ruleSetArray.begin() + it->begin,
            styleCache_.ruleSetArray.begin() + it->end,
            [](const auto& p1, const auto& p2) { return p1.second < p2.second; });
    }

    // Compute cascaded values.
    //
    for (const auto& r : styleCache_.ruleSetArray) {
        RuleSet* ruleSet = r.first;
        for (Declaration* declaration : ruleSet->declarations()) {
            styleCache_.cascadedValues[declaration->property()] = &declaration->value();
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
// This takes into account style sheet precedence (nested style sheets have
// higher precedence, as if it was a stronger CSS layer), as well as selector
// specificity, and finally order of appearance in a given style sheet.
//
// This does NOT take into account StylableObject inheritance (i.e., properties
// set of the parent StylableObject are ignored) and does not take into account
// default values.
//
// If there is no declared value for the given property, then
// a value of type ValueType::None is returned.
//
const Value* StylableObject::getStyleCascadedValue_(core::StringId property) const {

    // Defines a `None` value that we can return as const pointer
    static Value noneValue = Value::none();

    auto search = styleCache_.cascadedValues.find(property);
    if (search != styleCache_.cascadedValues.end()) {
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
// the returned Value is never of type ValueType::Inherit.
// However, the type could be ValueType::None if there is no known
// default value for the given property (this can be the case for custom
// properties which are missing from the style sheet).
//
const Value& StylableObject::getStyleComputedValue_(core::StringId property) const {

    // Defines values that we can return as const ref
    static Value noneValue = Value::none();
    static Value inheritValue = Value::inherit();

    // Get the cascaded value
    const Value* res = getStyleCascadedValue_(property);

    // Parse the value if not yet parsed, becomes `None` if parsing fails
    if (res->type() == ValueType::Unparsed) {
        Value* mutableValue = const_cast<Value*>(res);
        mutableValue->parse_(styleSpecTable()->get(property));
    }

    // If there is no cascaded value, try to see if we should inherit
    if (res->type() == ValueType::None) {
        const PropertySpec* spec = styleSpecTable()->get(property);
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
    if (res->type() == ValueType::Inherit) {
        StylableObject* parent = parentStylableObject();
        if (parent) {
            res = &parent->getStyleComputedValue_(property);
        }
        else {
            const PropertySpec* spec = styleSpecTable()->get(property);
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
