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

StyleValue Length::parse(StyleTokenIterator begin, StyleTokenIterator end) {
    if (begin + 1 != end) {
        return StyleValue::invalid();
    }
    else if (begin->type == StyleTokenType::Dimension) {
        LengthUnit unit;
        if (isValidLengthUnit(begin->codePointsValue, unit)) {
            return StyleValue::custom(Length(begin->toFloat(), unit));
        }
        else {
            return StyleValue::invalid();
        }
    }
    else {
        return StyleValue::invalid();
    }
}

StyleValue Percentage::parse(StyleTokenIterator begin, StyleTokenIterator end) {
    if (begin + 1 != end) {
        return StyleValue::invalid();
    }
    else if (begin->type == StyleTokenType::Percentage) {
        return StyleValue::custom(Percentage(begin->toFloat()));
    }
    else {
        return StyleValue::invalid();
    }
}

StyleValue LengthOrPercentage::parse(StyleTokenIterator begin, StyleTokenIterator end) {
    if (begin + 1 != end) {
        return StyleValue::invalid();
    }
    else if (begin->type == StyleTokenType::Percentage) {
        return StyleValue::custom(LengthOrPercentage(begin->toFloat()));
    }
    else if (begin->type == StyleTokenType::Dimension) {
        LengthUnit unit;
        if (isValidLengthUnit(begin->codePointsValue, unit)) {
            return StyleValue::custom(LengthOrPercentage(begin->toFloat(), unit));
        }
        else {
            return StyleValue::invalid();
        }
    }
    else {
        return StyleValue::invalid();
    }
}

StyleValue LengthOrAuto::parse(StyleTokenIterator begin, StyleTokenIterator end) {
    if (begin + 1 != end) {
        return StyleValue::invalid();
    }
    else if (begin->type == StyleTokenType::Identifier) {
        if (begin->codePointsValue == "auto") {
            return StyleValue::custom(LengthOrAuto());
        }
        else {
            return StyleValue::invalid();
        }
    }
    else if (begin->type == StyleTokenType::Dimension) {
        LengthUnit unit;
        if (isValidLengthUnit(begin->codePointsValue, unit)) {
            return StyleValue::custom(LengthOrAuto(begin->toFloat(), unit));
        }
        else {
            return StyleValue::invalid();
        }
    }
    else {
        return StyleValue::invalid();
    }
}

StyleValue
LengthOrPercentageOrAuto::parse(StyleTokenIterator begin, StyleTokenIterator end) {
    if (begin + 1 != end) {
        return StyleValue::invalid();
    }
    else if (begin->type == StyleTokenType::Percentage) {
        return StyleValue::custom(LengthOrPercentageOrAuto(begin->toFloat()));
    }
    else if (begin->type == StyleTokenType::Identifier) {
        if (begin->codePointsValue == "auto") {
            return StyleValue::custom(LengthOrPercentageOrAuto());
        }
        else {
            return StyleValue::invalid();
        }
    }
    else if (begin->type == StyleTokenType::Dimension) {
        LengthUnit unit;
        if (isValidLengthUnit(begin->codePointsValue, unit)) {
            return StyleValue::custom(LengthOrPercentageOrAuto(begin->toFloat(), unit));
        }
        else {
            return StyleValue::invalid();
        }
    }
    else {
        return StyleValue::invalid();
    }
}

StyleValue BorderRadius::parse(StyleTokenIterator begin, StyleTokenIterator end) {
    if (begin == end) {
        return StyleValue::invalid();
    }
    else if (begin + 1 == end) {
        StyleValue v = LengthOrPercentage::parse(begin, end);
        if (v.isValid()) {
            LengthOrPercentage lp = v.to<LengthOrPercentage>();
            return StyleValue::custom(BorderRadius(lp));
        }
        else {
            return StyleValue::invalid();
        }
    }
    else {
        // Middle tokens should all be whitespaces
        for (StyleTokenIterator it = begin + 1; it != (end - 1); ++it) {
            if (it->type != StyleTokenType::Whitespace) {
                return StyleValue::invalid();
            }
        }
        StyleValue v1 = LengthOrPercentage::parse(begin, begin + 1);
        StyleValue v2 = LengthOrPercentage::parse(end - 1, end);
        if (v1.isValid() && v2.isValid()) {
            LengthOrPercentage lp1 = v1.to<LengthOrPercentage>();
            LengthOrPercentage lp2 = v2.to<LengthOrPercentage>();
            return StyleValue::custom(BorderRadius(lp1, lp2));
        }
        else {
            return StyleValue::invalid();
        }
    }
}

BorderRadiuses::BorderRadiuses(const StylableObject* obj)
    : BorderRadiuses(
        obj->style(strings::border_top_left_radius).to<BorderRadius>(),
        obj->style(strings::border_top_right_radius).to<BorderRadius>(),
        obj->style(strings::border_bottom_right_radius).to<BorderRadius>(),
        obj->style(strings::border_bottom_left_radius).to<BorderRadius>()) {
}

} // namespace vgc::style
