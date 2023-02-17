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

#include <vgc/ui/numberedit.h>

#include <string>

#include <vgc/geometry/vec2f.h>
#include <vgc/ui/cursor.h>

namespace vgc::ui {

NumberEdit::NumberEdit()
    : LineEdit("") {

    setTextFromValue_();
}

NumberEditPtr NumberEdit::create() {
    return NumberEditPtr(new NumberEdit());
}

void NumberEdit::setValue(double v) {
    value_ = v;
    setTextFromValue_();
}

void NumberEdit::setTextFromValue_() {
    setText(core::format("{}", value_));
}

bool NumberEdit::onMouseEnter() {
    cursorChangerOnMouseHover_.set(Qt::SizeHorCursor);
    return true;
}

bool NumberEdit::onMouseLeave() {
    cursorChangerOnMouseHover_.clear();
    return true;
}

bool NumberEdit::onMouseMove(MouseEvent* /*event*/) {
    if (isSettingCursorPosition_) {
        isSettingCursorPosition_ = false;
        return false;
    }
    if (!isMousePressed_) {
        return false;
    }
    geometry::Vec2f newGlobalCursorPosition = globalCursorPosition();
    deltaPositionX += newGlobalCursorPosition.x() - globalCursorPositionOnMousePress_.x();

    isSettingCursorPosition_ = true;
    setGlobalCursorPosition(globalCursorPositionOnMousePress_);

    float speed = 1;
    double newValue_ = valueOnMousePress_ + speed * deltaPositionX;
    setValue(newValue_);

    return true;
}

bool NumberEdit::onMousePress(MouseEvent* event) {
    // Only support Left mouse button
    if (event->button() != MouseButton::Left) {
        return false;
    }
    isSettingCursorPosition_ = false;
    isMousePressed_ = true;
    valueOnMousePress_ = value_;
    globalCursorPositionOnMousePress_ = globalCursorPosition();
    deltaPositionX = 0;
    isSettingCursorPosition_ = false;
    cursorChangerOnValueDrag_.set(Qt::BlankCursor);
    return true;
}

bool NumberEdit::onMouseRelease(MouseEvent* event) {
    // Only support one mouse button at a time
    if (MouseButton::Left != event->button()) {
        return false;
    }
    cursorChangerOnValueDrag_.clear();
    isMousePressed_ = false;
    return true;
}

} // namespace vgc::ui
