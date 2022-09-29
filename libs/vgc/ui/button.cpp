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

    // XXX temporary
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
    if (action_) {
        // Remove connections with current action
        action_->aboutToBeDestroyed().disconnect(onActionAboutToBeDestroyed_());
        action_->changed().disconnect(onActionChangedSlot_());
    }
    action_ = action;
    if (action_) {
        // Set new connections
        action_->aboutToBeDestroyed().connect(onActionAboutToBeDestroyed_());
        action_->changed().connect(onActionChangedSlot_());

        // Update subwidgets
        onActionChanged_();
    }
}

void Button::onDestroyed() {
    // TODO: others?
    action_ = nullptr;
    iconWidget_ = nullptr;
    textLabel_ = nullptr;
    shortcutLabel_ = nullptr;
    SuperClass::onDestroyed();
}

void Button::onWidgetRemoved(Widget* /*child*/) {
    destroy();
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
                reload_ = true;
                requestRepaint();
            }
        }
        else {
            if (hasStyleClass(strings::pressed)) {
                removeStyleClass(strings::pressed);
                reload_ = true;
                requestRepaint();
            }
        }
        return true;
    }
    else {
        return false;
    }
}

/*
// From ActionButton
bool Button::onMousePress(MouseEvent*) {
    return tryClick_();
}
*/

// From ActionButton
/*
bool Button::tryClick_() {
    if (action_ && action_->trigger(this)) {
        onClicked();
        clicked().emit();
        return true;
    }
    return false;
}
*/

bool Button::onMousePress(MouseEvent* event) {
    if (event->button() == MouseButton::Left) {
        pressed().emit(this, event->position());
        addStyleClass(strings::pressed);
        isPressed_ = true;
        reload_ = true;
        requestRepaint();
        return true;

        // TODO: click now if clickOnPress is true
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
        reload_ = true;
        requestRepaint();
        return true;
    }
    else {
        return false;
    }
}

bool Button::onMouseEnter() {
    addStyleClass(strings::hovered);
    return false;

    // From original Button
    // requestRepaint();
    // return true;
}

bool Button::onMouseLeave() {
    removeStyleClass(strings::hovered);
    return false;

    // From original Button
    // requestRepaint();
    // return true;
}

void Button::onActionChanged_() {
    textLabel_->setText(action_->text());
    shortcutLabel_->setText(core::format("shortcutFor({})", action_->text()));
    requestGeometryUpdate();
    requestRepaint();
}

} // namespace vgc::ui
