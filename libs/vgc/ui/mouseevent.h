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
#include <vgc/ui/mousebutton.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(MouseEvent);

/// \class vgc::ui::HoverLockPolicy
/// \brief Specifies hover-locking behavior.
///
/// A policy for widgets to control the behavior of the mouse event system about
/// their hover-lock state. Typically child widgets are by default hover-locked on
/// mouse press and hover-unlocked on mouse release. This means that they keep
/// receiving mouse moves even if the mouse leaves their geometry. This default
/// behavior can be overriden using `ForceLock` or `ForceUnlock`.
///
enum class HoverLockPolicy {
    Default,
    ForceLock,
    ForceUnlock
};

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
        ModifierKeys modifierKeys,
        double timestamp,
        double pressure,
        bool isTablet);

public:
    /// Creates a MouseEvent.
    ///
    static MouseEventPtr create(
        MouseButton button,
        const geometry::Vec2f& position,
        ModifierKeys modifierKeys,
        double timestamp = 0,
        double pressure = -1.0,
        bool isTablet = false);

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

    /// Returns the time at which this event occured, in seconds, since some
    /// arbitrary point in time (for example, the application startup time, or
    /// the system startup time).
    ///
    /// Note that due to platform limitations, this timestamp is not always
    /// accurate. As a general rule of thumbs, it tends to be more accurate
    /// with pen tablet inputs than with mouse input.
    ///
    // XXX Make this a true timestamp (poll time) when possible, rather than
    // the time when Qt added the event to the event queue.
    //
    double timestamp() const {
        return timestamp_;
    }

    /// Returns whether this event comes from a tablet.
    ///
    bool isTablet() const {
        return isTablet_;
    }

    /// Returns whether there is pressure data associated with this event.
    ///
    bool hasPressure() const {
        return hasPressure_;
    }

    /// Returns the pressure of this tablet event. Returns 0 whenever
    /// hasPressure() is false.
    ///
    double pressure() const {
        return pressure_;
    }

    /// Returns the hover-lock policy that should be used when this event
    /// is returned from a handler in the bubbling phase.
    ///
    /// \sa HoverLockPolicy.
    ///
    HoverLockPolicy hoverLockPolicy() const {
        return hoverLockPolicy_;
    }

    /// Sets the hover-lock policy that should be used when this event
    /// is returned from a handler in the bubbling phase.
    ///
    /// \sa HoverLockPolicy.
    ///
    void setHoverLockPolicy(HoverLockPolicy hoverLockPolicy) {
        hoverLockPolicy_ = hoverLockPolicy;
    }

private:
    MouseButton button_ = {};
    geometry::Vec2f position_ = {};
    ModifierKeys modifierKeys_;
    HoverLockPolicy hoverLockPolicy_ = {};
    double timestamp_ = 0;
    double pressure_ = 0.f;
    bool hasPressure_ = false;
    bool isTablet_ = false;
};

} // namespace vgc::ui

#endif // VGC_UI_MOUSEEVENT_H
