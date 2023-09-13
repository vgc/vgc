// Copyright 2023 The VGC Developers
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

#include <vgc/ui/checkbox.h>

#include <vgc/core/array.h>
#include <vgc/core/paths.h>
#include <vgc/style/strings.h>
#include <vgc/style/types.h>
#include <vgc/ui/iconwidget.h>
#include <vgc/ui/preferredsizecalculator.h>
#include <vgc/ui/strings.h>

#include <vgc/ui/detail/paintutil.h>

namespace vgc::ui {

namespace {

namespace strings_ {

core::StringId back("back");
core::StringId front("front");

} // namespace strings_

} // namespace

Checkbox::Checkbox(CreateKey key)
    : Widget(key) {

    std::string iconPath = core::resourcePath("ui/icons/checkmark.svg");

    back_ = createChild<Widget>();
    front_ = createChild<IconWidget>(iconPath);

    addStyleClass(strings::Checkbox);
    back_->addStyleClass(strings_::back);
    front_->addStyleClass(strings_::front);
    updateStyleClasses_();

    requestGeometryUpdate();
}

CheckboxPtr Checkbox::create() {
    return core::createObject<Checkbox>();
}

void Checkbox::setCheckMode(CheckMode newMode) {
    if (checkMode_ != newMode) {
        checkMode_ = newMode;

        // Update state if current state is now unsupported
        bool hasCheckStateChanged = false;
        if (!supportsCheckState(checkState_)) {
            setCheckStateNoEmit_(CheckState::Unchecked);
            hasCheckStateChanged = true;
        }

        checkModeChanged().emit(this, checkMode());
        if (hasCheckStateChanged) {
            updateStyleClasses_();
            checkStateChanged().emit(this, checkState());
        }
    }
}

void Checkbox::setCheckState(CheckState newState) {
    if (checkState_ != newState) {
        if (!supportsCheckState(newState)) {
            VGC_WARNING(
                LogVgcUi,
                "Cannot assign {} state to {} checkbox.",
                detail::stateToStringId(newState),
                detail::modeToStringId(checkMode_));
            return;
        }
        setCheckStateNoEmit_(newState);
        updateStyleClasses_();
        checkStateChanged().emit(this, checkState());
    }
}

bool Checkbox::toggle() {
    if (isCheckable()) {
        if (isChecked()) {
            setCheckState(CheckState::Unchecked);
        }
        else {
            setCheckState(CheckState::Checked);
        }
        return true;
    }
    else {
        return false;
    }
}

bool Checkbox::click(const geometry::Vec2f& pos) {
    if (isCheckable()) {
        toggle();
        clicked().emit(this, pos);
        return true;
    }
    else {
        return false;
    }
}

bool Checkbox::onMouseMove(MouseMoveEvent* event) {
    if (isPressed_) {
        if (rect().contains(event->position())) {
            addStyleClass(strings::pressed);
        }
        else {
            removeStyleClass(strings::pressed);
        }
        return true;
    }
    else {
        return false;
    }
}

bool Checkbox::onMousePress(MousePressEvent* event) {
    if (event->button() == MouseButton::Left) {
        pressed().emit(this, event->position());
        addStyleClass(strings::pressed);
        isPressed_ = true;
        return true;
    }
    else {
        return false;
    }
}

bool Checkbox::onMouseRelease(MouseReleaseEvent* event) {
    if (isPressed_ && event->button() == MouseButton::Left) {
        released().emit(this, event->position());
        if (rect().contains(event->position())) {
            click(event->position());
        }
        removeStyleClass(strings::pressed);
        isPressed_ = false;
        return true;
    }
    else {
        return false;
    }
}

void Checkbox::onMouseEnter() {
    addStyleClass(strings::hovered);
}

void Checkbox::onMouseLeave() {
    removeStyleClass(strings::hovered);
}

geometry::Vec2f Checkbox::computePreferredSize() const {
    PreferredSizeCalculator calc(this);
    calc.add(back_->preferredSize());
    calc.addMargin(back_.get());
    calc.addPaddingAndBorder();
    return calc.compute();
}

void Checkbox::updateChildrenGeometry() {

    // Set `back` to fill the whole checkbox content rect.
    // TODO: take into account back's margins?
    geometry::Rect2f rect = contentRect();

    // Center `front` horizontally and vertically
    // TODO: take into account front's margins?
    geometry::Vec2f frontSize = front_->preferredSize();
    geometry::Vec2f frontPosition = rect.position() + 0.5f * (rect.size() - frontSize);

    back_->updateGeometry(rect.position(), rect.size());
    front_->updateGeometry(frontPosition, frontSize);
}

void Checkbox::updateStyleClasses_() {

    // Update `unchecked`, `checked`, `indeterminate` style class
    core::StringId newCheckStateStyleClass = detail::stateToStringId(checkState());
    replaceStyleClass(checkStateStyleClass_, newCheckStateStyleClass);
    checkStateStyleClass_ = newCheckStateStyleClass;

    // Update `uncheckable`, `bistate`, `tristate` style class
    core::StringId newCheckModeStyleClass = detail::modeToStringId(checkMode());
    replaceStyleClass(checkModeStyleClass_, newCheckModeStyleClass);
    checkModeStyleClass_ = newCheckModeStyleClass;

    // Update `checkable` style class
    core::StringId newCheckableStyleClass;
    if (isCheckable()) {
        newCheckableStyleClass = strings::checkable;
    }
    replaceStyleClass(checkableStyleClass_, newCheckableStyleClass);
    checkableStyleClass_ = newCheckableStyleClass;
}

} // namespace vgc::ui
