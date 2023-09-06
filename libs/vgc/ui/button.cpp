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

#include <QTimer>

#include <vgc/core/array.h>
#include <vgc/core/paths.h>
#include <vgc/graphics/strings.h>
#include <vgc/ui/iconwidget.h>
#include <vgc/ui/logcategories.h>
#include <vgc/ui/numbersetting.h>
#include <vgc/ui/panelarea.h>
#include <vgc/ui/strings.h>
#include <vgc/ui/tooltip.h>

#include <vgc/ui/detail/paintutil.h>

namespace vgc::ui {

Button::Button(CreateKey key, Action* action, FlexDirection layoutDirection)
    : Flex(key, layoutDirection, FlexWrap::NoWrap) {

    addStyleClass(strings::Button);

    iconWidget_ = createChild<IconWidget>();
    iconWidget_->addStyleClass(strings::icon);
    textLabel_ = createChild<Label>();
    textLabel_->addStyleClass(strings::text);
    shortcutLabel_ = createChild<Label>();
    shortcutLabel_->addStyleClass(strings::shortcut);

    shortcutLabel_->hide();
    iconWidget_->hide();

    setAction(action);
}

ButtonPtr Button::create(Action* action, FlexDirection layoutDirection) {
    return core::createObject<Button>(action, layoutDirection);
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

bool Button::isIconVisible() const {
    return iconWidget_ ? iconWidget_->visibility() == Visibility::Inherit : false;
}

void Button::setIconVisible(bool visible) {
    if (iconWidget_) {
        iconWidget_->setVisibility(visible ? Visibility::Inherit : Visibility::Invisible);
    }
}

bool Button::isShortcutVisible() const {
    return shortcutLabel_ ? shortcutLabel_->visibility() == Visibility::Inherit : false;
}

void Button::setShortcutVisible(bool visible) {
    if (shortcutLabel_) {
        shortcutLabel_->setVisibility(
            visible ? Visibility::Inherit : Visibility::Invisible);
    }
}

bool Button::isTextVisible() const {
    return textLabel_ ? textLabel_->visibility() == Visibility::Inherit : false;
}

void Button::setTextVisible(bool visible) {
    if (textLabel_) {
        textLabel_->setVisibility(visible ? Visibility::Inherit : Visibility::Invisible);
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
        action_->trigger(this);
        clicked().emit(this, pos);
        return true;
    }
    else {
        return false;
    }
}

bool Button::onMouseMove(MouseMoveEvent* event) {
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

bool Button::onMousePress(MousePressEvent* event) {
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

bool Button::onMouseRelease(MouseReleaseEvent* event) {
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

namespace {

namespace options {

ui::NumberSetting* tooltipStartDelay() {
    double defaultValue = 0.75;
    double min = 0;
    double max = 10;
    static ui::NumberSettingPtr setting = createDecimalNumberSetting(
        ui::settings::preferences(),
        "ui.button.tooltipStartDelay",
        "Button Tooltip Start Delay",
        defaultValue,
        min,
        max);
    return setting.get();
}

// The purpose of the stop delay is to keep the tooltip visible long enough for
// the mouse to travel the gap between adjacent buttons. This way, the next
// tooltip can be shown immediately without having to wait for the start delay
// again, and the new tooltip can replaces the old tooltip without blinking
// effect (and possibly in the future, with an animated transition).
//
ui::NumberSetting* tooltipStopDelay() {
    double defaultValue = 0.2;
    double min = 0;
    double max = 10;
    static ui::NumberSettingPtr setting = createDecimalNumberSetting(
        ui::settings::preferences(),
        "ui.button.tooltipStopDelay",
        "Button Tooltip Stop Delay",
        defaultValue,
        min,
        max);
    return setting.get();
}

} // namespace options

ButtonPtr& tooltipButton_() {
    static ButtonPtr button;
    return button;
}

Button* tooltipButton() {
    return tooltipButton_().getIfAlive();
}

TooltipPtr& tooltip_() {
    static TooltipPtr tooltip;
    return tooltip;
}

void destroyTooltip() {
    TooltipPtr& tooltip = tooltip_();
    if (tooltip) {
        tooltip->destroy();
    }
    tooltip = nullptr;
    tooltipButton_() = nullptr;
}

// TODO: reuse existing tooltip if in same window?
//       animate from old geometry to new geometry?
//
TooltipPtr getOrCreateTooltip(Button* button) {
    TooltipPtr& tooltip = tooltip_();
    if (tooltip) {
        tooltip->destroy();
    }
    tooltip = Tooltip::create();
    tooltipButton_() = button;
    return tooltip;
}

void showTooltip() {

    Button* button = tooltipButton();
    if (!button) {
        return;
    }

    Action* action = button->action();
    if (button->isTooltipEnabled() && action) {

        // Setup dialog content
        TooltipPtr tooltip = getOrCreateTooltip(button);
        tooltip->setText(action->name());
        const ShortcutArray& shortcuts = action->userShortcuts();
        if (shortcuts.isEmpty()) {
            tooltip->setShortcutVisible(false);
        }
        else {
            tooltip->setShortcut(shortcuts.first());
        }

        // Detect if widget is part of a PanelArea for better dialog location.
        PanelArea* area = nullptr;
        Widget* widget = button->parent();
        while (widget) {
            area = dynamic_cast<PanelArea*>(widget);
            if (area) {
                break;
            }
            widget = widget->parent();
        }

        // Show dialog
        if (area) {
            // TODO: decide left or right based on where is the area?
            tooltip->showAt(area, button, geometry::RectAlign::OutRight);
        }
        else {
            tooltip->showAt(button, geometry::RectAlign::OutBottomOutRight);
        }
    }
}

QTimer& createTooltipStartTimer_() {
    static QTimer timer;
    timer.setSingleShot(true);
    timer.callOnTimeout([] { showTooltip(); });
    return timer;
}

QTimer& tooltipStartTimer_() {
    static QTimer& timer = createTooltipStartTimer_();
    return timer;
}

QTimer& createTooltipStopTimer_() {
    static QTimer timer;
    timer.setSingleShot(true);
    timer.callOnTimeout([] { destroyTooltip(); });
    return timer;
}

QTimer& tooltipStopTimer_() {
    static QTimer& timer = createTooltipStopTimer_();
    return timer;
}

void setTimerInterval(QTimer& timer, ui::NumberSetting* setting) {
    double seconds = setting->value();
    int milliseconds = core::narrow_cast<int>(seconds * 1000);
    timer.setInterval(milliseconds);
}

void startTooltipStartTimer() {
    setTimerInterval(tooltipStartTimer_(), options::tooltipStartDelay());
    tooltipStopTimer_().stop();
    tooltipStartTimer_().start();
}

void startTooltipStopTimer() {
    setTimerInterval(tooltipStopTimer_(), options::tooltipStopDelay());
    tooltipStartTimer_().stop();
    tooltipStopTimer_().start();
}

void onTooltipEnter(Button* button) {
    tooltipButton_() = button;
    if (tooltip_()) {
        tooltipStopTimer_().stop();
        showTooltip();
    }
    else {
        startTooltipStartTimer();
    }
}

void onTooltipLeave(Button*) {
    startTooltipStopTimer();
}

} // namespace

void Button::onMouseEnter() {
    addStyleClass(strings::hovered);
    onTooltipEnter(this);
}

void Button::onMouseLeave() {
    removeStyleClass(strings::hovered);
    onTooltipLeave(this);
}

void Button::connectNewAction_() {
    if (action_) {
        action_->aboutToBeDestroyed().connect(onActionAboutToBeDestroyedSlot_());
        action_->propertiesChanged().connect(onActionPropertiesChangedSlot_());
        action_->checkStateChanged().connect(onActionCheckStateChangedSlot_());
        action_->enabledChanged().connect(onActionEnabledChangedSlot_());
        ui::userShortcuts()->changed().connect(onUserShortcutsChangedSlot_());
    }
}

void Button::disconnectOldAction_() {
    if (action_) {
        action_->aboutToBeDestroyed().disconnect(onActionAboutToBeDestroyedSlot_());
        action_->propertiesChanged().disconnect(onActionPropertiesChangedSlot_());
        action_->checkStateChanged().disconnect(onActionCheckStateChangedSlot_());
        action_->enabledChanged().disconnect(onActionEnabledChangedSlot_());
        ui::userShortcuts()->changed().disconnect(onUserShortcutsChangedSlot_());
    }
}

void Button::updateWidgetsBasedOnAction_() {

    // Update icon
    std::string iconFilePath;
    if (action_) {
        std::string_view iconUrl = action_->icon();
        iconFilePath = !iconUrl.empty() ? core::resourcePath(iconUrl) : "";
    }
    iconWidget_->setFilePath(iconFilePath);

    // Update text
    std::string_view text_ = text();
    textLabel_->setText(text_);

    // Update shortcut text
    std::string shortcutText;
    if (action_) {
        const ShortcutArray& shortcuts = action_->userShortcuts();
        if (!shortcuts.isEmpty()) {
            // We display the first shortcut, which is considered to be the
            // "primary" shortcut.
            shortcutText = core::format("{}", shortcuts.first());
        }
    }
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

    // Update `enabled`/`disabled` style class
    core::StringId newEnabledStyleClass;
    if (isActionEnabled()) {
        newEnabledStyleClass = strings::enabled;
    }
    else {
        newEnabledStyleClass = strings::disabled;
    }
    replaceStyleClass(enabledStyleClass_, newEnabledStyleClass);
    enabledStyleClass_ = newEnabledStyleClass;
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

void Button::onActionEnabledChanged_() {
    updateWidgetsBasedOnAction_();
    actionChanged().emit(action_);
}

void Button::onUserShortcutsChanged_() {
    updateWidgetsBasedOnAction_();
}

} // namespace vgc::ui
