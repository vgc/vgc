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

#include <vgc/ui/widget.h>

namespace vgc::ui {

MouseEvent::MouseEvent(
    double timestamp,
    ModifierKeys modifiers,
    const MouseEventData& data)

    : Event(timestamp, modifiers)
    , data_(data) {
}

MouseEventPtr
MouseEvent::create(double timestamp, ModifierKeys modifiers, const MouseEventData& data) {
    return MouseEventPtr(new MouseEvent(timestamp, modifiers, data));
}

PropagatedMouseEvent::PropagatedMouseEvent(
    double timestamp,
    ModifierKeys modifiers,
    const MouseEventData& data)

    : MouseEvent(timestamp, modifiers, data) {
}

PropagatedMouseEventPtr PropagatedMouseEvent::create(
    double timestamp,
    ModifierKeys modifiers,
    const MouseEventData& data) {

    return PropagatedMouseEventPtr(new PropagatedMouseEvent(timestamp, modifiers, data));
}

MousePressEvent::MousePressEvent(
    double timestamp,
    ModifierKeys modifiers,
    const MouseEventData& data)

    : PropagatedMouseEvent(timestamp, modifiers, data) {
}

MousePressEventPtr MousePressEvent::create(
    double timestamp,
    ModifierKeys modifiers,
    const MouseEventData& data) {

    return MousePressEventPtr(new MousePressEvent(timestamp, modifiers, data));
}

MouseMoveEvent::MouseMoveEvent(
    double timestamp,
    ModifierKeys modifiers,
    const MouseEventData& data)

    : PropagatedMouseEvent(timestamp, modifiers, data) {
}

MouseMoveEventPtr MouseMoveEvent::create(
    double timestamp,
    ModifierKeys modifiers,
    const MouseEventData& data) {

    return MouseMoveEventPtr(new MouseMoveEvent(timestamp, modifiers, data));
}

MouseReleaseEvent::MouseReleaseEvent(
    double timestamp,
    ModifierKeys modifiers,
    const MouseEventData& data)

    : PropagatedMouseEvent(timestamp, modifiers, data) {
}

MouseReleaseEventPtr MouseReleaseEvent::create(
    double timestamp,
    ModifierKeys modifiers,
    const MouseEventData& data) {

    return MouseReleaseEventPtr(new MouseReleaseEvent(timestamp, modifiers, data));
}

MouseHoverEvent::MouseHoverEvent(
    double timestamp,
    ModifierKeys modifiers,
    const MouseEventData& data)

    : MouseEvent(timestamp, modifiers, data) {
}

MouseHoverEventPtr MouseHoverEvent::create(
    double timestamp,
    ModifierKeys modifiers,
    const MouseEventData& data) {

    return MouseHoverEventPtr(new MouseHoverEvent(timestamp, modifiers, data));
}

MouseActionEvent::MouseActionEvent(
    double timestamp,
    ModifierKeys modifiers,
    const MouseEventData& data,
    Widget* widget)

    : MouseEvent(timestamp, modifiers, data)
    , widget_(widget) {
}

MouseActionEventPtr MouseActionEvent::create(
    double timestamp,
    ModifierKeys modifiers,
    const MouseEventData& data,
    Widget* widget) {

    return MouseActionEventPtr(new MouseActionEvent(timestamp, modifiers, data, widget));
}

MouseActionEventPtr MouseActionEvent::create(const MouseEvent* other, Widget* widget) {
    return MouseActionEventPtr(new MouseActionEvent(
        other->timestamp(), other->modifierKeys(), other->data(), widget));
}

// Note: the two functions below cannot be in the .h file since assigning an
// ObjPtr or calling getIfAlive() requires to have the full definition of the
// type (here, Widget), and we cannot include widget.h in mouseevent.h to avoid
// circular dependencies.

Widget* MouseActionEvent::widget() const {
    return widget_.getIfAlive();
}

void MouseActionEvent::setWidget(Widget* widget) {
    widget_ = widget;
}

} // namespace vgc::ui
