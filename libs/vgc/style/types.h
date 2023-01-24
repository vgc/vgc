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

#include <array>

#include <vgc/core/arithmetic.h>

#include <vgc/geometry/vec2f.h>

#include <vgc/style/api.h>
#include <vgc/style/metrics.h>
#include <vgc/style/value.h>

namespace vgc::style {

class StylableObject;

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
    constexpr Length()
        : value_(0)
        , unit_(LengthUnit::Dp) {
    }

    /// Constructs a length with the given value and unit.
    ///
    constexpr Length(float value, LengthUnit unit)
        : value_(value)
        , unit_(unit) {
    }

    /// Returns the numerical value of the length.
    ///
    constexpr float value() const {
        return value_;
    }

    /// Returns the unit of the length.
    ///
    constexpr LengthUnit unit() const {
        return unit_;
    }

    /// Returns the length converted to physical pixels, as a float.
    ///
    float toPx(const Metrics& metrics) const;

    /// Parses the given range of `StyleToken`s as a `Length`.
    ///
    /// Returns `StyleValue::invalid()` if the given tokens do not represent a
    /// valid `Length`. Otherwise, return a `StyleValue` holding a `Length`.
    ///
    static StyleValue parse(StyleTokenIterator begin, StyleTokenIterator end);

    /// Returns whether the two given `Length` are equal.
    ///
    friend constexpr bool operator==(const Length& l1, const Length& l2) {
        return l1.unit_ == l2.unit_ && l1.value_ == l2.value_;
    }

    /// Returns whether the two given `Length` are different.
    ///
    friend constexpr bool operator!=(const Length& l1, const Length& l2) {
        return !(l1 == l2);
    }

private:
    float value_;
    LengthUnit unit_;
};

namespace literals {

inline constexpr Length operator"" _dp(long double x) {
    return Length(static_cast<float>(x), LengthUnit::Dp);
}

inline constexpr Length operator"" _dp(unsigned long long int x) {
    return Length(static_cast<float>(x), LengthUnit::Dp);
}

} // namespace literals

/// \class vgc::style::Percentage
/// \brief A percentage value of a style property.
///
class VGC_STYLE_API Percentage {
public:
    /// Constructs a percentage of `0%`.
    ///
    constexpr Percentage()
        : value_(0) {
    }

    /// Constructs a percentage with the given value.
    ///
    constexpr Percentage(float value)
        : value_(value) {
    }

    /// Returns the numerical value of the percentage.
    ///
    constexpr float value() const {
        return value_;
    }

    /// Returns the `Percentage` converted to physical pixels, by multiplying
    /// the percentage with the given reference length.
    ///
    constexpr float toPx(float refLength) const {
        return value() * refLength * 0.01f;
    }

    /// Parses the given range of `StyleToken`s as a `Percentage`.
    ///
    /// Returns `StyleValue::invalid()` if the given tokens do not represent a
    /// valid `Percentage`. Otherwise, return a `StyleValue` holding a `Percentage`.
    ///
    static StyleValue parse(StyleTokenIterator begin, StyleTokenIterator end);

    /// Returns whether the two given `Percentage` are equal.
    ///
    friend constexpr bool operator==(const Percentage& p1, const Percentage& p2) {
        return p1.value_ == p2.value_;
    }

    /// Returns whether the two given `Percentage` are different.
    ///
    friend constexpr bool operator!=(const Percentage& p1, const Percentage& p2) {
        return !(p1 == p2);
    }

private:
    float value_ = 0;
};

/// \class vgc::style::LengthOrPercentage
/// \brief The value and unit of a style property that can be a length or a percentage.
///
class VGC_STYLE_API LengthOrPercentage {
public:
    /// Constructs a length of `0dp`.
    ///
    constexpr LengthOrPercentage()
        : value_(0)
        , unit_(LengthUnit::Dp)
        , isPercentage_(false) {
    }

    /// Constructs a length with the given value and unit.
    ///
    constexpr LengthOrPercentage(float value, LengthUnit unit)
        : value_(value)
        , unit_(unit)
        , isPercentage_(false) {
    }

    /// Constructs a percentage with the given value.
    ///
    constexpr LengthOrPercentage(float value)
        : value_(value)
        , unit_(LengthUnit::Dp) // unused
        , isPercentage_(true) {
    }

    /// Converts the given `Length` to a `LengthOrPercentage`.
    ///
    constexpr LengthOrPercentage(const Length& length)
        : value_(length.value())
        , unit_(length.unit())
        , isPercentage_(false) {
    }

    /// Converts the given `Percentage` to a `LengthOrPercentage`.
    ///
    constexpr LengthOrPercentage(const Percentage& percentage)
        : value_(percentage.value())
        , unit_(LengthUnit::Dp)
        , isPercentage_(true) {
    }

    /// Returns the numerical value of the length or percentage.
    ///
    constexpr float value() const {
        return value_;
    }

    /// Returns the unit of the length.
    ///
    constexpr LengthUnit unit() const {
        return unit_;
    }

    /// Returns whether this is a percentage.
    ///
    constexpr bool isPercentage() const {
        return isPercentage_;
    }

    /// Returns whether this is a length.
    ///
    constexpr bool isLength() const {
        return !isPercentage_;
    }

    /// Returns the `LengthOrPercentage` converted to physical pixels.
    ///
    /// The given `metrics` is used to convert a `Length` to `px`.
    ///
    /// The given `refLength` is used to convert a `Percentage` to `px`.
    ///
    float toPx(const Metrics& metrics, float refLength) const {
        return isPercentage_ ? Percentage(value_).toPx(refLength)
                             : Length(value_, unit_).toPx(metrics);
    }

    /// Parses the given range of `StyleToken`s as a `LengthOrPercentage`.
    ///
    /// Returns `StyleValue::invalid()` if the given tokens do not represent a
    /// valid `LengthOrPercentage`. Otherwise, return a `StyleValue` holding a
    /// `LengthOrPercentage`.
    ///
    static StyleValue parse(StyleTokenIterator begin, StyleTokenIterator end);

    /// Returns whether the two given `LengthOrPercentage` are equal.
    ///
    friend constexpr bool
    operator==(const LengthOrPercentage& v1, const LengthOrPercentage& v2) {
        return v1.isPercentage_ == v2.isPercentage_                //
               && (v1.isPercentage_ ? true : v1.unit_ == v2.unit_) //
               && v1.value_ == v2.value_;
    }

    /// Returns whether the two given `LengthOrPercentage` are different.
    ///
    friend constexpr bool
    operator!=(const LengthOrPercentage& v1, const LengthOrPercentage& v2) {
        return !(v1 == v2);
    }

private:
    float value_ = 0;
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
    constexpr LengthOrAuto()
        : value_(0)
        , unit_(LengthUnit::Dp)
        , isAuto_(true) {
    }

    /// Constructs a `LengthOrAuto` initialized to a length with the given value
    /// and unit.
    ///
    constexpr LengthOrAuto(float value, LengthUnit unit)
        : value_(value)
        , unit_(unit)
        , isAuto_(false) {
    }

    /// Converts the given `Length` to a `LengthOrAuto`.
    ///
    constexpr LengthOrAuto(const Length& length)
        : value_(length.value())
        , unit_(length.unit())
        , isAuto_(false) {
    }

    /// Returns whether this `LengthOrAuto` is `auto`.
    ///
    constexpr bool isAuto() const {
        return isAuto_;
    }

    /// Returns the numerical value of the length. This function assumes that
    /// `isAuto()` returns false.
    ///
    constexpr float value() const {
        return value_;
    }

    /// Returns the unit of the length. This function assumes that
    /// `isAuto()` returns false.
    ///
    constexpr LengthUnit unit() const {
        return unit_;
    }

    /// Returns the length converted to `px`.
    ///
    /// The `metrics` argument is used to convert a `Length` from to `px`.
    ///
    /// The `refLength` is used to convert a `Percentage` to `px`.
    ///
    float toPx(const Metrics& metrics, float valueIfAuto) const {
        return isAuto_ ? valueIfAuto : Length(value_, unit_).toPx(metrics);
    }

    /// Parses the given range of `StyleToken`s as a `LengthOrAuto`.
    ///
    /// Returns `StyleValue::invalid()` if the given tokens do not represent a
    /// valid `LengthOrAuto`. Otherwise, return a `StyleValue` holding a
    /// `LengthOrAuto`.
    ///
    static StyleValue parse(StyleTokenIterator begin, StyleTokenIterator end);

    /// Returns whether the two given `LengthOrAuto` are equal.
    ///
    friend constexpr bool operator==(const LengthOrAuto& v1, const LengthOrAuto& v2) {
        return v1.isAuto_ == v2.isAuto_
               && (v1.isAuto_
                       ? true
                       : Length(v1.value_, v1.unit_) == Length(v2.value_, v2.unit_));
    }

    /// Returns whether the two given `LengthOrAuto` are different.
    ///
    friend constexpr bool operator!=(const LengthOrAuto& v1, const LengthOrAuto& v2) {
        return !(v1 == v2);
    }

private:
    float value_;
    LengthUnit unit_;
    bool isAuto_;
};

/// \class vgc::style::LengthOrPercentageOrAuto
/// \brief A value which is either a `Length`, a `Percentage`, or the keyword `auto`.
///
class VGC_STYLE_API LengthOrPercentageOrAuto {
public:
    /// Constructs a `LengthOrPercentageOrAuto` initialized to a length with
    /// the given value and unit.
    ///
    constexpr LengthOrPercentageOrAuto(float value, LengthUnit unit)
        : value_(value)
        , unit_(unit)
        , type_(Type_::Length) {
    }

    /// Converts the given `Length` to a `LengthOrPercentageOrAuto`.
    ///
    constexpr LengthOrPercentageOrAuto(const Length& length)
        : value_(length.value())
        , unit_(length.unit())
        , type_(Type_::Length) {
    }

    /// Constructs a `LengthOrPercentageOrAuto` initialized to a percentage
    /// with the given value.
    ///
    constexpr LengthOrPercentageOrAuto(float value)
        : value_(value)
        , type_(Type_::Percentage) {
    }

    /// Constructs a `LengthOrPercentageOrAuto` initialized to `auto`.
    ///
    constexpr LengthOrPercentageOrAuto()
        : type_(Type_::Auto) {
    }

    /// Returns whether this `LengthOrPercentageOrAuto` is a length.
    ///
    constexpr bool isLength() const {
        return type_ == Type_::Length;
    }

    /// Returns whether this `LengthOrPercentageOrAuto` is a percentage.
    ///
    constexpr bool isPercentage() const {
        return type_ == Type_::Percentage;
    }

    /// Returns whether this `LengthOrPercentageOrAuto` is `auto`.
    ///
    constexpr bool isAuto() const {
        return type_ == Type_::Auto;
    }

    /// Returns the numerical value of the length or percentage. This function
    /// assumes that `isAuto()` returns false.
    ///
    constexpr float value() const {
        return value_;
    }

    /// Returns the unit of the length. This function assumes that
    /// `isLength()` returns true.
    ///
    constexpr LengthUnit unit() const {
        return unit_;
    }

    /// Converts this `LengthOrPercentageOrAuto` to a value in 'px'.
    ///
    /// The `scaleFactor` argument is used to convert a `Length` from `to `px`.
    ///
    /// The `refLength` is used to convert a `Percentage` to `px`.
    ///
    /// The `valueIfAuto` is the value that is returned if `isAuto()` is true.
    ///
    float toPx(const Metrics& metrics, float refLength, float valueIfAuto) const {
        switch (type_) {
        case Type_::Length:
            return Length(value_, unit_).toPx(metrics);
        case Type_::Percentage:
            return Percentage(value_).toPx(refLength);
        case Type_::Auto:
            return valueIfAuto;
        }
        return valueIfAuto; // silence warning
    }

    /// Parses the given range of `StyleToken`s as a `LengthOrPercentageOrAuto`.
    ///
    /// Returns `StyleValue::invalid()` if the given tokens do not represent a
    /// valid `LengthOrPercentageOrAuto`. Otherwise, return a `StyleValue`
    /// holding a `LengthOrPercentageOrAuto`.
    ///
    static StyleValue parse(StyleTokenIterator begin, StyleTokenIterator end);

    /// Returns whether the two given `LengthOrAuto` are equal.
    ///
    friend constexpr bool
    operator==(const LengthOrPercentageOrAuto& v1, const LengthOrPercentageOrAuto& v2) {
        switch (v1.type_) {
        case Type_::Length:
            return v2.isLength()
                   && Length(v1.value_, v1.unit_) == Length(v2.value_, v2.unit_);
        case Type_::Percentage:
            return v2.isPercentage() && Percentage(v1.value_) == Percentage(v2.value_);
        case Type_::Auto:
            return v2.isAuto();
        }
        return false; // silence warning
    }

    /// Returns whether the two given `LengthOrPercentageOrAuto` are different.
    ///
    friend constexpr bool
    operator!=(const LengthOrPercentageOrAuto& v1, const LengthOrPercentageOrAuto& v2) {
        return !(v1 == v2);
    }

private:
    enum class Type_ : Int8 {
        Length,
        Percentage,
        Auto
    };

    float value_ = 0;
    LengthUnit unit_ = LengthUnit::Dp;
    Type_ type_ = Type_::Auto;
};

/// \class vgc::style::BorderRadiusInPx
/// \brief Stores border radius information in physical pixels.
///
class BorderRadiusInPx {
public:
    /// Constructs a `BorderRadiusInPx` with both values set to `0dp`.
    ///
    constexpr BorderRadiusInPx()
        : radius_{0, 0} {
    }

    /// Constructs a `BorderRadiusInPx` with both horizontal and vertical
    /// radius values set to the given `radius`.
    ///
    constexpr BorderRadiusInPx(float radius)
        : radius_{radius, radius} {
    }

    /// Constructs a `BorderRadiusInPx` with the two given horizontal and vertical
    /// radius values.
    ///
    constexpr BorderRadiusInPx(float horizontalRadius, float verticalRadius)
        : radius_{horizontalRadius, verticalRadius} {
    }

    /// Returns the horizontal radius of this border radius.
    ///
    constexpr float horizontalRadius() const {
        return radius_[0];
    }

    /// Returns the vertical radius of this border radius.
    ///
    constexpr float verticalRadius() const {
        return radius_[1];
    }

    /// Returns the horizontal radius of this border radius as a reference.
    ///
    constexpr float& horizontalRadius() {
        return radius_[0];
    }

    /// Returns the vertical radius of this border radius as a reference.
    ///
    constexpr float& verticalRadius() {
        return radius_[1];
    }

    /// Sets the horizontal radius of this border radius.
    ///
    constexpr void setHorizontalRadius(float horizontalRadius) {
        radius_[0] = horizontalRadius;
    }

    /// Sets the vertical radius of this border radius.
    ///
    constexpr void setVerticalRadius(float verticalRadius) {
        radius_[1] = verticalRadius;
    }

    /// Returns the horizontal radius if `index` is `0`, and the vertical radius
    /// if `index` is `1`.
    ///
    constexpr float operator[](Int index) const {
        return radius_[static_cast<size_t>(index)];
    }

    /// Returns, as a reference, the horizontal radius if `index` is `0`, and
    /// the vertical radius if `index` is `1`.
    ///
    constexpr float& operator[](Int index) {
        return radius_[static_cast<size_t>(index)];
    }

    /// Returns a BorderRadiusInPx with the given offset applied.
    ///
    constexpr BorderRadiusInPx offsetted(float horizontal, float vertical) const {
        return BorderRadiusInPx(
            (std::max)(0.0f, radius_[0] + horizontal),
            (std::max)(0.0f, radius_[1] + vertical));
    }

    /// Returns whether the two given `BorderRadiusInPx` are equal.
    ///
    friend constexpr bool
    operator==(const BorderRadiusInPx& v1, const BorderRadiusInPx& v2) {
        // Note: std::array<T>::operator==() is not constexpr in C++17 (it is
        // in C++20), so we do it manually.
        return v1.radius_[0] == v2.radius_[0] && v1.radius_[1] == v2.radius_[1];
    }

    /// Returns whether the two given `BorderRadiusInPx` are different.
    ///
    friend constexpr bool
    operator!=(const BorderRadiusInPx& v1, const BorderRadiusInPx& v2) {
        return !(v1 == v2);
    }

private:
    std::array<float, 2> radius_;
};

/// \class vgc::style::BorderRadius
/// \brief A pair of LengthOrPercentage used to represent a rounded corner.
///
class VGC_STYLE_API BorderRadius {
public:
    /// Constructs a `BorderRadius` with both values set to `0dp`.
    ///
    constexpr BorderRadius()
        : horizontalRadius_(0, LengthUnit::Dp)
        , verticalRadius_(0, LengthUnit::Dp) {
    }

    /// Constructs a `BorderRadius` with both values set to the given
    /// `LengthOrPercentage`
    ///
    constexpr BorderRadius(const LengthOrPercentage& value)
        : horizontalRadius_(value)
        , verticalRadius_(value) {
    }

    /// Constructs a `BorderRadius` with the two given horizontal and vertical
    /// `LengthOrPercentage` radius values.
    ///
    constexpr BorderRadius(
        const LengthOrPercentage& horizontalRadius,
        const LengthOrPercentage& verticalRadius)

        : horizontalRadius_(horizontalRadius)
        , verticalRadius_(verticalRadius) {
    }

    /// Returns the horizontal radius of this border radius.
    ///
    constexpr LengthOrPercentage horizontalRadius() const {
        return horizontalRadius_;
    }

    /// Returns the vertical radius of this border radius.
    ///
    constexpr LengthOrPercentage verticalRadius() const {
        return verticalRadius_;
    }

    /// Converts the `BorderRadius` to physical pixels.
    ///
    /// The given `metrics` is used to convert non-percentage units to `px`,
    /// and the given `horizontalRefLength` (resp. `verticalRefLength`) is used
    /// to convert the horizontal radius (resp. vertical radius) when it is
    /// specified as a percentage.
    ///
    BorderRadiusInPx toPx(
        const Metrics& metrics,
        float horizontalRefLength,
        float verticalRefLength) const {

        return BorderRadiusInPx(
            horizontalRadius_.toPx(metrics, horizontalRefLength),
            verticalRadius_.toPx(metrics, verticalRefLength));
    }

    /// Parses the given range of `StyleToken`s as a `LengthOrAuto`.
    ///
    /// Returns `StyleValue::invalid()` if the given tokens do not represent a
    /// valid `LengthOrAuto`. Otherwise, return a `StyleValue` holding a
    /// `LengthOrAuto`.
    ///
    static StyleValue parse(StyleTokenIterator begin, StyleTokenIterator end);

    /// Returns whether the two given `BorderRadius` are equal.
    ///
    friend constexpr bool operator==(const BorderRadius& v1, const BorderRadius& v2) {
        return v1.horizontalRadius_ == v2.horizontalRadius_
               && v1.verticalRadius_ == v2.verticalRadius_;
    }

    /// Returns whether the two given `BorderRadius` are different.
    ///
    friend constexpr bool operator!=(const BorderRadius& v1, const BorderRadius& v2) {
        return !(v1 == v2);
    }

private:
    LengthOrPercentage horizontalRadius_;
    LengthOrPercentage verticalRadius_;
};

/// \class vgc::style::BorderRadiiInPx
/// \brief The border radii for the four corners in physical pixels
///
class BorderRadiiInPx {
public:
    /// Constructs a `BorderRadiiInPx` with all radii set to `0dp`.
    ///
    constexpr BorderRadiiInPx()
        : radii_{} {
    }

    /// Constructs a `BorderRadiiInPx` with all radii set to the given
    /// `BorderRadiiInPx`.
    ///
    constexpr BorderRadiiInPx(const BorderRadiusInPx& radius)
        : radii_{radius, radius, radius, radius} {
    }

    /// Constructs a `BorderRadii` with the top-left and bottom-right
    /// radii set to `topLeftAndBottomRight`, and the top-right and
    /// bottom-left radii set to `topRightAndBottomLeft`.
    ///
    constexpr BorderRadiiInPx(
        const BorderRadiusInPx& topLeftAndBottomRight,
        const BorderRadiusInPx& topRightAndBottomLeft)

        : radii_{
            topLeftAndBottomRight,
            topRightAndBottomLeft,
            topLeftAndBottomRight,
            topRightAndBottomLeft} {
    }

    /// Constructs a `BorderRadii` with the top-left radius set to
    /// `topLeft`, the top-right and bottom-left radii set to
    /// `topRightAndBottomLeft`, and the bottom-right radius set to
    /// `bottomRight`.
    ///
    constexpr BorderRadiiInPx(
        const BorderRadiusInPx& topLeft,
        const BorderRadiusInPx& topRightAndBottomLeft,
        const BorderRadiusInPx& bottomRight)

        : radii_{
            topLeft, //
            topRightAndBottomLeft,
            bottomRight,
            topRightAndBottomLeft} {
    }

    /// Constructs a `BorderRadii` with the four given `BorderRadius`.
    ///
    constexpr BorderRadiiInPx(
        const BorderRadiusInPx& topLeft,
        const BorderRadiusInPx& topRight,
        const BorderRadiusInPx& bottomRight,
        const BorderRadiusInPx& bottomLeft)

        : radii_{topLeft, topRight, bottomRight, bottomLeft} {
    }

    /// Returns the top left border radius.
    ///
    constexpr const BorderRadiusInPx& topLeft() const {
        return radii_[0];
    }

    /// Returns the top right border radius.
    ///
    constexpr const BorderRadiusInPx& topRight() const {
        return radii_[1];
    }

    /// Returns the bottom right border radius.
    ///
    constexpr const BorderRadiusInPx& bottomRight() const {
        return radii_[2];
    }

    /// Returns the bottom left border radius.
    ///
    constexpr const BorderRadiusInPx& bottomLeft() const {
        return radii_[3];
    }

    /// Returns the top left border radius as a reference.
    ///
    constexpr BorderRadiusInPx& topLeft() {
        return radii_[0];
    }

    /// Returns the top right border radius as a reference.
    ///
    constexpr BorderRadiusInPx& topRight() {
        return radii_[1];
    }

    /// Returns the bottom right border radius as a reference.
    ///
    constexpr BorderRadiusInPx& bottomRight() {
        return radii_[2];
    }

    /// Returns the bottom left border radius as a reference.
    ///
    constexpr BorderRadiusInPx& bottomLeft() {
        return radii_[3];
    }

    /// Sets the top left border radius.
    ///
    constexpr void setTopLeft(const BorderRadiusInPx& topLeft) {
        radii_[0] = topLeft;
    }

    /// Sets the top right border radius.
    ///
    constexpr void setTopRight(const BorderRadiusInPx& topRight) {
        radii_[1] = topRight;
    }

    /// Sets the bottom right border radius.
    ///
    constexpr void setBottomRight(const BorderRadiusInPx& bottomRight) {
        radii_[2] = bottomRight;
    }

    /// Sets the bottom left border radius.
    ///
    constexpr void setBottomLeft(const BorderRadiusInPx& bottomLeft) {
        radii_[3] = bottomLeft;
    }

    /// Returns one of the four border radius:
    ///
    /// - If `index` is `0`, returns the top-left radius.
    /// - If `index` is `1`, returns the top-right radius.
    /// - If `index` is `2`, returns the bottom-right radius.
    /// - If `index` is `3`, returns the bottom-left radius.
    ///
    constexpr const BorderRadiusInPx& operator[](Int index) const {
        return radii_[static_cast<size_t>(index)];
    }

    /// Returns a BorderRadiusInPx where each radius is non-negative, each
    /// horizontal radius does not exceed the given `width`, each vertical
    /// radius does not exceed the given `height`, and such that for each
    /// rectangle side, the sum of the two corresponding radii does not exceed
    /// the length of the rectangle side.
    ///
    /// The given `width` and `height` are assumed to be non-negative.
    ///
    constexpr BorderRadiiInPx clamped(float width, float height) const {
        BorderRadiiInPx res = *this;
        constexpr Int Horizontal = 0;
        constexpr Int Vertical = 1;
        clamp_(res.topLeft()[Horizontal], res.topRight()[Horizontal], width);
        clamp_(res.bottomLeft()[Horizontal], res.bottomRight()[Horizontal], width);
        clamp_(res.topLeft()[Vertical], res.bottomLeft()[Vertical], height);
        clamp_(res.topRight()[Vertical], res.bottomRight()[Vertical], height);
        return res;
    }

    /// Returns a BorderRadiusInPx with the given offset applied.
    ///
    constexpr BorderRadiiInPx offsetted(float horizontal, float vertical) const {
        return BorderRadiiInPx(
            radii_[0].offsetted(horizontal, vertical),
            radii_[1].offsetted(horizontal, vertical),
            radii_[2].offsetted(horizontal, vertical),
            radii_[3].offsetted(horizontal, vertical));
    }

    /// Returns a BorderRadiusInPx with the given offset applied.
    ///
    constexpr BorderRadiiInPx
    offsetted(float top, float right, float bottom, float left) const {
        return BorderRadiiInPx(
            topLeft().offsetted(left, top),
            topRight().offsetted(right, top),
            bottomRight().offsetted(right, bottom),
            bottomLeft().offsetted(left, bottom));
    }

    /// Returns whether the two given `BorderRadiiInPx` are equal.
    ///
    friend constexpr bool
    operator==(const BorderRadiiInPx& v1, const BorderRadiiInPx& v2) {
        // Note: std::array<T>::operator==() is not constexpr in C++17
        // (it is in C++20), so we do it manually.
        return v1.radii_[0] == v2.radii_[0]    //
               && v1.radii_[1] == v2.radii_[1] //
               && v1.radii_[2] == v2.radii_[2] //
               && v1.radii_[3] == v2.radii_[3];
    }

    /// Returns whether the two given `BorderRadiiInPx` are different.
    ///
    friend constexpr bool
    operator!=(const BorderRadiiInPx& v1, const BorderRadiiInPx& v2) {
        return !(v1 == v2);
    }

private:
    std::array<BorderRadiusInPx, 4> radii_;

    static constexpr void clamp_(float& x1, float& x2, float sumMax) {
        x1 = core::clamp(x1, 0.0f, sumMax);
        x2 = core::clamp(x2, 0.0f, sumMax);
        float overflow = (x1 + x2) - sumMax;
        if (overflow > 0) {
            float halfOverflow = 0.5f * overflow;
            x1 -= halfOverflow;
            x2 -= halfOverflow;
        }
    }
};

/// \class vgc::style::BorderRadii
/// \brief The border radii for the four corners
///
class VGC_STYLE_API BorderRadii {
public:
    /// Constructs a `BorderRadii` with all radii set to (0dp, 0dp).
    ///
    constexpr BorderRadii()
        : topLeft_()
        , topRight_()
        , bottomRight_()
        , bottomLeft_() {
    }

    /// Constructs a `BorderRadii` with all radii set to the given
    /// `BorderRadius`.
    ///
    constexpr BorderRadii(const BorderRadius& radius)
        : topLeft_(radius)
        , topRight_(radius)
        , bottomRight_(radius)
        , bottomLeft_(radius) {
    }

    /// Constructs a `BorderRadii` with the top-left and bottom-right
    /// radii set to `topLeftAndBottomRight`, and the top-right and
    /// bottom-left radii set to `topRightAndBottomLeft`.
    ///
    constexpr BorderRadii(
        const BorderRadius& topLeftAndBottomRight,
        const BorderRadius& topRightAndBottomLeft)

        : topLeft_(topLeftAndBottomRight)
        , topRight_(topRightAndBottomLeft)
        , bottomRight_(topLeftAndBottomRight)
        , bottomLeft_(topRightAndBottomLeft) {
    }

    /// Constructs a `BorderRadii` with the top-left radius set to
    /// `topLeft`, the top-right and bottom-left radii set to
    /// `topRightAndBottomLeft`, and the bottom-right radius set to
    /// `bottomRight`.
    ///
    constexpr BorderRadii(
        const BorderRadius& topLeft,
        const BorderRadius& topRightAndBottomLeft,
        const BorderRadius& bottomRight)

        : topLeft_(topLeft)
        , topRight_(topRightAndBottomLeft)
        , bottomRight_(bottomRight)
        , bottomLeft_(topRightAndBottomLeft) {
    }

    /// Constructs a `BorderRadii` with the four given `BorderRadius`.
    ///
    constexpr BorderRadii(
        const BorderRadius& topLeft,
        const BorderRadius& topRight,
        const BorderRadius& bottomRight,
        const BorderRadius& bottomLeft)

        : topLeft_(topLeft)
        , topRight_(topRight)
        , bottomRight_(bottomRight)
        , bottomLeft_(bottomLeft) {
    }

    /// Constructs a `BorderRadii` from the `border-radius` style properties
    /// of the given `StylableObject`.
    ///
    BorderRadii(const StylableObject* obj);

    /// Returns the top left border radius.
    ///
    constexpr const BorderRadius& topLeft() const {
        return topLeft_;
    }

    /// Returns the top right border radius.
    ///
    constexpr const BorderRadius& topRight() const {
        return topRight_;
    }

    /// Returns the bottom right border radius.
    ///
    constexpr const BorderRadius& bottomRight() const {
        return bottomRight_;
    }

    /// Returns the bottom left border radius.
    ///
    constexpr const BorderRadius& bottomLeft() const {
        return bottomLeft_;
    }

    /// Sets the top left border radius.
    ///
    constexpr void setTopLeft(const BorderRadius& topLeft) {
        topLeft_ = topLeft;
    }

    /// Sets the top right border radius.
    ///
    constexpr void setTopRight(const BorderRadius& topRight) {
        topRight_ = topRight;
    }

    /// Sets the bottom right border radius.
    ///
    constexpr void setBottomRight(const BorderRadius& bottomRight) {
        bottomRight_ = bottomRight;
    }

    /// Sets the bottom left border radius.
    ///
    constexpr void setBottomLeft(const BorderRadius& bottomLeft) {
        bottomLeft_ = bottomLeft;
    }

    /// Converts the `BorderRadius` to physical pixels.
    ///
    /// The given `metrics` is used to convert non-percentage units to `px`,
    /// and the given `horizontalRefLength` (resp. `verticalRefLength`) is used
    /// to convert horizontal radii (resp. vertical radii) when it is
    /// specified as a percentage.
    ///
    BorderRadiiInPx toPx(
        const Metrics& metrics,
        float horizontalRefLength,
        float verticalRefLength) const {
        return BorderRadiiInPx(
            topLeft_.toPx(metrics, horizontalRefLength, verticalRefLength),
            topRight_.toPx(metrics, horizontalRefLength, verticalRefLength),
            bottomRight_.toPx(metrics, horizontalRefLength, verticalRefLength),
            bottomLeft_.toPx(metrics, horizontalRefLength, verticalRefLength));
    }

    /// Returns whether the two given `BorderRadii` are equal.
    ///
    friend constexpr bool operator==(const BorderRadii& v1, const BorderRadii& v2) {
        return v1.topLeft_ == v2.topLeft_            //
               && v1.topRight_ == v2.topRight_       //
               && v1.bottomRight_ == v2.bottomRight_ //
               && v1.bottomLeft_ == v2.bottomLeft_;
    }

    /// Returns whether the two given `BorderRadii` are different.
    ///
    friend constexpr bool operator!=(const BorderRadii& v1, const BorderRadii& v2) {
        return !(v1 == v2);
    }

private:
    BorderRadius topLeft_;
    BorderRadius topRight_;
    BorderRadius bottomRight_;
    BorderRadius bottomLeft_;
};

} // namespace vgc::style

#endif // VGC_STYLE_TYPES_H
