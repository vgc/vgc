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
#include <vgc/ui/mousebutton.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(MouseEvent);

/// \class vgc::ui::MouseEventData
/// \brief A convenient class to store mouse-related event data.
///
class VGC_UI_API MouseEventData {
public:
    /// Creates a `MouseEventData` with default values.
    ///
    MouseEventData() {
    }

    /// Returns the mouse button of the event.
    ///
    MouseButton button() const {
        return button_;
    }

    /// Sets the mouse button of the event.
    ///
    void setButton(MouseButton button) {
        button_ = button;
    }

    /// Returns the position of the mouse cursor, in local coordinates, when
    /// the event occured.
    ///
    const geometry::Vec2f& position() const {
        return position_;
    }

    /// Returns the position of the mouse cursor, in local coordinates, when
    /// the event occured.
    ///
    geometry::Vec2f& position() {
        return position_;
    }

    /// Sets the position of the mouse cursor, in local coordinates, when the
    /// event occured.
    ///
    void setPosition(const geometry::Vec2f& position) {
        position_ = position;
    }

    /// Returns whether the event comes from a tablet.
    ///
    bool isTablet() const {
        return isTablet_;
    }

    /// Sets whether the event comes from a tablet.
    ///
    void setTablet(bool isTablet) {
        isTablet_ = isTablet;
    }

    /// Returns whether there is pressure data associated with the event.
    ///
    bool hasPressure() const {
        return hasPressure_;
    }

    /// Sets whether there is pressure data associated with the event.
    ///
    /// If `hasPressure` is false, this also sets the `pressure()` to `0`.
    ///
    void setHasPressure(bool hasPressure) {
        hasPressure_ = hasPressure;
        if (!hasPressure) {
            pressure_ = 0;
        }
    }

    /// Returns the pressure of the event. Returns 0 whenever `hasPressure()`
    /// is false.
    ///
    double pressure() const {
        return pressure_;
    }

    /// Sets the pressure of the event. If `hasPressure()` is false, then this
    /// function does nothing, that is, `pressure()` stays equal to 0.
    ///
    void setPressure(double pressure) {
        if (hasPressure()) {
            pressure_ = pressure;
        }
    }

private:
    MouseButton button_ = {};
    geometry::Vec2f position_ = {};
    double pressure_ = 0;
    bool hasPressure_ = false;
    bool isTablet_ = false;
};

/// \class vgc::ui::MouseEvent
/// \brief Base class to handle all mouse-related events (move, clicks, etc.)
///
class VGC_UI_API MouseEvent : public Event {
private:
    VGC_OBJECT(MouseEvent, Event)

protected:
    /// This is an implementation details. Please use
    /// MouseEvent::create() instead.
    ///
    MouseEvent(double timestamp, ModifierKeys modifiers, const MouseEventData& data);

public:
    /// Creates a MouseEvent.
    ///
    static MouseEventPtr create(
        double timestamp = 0,
        ModifierKeys modifiers = {},
        const MouseEventData& data = {});

    /// Returns the data associated with the MouseEvent as one convenient
    /// aggregate object.
    ///
    const MouseEventData& data() const {
        return data_;
    }

    /// Sets the data associated with the MouseEvent.
    ///
    /// Event handlers should typically not change this.
    ///
    void setData(const MouseEventData& data) {
        data_ = data;
    }

    /// Returns the mouse button that caused a mouse press or mouse release event.
    ///
    /// Returns `MouseButton::None` for mouse move events.
    ///
    MouseButton button() const {
        return data_.button();
    }

    /// Sets the mouse button of this event.
    ///
    /// Event handlers should typically not change this.
    ///
    void setButton(MouseButton button) {
        data_.setButton(button);
    }

    /// Returns the position of the mouse cursor, in local coordinates, when
    /// the event occured.
    ///
    const geometry::Vec2f& position() const {
        return data_.position();
    }

    /// Sets the position of the mouse cursor, in local coordinates, when the
    /// event occured.
    ///
    /// Event handlers should typically not change this, unless implementing
    /// some custom mouse event propagation algorithm that require mapping
    /// coordinates from one space to another.
    ///
    void setPosition(const geometry::Vec2f& position) {
        data_.setPosition(position);
    }

    /// Returns the X-coordinate of the position of the mouse cursor, in local
    /// coordinates, when the event occured. This is equivalent to `pos()[0]`.
    ///
    float x() const {
        return data_.position()[0];
    }

    /// Sets the X-coordinate of the position of the mouse cursor, in local
    /// coordinates. This method should typically only be used when
    /// implementing mouse event propagation.
    ///
    /// Event handlers should typically not change this, unless implementing
    /// some custom mouse event propagation algorithm that require mapping
    /// coordinates from one space to another.
    ///
    void setX(float x) {
        data_.position()[0] = x;
    }

    /// Returns the Y-coordinate of the position of the mouse cursor, in local
    /// coordinates, when the event occurs. This is equivalent to `pos()[1]`.
    ///
    float y() const {
        return position()[1];
    }

    /// Sets the Y-coordinate of the position of the mouse cursor, in local
    /// coordinates.
    ///
    /// Event handlers should typically not change this, unless implementing
    /// some custom mouse event propagation algorithm that require mapping
    /// coordinates from one space to another.
    ///
    void setY(float y) {
        data_.position()[1] = y;
    }

    /// Returns whether this event comes from a tablet.
    ///
    bool isTablet() const {
        return data_.isTablet();
    }

    /// Sets whether the event comes from a tablet.
    ///
    /// Event handlers should typically not change this.
    ///
    void setTablet(bool isTablet) {
        data_.setTablet(isTablet);
    }

    /// Returns whether there is pressure data associated with this event.
    ///
    bool hasPressure() const {
        return data_.hasPressure();
    }

    /// Sets whether there is pressure data associated with the event.
    ///
    /// If `hasPressure` is false, this also sets the `pressure()` to `0`.
    ///
    /// Event handlers should typically not change this.
    ///
    void setHasPressure(bool hasPressure) {
        data_.setHasPressure(hasPressure);
    }

    /// Returns the pressure of this tablet event. Returns 0 whenever
    /// hasPressure() is false.
    ///
    double pressure() const {
        return data_.pressure();
    }

    /// Sets the pressure of the event.
    ///
    /// If `hasPressure()` is false, then this function does nothing, that is,
    /// `pressure()` stays equal to 0.
    ///
    /// Event handlers should typically not change this.
    ///
    void setPressure(double pressure) {
        data_.setPressure(pressure);
    }

private:
    friend Widget;

    MouseEventData data_ = {};
};

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

VGC_DECLARE_OBJECT(PropagatedMouseEvent);

/// \class vgc::ui::PropagatedMouseEvent
/// \brief Base class for MousePress, MouseMove, and MouseRelease events.
///
class VGC_UI_API PropagatedMouseEvent : public MouseEvent, public PropagatedEvent {
private:
    VGC_OBJECT(PropagatedMouseEvent, MouseEvent)

protected:
    /// Protected constructor for `PropagatedMouseEvent`. You typically want
    /// to use the public method `PropagatedMouseEvent::create()` instead.
    ///
    PropagatedMouseEvent(
        double timestamp,
        ModifierKeys modifiers,
        const MouseEventData& data);

public:
    /// Creates a PropagatedMouseEvent.
    ///
    static PropagatedMouseEventPtr create(
        double timestamp = 0,
        ModifierKeys modifiers = {},
        const MouseEventData& data = {});

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
    HoverLockPolicy hoverLockPolicy_ = {};
};

VGC_DECLARE_OBJECT(MousePressEvent);

/// \class vgc::ui::MousePressEvent
/// \brief A MousePress event propagated through the widget hierarchy.
///
class VGC_UI_API MousePressEvent : public PropagatedMouseEvent {
private:
    VGC_OBJECT(MousePressEvent, PropagatedMouseEvent)

protected:
    /// Protected constructor for `MousePressEvent`. You typically want
    /// to use the public method `MousePressEvent::create()` instead.
    ///
    MousePressEvent(double timestamp, ModifierKeys modifiers, const MouseEventData& data);

public:
    /// Creates a `MousePressEvent`.
    ///
    static MousePressEventPtr create(
        double timestamp = 0,
        ModifierKeys modifiers = {},
        const MouseEventData& data = {});
};

VGC_DECLARE_OBJECT(MouseMoveEvent);

/// \class vgc::ui::MouseMoveEvent
/// \brief A MouseMove event propagated through the widget hierarchy.
///
class VGC_UI_API MouseMoveEvent : public PropagatedMouseEvent {
private:
    VGC_OBJECT(MouseMoveEvent, PropagatedMouseEvent)

protected:
    /// Protected constructor for `MouseMoveEvent`. You typically want
    /// to use the public method `MouseMoveEvent::create()` instead.
    ///
    MouseMoveEvent(double timestamp, ModifierKeys modifiers, const MouseEventData& data);

public:
    /// Creates a `MouseMoveEvent`.
    ///
    static MouseMoveEventPtr create(
        double timestamp = 0,
        ModifierKeys modifiers = {},
        const MouseEventData& data = {});
};

VGC_DECLARE_OBJECT(MouseReleaseEvent);

/// \class vgc::ui::MouseReleaseEvent
/// \brief A MouseRelease event propagated through the widget hierarchy.
///
class VGC_UI_API MouseReleaseEvent : public PropagatedMouseEvent {
private:
    VGC_OBJECT(MouseReleaseEvent, PropagatedMouseEvent)

protected:
    /// Protected constructor for `MouseReleaseEvent`. You typically want
    /// to use the public method `MouseReleaseEvent::create()` instead.
    ///
    MouseReleaseEvent(
        double timestamp,
        ModifierKeys modifiers,
        const MouseEventData& data);

public:
    /// Creates a `MouseReleaseEvent`.
    ///
    static MouseReleaseEventPtr create(
        double timestamp = 0,
        ModifierKeys modifiers = {},
        const MouseEventData& data = {});
};

VGC_DECLARE_OBJECT(MouseHoverEvent);

/// \class vgc::ui::MouseHoverEvent
/// \brief An event delivered when a widget is hovered by the mouse.
///
class VGC_UI_API MouseHoverEvent : public MouseEvent {
private:
    VGC_OBJECT(MouseHoverEvent, MouseEvent)

protected:
    /// Protected constructor for `MouseHoverEvent`. You typically want
    /// to use the public method `MouseHoverEvent::create()` instead.
    ///
    MouseHoverEvent(double timestamp, ModifierKeys modifiers, const MouseEventData& data);

public:
    /// Creates a `MouseReleaseEvent`.
    ///
    static MouseHoverEventPtr create(
        double timestamp = 0,
        ModifierKeys modifiers = {},
        const MouseEventData& data = {});
};

VGC_DECLARE_OBJECT(Widget);
VGC_DECLARE_OBJECT(MouseActionEvent);

/// \class vgc::ui::MouseActionEvent
/// \brief A mouse event delivered to an action, typically initiated via a shortcut.
///
class VGC_UI_API MouseActionEvent : public MouseEvent {
private:
    VGC_OBJECT(MouseActionEvent, MouseEvent)

protected:
    /// Protected constructor for `MouseActionEvent`. You typically want
    /// to use the public method `MouseActionEvent::create()` instead.
    ///
    MouseActionEvent(
        double timestamp,
        ModifierKeys modifiers,
        const MouseEventData& data,
        Widget* widget);

public:
    /// Creates a `MouseActionEvent`.
    ///
    static MouseActionEventPtr create(
        double timestamp = 0,
        ModifierKeys modifiers = {},
        const MouseEventData& data = {},
        Widget* widget = nullptr);

    /// Creates a `MouseActionEvent` whose data are initialized based on the
    /// given mouse event.
    ///
    static MouseActionEventPtr create(const MouseEvent* other, Widget* widget = nullptr);

    /// Returns the widget from which the action was triggered.
    ///
    Widget* widget() const;

    /// Sets the widget from which the action was triggered.
    ///
    /// Event handlers should typically not change this.
    ///
    void setWidget(Widget* widget);

private:
    WidgetPtr widget_;
};

} // namespace vgc::ui

#endif // VGC_UI_MOUSEEVENT_H
