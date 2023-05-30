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

#ifndef VGC_UI_EVENT_H
#define VGC_UI_EVENT_H

#include <vgc/core/innercore.h>
#include <vgc/ui/api.h>
#include <vgc/ui/modifierkey.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(Event);
VGC_DECLARE_OBJECT(Widget);

/// \class vgc::ui::Event
/// \brief Base class of all events in the user interface.
///
class VGC_UI_API Event : public core::Object {
private:
    VGC_OBJECT(Event, core::Object)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

protected:
    /// This is an implementation detail. please use Event::create() instead.
    ///
    Event(double timestamp, ModifierKeys modifiers);

public:
    /// Creates an Event.
    ///
    static EventPtr create();

    /// Creates an Event.
    ///
    static EventPtr create(double timestamp = 0, ModifierKeys modifiers = {});

    /// Returns the time at which this event occured, in seconds, since some
    /// arbitrary point in time (for example, the application startup time, or
    /// the system startup time).
    ///
    /// Note that due to platform limitations, this timestamp is not always
    /// accurate. As a general rule of thumbs, it tends to be more accurate
    /// with pen tablet inputs than with mouse input.
    ///
    /// \sa `setTimestamp()`.
    ///
    // XXX Make this a true timestamp (poll time) when possible, rather than
    // the time when Qt added the event to the event queue.
    //
    double timestamp() const {
        return timestamp_;
    }

    /// Sets the time at which this event occured.
    ///
    /// \sa `timestamp()`.
    ///
    void setTimestamp(double t) {
        timestamp_ = t;
    }

    /// Returns the modifier keys (Ctrl, Shift, etc.) that were pressed
    /// when this event was generated.
    ///
    /// \sa `setModifierKeys()`.
    ///
    ModifierKeys modifierKeys() const {
        return modifierKeys_;
    }

    /// Sets the modifier keys of this event.
    ///
    /// \sa `modifierKeys()`.
    ///
    void setModifierKeys(ModifierKeys modifierKeys) {
        modifierKeys_ = modifierKeys;
    }

private:
    friend Widget;

    double timestamp_ = 0;
    ModifierKeys modifierKeys_ = {};
};

/// \class vgc::ui::PropagatedEvent
/// \brief Base class of all events propagated through the widget hierarchy.
///
/// Some events are propagated through the widget hierachy (for example,
/// `Widget::onMouseMove()`), while some events are directly handled without
/// propagation (for example, actions triggered via a shortcut).
///
/// This class is used as base class for all events that do require propagation
/// through the widget hierarchy.
///
/// Note that this class is designed to be used with multiple inheritance in
/// mind, and therefore does not inherit from `Event` itself, in order to avoid
/// the diamond problem.
///
class VGC_UI_API PropagatedEvent {
public:
    /// Creates a `PropagatedEvent`.
    ///
    PropagatedEvent() {
    }

    /// Returns whether a handler requested to stop propagating this event.
    ///
    bool isStopPropagationRequested() const {
        return isStopPropagationRequested_;
    }

    /// Tells the mouse event system to stop propagating this event.
    ///
    void stopPropagation() {
        isStopPropagationRequested_ = true;
    }

    /// Returns whether a handler already handled this event.
    ///
    bool isHandled() const {
        return handled_;
    }

private:
    friend Widget;

    bool isStopPropagationRequested_ = false;
    bool handled_ = false;
};

} // namespace vgc::ui

#endif // VGC_UI_EVENT_H
