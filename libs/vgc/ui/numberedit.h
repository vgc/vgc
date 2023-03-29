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

#ifndef VGC_UI_NUMBEREDIT_H
#define VGC_UI_NUMBEREDIT_H

#include <vgc/core/arithmetic.h>
#include <vgc/ui/lineedit.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(NumberEdit);

class VGC_UI_API NumberEdit : public LineEdit {
private:
    VGC_OBJECT(NumberEdit, LineEdit)

protected:
    /// This is an implementation details. Please use
    /// NumberEdit::create() instead.
    ///
    NumberEdit();

public:
    /// Creates a `NumberEdit`.
    ///
    static NumberEditPtr create();

    /// Returns the value of this `NumberEdit`.
    ///
    /// \sa `setValue()`.
    ///
    double value() const {
        return value_;
    }

    /// Sets the value of this `NumberEdit`.
    ///
    /// Note that after calling this function, `value()` may not be equal to
    /// the given `value` as a result of rounding to the allowed precision and
    /// clamping to the `minimum()` and `maximum()`.
    ///
    /// \sa `value()`.
    ///
    void setValue(double value);

    /// This signal is emitted whenever `value()` changes.
    ///
    VGC_SIGNAL(valueChanged, (double, value))

    /// Returns by how much should the value change when increasing it by one
    /// "step" (e.g., dragging by a few pixels, using the mouse wheel, clicking
    /// on the up arrow, etc.).
    ///
    /// \sa `setStep()`.
    ///
    double step() const {
        return step_;
    }

    /// Sets by how much should the value change when increasing it by one
    /// "step".
    ///
    /// \sa `step()`.
    ///
    void setStep(double step);

    /// Returns the minimim value of this `NumberEdit`.
    ///
    /// \sa `maximum()`, `setMinimum()`, `setMaximum()`, `setRange()`.
    ///
    double minimum() const {
        return minimum_;
    }

    /// Sets the minimim value of this `NumberEdit`.
    ///
    /// The `maximum()` and `value()` may be automatically changed in order for
    /// the range to stay valid (minimum <= maximum) and the value to fit in
    /// the range.
    ///
    /// Note that after calling this function, `minimum()` may not be equal to
    /// the given `min` as a result of rounding to the allowed precision.
    ///
    /// \sa `minimum()`, `maximum()`, `setMaximum()`, `setRange()`.
    ///
    void setMinimum(double min);

    /// Returns the maximum value of this `NumberEdit`.
    ///
    /// \sa `minimum()`, `setMinimum()`, `setMaximum()`, `setRange()`.
    ///
    double maximum() const {
        return maximum_;
    }

    /// Sets the maximim value of this `NumberEdit`.
    ///
    /// The `minimum()` and `value()` may be automatically changed in order for
    /// the range to stay valid (minimum <= maximum) and the value to fit in
    /// the range.
    ///
    /// Note that after calling this function, `maximum()` may not be equal to
    /// the given `max` as a result of rounding to the allowed precision.
    ///
    /// \sa `minimum()`, `maximum()`, `setMinimum()`, `setRange()`.
    ///
    void setMaximum(double max);

    /// Sets the minimum and maximim value of this `NumberEdit`.
    ///
    /// This is a convenient function equivalent to:
    ///
    /// ```cpp
    /// setMinimum(min);
    /// setMaximum(max);
    /// ```
    ///
    /// \sa `minimum()`, `maximum()`, `setMinimum()`, `setMaximum()`.
    ///
    void setRange(double min, double max) {
        setMinimum(min);
        setMaximum(max);
    }

    /// Returns the precision of this `NumberEdit`, that is, how many decimals
    /// or significant digits input numbers are rounded to.
    ///
    /// \sa `setPrecision()`, `setDecimals()`, `setSignificantDigits()`.
    ///
    core::Precision precision() const {
        return precision_;
    }

    /// Sets the precision of this `NumberEdit`, that is, how many decimals or
    /// significant digits input numbers are rounded to.
    ///
    /// The `value()`, `minimum()`, and `maximum()` are automatically rounded
    /// to the new precision.
    ///
    /// \sa `precision()`, `setDecimals()`, `setSignificantDigits()`.
    ///
    void setPrecision(core::Precision precision);

    /// Sets the precision of this `NumberEdit` to a fixed number of decimals.
    ///
    /// Note that the range of supported `numDecimals` is from `-128` to `127`.
    ///
    /// This is a convenient method equivalent to:
    ///
    /// ```
    /// setPrecision(Precision(PrecisionMode::Decimals, numDecimals));
    /// ```
    ///
    /// \sa `precision()`, `setPrecision()`, `setSignificantDigits()`.
    ///
    void setDecimals(Int numDecimals) {
        setPrecision({core::PrecisionMode::Decimals, static_cast<Int8>(numDecimals)});
    }

    /// Sets the precision of this `NumberEdit` to a fixed number of significant
    /// digits.
    ///
    /// Note that the range of supported `numDigits` is from `-127` to `128`.
    ///
    /// This is a convenient method equivalent to:
    ///
    /// ```
    /// setPrecision(Precision(PrecisionMode::SignificantDigits, numDigits));
    /// ```
    ///
    /// \sa `precision()`, `setPrecision()`, `setDecimals()`.
    ///
    void setSignificantDigits(Int numDigits) {
        setPrecision(
            {core::PrecisionMode::SignificantDigits, static_cast<Int8>(numDigits)});
    }

protected:
    // Reimplementation of Widget virtual methods
    bool onMouseEnter() override;
    bool onMouseLeave() override;
    bool onMouseMove(MouseEvent* event) override;
    bool onMousePress(MouseEvent* event) override;
    bool onMouseRelease(MouseEvent* event) override;
    bool onFocusIn(FocusReason reason) override;
    bool onFocusOut(FocusReason reason) override;
    bool onKeyPress(KeyEvent* event) override;

private:
    // Current value
    double value_ = 0;

    // Parameters
    double step_ = 1;
    double minimum_ = 0;
    double maximum_ = 100;
    core::Precision precision_ = {core::PrecisionMode::Decimals, 0};
    double roundedValue_(double v);
    double clampedAndRoundedValue_(double v);

    // Value before drag or text editing starts
    double oldValue_;

    // Drag mode
    bool isDragInfiniteMode_ = false;
    bool isDragInitiated_ = false;
    bool isDragEpsilonReached_ = false;
    bool skipNextMouseMove_ = false;
    geometry::Vec2f mousePositionOnMousePress_;
    float deltaPositionX_ = 0;

    // Text mode
    void setTextFromValue_();
    void setValueFromText_(double valueIfInvalid);
    void onTextChanged_();
    VGC_SLOT(onTextChangedSlot_, onTextChanged_)

    // Switch between modes
    bool isTextMode_ = true;
    void setTextMode_(bool isTextMode);

    // Cursor Handling
    ui::CursorChanger cursorChangerOnMouseHover_;
    ui::CursorChanger cursorChangerOnValueDrag_;
    void updateCursor_();
};

} // namespace vgc::ui

#endif // VGC_UI_NUMBEREDIT_H
