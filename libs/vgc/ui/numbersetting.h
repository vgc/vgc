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

#ifndef VGC_UI_NUMBERSETTING_H
#define VGC_UI_NUMBERSETTING_H

#include <vgc/core/arithmetic.h>
#include <vgc/ui/api.h>
#include <vgc/ui/setting.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(NumberSetting);

/// \class vgc::ui::NumberSetting
/// \brief A `Setting` subclass for numbers
///
class VGC_UI_API NumberSetting : public Setting {
private:
    VGC_OBJECT(NumberSetting, Setting)

protected:
    NumberSetting(
        CreateKey,
        Settings* settings,
        std::string_view key,
        std::string_view label,
        double defaultValue,
        double min,
        double max,
        core::Precision precision,
        double step);

public:
    /// Creates a `NumberSetting`.
    ///
    static NumberSettingPtr create(
        Settings* settings,
        std::string_view key,
        std::string_view label,
        double defaultValue,
        double min,
        double max,
        core::Precision precision,
        double step);

    /// Returns the default value of this `NumberSetting`.
    ///
    double defaultValue() const {
        return defaultValue_;
    }

    /// Returns the current value of this `NumberSetting`.
    ///
    double value() const;

    /// Sets the value of this `NumberSetting`.
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

    /// Returns the minimim value of this `NumberSetting`.
    ///
    /// \sa `maximum()`, `setMinimum()`, `setMaximum()`, `setRange()`.
    ///
    double minimum() const {
        return minimum_;
    }

    /// Sets the minimim value of this `NumberSetting`.
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

    /// Returns the maximum value of this `NumberSetting`.
    ///
    /// \sa `minimum()`, `setMinimum()`, `setMaximum()`, `setRange()`.
    ///
    double maximum() const {
        return maximum_;
    }

    /// Sets the maximim value of this `NumberSetting`.
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

    /// Sets the minimum and maximim value of this `NumberSetting`.
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

    /// Returns the precision of this `NumberSetting`, that is, how many decimals
    /// or significant digits input numbers are rounded to.
    ///
    /// \sa `setPrecision()`, `setDecimals()`, `setSignificantDigits()`.
    ///
    core::Precision precision() const {
        return precision_;
    }

    /// Sets the precision of this `NumberSetting`, that is, how many decimals or
    /// significant digits input numbers are rounded to.
    ///
    /// The `value()`, `minimum()`, and `maximum()` are automatically rounded
    /// to the new precision.
    ///
    /// \sa `precision()`, `setDecimals()`, `setSignificantDigits()`.
    ///
    void setPrecision(core::Precision precision);

    /// Sets the precision of this `NumberSetting` to a fixed number of decimals.
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

    /// Sets the precision of this `NumberSetting` to a fixed number of significant
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

private:
    double defaultValue_ = 0;
    double minimum_ = 0;
    double maximum_ = 100;
    double step_ = 1;
    core::Precision precision_ = {core::PrecisionMode::Decimals, 0};

    mutable bool isValueClampedAndRounded_ = false;
    void clampAndRound_();
    double roundedValue_(double v) const;
    double clampedAndRoundedValue_(double v) const;
};

/// Creates a `NumberSetting` whose precision is `PrecisionMode::Decimals`,
/// with the given default value, range, number of decimals, and step.
///
inline NumberSettingPtr createDecimalNumberSetting(
    Settings* settings,
    std::string_view key,
    std::string_view label,
    double defaultValue = 0,
    double min = 0,
    double max = 100,
    Int numDecimals = 10,
    double step = 1) {

    return NumberSetting::create(
        settings,
        key,
        label,
        defaultValue,
        min,
        max,
        {core::PrecisionMode::Decimals, static_cast<Int8>(numDecimals)},
        step);
}

/// Creates a `NumberSetting` whose precision is `{PrecisionMode::Decimals, 1}`
/// (that is, an integer), with the given default value, range, and step.
///
inline NumberSettingPtr createIntegerNumberSetting(
    Settings* settings,
    std::string_view key,
    std::string_view label,
    double defaultValue = 0,
    double min = 0,
    double max = 100,
    double step = 1) {

    return NumberSetting::create(
        settings,
        key,
        label,
        defaultValue,
        min,
        max,
        {core::PrecisionMode::Decimals, 1},
        step);
}

} // namespace vgc::ui

#endif // VGC_UI_NUMBERSETTING_H
