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

    /// Returns the numerical value of the length as a `float`.
    ///
    float valuef() const {
        return static_cast<float>(value_);
    }

    /// Returns the unit of the length.
    ///
    LengthUnit unit() const {
        return unit_;
    }

    /// Returns the length converted to physical pixels, as a double. The
    /// `scaleFactor` argument represents how many physical pixels there is in
    /// a device-independent pixel (dp).
    ///
    double toPx(double scaleFactor) const;

    /// Returns the length converted to physical pixels, as a float. The
    /// `scaleFactor` argument represents how many physical pixels there is in
    /// a device-independent pixel (dp).
    ///
    float toPx(float scaleFactor) const;

    /// Parses the given range of `StyleToken`s as a `Length`.
    ///
    /// Returns `StyleValue::invalid()` if the given tokens do not represent a
    /// valid `Length`. Otherwise, return a `StyleValue` holding a `Length`.
    ///
    static StyleValue parse(StyleTokenIterator begin, StyleTokenIterator end);

    /// Returns whether the two given `Length` are equal.
    ///
    friend bool operator==(const Length& l1, const Length& l2) {
        return l1.unit_ == l2.unit_ && l1.value_ == l2.value_;
    }

    /// Returns whether the two given `Length` are different.
    ///
    friend bool operator!=(const Length& l1, const Length& l2) {
        return !(l1 == l2);
    }

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

    /// Returns the numerical value of the percentage as a `float`.
    ///
    float valuef() const {
        return static_cast<float>(value_);
    }

    /// Returns the `Percentage` converted to physical pixels, by multiplying
    /// the percentage with the given reference length.
    ///
    template<typename Float>
    Float toPx(Float refLength) const {
        return static_cast<Float>(value_) * refLength * static_cast<Float>(0.01);
    }

    /// Parses the given range of `StyleToken`s as a `Percentage`.
    ///
    /// Returns `StyleValue::invalid()` if the given tokens do not represent a
    /// valid `Percentage`. Otherwise, return a `StyleValue` holding a `Percentage`.
    ///
    static StyleValue parse(StyleTokenIterator begin, StyleTokenIterator end);

    /// Returns whether the two given `Percentage` are equal.
    ///
    friend bool operator==(const Percentage& p1, const Percentage& p2) {
        return p1.value_ == p2.value_;
    }

    /// Returns whether the two given `Percentage` are different.
    ///
    friend bool operator!=(const Percentage& p1, const Percentage& p2) {
        return !(p1 == p2);
    }

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

    /// Returns the numerical value of the length or percentage as a `float`.
    ///
    float valuef() const {
        return static_cast<float>(value_);
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

    /// Returns whether this is a length.
    ///
    bool isLength() const {
        return !isPercentage_;
    }

    /// Returns the `LengthOrPercentage` converted to physical pixels.
    ///
    /// The `scaleFactor` argument is used to convert a `Length` from `dp` to `px`.
    ///
    /// The`refLength` is used to convert a `Percentage` to `px`.
    ///
    template<typename Float>
    Float toPx(Float scaleFactor, Float refLength) const {
        return isPercentage_ ? Percentage(value_).toPx(refLength)
                             : Length(value_, unit_).toPx(scaleFactor);
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
    friend bool operator==(const LengthOrPercentage& v1, const LengthOrPercentage& v2) {
        return v1.isPercentage_ == v2.isPercentage_                //
               && (v1.isPercentage_ ? v1.unit_ == v2.unit_ : true) //
               && v1.value_ == v2.value_;
    }

    /// Returns whether the two given `LengthOrPercentage` are different.
    ///
    friend bool operator!=(const LengthOrPercentage& v1, const LengthOrPercentage& v2) {
        return !(v1 == v2);
    }

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

    /// Returns the numerical value of the length as a `float`. This function
    /// assumes that `isAuto()` returns false.
    ///
    float valuef() const {
        return static_cast<float>(value_);
    }

    /// Returns the unit of the length. This function assumes that
    /// `isAuto()` returns false.
    ///
    LengthUnit unit() const {
        return unit_;
    }

    /// Returns the length converted to physical pixels, as a double. The
    /// `scaleFactor` argument represents how many physical pixels there is in
    /// a device-independent pixel (dp). The `valueIfAuto` is the value that
    /// should be returned if `isAuto()` is true;
    ///
    template<typename Float>
    Float toPx(Float scaleFactor, Float valueIfAuto) const {
        return isAuto_ ? valueIfAuto : Length(value_, unit_).toPx(scaleFactor);
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
    friend bool operator==(const LengthOrAuto& v1, const LengthOrAuto& v2) {
        return v1.isAuto_ == v2.isAuto_
               && (v1.isAuto_
                       ? true
                       : Length(v1.value_, v1.unit_) == Length(v2.value_, v2.unit_));
    }

    /// Returns whether the two given `LengthOrAuto` are different.
    ///
    friend bool operator!=(const LengthOrAuto& v1, const LengthOrAuto& v2) {
        return !(v1 == v2);
    }

private:
    double value_ = 0;
    LengthUnit unit_ = LengthUnit::Dp;
    bool isAuto_ = true;
};

/// \class vgc::style::BorderRadiusInPx
/// \brief Stores border radius information in physical pixels.
///
template<typename Float>
class BorderRadiusInPx {
public:
    /// Constructs a `BorderRadiusInPx` with both values set to `0dp`.
    ///
    BorderRadiusInPx() {
    }

    /// Constructs a `BorderRadiusInPx` with both horizontal and vertical
    /// radius values set to the given `radius`.
    ///
    BorderRadiusInPx(Float radius)
        : radius_{radius, radius} {
    }

    /// Constructs a `BorderRadiusInPx` with the two given horizontal and vertical
    /// radius values.
    ///
    BorderRadiusInPx(Float horizontalRadius, Float verticalRadius)
        : radius_{horizontalRadius, verticalRadius} {
    }

    /// Returns the horizontal radius of this border radius.
    ///
    Float horizontalRadius() const {
        return radius_[0];
    }

    /// Returns the vertical radius of this border radius.
    ///
    Float verticalRadius() const {
        return radius_[1];
    }

    /// Returns the horizontal radius if `index` is `0`, and the vertical radius
    /// if `index` is `1`.
    ///
    Float operator[](Int index) const {
        return radius_[static_cast<size_t>(index)];
    }

    /// Returns a BorderRadiusInPx with the given offset applied.
    ///
    BorderRadiusInPx offsetted(Float horizontal, Float vertical) const {
        return BorderRadiusInPx(
            (std::max)(static_cast<Float>(0), radius_[0] + horizontal),
            (std::max)(static_cast<Float>(0), radius_[1] + vertical));
    }

    /// Returns whether the two given `BorderRadiusInPx` are equal.
    ///
    friend bool operator==(const BorderRadiusInPx& v1, const BorderRadiusInPx& v2) {
        return v1.radius_ == v2.radius_;
    }

    /// Returns whether the two given `BorderRadiusInPx` are different.
    ///
    friend bool operator!=(const BorderRadiusInPx& v1, const BorderRadiusInPx& v2) {
        return !(v1 == v2);
    }

private:
    std::array<Float, 2> radius_;
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

    /// Converts the `BorderRadius` to physical pixels.
    ///
    /// The given `scaleFactor` is used to convert `dp` units to `px`, and the given
    /// `horizontalRefLength` (resp. `verticalRefLength`) is used to convert
    /// the horizontal radius (resp. vertical radius) when it is specified as a
    /// percentage.
    ///
    template<typename Float>
    BorderRadiusInPx<Float>
    toPx(Float scaleFactor, Float horizontalRefLength, Float verticalRefLength) const {
        return BorderRadiusInPx<Float>(
            horizontalRadius_.toPx(scaleFactor, horizontalRefLength),
            verticalRadius_.toPx(scaleFactor, verticalRefLength));
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
    friend bool operator==(const BorderRadius& v1, const BorderRadius& v2) {
        return v1.horizontalRadius_ == v2.horizontalRadius_
               && v1.verticalRadius_ == v2.verticalRadius_;
    }

    /// Returns whether the two given `BorderRadius` are different.
    ///
    friend bool operator!=(const BorderRadius& v1, const BorderRadius& v2) {
        return !(v1 == v2);
    }

private:
    LengthOrPercentage horizontalRadius_;
    LengthOrPercentage verticalRadius_;
};

/// \class vgc::style::BorderRadiusesInPx
/// \brief The border radiuses for the four corners in physical pixels
///
template<typename Float>
class BorderRadiusesInPx {
public:
    /// Constructs a `BorderRadiusesInPx` with all radiuses set to the given
    /// `BorderRadiusesInPx`.
    ///
    BorderRadiusesInPx(const BorderRadiusInPx<Float>& radius)
        : radiuses_{radius, radius, radius, radius} {
    }

    /// Constructs a `BorderRadiuses` with the top-left and bottom-right
    /// radiuses set to `topLeftAndBottomRight`, and the top-right and
    /// bottom-left radiuses set to `topRightAndBottomLeft`.
    ///
    BorderRadiusesInPx(
        const BorderRadiusInPx<Float>& topLeftAndBottomRight,
        const BorderRadiusInPx<Float>& topRightAndBottomLeft)

        : radiuses_{
            topLeftAndBottomRight,
            topRightAndBottomLeft,
            topLeftAndBottomRight,
            topRightAndBottomLeft} {
    }

    /// Constructs a `BorderRadiuses` with the top-left radius set to
    /// `topLeft`, the top-right and bottom-left radiuses set to
    /// `topRightAndBottomLeft`, and the bottom-right radius set to
    /// `bottomRight`.
    ///
    BorderRadiusesInPx(
        const BorderRadiusInPx<Float>& topLeft,
        const BorderRadiusInPx<Float>& topRightAndBottomLeft,
        const BorderRadiusInPx<Float>& bottomRight)

        : radiuses_{
            topLeft, //
            topRightAndBottomLeft,
            bottomRight,
            topRightAndBottomLeft} {
    }

    /// Constructs a `BorderRadiuses` with the four given `BorderRadius`.
    ///
    BorderRadiusesInPx(
        const BorderRadiusInPx<Float>& topLeft,
        const BorderRadiusInPx<Float>& topRight,
        const BorderRadiusInPx<Float>& bottomRight,
        const BorderRadiusInPx<Float>& bottomLeft)

        : radiuses_{topLeft, topRight, bottomRight, bottomLeft} {
    }

    /// Returns the top left border radius.
    ///
    const BorderRadiusInPx<Float>& topLeft() const {
        return radiuses_[0];
    }

    /// Returns the top right border radius.
    ///
    const BorderRadiusInPx<Float>& topRight() const {
        return radiuses_[1];
    }

    /// Returns the bottom right border radius.
    ///
    const BorderRadiusInPx<Float>& bottomRight() const {
        return radiuses_[2];
    }

    /// Return thes bottom left border radius.
    ///
    const BorderRadiusInPx<Float>& bottomLeft() const {
        return radiuses_[3];
    }

    /// Sets the top left border radius.
    ///
    void setTopLeft(const BorderRadiusInPx<Float>& topLeft) {
        radiuses_[0] = topLeft;
    }

    /// Sets the top right border radius.
    ///
    void setTopRight(const BorderRadiusInPx<Float>& topRight) {
        radiuses_[1] = topRight;
    }

    /// Sets the bottom right border radius.
    ///
    void setBottomRight(const BorderRadiusInPx<Float>& bottomRight) {
        radiuses_[2] = bottomRight;
    }

    /// Sets the bottom left border radius.
    ///
    void setBottomLeft(const BorderRadiusInPx<Float>& bottomLeft) {
        radiuses_[3] = bottomLeft;
    }

    /// Returns one of the four border radius:
    ///
    /// - If `index` is `0`, returns the top-left radius.
    /// - If `index` is `1`, returns the top-right radius.
    /// - If `index` is `2`, returns the bottom-right radius.
    /// - If `index` is `3`, returns the bottom-left radius.
    ///
    const BorderRadiusInPx<Float>& operator[](Int index) const {
        return radiuses_[static_cast<size_t>(index)];
    }

    /// Returns a BorderRadiusInPx with the given offset applied.
    ///
    BorderRadiusesInPx offsetted(Float horizontal, Float vertical) const {
        return BorderRadiusesInPx(
            radiuses_[0].offsetted(horizontal, vertical),
            radiuses_[1].offsetted(horizontal, vertical),
            radiuses_[2].offsetted(horizontal, vertical),
            radiuses_[3].offsetted(horizontal, vertical));
    }

    /// Returns a BorderRadiusInPx with the given offset applied.
    ///
    BorderRadiusesInPx offsetted(Float top, Float right, Float bottom, Float left) const {
        return BorderRadiusesInPx(
            topLeft().offsetted(left, top),
            topRight().offsetted(right, top),
            bottomRight().offsetted(right, bottom),
            bottomLeft().offsetted(left, bottom));
    }

    /// Returns whether the two given `BorderRadiusesInPx` are equal.
    ///
    friend bool operator==(const BorderRadiusesInPx& v1, const BorderRadiusesInPx& v2) {
        return v1.radiuses_ == v2.radiuses_;
    }

    /// Returns whether the two given `BorderRadiusesInPx` are different.
    ///
    friend bool operator!=(const BorderRadiusesInPx& v1, const BorderRadiusesInPx& v2) {
        return !(v1 == v2);
    }

private:
    std::array<BorderRadiusInPx<Float>, 4> radiuses_;
};

/// \class vgc::style::BorderRadiuses
/// \brief The border radiuses for the four corners
///
class VGC_STYLE_API BorderRadiuses {
public:
    /// Constructs a `BorderRadiuses` with all radiuses set to (0dp, 0dp).
    ///
    BorderRadiuses()
        : topLeft_()
        , topRight_()
        , bottomRight_()
        , bottomLeft_() {
    }

    /// Constructs a `BorderRadiuses` with all radiuses set to the given
    /// `BorderRadius`.
    ///
    BorderRadiuses(const BorderRadius& radius)
        : topLeft_(radius)
        , topRight_(radius)
        , bottomRight_(radius)
        , bottomLeft_(radius) {
    }

    /// Constructs a `BorderRadiuses` with the top-left and bottom-right
    /// radiuses set to `topLeftAndBottomRight`, and the top-right and
    /// bottom-left radiuses set to `topRightAndBottomLeft`.
    ///
    BorderRadiuses(
        const BorderRadius& topLeftAndBottomRight,
        const BorderRadius& topRightAndBottomLeft)

        : topLeft_(topLeftAndBottomRight)
        , topRight_(topRightAndBottomLeft)
        , bottomRight_(topLeftAndBottomRight)
        , bottomLeft_(topRightAndBottomLeft) {
    }

    /// Constructs a `BorderRadiuses` with the top-left radius set to
    /// `topLeft`, the top-right and bottom-left radiuses set to
    /// `topRightAndBottomLeft`, and the bottom-right radius set to
    /// `bottomRight`.
    ///
    BorderRadiuses(
        const BorderRadius& topLeft,
        const BorderRadius& topRightAndBottomLeft,
        const BorderRadius& bottomRight)

        : topLeft_(topLeft)
        , topRight_(topRightAndBottomLeft)
        , bottomRight_(bottomRight)
        , bottomLeft_(topRightAndBottomLeft) {
    }

    /// Constructs a `BorderRadiuses` with the four given `BorderRadius`.
    ///
    BorderRadiuses(
        const BorderRadius& topLeft,
        const BorderRadius& topRight,
        const BorderRadius& bottomRight,
        const BorderRadius& bottomLeft)

        : topLeft_(topLeft)
        , topRight_(topRight)
        , bottomRight_(bottomRight)
        , bottomLeft_(bottomLeft) {
    }

    /// Returns the top left border radius.
    ///
    const BorderRadius& topLeft() const {
        return topLeft_;
    }

    /// Returns the top right border radius.
    ///
    const BorderRadius& topRight() const {
        return topRight_;
    }

    /// Returns the bottom right border radius.
    ///
    const BorderRadius& bottomRight() const {
        return bottomRight_;
    }

    /// Return thes bottom left border radius.
    ///
    const BorderRadius& bottomLeft() const {
        return bottomLeft_;
    }

    /// Sets the top left border radius.
    ///
    void setTopLeft(const BorderRadius& topLeft) {
        topLeft_ = topLeft;
    }

    /// Sets the top right border radius.
    ///
    void setTopRight(const BorderRadius& topRight) {
        topRight_ = topRight;
    }

    /// Sets the bottom right border radius.
    ///
    void setBottomRight(const BorderRadius& bottomRight) {
        bottomRight_ = bottomRight;
    }

    /// Sets the bottom left border radius.
    ///
    void setBottomLeft(const BorderRadius& bottomLeft) {
        bottomLeft_ = bottomLeft;
    }

    /// Converts the `BorderRadius` to physical pixels.
    ///
    /// The given `scaleFactor` is used to convert `dp` units to `px`, and the given
    /// `horizontalRefLength` (resp. `verticalRefLength`) is used to convert
    /// horizontal radiuses (resp. vertical radiuses) when it is specified as a
    /// percentage.
    ///
    template<typename Float>
    BorderRadiusesInPx<Float>
    toPx(Float scaleFactor, Float horizontalRefLength, Float verticalRefLength) const {
        return BorderRadiusesInPx<Float>(
            topLeft_.toPx(scaleFactor, horizontalRefLength, verticalRefLength),
            topRight_.toPx(scaleFactor, horizontalRefLength, verticalRefLength),
            bottomRight_.toPx(scaleFactor, horizontalRefLength, verticalRefLength),
            bottomLeft_.toPx(scaleFactor, horizontalRefLength, verticalRefLength));
    }

    /// Returns whether the two given `BorderRadiuses` are equal.
    ///
    friend bool operator==(const BorderRadiuses& v1, const BorderRadiuses& v2) {
        return v1.topLeft_ == v2.topLeft_            //
               && v1.topRight_ == v2.topRight_       //
               && v1.bottomRight_ == v2.bottomRight_ //
               && v1.bottomLeft_ == v2.bottomLeft_;
    }

    /// Returns whether the two given `BorderRadiuses` are different.
    ///
    friend bool operator!=(const BorderRadiuses& v1, const BorderRadiuses& v2) {
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
