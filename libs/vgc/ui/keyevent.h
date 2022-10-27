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
    KeyEvent(Key key, std::string text, ModifierKeys modifierKeys);

public:
    /// Creates a KeyEvent.
    ///
    static KeyEventPtr create(Key key, std::string text, ModifierKeys modifierKeys);

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
        text_ = text;
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
    Key key_;
    std::string text_;
    ModifierKeys modifierKeys_;
};

} // namespace vgc::ui

#endif // VGC_UI_KEYEVENT_H
