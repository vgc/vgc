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

#ifndef VGC_STYLE_TYPES_H
#define VGC_STYLE_TYPES_H

#include <vgc/core/arithmetic.h>
#include <vgc/style/api.h>
#include <vgc/style/style.h>

namespace vgc::style {

/// \enum vgc::style::LengthUnit
/// \brief The unit of a length style property.
///
enum class LengthUnit : UInt8 {
    Dp
};

/// \class vgc::style::Length
/// \brief The value and unit of a length style property.
///
class VGC_STYLE_API Length {
public:
    /// Constructs a length of `0dp`.
    ///
    Length() {
    }

    /// Constructs a length with the given value and unit.
    ///
    Length(double value, LengthUnit unit)
        : value_(value)
        , unit_(unit) {
    }

    /// Returns the numerical value of the length.
    ///
    double value() const {
        return value_;
    }

    /// Returns the unit of the length.
    ///
    LengthUnit unit() const {
        return unit_;
    }

    /// Parses the given range of `StyleToken`s as a `Length`.
    ///
    /// Returns `StyleValue::invalid()` if the given tokens do not represent a
    /// valid `Length`. Otherwise, return a `StyleValue` holding a `Length`.
    ///
    static StyleValue parse(StyleTokenIterator begin, StyleTokenIterator end);

private:
    double value_ = 0;
    LengthUnit unit_ = LengthUnit::Dp;
};

/// \class vgc::style::Percentage
/// \brief A percentage value of a style property.
///
class VGC_STYLE_API Percentage {
public:
    /// Constructs a percentage of `0%`.
    ///
    Percentage() {
    }

    /// Constructs a percentage with the given value.
    ///
    Percentage(double value)
        : value_(value) {
    }

    /// Returns the numerical value of the percentage.
    ///
    double value() const {
        return value_;
    }

    /// Parses the given range of `StyleToken`s as a `Percentage`.
    ///
    /// Returns `StyleValue::invalid()` if the given tokens do not represent a
    /// valid `Percentage`. Otherwise, return a `StyleValue` holding a `Percentage`.
    ///
    static StyleValue parse(StyleTokenIterator begin, StyleTokenIterator end);

private:
    double value_ = 0;
};

/// \class vgc::style::LengthOrPercentage
/// \brief The value and unit of a style property that can be a length or a percentage.
///
class VGC_STYLE_API LengthOrPercentage {
public:
    /// Constructs a length of `0dp`.
    ///
    LengthOrPercentage() {
    }

    /// Constructs a length with the given value and unit.
    ///
    LengthOrPercentage(double value, LengthUnit unit)
        : value_(value)
        , unit_(unit) {
    }

    /// Constructs a percentage with the given value.
    ///
    LengthOrPercentage(double value)
        : value_(value)
        , isPercentage_(true) {
    }

    /// Returns the numerical value of the length or percentage.
    ///
    double value() const {
        return value_;
    }

    /// Returns the unit of the length.
    ///
    LengthUnit unit() const {
        return unit_;
    }

    /// Returns whether this is a percentage.
    ///
    bool isPercentage() const {
        return isPercentage_;
    }

    /// Parses the given range of `StyleToken`s as a `LengthOrPercentage`.
    ///
    /// Returns `StyleValue::invalid()` if the given tokens do not represent a
    /// valid `LengthOrPercentage`. Otherwise, return a `StyleValue` holding a
    /// `LengthOrPercentage`.
    ///
    static StyleValue parse(StyleTokenIterator begin, StyleTokenIterator end);

private:
    double value_ = 0;
    LengthUnit unit_ = LengthUnit::Dp;
    bool isPercentage_ = false;
};

/// \class vgc::style::LengthOrAuto
/// \brief A value which is either a `Length` or the keyword `auto`.
///
class VGC_STYLE_API LengthOrAuto {
public:
    /// Constructs a `LengthOrAuto` initialized to `auto`.
    ///
    LengthOrAuto() {
    }

    /// Constructs a `LengthOrAuto` initialized to a length with the given value
    /// and unit.
    ///
    LengthOrAuto(double value, LengthUnit unit)
        : value_(value)
        , unit_(unit)
        , isAuto_(false) {
    }

    /// Returns whether this `LengthOrAuto` is `auto`.
    ///
    bool isAuto() const {
        return isAuto_;
    }

    /// Returns the numerical value of the length. This function assumes that
    /// `isAuto()` returns false.
    ///
    double value() const {
        return value_;
    }

    /// Returns the unit of the length. This function assumes that
    /// `isAuto()` returns false.
    ///
    LengthUnit unit() const {
        return unit_;
    }

    /// Parses the given range of `StyleToken`s as a `LengthOrAuto`.
    ///
    /// Returns `StyleValue::invalid()` if the given tokens do not represent a
    /// valid `LengthOrAuto`. Otherwise, return a `StyleValue` holding a
    /// `LengthOrAuto`.
    ///
    static StyleValue parse(StyleTokenIterator begin, StyleTokenIterator end);

private:
    double value_ = 0;
    LengthUnit unit_ = LengthUnit::Dp;
    bool isAuto_ = true;
};

/// \class vgc::style::BorderRadius
/// \brief A pair of LengthOrPercentage used to represent a rounded corner.
///
class VGC_STYLE_API BorderRadius {
public:
    /// Constructs a `BorderRadius` with both values set to `0dp`.
    ///
    BorderRadius() {
    }

    /// Constructs a `BorderRadius` with both values set to the given
    /// `LengthOrPercentage`
    ///
    BorderRadius(const LengthOrPercentage& value)
        : horizontalRadius_(value)
        , verticalRadius_(value) {
    }

    /// Constructs a `BorderRadius` with the two given horizontal and vertical
    /// `LengthOrPercentage` radius values.
    ///
    BorderRadius(
        const LengthOrPercentage& horizontalRadius,
        const LengthOrPercentage& verticalRadius)

        : horizontalRadius_(horizontalRadius)
        , verticalRadius_(verticalRadius) {
    }

    /// Returns the horizontal radius of this border radius.
    ///
    LengthOrPercentage horizontalRadius() const {
        return horizontalRadius_;
    }

    /// Returns the vertical radius of this border radius.
    ///
    LengthOrPercentage verticalRadius() const {
        return verticalRadius_;
    }

    /// Parses the given range of `StyleToken`s as a `LengthOrAuto`.
    ///
    /// Returns `StyleValue::invalid()` if the given tokens do not represent a
    /// valid `LengthOrAuto`. Otherwise, return a `StyleValue` holding a
    /// `LengthOrAuto`.
    ///
    static StyleValue parse(StyleTokenIterator begin, StyleTokenIterator end);

private:
    LengthOrPercentage horizontalRadius_;
    LengthOrPercentage verticalRadius_;
};

} // namespace vgc::style

#endif // VGC_STYLE_TYPES_H
