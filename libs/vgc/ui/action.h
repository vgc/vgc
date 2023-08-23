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

#ifndef VGC_UI_ACTION_H
#define VGC_UI_ACTION_H

#include <string>
#include <string_view>

#include <vgc/core/enum.h>
#include <vgc/core/object.h>
#include <vgc/ui/actiongroup.h>
#include <vgc/ui/api.h>
#include <vgc/ui/command.h>
#include <vgc/ui/shortcut.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(Action);
VGC_DECLARE_OBJECT(Widget);
VGC_DECLARE_OBJECT(Menu);
VGC_DECLARE_OBJECT(MouseEvent);

/// \class vgc::ui::Action
/// \brief Represents an action that can be triggered via menu items, shortcuts, etc.
///
class VGC_UI_API Action : public core::Object {
private:
    VGC_OBJECT(Action, core::Object)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

protected:
    Action(CreateKey, core::StringId commandId);
    Action(CreateKey, core::StringId commandId, std::string_view text);

public:
    /// Creates an action.
    ///
    static ActionPtr create(core::StringId commandId);

    /// Creates an action with the given text.
    ///
    static ActionPtr create(core::StringId commandId, std::string_view text);

    // ----------------------- Action Properties -------------------------------

    /// Returns the command associated with this action.
    ///
    const Command* command() const {
        return command_;
    }

    /// Returns the ID of the command associated with this action.
    ///
    core::StringId commandId() const {
        return command_->id();
    }

    /// Returns the type of the command associated with this action.
    ///
    CommandType type() const {
        return command_->type();
    }

    /// Returns the name of the command associated with this action.
    ///
    /// \sa `text()`, `setText()`.
    ///
    std::string_view name() const {
        return command_->name();
    }

    /// Returns the icon URL of the command associated with this action.
    ///
    std::string_view icon() const {
        return command_->icon();
    }

    /// Returns the displayed text for this action.
    ///
    /// By default, this is the same as the `name()` of the action, but it can
    /// be changed dynamically to something more specific depending on the
    /// context.
    ///
    /// \sa `setText()`
    ///
    std::string_view text() const {
        return text_;
    }

    /// Sets the displayed text for this action.
    ///
    /// \sa `text()`
    ///
    void setText(std::string_view text);

    /// Returns the default shortcuts (as registered in the global
    /// `defaultShortcuts()`) associated with this action.
    ///
    const ShortcutArray& defaultShortcuts() const {
        return ui::defaultShortcuts(commandId());
    }

    /// Returns the users shortcuts (as registered in the global
    /// `userShortcuts()` map) associated with this action.
    ///
    const ShortcutArray& userShortcuts() const {
        return ui::userShortcuts(commandId());
    }

    /// Returns the shortcut context of the command associated with this action.
    ///
    ShortcutContext shortcutContext() const {
        return command_->shortcutContext();
    }

    /// Returns the `CheckMode` of the action.
    ///
    /// \sa `setCheckMode()`, `isCheckable()`.
    ///
    CheckMode checkMode() const {
        return checkMode_;
    }

    /// Sets the `CheckMode` of the `Action`.
    ///
    /// \sa `checkMode()`, `setCheckable()`.
    ///
    void setCheckMode(CheckMode checkMode);
    VGC_SLOT(setCheckMode)

    /// Returns `true` if the `checkMode()` of the `Action` is either
    /// `Bistate` or `Tristate`. Otherwise, return `false`.
    ///
    /// \sa `setCheckable()`, `checkMode()`.
    ///
    bool isCheckable() const {
        return checkMode_ != CheckMode::Uncheckable;
    }

    /// Sets the action's `CheckMode` to either `Bistate` (if `isCheckable`
    /// is true), or `Uncheckable` (if `isCheckable` is false).
    ///
    /// \sa `isCheckable()`, `setCheckMode()`.
    ///
    void setCheckable(bool isCheckable) {
        setCheckMode(isCheckable ? CheckMode::Bistate : CheckMode::Uncheckable);
    }
    VGC_SLOT(setCheckable)

    /// This signal is emitted whenever the action "properties" have changed,
    /// which are: `text()`, `shortcut()`, and `checkMode()`.
    ///
    /// Note that this signal is NOT emitted when `group()`, `isEnabled()`, or
    /// `checkState()` changes or changes. If you wish to listen for these
    /// changes, use `groupChanged()`, `enabledChanged()`, and
    /// `checkStateChanged()` instead.
    ///
    VGC_SIGNAL(propertiesChanged)

    // -------------------------- Action Group ---------------------------------

    /// Returns the `ActionGroup` this `Action` belongs to. Returns `nullptr`
    /// if this `Action` doesn't belong to any `ActionGroup`.
    ///
    ///\sa `setGroup()`, `groupChanged()`, `ActionGroup::addAction()`,
    /// `ActionGroup::removeAction()`.
    ///
    ActionGroup* group() const {
        return group_;
    }

    /// Sets the `ActionGroup` this `Action` belongs to.
    ///
    /// This is equivalent to removing the action from its current group
    /// via `ActionGroup::removeAction(this)`, then adding it to its new group
    /// via `ActionGroup::addAction(this)`.
    ///
    /// \sa `group()`, `groupChanged()`, `ActionGroup::addAction()`,
    /// `ActionGroup::removeAction()`.
    ///
    void setGroup(ActionGroup* group);

    /// This signal is emitted whenever the `group()` of this action changed.
    /// Note that the new group will be `nullptr` if the action isn't part of
    /// any group after the change.
    ///
    /// \sa `group()`, `setGroup()`.
    ///
    VGC_SIGNAL(groupChanged, (ActionGroup*, group))

    // ------------------------- Action State ---------------------------------

    /// Returns whether this action is enabled or not.
    ///
    bool isEnabled() const {
        return isEnabled_;
    }

    /// Sets the disabled state of this action.
    ///
    void setEnabled(bool enabled);
    VGC_SLOT(setEnabled)

    /// This signal is emitted whenever `isEnabled()` changes.
    ///
    VGC_SIGNAL(enabledChanged, (bool, enabled))

    /// Returns the `CheckState` of the action.
    ///
    /// \sa `setCheckState()`, `isChecked()`.
    ///
    CheckState checkState() const {
        return checkState_;
    }

    /// Returns whether the action supports the given state.
    ///
    /// For `Uncheckable` actions, the only supported state is `Unchecked`.
    ///
    /// For `Bistate` actions, the supported states are `Unchecked` and `Checked`.
    ///
    /// For `Tristate` actions, the supported states are `Unchecked`, `Checked`, and
    /// `Indeterminate`.
    ///
    /// \sa `checkState()`, `checkMode()`.
    ///
    bool supportsCheckState(CheckState checkState) const;

    /// Sets the `CheckState` of the action.
    ///
    /// If the action doesn't support the given state (see
    /// `supportsCheckState()`), then the state isn't changed and a warning is
    /// emitted.
    ///
    /// \sa `checkState()`, `setChecked()`.
    ///
    void setCheckState(CheckState checkState);
    VGC_SLOT(setCheckState)

    /// This signal is emitted when the action check state changed.
    ///
    /// \sa setCheckState(), checkState().
    ///
    VGC_SIGNAL(checkStateChanged, (Action*, action), (CheckState, checkState));

    /// Returns whether the action's `CheckState` is `Checked`.
    ///
    /// \sa `setChecked()`, `checkState()`.
    ///
    bool isChecked() const {
        return checkState_ == CheckState::Checked;
    }

    /// Sets the action's `CheckState` to either `Checked` (if `isChecked`
    /// is true), or `Unchecked` (if `isChecked` is false).
    ///
    /// \sa `isChecked()`, `setCheckState()`.
    ///
    void setChecked(bool isChecked) {
        setCheckState(isChecked ? CheckState::Checked : CheckState::Unchecked);
    }
    VGC_SLOT(setChecked)

    /// Toggles the check state of the `Action`.
    ///
    /// This has different meaning depending on the `CheckMode` of the action
    /// as well as the action's group `CheckPolicy`.
    ///
    /// If the action is `Uncheckable` then this function does nothing.
    ///
    /// If the action is `Bistate` then this function switches between
    /// `Checked` and `Unchecked`, unless the action is part of a group whose
    /// policy is `ExactlyOne`, in which case the action stays `Checked` if it
    /// was already `Checked`.
    ///
    /// If the action is `Tristate`:
    /// - If its state is `Indeterminate`, then this function changes its state
    ///   to `Checked`.
    /// - If its state is `Checked` or `Unchecked`, then this function behaves
    ///   as if the action was `Bistate`.
    ///
    /// Note that after calling this function (or clicking on a action), the
    /// action state will never be `Indeterminate`. The `Indeterminate` state
    /// can only be set programatically via `setCheckState()`.
    ///
    /// \sa `setCheckState()`, `setChecked()`.
    ///
    bool toggle();
    VGC_SLOT(toggle)

    /// If the action is not disabled, triggers the action and returns true.
    /// Otherwise returns false.
    ///
    /// This will cause the triggered signal to be emitted.
    ///
    /// \sa `triggered()`.
    ///
    bool trigger(Widget* from = nullptr);
    VGC_SLOT(trigger)

    /// This signal is emitted whenever the action is activated by the user
    /// (for example, clicking on a button associated with this action), or
    /// when the `trigger()` method is called.
    ///
    /// \sa `trigger()`.
    ///
    VGC_SIGNAL(triggered, (Widget*, from))

    // ------------------------- Event Handling ---------------------------------

    // XXX Use subclasses (e.g. MouseDragAction) and define handlers there?

    /// This event handler is called whenever the action is of type `MouseClick`
    /// and the action is triggered, either via mouse press, key press, mouse release,
    /// or key release.
    ///
    /// The default implementation does nothing.
    ///
    virtual void onMouseClick(MouseEvent* event);

    /// This event handler is called whenever the action is of type `MouseDrag`
    /// and the action is initiated, either via mouse press of key press.
    ///
    /// The default implementation does nothing.
    ///
    /// \sa `onMouseDragMove()`, `onMouseDragConfirm()`, `onMouseDragCancel()`.
    ///
    virtual void onMouseDragStart(MouseEvent* event);

    /// This event handler is called whenever the action is of type `MouseDrag`
    /// and the mouse is moved after the action being initiated. The method
    /// `onMouseDragStart()` is guaranteed to have been called before this
    /// method is called.
    ///
    /// The default implementation does nothing.
    ///
    /// \sa `onMouseDragStart()`, `onMouseDragConfirm()`, `onMouseDragCancel()`.
    ///
    virtual void onMouseDragMove(MouseEvent* event);

    /// This event handler is called whenever the action is of type `MouseDrag`
    /// and the user indicates that he wants to finish the action by confirming it.
    ///
    /// This confirmation typically happened via mouse press, key press, mouse
    /// release, or key release.
    ///
    /// The method `onMouseDragStart()` is guaranteed to have been called
    /// before this method is called.
    ///
    /// The default implementation does nothing.
    ///
    /// \sa `onMouseDragStart()`, `onMouseDragMove()`, `onMouseDragCancel()`.
    ///
    virtual void onMouseDragConfirm(MouseEvent* event);

    /// This event handler is called whenever the action is of type `MouseDrag`
    /// and the user indicates that he wants to finish the action by cancelling
    /// it.
    ///
    /// This cancellation typically happened via the `Esc` key press.
    ///
    /// The method `onMouseDragStart()` is guaranteed to have been called
    /// before this method is called.
    ///
    /// The default implementation does nothing.
    ///
    /// \sa `onMouseDragStart()`, `onMouseDragMove()`, `onMouseDragConfirm()`.
    ///
    virtual void onMouseDragCancel(MouseEvent* event);

private:
    friend Menu;
    friend ActionGroup;

    const Command* command_;
    std::string text_;
    ui::ActionGroup* group_ = nullptr;
    bool isEnabled_ = true;
    bool isMenu_ = false;
    CheckMode checkMode_ = CheckMode::Uncheckable;
    CheckState checkState_ = CheckState::Unchecked;
    CheckState lastEmittedCheckState_ = CheckState::Unchecked;

    // Directly sets the new state, ignoring policy and emitting no signals
    void setCheckStateNoEmit_(CheckState newState);

    // Emits checkStateChanged() if current state != last emitted state
    void emitPendingCheckState_();
};

} // namespace vgc::ui

#endif // VGC_UI_ACTION_H
