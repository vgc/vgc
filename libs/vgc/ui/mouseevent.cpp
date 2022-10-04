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

#include <vgc/ui/mouseevent.h>

namespace vgc::ui {

MouseEvent::MouseEvent(
    MouseButton button,
    const geometry::Vec2f& position,
    ModifierKeys modifierKeys,
    uint64_t timestamp,
    double pressure)

    : Event()
    , button_(button)
    , position_(position)
    , modifierKeys_(modifierKeys)
    , timestamp_(timestamp)
    , pressure_((std::max)(0.0, pressure))
    , hasPressure_(pressure >= 0.0) {
}

/* static */
MouseEventPtr MouseEvent::create(
    MouseButton button,
    const geometry::Vec2f& position,
    ModifierKeys modifierKeys,
    uint64_t timestamp,
    double pressure) {

    return MouseEventPtr(
        new MouseEvent(button, position, modifierKeys, timestamp, pressure));
}

} // namespace vgc::ui
