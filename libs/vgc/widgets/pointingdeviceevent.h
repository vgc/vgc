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

#ifndef VGC_WIDGETS_POINTINGDEVICEEVENT_H
#define VGC_WIDGETS_POINTINGDEVICEEVENT_H

#include <QMouseEvent>
#include <QTabletEvent>
#include <vgc/geometry/vec2d.h>
#include <vgc/widgets/api.h>

namespace vgc::widgets {

/// \class vgc::widgets::PointingDeviceEvent
/// \brief Stores information about pointing device events such as mouse
/// events, tablet events, etc.
///
/// Context: in Qt, there exist several classes inheriting QEvent which are
/// about pointing device events, such as QMouseEvent and QTabletEvent. These
/// are handled in various event handlers, such as mousePressEvent(),
/// mouseMoveEvent(), mouseReleaseEvent, and tabletEvent().
///
/// However, it is often useful to handle all these events similarly, which
/// means that we delegate most of the functionality to delegate functions that
/// do not know or care whether the event was emitted from a mouse or a tablet.
/// This PointingDeviceEvent class is a convenient data holder to pass input
/// data to these delegates.
///
/// Note that PointingDeviceEvent does not inherit from QEvent. This is
/// intentional: we do not send this "event" via QCoreApplication::sendEvent()
/// or postEvent(), but rather we just instanciate a PointingDeviceEvent within
/// the existing Qt event handlers and directly call the delegate functions
/// without using the event queue. In a nutshell, this is not an actual Qt
/// event, but a convenient class to implement mouse and tablet events in a
/// uniform manner.
///
/// Currently, only mouse events and tablet events are supported by this class.
/// In the future, we might support other types of pointing device events, such
/// as QTouchEvent or QNativeGestureEvent.
///
/// \sa QEvent, QInputEvent, QMouseEvent, QTabletEvent.
///
class VGC_WIDGETS_API PointingDeviceEvent {
public:
    /// Creates a PointingDeviceEvent from a non-null QMouseEvent \p event.
    ///
    PointingDeviceEvent(QMouseEvent* event);

    /// Creates a PointingDeviceEvent from a non-null QTabletEvent \p event.
    ///
    PointingDeviceEvent(QTabletEvent* event);

    /// Returns the QEvent::Type of this PointingDeviceEvent.
    /// It can be one of these:
    /// - QEvent::MouseButtonPress
    /// - QEvent::MouseMove
    /// - QEvent::MouseButtonRelease
    /// - QEvent::TabletPress
    /// - QEvent::TabletMove
    /// - QEvent::TabletRelease
    ///
    /// \sa isMouseEvent() and isTabletEvent().
    ///
    QEvent::Type type() const;

    /// Returns whether this InputDeviceEvent comes from a QMouseEvent. This is
    /// equivalent to verifying whether type() is either MouseButtonPress,
    /// MouseMove, or MouseButtonRelease.
    ///
    /// \sa type().
    ///
    bool isMouseEvent() const;

    /// Returns whether this InputDeviceEvent comes from a QTabletEvent. This is
    /// equivalent to verifying whether type() is either TabletPress,
    /// TabletMove, or TabletRelease.
    ///
    /// \sa type().
    ///
    bool isTabletEvent() const;

    /// Returns the keyboard modifier flags that existed immediately before the
    /// event occurred.
    ///
    /// \sa QInputEvent::modifiers().
    ///
    Qt::KeyboardModifiers modifiers() const;

    /// Returns the window system's timestamp for this event. It will normally
    /// be in milliseconds since some arbitrary point in time, such as the time
    /// when the system was started.
    ///
    /// \sa QInputEvent::timestamp().
    ///
    unsigned long timestamp() const;

    /// Returns the button that caused the event.
    ///
    /// Note that the returned value is always Qt::NoButton for mouse/tablet
    /// move events.
    ///
    /// \sa buttons(), QMouseEvent::button(), QTabletEvent::button(), and
    /// Qt::MouseButton.
    ///
    Qt::MouseButton button() const;

    /// Returns the button state when the event was generated. The button state
    /// is a combination of Qt::LeftButton, Qt::RightButton, Qt::MiddleButton
    /// using the OR operator. For mouse/tablet move events, this is all buttons that
    /// are pressed down. For mouse/tablet press events this includes
    /// the button that caused the event. For mouse/tablet release events this
    /// excludes the button that caused the event.
    ///
    /// \sa button(), QMouseEvent::buttons(), QTabletEvent::buttons(), and
    /// Qt::MouseButton.
    ///
    Qt::MouseButtons buttons() const;

    /// Returns the position of the cursor, relative to the widget that
    /// received the event.
    ///
    /// \sa QMouseEvent::localPos() and QTabletEvent::posF().
    ///
    geometry::Vec2d pos() const;

    /// Returns whether there is pressure data associated with this
    /// PointingDeviceEvent. This is currently equivalent to isTabletEvent(),
    /// but may be more generic when other pointing device are supported.
    ///
    bool hasPressure() const;

    /// isTabletEvent, but in the future there may
    /// Returns the pressure of this tablet event. Returns 0 whenever
    /// hasPressure() is false.
    ///
    double pressure() const;

private:
    QEvent::Type type_;
    Qt::KeyboardModifiers modifiers_;
    unsigned long timestamp_;
    Qt::MouseButton button_;
    Qt::MouseButtons buttons_;
    geometry::Vec2d pos_;
    bool hasPressure_;
    double pressure_;
};

} // namespace vgc::widgets

#endif // VGC_WIDGETS_POINTINGDEVICEEVENT_H
