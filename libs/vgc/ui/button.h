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
#include <vgc/ui/buttongroup.h>
#include <vgc/ui/widget.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(Button);

/// \class vgc::ui::Button
/// \brief A button widget.
///
class VGC_UI_API Button : public Widget {
private:
    VGC_OBJECT(Button, Widget)

protected:
    /// This is an implementation details. Please use
    /// Button::create(text) instead.
    ///
    Button(std::string_view text);

public:
    /// Creates a Button.
    ///
    static ButtonPtr create();

    /// Creates a Button with the given text.
    ///
    static ButtonPtr create(std::string_view text);

    /// Returns the Button's text.
    ///
    const std::string& text() const {
        return richText_->text();
    }

    /// Sets the Button's text.
    ///
    void setText(std::string_view text);

    /// Returns the `CheckMode` of the button.
    ///
    /// \sa `setCheckMode()`, `isCheckable()`.
    ///
    CheckMode checkMode() const {
        return checkMode_;
    }

    /// Sets the `CheckMode` of the `Button`.
    ///
    /// \sa `checkMode()`, `setCheckable()`.
    ///
    void setCheckMode(CheckMode checkMode);
    VGC_SLOT(setCheckMode)

    /// Returns `true` if the `checkMode()` of the `Button` is either
    /// `Bistate` or `Tristate`. Otherwise, return `false`.
    ///
    /// \sa `setCheckable()`, `checkMode()`.
    ///
    bool isCheckable() const {
        return checkMode_ != CheckMode::Uncheckable;
    }

    /// Sets the button's `CheckMode` to either `Bistate` (if `isCheckable`
    /// is true), or `Uncheckable` (if `isCheckable` is false).
    ///
    /// \sa `isCheckable()`, `setCheckMode()`.
    ///
    void setCheckable(bool isCheckable) {
        setCheckMode(isCheckable ? CheckMode::Bistate : CheckMode::Uncheckable);
    }
    VGC_SLOT(setCheckable)

    /// Returns the `CheckState` of the button.
    ///
    /// \sa `setCheckState()`, `isChecked()`.
    ///
    CheckState checkState() const {
        return checkState_;
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
    bool supportsCheckState(CheckState checkState) const;

    /// Sets the `CheckState` of the button.
    ///
    /// If the button doesn't support the given state (see
    /// `supportsCheckState()`), then the state isn't changed and a warning is
    /// emitted.
    ///
    /// \sa `checkState()`, `setChecked()`.
    ///
    void setCheckState(CheckState checkState);
    VGC_SLOT(setCheckState)

    /// Returns whether the button's `CheckState` is `Checked`.
    ///
    /// \sa `setChecked()`, `checkState()`.
    ///
    bool isChecked() const {
        return checkState_ == CheckState::Checked;
    }

    /// Sets the button's `CheckState` to either `Checked` (if `isChecked`
    /// is true), or `Unchecked` (if `isChecked` is false).
    ///
    /// \sa `isChecked()`, `setCheckState()`.
    ///
    void setChecked(bool isChecked) {
        setCheckState(isChecked ? CheckState::Checked : CheckState::Unchecked);
    }
    VGC_SLOT(setChecked)

    /// Toggles the check state of the `Button`.
    ///
    /// This has different meaning depending on the `CheckMode` of the button
    /// as well as the button's group `CheckPolicy`.
    ///
    /// If the button is `Uncheckable` then this function does nothing.
    ///
    /// If the button is `Bistate` then this function switches between
    /// `Checked` and `Unchecked`, unless the button is part of a group whose
    /// policy is `ExactlyOne`, in which case the button stays `Checked` if it
    /// was already `Checked`.
    ///
    /// If the button is `Tristate`:
    /// - If its state is `Indeterminate`, then this function changes its state
    ///   to `Checked`.
    /// - If its state is `Checked` or `Unchecked`, then this function behaves
    ///   as if the button was `Bistate`.
    ///
    /// Note that after calling this function (or clicking on a button), the
    /// button state will never be `Indeterminate`. The `Indeterminate` state
    /// can only be set programatically via `setCheckState()`.
    ///
    /// \sa `setCheckState()`, `setChecked()`.
    ///
    void toggle();
    VGC_SLOT(toggle)

    /// Returns the `ButtonGroup` this `Button` belongs to. Returns `nullptr`
    /// if this `Button` doesn't belong to any `ButtonGroup`.
    ///
    /// \sa `ButtonGroup::addButton()`, `ButtonGroup::removeButton()`.
    ///
    ButtonGroup* group() const {
        return group_;
    }

    /// Clicks the button at position `pos` in local coordinates.
    ///
    /// This will cause the clicked signal to be emitted.
    ///
    /// \sa clicked()
    ///
    void click(const geometry::Vec2f& pos);

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

    /// This signal is emitted when the button check state changed.
    ///
    /// \sa setCheckState(), checkState().
    ///
    VGC_SIGNAL(checkStateChanged, (Button*, button), (CheckState, checkState));

    // Reimplementation of StylableObject virtual methods
    style::StylableObject* firstChildStylableObject() const override;
    style::StylableObject* lastChildStylableObject() const override;

    // Reimplementation of Widget virtual methods
    void onResize() override;
    void onPaintCreate(graphics::Engine* engine) override;
    void onPaintDraw(graphics::Engine* engine, PaintOptions options) override;
    void onPaintDestroy(graphics::Engine* engine) override;
    bool onMouseMove(MouseEvent* event) override;
    bool onMousePress(MouseEvent* event) override;
    bool onMouseRelease(MouseEvent* event) override;
    bool onMouseEnter() override;
    bool onMouseLeave() override;

protected:
    geometry::Vec2f computePreferredSize() const override;

private:
    friend class ButtonGroup;

    graphics::RichTextPtr richText_;
    graphics::GeometryViewPtr triangles_;
    ui::ButtonGroup* group_ = nullptr;
    bool reload_ = true;
    bool isPressed_ = false;
    CheckMode checkMode_ = CheckMode::Uncheckable;
    CheckState checkState_ = CheckState::Unchecked;
    CheckState lastEmittedCheckState_ = CheckState::Unchecked;
    bool emitting_ = false;

    // Directly sets the new state, ignoring policy and emitting no signals
    void setCheckStateNoEmit_(CheckState newState);

    // Informs the world about the new state:
    // - updates style classes
    // - emits checkStateChanged
    void emitPendingCheckState_();
};

} // namespace vgc::ui

#endif // VGC_UI_BUTTON_H
