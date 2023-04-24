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

#include <vgc/ui/numbersetting.h>

namespace vgc::ui {

NumberSetting::NumberSetting(
    Settings* settings,
    std::string_view key,
    std::string_view label,
    double defaultValue,
    double min,
    double max,
    core::Precision precision,
    double step)

    : Setting(settings, key, label)
    , defaultValue_(defaultValue)
    , minimum_(min)
    , maximum_(max)
    , step_(step)
    , precision_(precision) {
}

NumberSettingPtr NumberSetting::create(
    Settings* settings,
    std::string_view key,
    std::string_view label,
    double defaultValue,
    double min,
    double max,
    core::Precision precision,
    double step) {

    return NumberSettingPtr(
        new NumberSetting(settings, key, label, defaultValue, min, max, precision, step));
}

double NumberSetting::value() const {

    // Get currently stored value.
    //
    double value_ = settings()->getOrSetDoubleValue(key(), defaultValue_);

    // Clamp and round value to given range and precision.
    //
    // Note: we must defer clamping/rounding as we do here, otherwise calling
    // several setters (min, max, or precision) would apply the constraint at
    // each step, instead of waiting for all constraints to be set.
    //
    // Example:
    //   stored setting on file = 200
    //   NumberSetting setting = NumberSetting::create(...);
    //   settings->setRange(10, 1000);
    //
    //  => This would start by calling setMinimum(10), which if not deferred
    //     would clamp 200 to (10, 100), i.e. change it to 100, before then
    //     increasing the maximum.
    //
    if (!isValueClampedAndRounded_) {
        double newValue = clampedAndRoundedValue_(value_);
        isValueClampedAndRounded_ = true;
        if (value_ != newValue) {
            settings()->setDoubleValue(key(), newValue);
            value_ = newValue;
        }
    }

    return value_;
}

void NumberSetting::setValue(double newValue) {

    // Set new value
    double oldValue = value();
    if (oldValue == newValue) {
        return;
    }
    newValue = clampedAndRoundedValue_(newValue);
    if (oldValue == newValue) {
        return;
    }
    settings()->setDoubleValue(key(), newValue);

    // Emit signal
    valueChanged().emit(newValue);
}

void NumberSetting::setMinimum(double min) {

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
    clampAndRound_();
}

void NumberSetting::setMaximum(double max) {

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
    clampAndRound_();
}

void NumberSetting::setPrecision(core::Precision precision) {
    if (precision_ == precision) {
        return;
    }
    precision_ = precision;
    setMinimum(roundedValue_(minimum_));
    setMaximum(roundedValue_(maximum_));
    clampAndRound_();
}

void NumberSetting::setStep(double step) {
    step_ = step;
}

void NumberSetting::clampAndRound_() {
    defaultValue_ = clampedAndRoundedValue_(defaultValue_);
    isValueClampedAndRounded_ = false;
}

double NumberSetting::roundedValue_(double v) const {
    return vgc::core::round(v, precision_);
}

double NumberSetting::clampedAndRoundedValue_(double v) const {
    double res = core::clamp(v, minimum(), maximum());
    return roundedValue_(res);
}

} // namespace vgc::ui
