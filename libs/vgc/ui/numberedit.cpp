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

    addStyleClass(strings::NumberEdit);
    setTextMode(false);
    setTextFromValue_();
}

NumberEditPtr NumberEdit::create() {
    return NumberEditPtr(new NumberEdit());
}

void NumberEdit::setValue(double v) {
    value_ = v;
    setTextFromValue_();
}

void NumberEdit::setTextMode(bool isTextMode) {
    isTextMode_ = isTextMode;
    if (isTextMode_) {
        setFocusPolicy(FocusPolicy::Click | FocusPolicy::Tab);
    }
    else {
        setFocusPolicy(FocusPolicy::Never);
    }
}

bool NumberEdit::onMouseEnter() {

    // Delegate to LineEdit in case of text mode
    if (isTextMode_) {
        return LineEdit::onMouseEnter();
    }

    cursorChangerOnMouseHover_.set(Qt::SizeHorCursor);
    return true;
}

bool NumberEdit::onMouseLeave() {

    // Delegate to LineEdit in case of text mode
    if (isTextMode_) {
        return LineEdit::onMouseLeave();
    }

    cursorChangerOnMouseHover_.clear();
    return true;
}

bool NumberEdit::onMouseMove(MouseEvent* event) {

    // Delegate to LineEdit in case of text mode
    if (isTextMode_) {
        return LineEdit::onMouseMove(event);
    }

    // When dragging in absolute mode, calling setGlobalCursorPosition() might
    // generate a mouse event in some platforms. Skipping one mouse event prevents
    // infinite loops (possibly at the cause of missing real useful mouse events,
    // but it's more important to prevent infinite loops and there is no perfect
    // solution for this problem).
    //
    if (isAbsoluteMode_ && skipNextMouseMove_) {
        skipNextMouseMove_ = false;
        return false;
    }

    if (!isDragInitiated_) {
        return false;
    }

    if (isAbsoluteMode_) {
        geometry::Vec2f newMousePosition = globalCursorPosition();
        deltaPositionX_ += newMousePosition.x() - mousePositionOnMousePress_.x();
        skipNextMouseMove_ = true;
        setGlobalCursorPosition(mousePositionOnMousePress_);
    }
    else {
        geometry::Vec2f newMousePosition = event->position();
        deltaPositionX_ = newMousePosition.x() - mousePositionOnMousePress_.x();
    }

    if (isDragEpsilonReached_) {
        float speed = 1;
        double newValue_ = valueOnMousePress_ + speed * deltaPositionX_;
        setValue(newValue_);
    }
    else if (std::abs(deltaPositionX_) > dragEpsilon_) {
        // TODO: is this correctly handling high-DPI screens?
        // We may have to do dragEpsilon * scaleFactor() when not in absolute mode.
        isDragEpsilonReached_ = true;
        if (isAbsoluteMode_) {
            deltaPositionX_ = 0;
        }
        else {
            mousePositionOnMousePress_ = event->position();
        }
    }

    return true;
}

bool NumberEdit::onMousePress(MouseEvent* event) {

    // Delegate to LineEdit in case of text mode
    if (isTextMode_) {
        return LineEdit::onMousePress(event);
    }

    // Only drag on left mouse button
    if (event->button() != MouseButton::Left) {
        return false;
    }

    // Store current value and enter drag mode
    valueOnMousePress_ = value_;
    isDragInitiated_ = true;
    isDragEpsilonReached_ = false;

    // Detect whether we should perform dragging by using the absolute cursor
    // position (which enable infinite drag when using a mouse), or via using the
    // local cursor position (which is typically necessary for graphics tablets).
    //
    if (event->hasPressure()) {
        //     ^^^^^^^^^^^^^ TODO: Implement and use event->isAbsolute() instead?
        // Example of scenarios that may not be properly supported right now:
        // 1. A graphics tablet which does not have pressure
        // 2. A graphics tablet which does have pressure but is in relative mode.
        //
        isAbsoluteMode_ = false;
    }
    else {
        isAbsoluteMode_ = true;
    }

    // Initialize dragging
    if (isAbsoluteMode_) {
        mousePositionOnMousePress_ = globalCursorPosition();
        deltaPositionX_ = 0;
        skipNextMouseMove_ = false;

        // Hide cursor
        // Note: we intentially choose to keep it visible when not in absolute mode.
        cursorChangerOnValueDrag_.set(Qt::BlankCursor);
    }
    else {
        mousePositionOnMousePress_ = event->position();
    }

    return true;
}

bool NumberEdit::onMouseRelease(MouseEvent* event) {

    // Delegate to LineEdit in case of text mode
    if (isTextMode_) {
        return LineEdit::onMouseRelease(event);
    }

    // Only drag on left mouse button
    if (MouseButton::Left != event->button()) {
        return false;
    }

    if (isDragInitiated_ && !isDragEpsilonReached_) {
        setTextMode(true);
        cursorChangerOnValueDrag_.clear();
        cursorChangerOnMouseHover_.clear();
        // TODO: How to set the line edit ibeam cursor?
        //       How to go back to no text mode? (onKeyboardFocusLost?)
        // => There is onFocusIn() / onFocusOut().
        // Although we need to be mindful of the focus reason: what do we want to
        // do if the current window lose focus, but the number edit was in text mode
        // and still had the focus within the tree? Should we turn it to no text mode
        // automatically?

        // TODO: automatically select all the text? (like in Blender)
        // Or set the cursor where the user clicked?

        // TODO: set numeric value from entered text value.
    }
    else {
        // TODO: Anything here?
    }

    cursorChangerOnValueDrag_.clear();
    isDragInitiated_ = false;
    isDragEpsilonReached_ = false;
    skipNextMouseMove_ = false;
    return true;
}

bool NumberEdit::onFocusIn(FocusReason reason) {
    // TODO: Is this correct?
    return LineEdit::onFocusIn(reason);
}

bool NumberEdit::onFocusOut(FocusReason reason) {
    // TODO: Is this correct?
    setTextMode(false);
    return LineEdit::onFocusOut(reason);
}

void NumberEdit::setTextFromValue_() {
    setText(core::format("{}", value_));
}

} // namespace vgc::ui
