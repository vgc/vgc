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

#include <vgc/ui/button.h>

#include <vgc/core/array.h>
#include <vgc/graphics/strings.h>
#include <vgc/ui/logcategories.h>
#include <vgc/ui/strings.h>

#include <vgc/ui/detail/paintutil.h>

namespace vgc::ui {

Button::Button(Action* action, FlexDirection layoutDirection)
    : Flex(layoutDirection, FlexWrap::NoWrap) {

    addStyleClass(strings::Button);

    iconWidget_ = createChild<Widget>();
    iconWidget_->addStyleClass(core::StringId("Icon"));
    textLabel_ = createChild<Label>();
    textLabel_->addStyleClass(core::StringId("Text"));
    shortcutLabel_ = createChild<Label>();
    shortcutLabel_->addStyleClass(core::StringId("Shortcut"));

    shortcutLabel_->hide();
    iconWidget_->hide();

    setAction(action);
}

ButtonPtr Button::create(Action* action, FlexDirection layoutDirection) {
    return ButtonPtr(new Button(action, layoutDirection));
}

void Button::setAction(Action* action) {
    if (action_ == action) {
        return;
    }
    disconnectOldAction_();
    action_ = action;
    connectNewAction_();
    updateWidgetsBasedOnAction_();
    actionChanged().emit(action);
}

void Button::onDestroyed() {
    action_ = nullptr;
    iconWidget_ = nullptr;
    textLabel_ = nullptr;
    shortcutLabel_ = nullptr;
    SuperClass::onDestroyed();
}

void Button::setActive(bool isActive) {
    if (isActive_ == isActive) {
        return;
    }
    isActive_ = isActive;
    if (isActive) {
        addStyleClass(strings::active);
    }
    else {
        removeStyleClass(strings::active);
    }
}

void Button::setShortcutVisible(bool visible) {
    if (shortcutLabel_) {
        shortcutLabel_->setVisibility(
            visible ? Visibility::Inherit : Visibility::Invisible);
    }
}

bool Button::toggle() {
    if (isClickable()) {
        return action_->toggle();
    }
    else {
        return false;
    }
}

bool Button::click(const geometry::Vec2f& pos) {
    if (isClickable()) {
        if (action_->isCheckable()) {
            action_->toggle();
            // Note: even if toggle() returns false (i.e., the check state
            // didn't change), we still want click() to return true, because
            // the click was indeed performed.
        }
        else {
            action_->trigger(this);
        }
        clicked().emit(this, pos);
        return true;
    }
    else {
        return false;
    }
}

bool Button::onMouseMove(MouseEvent* event) {
    if (isPressed_) {
        if (rect().contains(event->position())) {
            if (!hasStyleClass(strings::pressed)) {
                addStyleClass(strings::pressed);
            }
        }
        else {
            if (hasStyleClass(strings::pressed)) {
                removeStyleClass(strings::pressed);
            }
        }
        return true;
    }
    else {
        return false;
    }
}

bool Button::onMousePress(MouseEvent* event) {
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

bool Button::onMouseRelease(MouseEvent* event) {
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

bool Button::onMouseEnter() {
    addStyleClass(strings::hovered);
    return false;
}

bool Button::onMouseLeave() {
    removeStyleClass(strings::hovered);
    return false;
}

void Button::connectNewAction_() {
    if (action_) {
        action_->aboutToBeDestroyed().connect(onActionAboutToBeDestroyedSlot_());
        action_->propertiesChanged().connect(onActionPropertiesChangedSlot_());
        action_->checkStateChanged().connect(onActionCheckStateChangedSlot_());
    }
}

void Button::disconnectOldAction_() {
    if (action_) {
        action_->aboutToBeDestroyed().disconnect(onActionAboutToBeDestroyedSlot_());
        action_->propertiesChanged().disconnect(onActionPropertiesChangedSlot_());
        action_->checkStateChanged().disconnect(onActionCheckStateChangedSlot_());
    }
}

void Button::updateWidgetsBasedOnAction_() {
    std::string_view text_ = text();
    std::string shortcutText = core::format("{}", shortcut());
    textLabel_->setText(text_);
    shortcutLabel_->setText(shortcutText);

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

void Button::onActionAboutToBeDestroyed_() {
    if (!hasReachedStage(core::ObjectStage::AboutToBeDestroyed)) {
        setAction(nullptr);
    }
}

void Button::onActionPropertiesChanged_() {
    updateWidgetsBasedOnAction_();
    actionChanged().emit(action_);
}

void Button::onActionCheckStateChanged_() {
    updateWidgetsBasedOnAction_();
    actionChanged().emit(action_);
}

} // namespace vgc::ui
