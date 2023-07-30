// Copyright 2022 The VGC Developers
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

#ifndef VGC_UI_KEYEVENT_H
#define VGC_UI_KEYEVENT_H

#include <vgc/core/arithmetic.h>

#include <string>

#include <vgc/ui/event.h>
#include <vgc/ui/key.h>
#include <vgc/ui/modifierkey.h>

namespace vgc::ui {

/// \class vgc::ui::KeyEventData
/// \brief A convenient class to store keyboard-related event data.
///
class VGC_UI_API KeyEventData {
public:
    /// Creates a `KeyEventData` with default values.
    ///
    KeyEventData() {
    }

    /// Returns the key that caused a key press or key release event.
    ///
    /// This may return `Key::None`, for example for key press events generated
    /// from complex input methods where `text()` is used instead.
    ///
    Key key() const {
        return key_;
    }

    /// Sets the key of this `KeyEvent`.
    ///
    void setKey(Key key) {
        key_ = key;
    }

    /// Returns the text associated with this key event. This could for example
    /// be composed characters via dead keys or other complex input methods
    /// events.
    ///
    const std::string& text() const {
        return text_;
    }

    /// Sets the text associated with this key event.
    ///
    void setText(std::string text) {
        text_ = std::move(text);
    }

private:
    Key key_ = {};
    std::string text_;
};

VGC_DECLARE_OBJECT(KeyEvent);

/// \class vgc::ui::KeyEvent
/// \brief Class to handle keyboard key presses and key releases.
///
class VGC_UI_API KeyEvent : public Event {
private:
    VGC_OBJECT(KeyEvent, Event)

protected:
    /// This is an implementation details. Please use
    /// KeyEvent::create() instead.
    ///
    KeyEvent(
        CreateKey,
        double timestamp,
        ModifierKeys modifiers,
        const KeyEventData& data);

public:
    /// Creates a KeyEvent.
    ///
    static KeyEventPtr create(
        double timestamp = 0,
        ModifierKeys modifiers = {},
        const KeyEventData& data = {});

    /// Returns the data associated with the KeyEvent as one convenient
    /// aggregate object.
    ///
    const KeyEventData& data() const {
        return data_;
    }

    /// Sets the data associated with the MouseEvent.
    ///
    /// Event handlers should typically not change this.
    ///
    void setData(const KeyEventData& data) {
        data_ = data;
    }

    /// Returns the key that caused a key press or key release event.
    ///
    /// This may return `Key::None`, for example for key press events generated
    /// from complex input methods where `text()` is used instead.
    ///
    Key key() const {
        return data_.key();
    }

    /// Sets the key of this `KeyEvent`.
    ///
    /// Event handlers should typically not change this.
    ///
    void setKey(Key key) {
        data_.setKey(key);
    }

    /// Returns the text associated with this key event. This could for example
    /// be composed characters via dead keys or other complex input methods
    /// events.
    ///
    const std::string& text() const {
        return data_.text();
    }

    /// Sets the text associated with this key event.
    ///
    /// Event handlers should typically not change this.
    ///
    void setText(std::string text) {
        data_.setText(std::move(text));
    }

private:
    KeyEventData data_;
};

VGC_DECLARE_OBJECT(PropagatedKeyEvent);

/// \class vgc::ui::PropagatedKeyEvent
/// \brief Base class for KeyPress and KeyRelease events.
///
class VGC_UI_API PropagatedKeyEvent : public KeyEvent, public PropagatedEvent {
private:
    VGC_OBJECT(PropagatedKeyEvent, KeyEvent)

protected:
    /// Protected constructor for `KeyEventData`. You typically want
    /// to use the public method `KeyEventData::create()` instead.
    ///
    PropagatedKeyEvent(
        CreateKey,
        double timestamp,
        ModifierKeys modifiers,
        const KeyEventData& data);

public:
    /// Creates a MouseEvent.
    ///
    static PropagatedKeyEventPtr create(
        double timestamp = 0,
        ModifierKeys modifiers = {},
        const KeyEventData& data = {});
};

VGC_DECLARE_OBJECT(KeyPressEvent);

/// \class vgc::ui::KeyPressEvent
/// \brief A KeyPress event propagated through the widget hierarchy.
///
class VGC_UI_API KeyPressEvent : public PropagatedKeyEvent {
private:
    VGC_OBJECT(KeyPressEvent, PropagatedKeyEvent)

protected:
    /// Protected constructor for `KeyPressEvent`. You typically want
    /// to use the public method `KeyPressEvent::create()` instead.
    ///
    KeyPressEvent(
        CreateKey,
        double timestamp,
        ModifierKeys modifiers,
        const KeyEventData& data);

public:
    /// Creates a `KeyPressEvent`.
    ///
    static KeyPressEventPtr create(
        double timestamp = 0,
        ModifierKeys modifiers = {},
        const KeyEventData& data = {});
};

VGC_DECLARE_OBJECT(KeyReleaseEvent);

/// \class vgc::ui::KeyReleaseEvent
/// \brief A KeyRelease event propagated through the widget hierarchy.
///
class VGC_UI_API KeyReleaseEvent : public PropagatedKeyEvent {
private:
    VGC_OBJECT(KeyReleaseEvent, PropagatedKeyEvent)

protected:
    /// Protected constructor for `KeyReleaseEvent`. You typically want
    /// to use the public method `KeyReleaseEvent::create()` instead.
    ///
    KeyReleaseEvent(
        CreateKey,
        double timestamp,
        ModifierKeys modifiers,
        const KeyEventData& data);

public:
    /// Creates a `KeyReleaseEvent`.
    ///
    static KeyReleaseEventPtr create(
        double timestamp = 0,
        ModifierKeys modifiers = {},
        const KeyEventData& data = {});
};

} // namespace vgc::ui

#endif // VGC_UI_KEYEVENT_H
