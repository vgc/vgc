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

#include <vgc/ui/keyevent.h>

namespace vgc::ui {

KeyEvent::KeyEvent(
    CreateKey key,
    double timestamp,
    ModifierKeys modifiers,
    const KeyEventData& data)

    : Event(key, timestamp, modifiers)
    , data_(data) {
}

KeyEventPtr
KeyEvent::create(double timestamp, ModifierKeys modifiers, const KeyEventData& data) {
    return core::createObject<KeyEvent>(timestamp, modifiers, data);
}

PropagatedKeyEvent::PropagatedKeyEvent(
    CreateKey key,
    double timestamp,
    ModifierKeys modifiers,
    const KeyEventData& data)

    : KeyEvent(key, timestamp, modifiers, data) {
}

PropagatedKeyEventPtr PropagatedKeyEvent::create(
    double timestamp,
    ModifierKeys modifiers,
    const KeyEventData& data) {

    return core::createObject<PropagatedKeyEvent>(timestamp, modifiers, data);
}

KeyPressEvent::KeyPressEvent(
    CreateKey key,
    double timestamp,
    ModifierKeys modifiers,
    const KeyEventData& data)

    : PropagatedKeyEvent(key, timestamp, modifiers, data) {
}

KeyPressEventPtr KeyPressEvent::create(
    double timestamp,
    ModifierKeys modifiers,
    const KeyEventData& data) {

    return core::createObject<KeyPressEvent>(timestamp, modifiers, data);
}

KeyReleaseEvent::KeyReleaseEvent(
    CreateKey key,
    double timestamp,
    ModifierKeys modifiers,
    const KeyEventData& data)

    : PropagatedKeyEvent(key, timestamp, modifiers, data) {
}

KeyReleaseEventPtr KeyReleaseEvent::create(
    double timestamp,
    ModifierKeys modifiers,
    const KeyEventData& data) {

    return core::createObject<KeyReleaseEvent>(timestamp, modifiers, data);
}

} // namespace vgc::ui
