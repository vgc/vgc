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
    setTextMode_(false);
    setTextFromValue_();
}

NumberEditPtr NumberEdit::create() {
    return NumberEditPtr(new NumberEdit());
}

void NumberEdit::setValue(double value) {

    // Set new value
    if (value_ == value) {
        return;
    }
    double newValue = clampedAndRoundedValue_(value);
    if (value_ == newValue) {
        return;
    }
    value_ = newValue;

    // Update text and emit signal
    setTextFromValue_();
    valueChanged().emit(value_);
}

void NumberEdit::setStep(double step) {
    step_ = step;
}

void NumberEdit::setMinimum(double min) {

    // Set new minimum
    if (minimum_ == min) {
        return;
    }
    double newMin = roundedValue_(min);
    if (minimum_ == newMin) {
        return;
    }
    minimum_ = newMin;

    // Ensure range is valid (min <= max)
    if (maximum_ < minimum_) {
        maximum_ = minimum_;
    }

    // Fit value in new range
    setValue(value_);
}

void NumberEdit::setMaximum(double max) {

    // Set new minimum
    if (maximum_ == max) {
        return;
    }
    double newMax = roundedValue_(max);
    if (maximum_ == newMax) {
        return;
    }
    maximum_ = newMax;

    // Ensure range is valid (min <= max)
    if (maximum_ < minimum_) {
        minimum_ = maximum_;
    }

    // Fit value in new range
    setValue(value_);
}

bool NumberEdit::onMouseEnter() {

    // Call the base method first, to ensure that the IBeam cursor is on the
    // cursor stack, so that we can transition from drag mode to text mode by
    // simply clearing the NumberEdit custom cursors.
    //
    LineEdit::onMouseEnter();
    updateCursor_();
    return true;
}

bool NumberEdit::onMouseLeave() {
    cursorChangerOnMouseHover_.clear();
    LineEdit::onMouseLeave();
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
        geometry::Vec2f newMousePosition = globalCursorPosition(); // in dp
        deltaPositionX_ += newMousePosition.x() - mousePositionOnMousePress_.x();
        skipNextMouseMove_ = true;
        setGlobalCursorPosition(mousePositionOnMousePress_);
    }
    else {
        geometry::Vec2f newMousePosition = event->position(); // in px
        deltaPositionX_ = newMousePosition.x() - mousePositionOnMousePress_.x();
        deltaPositionX_ /= styleMetrics().scaleFactor();
    }

    constexpr float dragEpsilon_ = 3;

    if (isDragEpsilonReached_) {
        double speed = step() * 0.25; // 4dp per step
        double newValue_ = oldValue_ + speed * static_cast<double>(deltaPositionX_);
        setValue(newValue_);
    }
    else if (std::abs(deltaPositionX_) > dragEpsilon_) {
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
    oldValue_ = value_;
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
        updateCursor_();
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

    // Switch to text mode on click or drag < epsilon.
    //
    if (isDragInitiated_ && !isDragEpsilonReached_) {
        setTextMode_(true);
    }

    // Clear cursors and other values
    isDragInitiated_ = false;
    isDragEpsilonReached_ = false;
    skipNextMouseMove_ = false;
    updateCursor_();
    return true;
}

bool NumberEdit::onFocusIn(FocusReason reason) {
    return LineEdit::onFocusIn(reason);
}

bool NumberEdit::onFocusOut(FocusReason reason) {

    if (reason != FocusReason::Window  //
        && reason != FocusReason::Menu //
        && reason != FocusReason::Popup) {

        if (isTextMode_) {
            setValueFromText_();
            setTextMode_(false);
        }
    }
    return LineEdit::onFocusOut(reason);
}

bool NumberEdit::onKeyPress(KeyEvent* event) {
    if (!isTextMode_) {
        return false;
    }
    if (event->key() == Key::Escape) {
        setValue(oldValue_);
        setTextMode_(false);
    }
    if (event->key() == Key::Enter || event->key() == Key::Return) {
        setValueFromText_();
        setTextMode_(false);
    }
    else {
        LineEdit::onKeyPress(event);
    }
    return true;
}

double NumberEdit::roundedValue_(double v) {
    // TODO: something like std::round(v / precision) * precision
    return v;
}

double NumberEdit::clampedAndRoundedValue_(double v) {
    double res = core::clamp(v, minimum(), maximum());
    return roundedValue_(res);
}

void NumberEdit::setTextFromValue_() {
    setText(core::format("{}", value_));
}

void NumberEdit::setValueFromText_() {
    double newValue = oldValue_;
    const std::string& newText = text();
    if (newText.empty()) {
        newValue = 0;
    }
    else {
        try {
            newValue = core::parse<double>(newText);
        }
        catch (const core::ParseError&) {
            newValue = oldValue_;
        }
        catch (const core::RangeError&) {
            newValue = oldValue_;
        }
    }
    setValue(newValue);
}

void NumberEdit::setTextMode_(bool isTextMode) {
    isTextMode_ = isTextMode;
    if (isTextMode_) {
        setFocusPolicy(FocusPolicy::Click | FocusPolicy::Tab);
        moveCursor(graphics::RichTextMoveOperation::StartOfText);
        moveCursor(graphics::RichTextMoveOperation::EndOfText, true);
        setFocus(FocusReason::Mouse);
    }
    else {
        setFocusPolicy(FocusPolicy::Never);
        clearFocus(FocusReason::Other);
    }
    updateCursor_();
}

void NumberEdit::updateCursor_() {
    if (isTextMode_) {
        cursorChangerOnMouseHover_.clear();
        cursorChangerOnValueDrag_.clear();
    }
    else {
        if (isHovered()) {
            cursorChangerOnMouseHover_.set(Qt::SizeHorCursor);
        }
        else {
            cursorChangerOnMouseHover_.clear();
        }
        if (isDragInitiated_ && isAbsoluteMode_) {
            cursorChangerOnValueDrag_.set(Qt::BlankCursor);
        }
        else {
            cursorChangerOnValueDrag_.clear();
        }
    }
}

} // namespace vgc::ui
