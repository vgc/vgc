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

#ifndef VGC_UI_CHECKBOX_H
#define VGC_UI_CHECKBOX_H

#include <vgc/ui/widget.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(IconWidget);
VGC_DECLARE_OBJECT(Checkbox);

/// \class vgc::ui::Checkbox
/// \brief Widget to switch between unchecked, checked, and indeterminate state.
///
class VGC_UI_API Checkbox : public Widget {
private:
    VGC_OBJECT(Checkbox, Widget)

protected:
    Checkbox(CreateKey);

public:
    /// Creates a Checkbox.
    ///
    static CheckboxPtr create();

    /// Returns the `CheckMode` of the checkbox.
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

    /// This signal is emitted when the `CheckMode` of the checkbox changed.
    ///
    /// \sa `setCheckMode()`, `checkMode()`.
    ///
    VGC_SIGNAL(checkModeChanged, (Checkbox*, checkbox), (CheckMode, checkMode))

    /// Returns `true` if the `checkMode()` of the checkbox is either
    /// `Bistate` or `Tristate`. Otherwise, return `false`.
    ///
    /// \sa `setCheckable()`, `checkMode()`.
    ///
    bool isCheckable() const {
        return checkMode_ != CheckMode::Uncheckable;
    }

    /// Sets the checkbox's `CheckMode` to either `Bistate` (if `isCheckable`
    /// is true), or `Uncheckable` (if `isCheckable` is false).
    ///
    /// \sa `isCheckable()`, `setCheckMode()`.
    ///
    void setCheckable(bool isCheckable) {
        setCheckMode(isCheckable ? CheckMode::Bistate : CheckMode::Uncheckable);
    }
    VGC_SLOT(setCheckable)

    /// Returns the `CheckState` of the checkbox.
    ///
    /// \sa `setCheckState()`, `isChecked()`.
    ///
    CheckState checkState() const {
        return checkState_;
    }

    /// Returns whether the checkbox supports the given state.
    ///
    /// For `Uncheckable` checkboxes, the only supported state is `Unchecked`.
    ///
    /// For `Bistate` checkboxes, the supported states are `Unchecked` and `Checked`.
    ///
    /// For `Tristate` checkboxes, the supported states are `Unchecked`, `Checked`, and
    /// `Indeterminate`.
    ///
    /// \sa `checkState()`, `checkMode()`.
    ///
    bool supportsCheckState(CheckState checkState) const {
        return ui::supportsCheckState(checkMode(), checkState);
    }

    /// Sets the `CheckState` of the checkbox.
    ///
    /// If the checkbox doesn't support the given state (see
    /// `supportsCheckState()`), then the state isn't changed and a warning is
    /// emitted.
    ///
    /// \sa `checkState()`, `setChecked()`.
    ///
    void setCheckState(CheckState checkState);
    VGC_SLOT(setCheckState)

    /// This signal is emitted when the checkbox `CheckState` changed.
    ///
    /// \sa `setCheckState()`, `checkState()`.
    ///
    VGC_SIGNAL(checkStateChanged, (Checkbox*, checkbox), (CheckState, checkState))

    /// Returns whether the checkbox's `CheckState` is `Checked`.
    ///
    /// \sa `setChecked()`, `checkState()`.
    ///
    bool isChecked() const {
        return checkState_ == CheckState::Checked;
    }

    /// Sets the checkbox's `CheckState` to either `Checked` (if `isChecked`
    /// is true), or `Unchecked` (if `isChecked` is false).
    ///
    /// \sa `isChecked()`, `setCheckState()`.
    ///
    void setChecked(bool isChecked) {
        setCheckState(isChecked ? CheckState::Checked : CheckState::Unchecked);
    }
    VGC_SLOT(setChecked)

    /// Toggles the checkbox.
    ///
    /// Returns true if the state of the checkbox was indeed changed as a
    /// result of calling this function.
    ///
    bool toggle();
    VGC_SLOT(toggle)

    /// Clicks the checkbox at position `pos` in local coordinates.
    ///
    /// This will cause the clicked signal to be emitted.
    ///
    /// Returns true if the click was effective, that is, if the checkbox was
    /// checked.
    ///
    /// \sa `clicked()`
    ///
    bool click(const geometry::Vec2f& pos);
    VGC_SLOT(click)

    /// This signal is emitted when:
    ///
    /// - the checkbox is clicked by the user (i.e., a mouse press
    ///   was followed by a mouse release within the checkbox), or
    ///
    /// - the `click()` method is called.
    ///
    /// \sa `pressed()`, `released()`.
    ///
    VGC_SIGNAL(clicked, (Checkbox*, checkbox), (const geometry::Vec2f&, pos))

    /// This signal is emitted when the checkbox is pressed, that is, there has
    /// been a mouse press, but not yet a mouse release.
    ///
    /// This means that the checkbox is about to be checkboxd, unless the user
    /// cancels the action (for example, drags the mouse out of the checkbox
    /// before releasing the mouse button)
    ///
    /// \sa `released()`, `clicked()`.
    ///
    VGC_SIGNAL(pressed, (Checkbox*, checkbox), (const geometry::Vec2f&, pos));

    /// This signal is emitted when the button is released.
    ///
    /// \sa `pressed()`, `clicked()`.
    ///
    VGC_SIGNAL(released, (Checkbox*, checkbox), (const geometry::Vec2f&, pos));

protected:
    bool onMouseMove(MouseMoveEvent* event) override;
    bool onMousePress(MousePressEvent* event) override;
    bool onMouseRelease(MouseReleaseEvent* event) override;
    void onMouseEnter() override;
    void onMouseLeave() override;
    geometry::Vec2f computePreferredSize() const override;
    void updateChildrenGeometry() override;

private:
    // State
    CheckMode checkMode_ = CheckMode::Bistate;
    CheckState checkState_ = CheckState::Unchecked;

    // Subwidgets
    WidgetPtr back_;
    IconWidgetPtr front_;

    // Style
    bool isPressed_ = false;
    core::StringId checkStateStyleClass_;
    core::StringId checkableStyleClass_;
    core::StringId checkModeStyleClass_;
    void updateStyleClasses_();

    // Directly sets the new state, ignoring policy and emitting no signals
    void setCheckStateNoEmit_(CheckState newState) {
        checkState_ = newState;
    }
};

} // namespace vgc::ui

#endif // VGC_UI_CHECKBOX_H
