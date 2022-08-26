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

} // namespace vgc::style

#endif // VGC_STYLE_STYLE_H
