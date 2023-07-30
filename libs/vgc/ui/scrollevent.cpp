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

#include <vgc/ui/mouseevent.h>
#include <vgc/ui/scrollevent.h>

namespace vgc::ui {

namespace {

// XXX Does this make sense? Or should Scroll event not inherit from MouseEvent?
//
MouseEventData makeMouseEventData(const geometry::Vec2f& position) {
    MouseEventData data;
    data.setPosition(position);
    data.setButton(MouseButton::Middle);
    data.setHasPressure(false);
    return data;
}

} // namespace

ScrollEvent::ScrollEvent(
    CreateKey key,
    double timestamp,
    ModifierKeys modifiers,
    const geometry::Vec2f& position,
    const geometry::Vec2f& scrollDelta,
    Int horizontalSteps,
    Int verticalSteps)

    : PropagatedMouseEvent(key, timestamp, modifiers, makeMouseEventData(position))
    , scrollDelta_(scrollDelta)
    , horizontalSteps_(horizontalSteps)
    , verticalSteps_(verticalSteps) {
}

/* static */
ScrollEventPtr ScrollEvent::create(
    double timestamp,
    ModifierKeys modifiers,
    const geometry::Vec2f& position,
    const geometry::Vec2f& scrollDelta,
    Int horizontalSteps,
    Int verticalSteps) {

    return core::createObject<ScrollEvent>(
        timestamp, modifiers, position, scrollDelta, horizontalSteps, verticalSteps);
}

} // namespace vgc::ui
