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
#include <vgc/ui/window.h>

// for requesting to capture mouse in macos
#ifdef VGC_OS_MACOS
#    include <QTimer>
#    include <vgc/ui/macospermissions.h>
#    include <vgc/ui/messagedialog.h>
#endif

namespace vgc::ui {

NumberEdit::NumberEdit(CreateKey key)
    : LineEdit(key, "") {

    addStyleClass(strings::NumberEdit);
    setTextMode_(false);
    setTextFromValue_();
    textChanged().connect(onTextChangedSlot_());
}

NumberEditPtr NumberEdit::create() {
    return core::createObject<NumberEdit>();
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
    setValue(clampedAndRoundedValue_(value_));
}

void NumberEdit::setMaximum(double max) {

    // Set new maximum
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
    setValue(clampedAndRoundedValue_(value_));
}

void NumberEdit::setPrecision(core::Precision precision) {
    if (precision_ == precision) {
        return;
    }
    precision_ = precision;
    setMinimum(roundedValue_(minimum_));
    setMaximum(roundedValue_(maximum_));
    setValue(clampedAndRoundedValue_(value_));
}

void NumberEdit::onMouseEnter() {

    // Call the base method first, to ensure that the IBeam cursor is on the
    // cursor stack, so that we can transition from drag mode to text mode by
    // simply clearing the NumberEdit custom cursors.
    //
    LineEdit::onMouseEnter();
    updateCursor_();
}

void NumberEdit::onMouseLeave() {
    cursorChangerOnMouseHover_.clear();
    LineEdit::onMouseLeave();
}

bool NumberEdit::onMouseMove(MouseMoveEvent* event) {

    // Delegate to LineEdit in case of text mode
    if (isTextMode_) {
        return LineEdit::onMouseMove(event);
    }

    // When dragging in infinite mode, calling setGlobalCursorPosition() might
    // generate a mouse event in some platforms. Skipping one mouse event prevents
    // infinite loops (possibly at the risk of missing real useful mouse events,
    // but it's more important to prevent infinite loops and there is no perfect
    // solution for this problem).
    //
    if (isDragInfiniteMode_ && skipNextMouseMove_) {
        skipNextMouseMove_ = false;
        return false;
    }

    if (!isDragInitiated_) {
        return false;
    }

    if (isDragInfiniteMode_) {

        // Compute delta based on system-queried global cursor position.
        //
        // Note that currently, globalCursorPosition() is always an integer
        // (because internally, we use QCursor::pos() returning an int), while
        // mouse events can be subpixels. So we could have several mouse events
        // before the value globalCursorPosition() actually changes.
        //
        //                       ----  event 1 -- event 2 -- event 2 ---->
        //
        //   actual cursor position     800        800.3      800.6
        //   globalCursorPosition()     800        800        801
        //
        // Until such a change happen, it's important not to call
        // setGlobalCursorPosition(mousePositionOnMousePress_), otherwise
        // moving the cursor slowly might never change the value of the number
        // edit. Hence the `if` test below.
        //
        geometry::Vec2f newMousePosition = globalCursorPosition();
        float dx = newMousePosition.x() - mousePositionOnMousePress_.x();
        if (std::abs(dx) > 0.5) {
            Window* window = this->window();
            float s = window ? window->globalToWindowScale() : 1.0f;
            deltaPositionX_ += s * dx;
            skipNextMouseMove_ = true;
            setGlobalCursorPosition(mousePositionOnMousePress_);
        }
    }
    else {
        geometry::Vec2f newMousePosition = event->position();
        deltaPositionX_ = newMousePosition.x() - mousePositionOnMousePress_.x();
    }

    using namespace style::literals;
    constexpr style::Length lengthPerStep = 4_dp;
    float pxPerStep = lengthPerStep.toPx(styleMetrics());

    if (std::abs(deltaPositionX_) > pxPerStep) {
        isDragEpsilonReached_ = true;
    }
    if (isDragEpsilonReached_) {
        float numSteps = std::trunc(deltaPositionX_ / pxPerStep);
        double deltaValue = static_cast<double>(numSteps) * step();
        double newValue_ = oldValue_ + deltaValue;
        setValue(newValue_);
    }

    return true;
}

bool NumberEdit::onMousePress(MousePressEvent* event) {

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

    // Detect whether to use:
    //
    // 1. "standard dragging", where the cursor stays visible and can
    //    get stuck at the edge of the screen.
    //
    // 2. "infinite dragging", where we hide the cursor and restore it to its
    //    initial position after each move, remembering the deltas.
    //
    // Infinite mode requires the ability to set the global cursor position,
    // which is not always possible depending on the platform and app
    // permissions.
    //
    // Infinite mode is not possible when using a graphics tablet in "absolute
    // mode" (the typical mode), that is, when there is a mapping between the
    // physical pen location and the cursor location.
    //
    if (canSetGlobalCursorPosition() && !event->hasPressure()) {
        //                              ^^^^^^^^^^^^^^^^^^^^^
        //            TODO: Implement and use !event->isAbsolute() instead?
        //
        // Example of scenarios that may not be properly supported right now:
        // 1. A graphics tablet which does not have pressure
        // 2. A graphics tablet which does have pressure but is in relative mode.
        //
        isDragInfiniteMode_ = true;
    }
    else {
        isDragInfiniteMode_ = false;
    }

    // Initialize dragging
    if (isDragInfiniteMode_) {
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

#ifdef VGC_OS_MACOS

namespace {

// Prevent opening two or more "enable infinite drag?" dialogs at the same time.
//
std::atomic<Int> numInfiniteDragDialogs = 0;

using QTimerSharedPtr = std::shared_ptr<QTimer>;
using QTimerWeakPtr = std::weak_ptr<QTimer>;

void createEnableInfiniteDragDialogAndTimer(
    MessageDialogPtr& dialog,
    QTimerSharedPtr& timer) {

    Int i = numInfiniteDragDialogs.fetch_add(1);
    if (i > 0) {
        // There is already a dialog opened (or about to be opened).
        // Let's decrease back the count and do nothing.
        //
        --numInfiniteDragDialogs;
    }
    else {
        // There is not yet any dialog.
        // Let's create a new one, decreasing the count on destruction.
        //
        dialog = MessageDialog::create();
        timer = std::make_shared<QTimer>();
        dialog->aboutToBeDestroyed().connect(
            [keepAlive = timer]() { --numInfiniteDragDialogs; });
    }
}

void macosEnableInfiniteDrag(
    bool couldHaveBeenInfinite,
    bool wasInfinite,
    Widget* widget) {

    std::string_view key = "ui.numberEdit.macOsInfiniteDragPermission.dontAskAgain";
    bool shouldAskAgain = MessageDialog::shouldAskAgain(key);
    if (couldHaveBeenInfinite && !wasInfinite && shouldAskAgain) {

        // Create dialog and timer. We use ObjectPtr to capture it safely in the lambda.
        //
        MessageDialogPtr dialog;
        QTimerSharedPtr timer;
        createEnableInfiniteDragDialogAndTimer(dialog, timer);
        if (!dialog || !timer) {
            return;
        }
        dialog->setTitle("Enable infinite drag?");
        dialog->addText("This allows you to drag-edit the value of number fields");
        dialog->addText("without your mouse getting stuck at the edge of the screen.");
        dialog->addText("");
        dialog->addText("This requires your permission to capture the mouse cursor.");
        dialog->addDontAskAgainCheckbox(key);

        // Periodically check permissions.
        // We use a weak_ptr here to prevent the timer from keeping itself alive.
        //
        QTimerWeakPtr timerWeakPtr = timer;
        timer->setInterval(1000);
        timer->callOnTimeout([=]() {
            if (hasAccessibilityPermissions()) {
                if (auto timerSharedPtr = timerWeakPtr.lock()) {
                    timerSharedPtr->stop();
                }
                dialog->clear();
                dialog->addCenteredText("You're all set!");
                dialog->addButton("OK", [=]() { dialog->destroy(); });
            }
        });

        // Dialog buttons
        dialog->addButton("No", [=]() { dialog->destroy(); });
        dialog->addButton("Yes", [=]() {
            openAccessibilityPermissions();
            if (auto timer = timerWeakPtr.lock()) {
                timer->start();
            }
            dialog->clear();
            dialog->setTitle("Turn on accessibility");
            dialog->addText(
                "To enable infinite drag, select the VGC Illustration checkbox");
            dialog->addText("in Security & Privacy > Accessibility.");
            dialog->addButton("Cancel", [=]() { dialog->destroy(); });
            dialog->addButton("Turn On Accessibility", [=]() { //
                openAccessibilityPermissions();
            });
        });

        // Init
        dialog->showOutsidePanelArea(widget);
    }
}

} // namespace

#endif // VGC_OS_MACOS

bool NumberEdit::onMouseRelease(MouseReleaseEvent* event) {

    // Delegate to LineEdit in case of text mode
    if (isTextMode_) {
        return LineEdit::onMouseRelease(event);
    }

    // Only drag on left mouse button
    if (MouseButton::Left != event->button()) {
        return false;
    }

    // Switch to text mode on click or drag < epsilon
    if (isDragInitiated_ && !isDragEpsilonReached_) {
        setTextMode_(true);
    }

    // Conditionally ask for permission to enable infinite drag on macOS
#ifdef VGC_OS_MACOS
    bool wasDrag = isDragInitiated_ && isDragEpsilonReached_;
    bool wasInfinite = isDragInfiniteMode_;
    bool couldHaveBeenInfinite = wasDrag && !event->hasPressure();
    macosEnableInfiniteDrag(couldHaveBeenInfinite, wasInfinite, this);
#endif

    // Clear cursors and other values
    isDragInitiated_ = false;
    isDragEpsilonReached_ = false;
    skipNextMouseMove_ = false;
    updateCursor_();
    return true;
}

void NumberEdit::onFocusStackOut(FocusReason reason) {
    if (isTextMode_) {
        setValueFromText_(oldValue_);
        setTextFromValue_();
        setTextMode_(false);
    }
    LineEdit::onFocusStackOut(reason);
}

bool NumberEdit::onKeyPress(KeyPressEvent* event) {
    if (!isTextMode_) {
        return false;
    }
    else if (event->key() == Key::Escape) {
        setValue(oldValue_);
        setTextFromValue_();
        setTextMode_(false);
        return true;
    }
    else if (event->key() == Key::Enter || event->key() == Key::Return) {
        setValueFromText_(oldValue_);
        setTextFromValue_();
        setTextMode_(false);
        return true;
    }
    else {
        return LineEdit::onKeyPress(event);
    }
}

double NumberEdit::roundedValue_(double v) {
    return vgc::core::round(v, precision_);
}

double NumberEdit::clampedAndRoundedValue_(double v) {
    double res = core::clamp(v, minimum(), maximum());
    return roundedValue_(res);
}

void NumberEdit::setTextFromValue_() {
    setText(core::format("{}", value_));
}

void NumberEdit::setValueFromText_(double valueIfInvalid) {
    double newValue = valueIfInvalid;
    const std::string& newText = text();
    if (newText.empty()) {
        newValue = 0;
    }
    else {
        try {
            newValue = core::parse<double>(newText);
        }
        catch (const core::ParseError&) {
            newValue = valueIfInvalid;
        }
        catch (const core::RangeError&) {
            newValue = valueIfInvalid;
        }
    }
    setValue(newValue);
}

void NumberEdit::onTextChanged_() {

    // Handle case when the text is changed programatically via setText().
    //
    if (!isTextMode_) {
        double valueIfInvalid = value();
        setValueFromText_(valueIfInvalid);
        setTextFromValue_();
    }
}

void NumberEdit::setTextMode_(bool isTextMode) {
    isTextMode_ = isTextMode;
    if (isTextMode_) {
        oldValue_ = value();
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
            Qt::CursorShape cursorShape = Qt::SizeHorCursor;
#ifdef VGC_OS_MACOS
            // SizeHorCursor is currently ugly on macOS, so we use another one
            // (see https://github.com/vgc/vgc/issues/1131)
            cursorShape = Qt::SplitHCursor;
#endif
            cursorChangerOnMouseHover_.set(cursorShape);
        }
        else {
            cursorChangerOnMouseHover_.clear();
        }
        if (isDragInitiated_ && isDragInfiniteMode_) {
            cursorChangerOnValueDrag_.set(Qt::BlankCursor);
        }
        else {
            cursorChangerOnValueDrag_.clear();
        }
    }
}

} // namespace vgc::ui
