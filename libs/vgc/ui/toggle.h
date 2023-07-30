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

#ifndef VGC_UI_TOGGLE_H
#define VGC_UI_TOGGLE_H

#include <vgc/ui/widget.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(Toggle);

/// \class vgc::ui::Toggle
/// \brief Widget to switch bettwen ON/OFF state.
///
class VGC_UI_API Toggle : public Widget {
private:
    VGC_OBJECT(Toggle, Widget)

protected:
    Toggle(CreateKey);

public:
    /// Creates a Toggle.
    ///
    static TogglePtr create();

    /// Returns whether the toggle state is ON (true) or OFF (false).
    ///
    /// \sa `setState()`, `toggled()`.
    ///
    bool state() const {
        return state_;
    }

    /// Sets whether the toggle state is ON (true) or OFF (false).
    ///
    /// \sa `state()`, `toggled()`.
    ///
    void setState(bool state);

    /// Toggles the toggle.
    ///
    /// Returns true if the state of the toggle was indeed changed as a
    /// result of calling this function. Examples why the state doesn't
    /// change are:
    ///
    /// - The toggle is disabled
    ///
    bool toggle();
    VGC_SLOT(toggle)

    /// Clicks the toggle at position `pos` in local coordinates.
    ///
    /// This will cause the clicked signal to be emitted.
    ///
    /// Returns true if the click was effective, that is, if the toggle was
    /// enabled.
    ///
    /// \sa `clicked()`
    ///
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
    VGC_SIGNAL(clicked, (Toggle*, toggle), (const geometry::Vec2f&, pos));

    /// This signal is emitted when the toggle state changes.
    ///
    /// \sa `state()`, `setState()`.
    ///
    VGC_SIGNAL(toggled, (bool, state))

    /// This signal is emitted when the toggle is pressed, that is, there has
    /// been a mouse press, but not yet a mouse release.
    ///
    /// This means that the toggle is about to be toggled, unless the user
    /// cancels the action (for example, drags the mouse out of the toggle
    /// before releasing the mouse button)
    ///
    /// \sa `released()`, `toggled()`.
    ///
    VGC_SIGNAL(pressed, (Toggle*, toggle), (const geometry::Vec2f&, pos));

    /// This signal is emitted when the button is released.
    ///
    /// \sa `pressed()`, `toggled()`.
    ///
    VGC_SIGNAL(released, (Toggle*, toggle), (const geometry::Vec2f&, pos));

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
    bool state_ = false;
    static constexpr bool off = false;
    static constexpr bool on = true;

    // Subwidgets
    WidgetPtr back_;
    WidgetPtr front_;

    // Style
    bool isPressed_ = false;
    void updateStyleClasses_();
};

} // namespace vgc::ui

#endif // VGC_UI_TOGGLE_H
