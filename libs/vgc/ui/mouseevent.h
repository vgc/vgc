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

#ifndef VGC_UI_MOUSEEVENT_H
#define VGC_UI_MOUSEEVENT_H

#include <vgc/geometry/vec2f.h>
#include <vgc/ui/event.h>
#include <vgc/ui/modifierkey.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(MouseEvent);

// clang-format off

/// \class vgc::ui::MouseButton
/// \brief Enumeration of all possible mouse buttons
///
enum class MouseButton : UInt32 {
    None    = 0x00000000,
    Left    = 0x00000001,
    Right   = 0x00000002,
    Middle  = 0x00000004,
    Extra1  = 0x00000008,
    Extra2  = 0x00000010,
    Extra3  = 0x00000020,
    Extra4  = 0x00000040,
    Extra5  = 0x00000080,
    Extra6  = 0x00000100,
    Extra7  = 0x00000200,
    Extra8  = 0x00000400,
    Extra9  = 0x00000800,
    Extra10 = 0x00001000,
    Extra11 = 0x00002000,
    Extra12 = 0x00004000,
    Extra13 = 0x00008000,
    Extra14 = 0x00010000,
    Extra15 = 0x00020000,
    Extra16 = 0x00040000,
    Extra17 = 0x00080000,
    Extra18 = 0x00100000,
    Extra19 = 0x00200000,
    Extra20 = 0x00400000,
    Extra21 = 0x00800000,
    Extra22 = 0x01000000,
    Extra23 = 0x02000000,
    Extra24 = 0x04000000
};

// clang-format on

/// \class vgc::ui::MouseEvent
/// \brief Class to handle mouse move, clicks, etc.
///
class VGC_UI_API MouseEvent : public Event {
private:
    VGC_OBJECT(MouseEvent, Event)

protected:
    /// This is an implementation details. Please use
    /// MouseEvent::create() instead.
    ///
    MouseEvent(
        MouseButton button,
        const geometry::Vec2f& position,
        ModifierKeys modifierKeys);

public:
    /// Creates a MouseEvent.
    ///
    static MouseEventPtr create(
        MouseButton button,
        const geometry::Vec2f& position,
        ModifierKeys modifierKeys);

    /// Returns the mouse button that caused a mouse press or mouse release event.
    /// Returns `MouseButton::None` for mouse move events.
    ///
    MouseButton button() const {
        return button_;
    }

    /// Sets the mouse button of this `MouseEvent`.
    ///
    void setButton(MouseButton button) {
        button_ = button;
    }

    /// Returns the position of the mouse cursor, in local coordinates, when
    /// the event occurs.
    ///
    const geometry::Vec2f& position() const {
        return position_;
    }

    /// Sets the position of the mouse cursor, in local coordinates. This
    /// method should typically only be used when implementing mouse event
    /// propagation.
    ///
    void setPosition(const geometry::Vec2f& position) {
        position_ = position;
    }

    /// Returns the X-coordinate of the position of the mouse cursor, in local
    /// coordinates, when the event occurs. This is equivalent to `pos()[0]`.
    ///
    float x() const {
        return position_[0];
    }

    /// Sets the X-coordinate of the position of the mouse cursor, in local
    /// coordinates. This method should typically only be used when
    /// implementing mouse event propagation.
    ///
    void setX(float x) {
        position_[0] = x;
    }

    /// Returns the Y-coordinate of the position of the mouse cursor, in local
    /// coordinates, when the event occurs. This is equivalent to `pos()[1]`.
    ///
    float y() const {
        return position_[1];
    }

    /// Sets the Y-coordinate of the position of the mouse cursor, in local
    /// coordinates. This method should typically only be used when
    /// implementing mouse event propagation.
    ///
    void setY(float y) {
        position_[1] = y;
    }

    /// Returns the modifier keys (Ctrl, Shift, etc.) that were pressed
    /// when this even was generated.
    ///
    ModifierKeys modifierKeys() const {
        return modifierKeys_;
    }

    /// Sets the modifier keys of this event.
    ///
    void setModifierKeys(ModifierKeys modifierKeys) {
        modifierKeys_ = modifierKeys;
    }

private:
    MouseButton button_;
    geometry::Vec2f position_;
    ModifierKeys modifierKeys_;
};

} // namespace vgc::ui

#endif // VGC_UI_MOUSEEVENT_H