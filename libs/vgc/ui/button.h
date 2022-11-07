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

#ifndef VGC_UI_BUTTON_H
#define VGC_UI_BUTTON_H

#include <string>
#include <string_view>

#include <vgc/geometry/vec2f.h>
#include <vgc/graphics/richtext.h>
#include <vgc/ui/flex.h>
#include <vgc/ui/label.h>
#include <vgc/ui/widget.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(Button);

/// \class vgc::ui::Button
/// \brief A clickable widget that represents an action and triggers it on click/hover.
///
/// A button is a clickable widget that triggers an action on click/hover.
///
/// Note that a button can only exist with respect to an action, and therefore
/// self-destroy itself if the action is destroyed.
///
class VGC_UI_API Button : public Flex {
private:
    VGC_OBJECT(Button, Flex)

protected:
    Button(Action* action, FlexDirection layoutDirection);

public:
    /// Creates a Button associated with the given `action`.
    ///
    /// If you do not wish this button to be associated with an action, or if
    /// you want to provide the action later, you can pass `nullptr` to this
    /// function, and later call `setAction()`.
    ///
    static ButtonPtr
    create(Action* action, FlexDirection layoutDirection = FlexDirection::Column);

    /// Returns the action associated with this button.
    /// The is the action that will be trigerred when the button is clicked.
    ///
    Action* action() const {
        return action_;
    }

    /// Sets the action associated with this button.
    ///
    void setAction(Action* action);

    /// This signal is emitted whenever the action associated with this button
    /// changed.
    ///
    /// This can happen either because the properties of the action changed
    /// (e.g., as a result of `action()->setText()`), or because the button is
    /// now associated with another action (e.g., as a result of `setAction()`
    /// was called).
    ///
    /// Note that the `action` parameter of this signal is null if this signal
    /// is a result of setting a null action to the button.
    ///
    VGC_SIGNAL(actionChanged, (Action*, action))

    /// Returns whether this button has an action and this action is enabled.
    ///
    bool isActionEnabled() const {
        return action_ && action_->isEnabled();
    }

    /// Returns whether this button is clickable.
    ///
    /// For now, this is equivalent to `isActionEnabled()`.
    ///
    /// In the future, we are planning to implement `Widget::isEnabled()`, and
    /// a button will be clickable if and only if it is enabled (as a Widget)
    /// and the button's action is enabled.
    ///
    bool isClickable() const {
        return isActionEnabled(); // Later: && Widget::isEnabled();
    }

    /// Returns the Button's text.
    ///
    std::string_view text() const {
        return action_ ? action_->text() : "";
    }

    /// Returns the Button's shortcut.
    ///
    Shortcut shortcut() const {
        return action_ ? action_->shortcut() : Shortcut();
    }

    /// Returns whether the shortcut is visible.
    ///
    bool isShortcutVisible() const {
        return shortcutLabel_ ? shortcutLabel_->visibility() == Visibility::Inherit
                              : false;
    }

    /// Sets whether the shortcut is visible. By default, it is hidden.
    ///
    void setShortcutVisible(bool visible);

    /// Returns the `CheckMode` of the button's action.
    ///
    /// Returns `Unckeckable` if the button has no associated action.
    ///
    /// \sa `Action::setCheckMode()`, `isCheckable()`.
    ///
    CheckMode checkMode() const {
        return action_ ? action_->checkMode() : CheckMode::Uncheckable;
    }

    /// Returns `true` if the `checkMode()` of the `Button` is either
    /// `Bistate` or `Tristate`. Otherwise, return `false`.
    ///
    /// \sa `Action::setCheckable()`, `checkMode()`.
    ///
    bool isCheckable() const {
        return checkMode() != CheckMode::Uncheckable;
    }

    /// Returns the `CheckState` of the button.
    ///
    /// Returns `Unckecked` if the button has no associated action.
    ///
    /// \sa `setCheckState()`, `isChecked()`.
    ///
    CheckState checkState() const {
        return action_ ? action_->checkState() : CheckState::Unchecked;
    }

    /// Returns whether the button supports the given state.
    ///
    /// For `Uncheckable` buttons, the only supported state is `Unchecked`.
    ///
    /// For `Bistate` buttons, the supported states are `Unchecked` and `Checked`.
    ///
    /// For `Tristate` buttons, the supported states are `Unchecked`, `Checked`, and
    /// `Indeterminate`.
    ///
    /// \sa `checkState()`, `checkMode()`.
    ///
    bool supportsCheckState(CheckState checkState) const {
        return checkState == CheckState::Unchecked
               || (action_ && action_->supportsCheckState(checkState));
    }

    /// Returns whether the button's `CheckState` is `Checked`.
    ///
    /// \sa `setChecked()`, `checkState()`.
    ///
    bool isChecked() const {
        return checkState() == CheckState::Checked;
    }

    /// Toggles the check state of the `Button`.
    ///
    /// Returns true if the check state of the button was indeed changed as a
    /// result of calling this function. Examples why the check state doesn't
    /// change are:
    ///
    /// - The button is disabled, or
    /// - The button's action is disabled, or
    /// - the button's action is not checkable, or
    /// - the button's action is already checked and part of an exclusive group.
    ///
    /// \sa `Action::toggle()`, `setCheckState()`, `setChecked()`.
    ///
    bool toggle();
    VGC_SLOT(toggle)

    /// Clicks the button at position `pos` in local coordinates.
    ///
    /// This will cause the clicked signal to be emitted.
    ///
    /// Returns true if the click was effective, that is, if the button was
    /// clickable.
    ///
    /// \sa clicked()
    ///
    // Later: "if the button was clickable" (\sa `isClickable()`).
    //
    bool click(const geometry::Vec2f& pos);
    VGC_SLOT(click)

    /// This signal is emitted when:
    ///
    /// - the button is clicked by the user (i.e., a mouse press
    ///   was followed by a mouse release within the button), or
    ///
    /// - the click() method is called.
    ///
    /// \sa pressed(), released()
    ///
    VGC_SIGNAL(clicked, (Button*, button), (const geometry::Vec2f&, pos));

    /// This signal is emitted when the button is pressed.
    ///
    /// \sa released(), clicked()
    ///
    VGC_SIGNAL(pressed, (Button*, button), (const geometry::Vec2f&, pos));

    /// This signal is emitted when the button is released.
    ///
    /// \sa pressed(), clicked()
    ///
    VGC_SIGNAL(released, (Button*, button), (const geometry::Vec2f&, pos));

    // Reimplementation of Widget virtual methods
    bool onMouseMove(MouseEvent* event) override;
    bool onMousePress(MouseEvent* event) override;
    bool onMouseRelease(MouseEvent* event) override;
    bool onMouseEnter() override;
    bool onMouseLeave() override;

protected:
    // Reimplementation of Object virtual methods
    void onDestroyed() override;

    Widget* iconWidget() const {
        return iconWidget_;
    }

    Label* textLabel() const {
        return textLabel_;
    }

    Label* shortcutLabel() const {
        return shortcutLabel_;
    }

    void setActive(bool isActive);

private:
    // Components
    Action* action_ = nullptr;
    Widget* iconWidget_ = nullptr;
    Label* textLabel_ = nullptr;
    Label* shortcutLabel_ = nullptr;

    // Style
    bool isPressed_ = false;
    bool isActive_ = false;
    core::StringId checkStateStyleClass_;
    core::StringId checkableStyleClass_;
    core::StringId checkModeStyleClass_;
    core::StringId enabledStyleClass_;

    // Behavior
    void connectNewAction_();
    void disconnectOldAction_();
    void updateWidgetsBasedOnAction_();
    void onActionAboutToBeDestroyed_();
    void onActionPropertiesChanged_();
    void onActionCheckStateChanged_();
    void onActionEnabledChanged_();

    VGC_SLOT(onActionAboutToBeDestroyedSlot_, onActionAboutToBeDestroyed_);
    VGC_SLOT(onActionPropertiesChangedSlot_, onActionPropertiesChanged_);
    VGC_SLOT(onActionCheckStateChangedSlot_, onActionCheckStateChanged_);
    VGC_SLOT(onActionEnabledChangedSlot_, onActionEnabledChanged_);
};

} // namespace vgc::ui

#endif // VGC_UI_BUTTON_H
