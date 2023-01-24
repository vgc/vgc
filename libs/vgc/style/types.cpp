// Copyright 2022 The VGC Developers
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

#include <vgc/style/types.h>

#include <vgc/style/strings.h>
#include <vgc/style/stylableobject.h>

namespace vgc::style {

namespace {

bool isValidLengthUnit(std::string_view unitString, LengthUnit& unitEnum) {
    if (unitString == "dp") {
        unitEnum = LengthUnit::Dp;
        return true;
    }
    else {
        return false;
    }
}

float convertToPx(float value, style::LengthUnit unit, float scaleFactor) {
    switch (unit) {
    case style::LengthUnit::Dp:
        return value * scaleFactor;
    }
    return 0;
}

} // namespace

float Length::toPx(const Metrics& metrics) const {
    return convertToPx(value(), unit(), metrics.scaleFactor());
}

Value Length::parse(StyleTokenIterator begin, StyleTokenIterator end) {
    if (begin + 1 != end) {
        return Value::invalid();
    }
    else if (begin->type() == StyleTokenType::Dimension) {
        LengthUnit unit;
        if (isValidLengthUnit(begin->stringValue(), unit)) {
            return Value::custom(Length(begin->floatValue(), unit));
        }
        else {
            return Value::invalid();
        }
    }
    else {
        return Value::invalid();
    }
}

Value Percentage::parse(StyleTokenIterator begin, StyleTokenIterator end) {
    if (begin + 1 != end) {
        return Value::invalid();
    }
    else if (begin->type() == StyleTokenType::Percentage) {
        return Value::custom(Percentage(begin->floatValue()));
    }
    else {
        return Value::invalid();
    }
}

Value LengthOrPercentage::parse(StyleTokenIterator begin, StyleTokenIterator end) {
    if (begin + 1 != end) {
        return Value::invalid();
    }
    else if (begin->type() == StyleTokenType::Percentage) {
        return Value::custom(LengthOrPercentage(begin->floatValue()));
    }
    else if (begin->type() == StyleTokenType::Dimension) {
        LengthUnit unit;
        if (isValidLengthUnit(begin->stringValue(), unit)) {
            return Value::custom(LengthOrPercentage(begin->floatValue(), unit));
        }
        else {
            return Value::invalid();
        }
    }
    else {
        return Value::invalid();
    }
}

Value LengthOrAuto::parse(StyleTokenIterator begin, StyleTokenIterator end) {
    if (begin + 1 != end) {
        return Value::invalid();
    }
    else if (begin->type() == StyleTokenType::Identifier) {
        if (begin->stringValue() == "auto") {
            return Value::custom(LengthOrAuto());
        }
        else {
            return Value::invalid();
        }
    }
    else if (begin->type() == StyleTokenType::Dimension) {
        LengthUnit unit;
        if (isValidLengthUnit(begin->stringValue(), unit)) {
            return Value::custom(LengthOrAuto(begin->floatValue(), unit));
        }
        else {
            return Value::invalid();
        }
    }
    else {
        return Value::invalid();
    }
}

Value LengthOrPercentageOrAuto::parse(StyleTokenIterator begin, StyleTokenIterator end) {
    if (begin + 1 != end) {
        return Value::invalid();
    }
    else if (begin->type() == StyleTokenType::Percentage) {
        return Value::custom(LengthOrPercentageOrAuto(begin->floatValue()));
    }
    else if (begin->type() == StyleTokenType::Identifier) {
        if (begin->stringValue() == "auto") {
            return Value::custom(LengthOrPercentageOrAuto());
        }
        else {
            return Value::invalid();
        }
    }
    else if (begin->type() == StyleTokenType::Dimension) {
        LengthUnit unit;
        if (isValidLengthUnit(begin->stringValue(), unit)) {
            return Value::custom(LengthOrPercentageOrAuto(begin->floatValue(), unit));
        }
        else {
            return Value::invalid();
        }
    }
    else {
        return Value::invalid();
    }
}

Value BorderRadius::parse(StyleTokenIterator begin, StyleTokenIterator end) {
    if (begin == end) {
        return Value::invalid();
    }
    else if (begin + 1 == end) {
        Value v = LengthOrPercentage::parse(begin, end);
        if (v.isValid()) {
            LengthOrPercentage lp = v.to<LengthOrPercentage>();
            return Value::custom(BorderRadius(lp));
        }
        else {
            return Value::invalid();
        }
    }
    else {
        // Middle tokens should all be whitespaces
        for (StyleTokenIterator it = begin + 1; it != (end - 1); ++it) {
            if (it->type() != StyleTokenType::Whitespace) {
                return Value::invalid();
            }
        }
        Value v1 = LengthOrPercentage::parse(begin, begin + 1);
        Value v2 = LengthOrPercentage::parse(end - 1, end);
        if (v1.isValid() && v2.isValid()) {
            LengthOrPercentage lp1 = v1.to<LengthOrPercentage>();
            LengthOrPercentage lp2 = v2.to<LengthOrPercentage>();
            return Value::custom(BorderRadius(lp1, lp2));
        }
        else {
            return Value::invalid();
        }
    }
}

BorderRadii::BorderRadii(const StylableObject* obj)
    : BorderRadii(
        obj->style(strings::border_top_left_radius).to<BorderRadius>(),
        obj->style(strings::border_top_right_radius).to<BorderRadius>(),
        obj->style(strings::border_bottom_right_radius).to<BorderRadius>(),
        obj->style(strings::border_bottom_left_radius).to<BorderRadius>()) {
}

} // namespace vgc::style
