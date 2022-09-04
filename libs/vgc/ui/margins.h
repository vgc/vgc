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

#ifndef VGC_UI_MARGINS_H
#define VGC_UI_MARGINS_H

#include <vgc/geometry/rect2f.h>
#include <vgc/geometry/vec4f.h>
#include <vgc/ui/api.h>

namespace vgc::ui {

/// \class vgc::ui::Margins
/// \brief Represents a set of 4 margins for the 4 sides of a UI element.
///
/// Note that margins are allowed to be negative, in which case adding a margin
/// to a rect would shrink the rect.
///
// VGC_UI_API <- Omitted on purpose.
//               If needed, manually export individual functions.
class Margins {
private:
    enum Indices_ {
        Top = 0,
        Right,
        Bottom,
        Left
    };

public:
    /// Constructs a `Margins` with all margins set to 0.
    ///
    constexpr Margins()
        : v_() {
    }

    /// Constructs a `Margins` with all margins set to the given `margin`.
    ///
    constexpr Margins(float margin)
        : v_(margin, margin, margin, margin) {
    }

    /// Constructs a `Margins` with the top and bottom margins set to `topBottom`,
    /// and the left and right margins set to `leftRight`.
    ///
    constexpr Margins(float topBottom, float leftRight)
        : v_(topBottom, leftRight, topBottom, leftRight) {
    }

    /// Constructs a `Margins` with the given margins.
    ///
    constexpr Margins(float top, float right, float bottom, float left)
        : v_(top, right, bottom, left) {
    }

    /// Constructs a `Margins` with top, right, bottom, and left respectively
    /// set to x, y, z, and w of `v`.
    ///
    explicit constexpr Margins(const geometry::Vec4f& v)
        : v_(v) {
    }

    /// Constructs a `Margins` as the space between an outer rectangle and an inner rectangle.
    ///
    constexpr Margins(
        const geometry::Rect2f& outerRect,
        const geometry::Rect2f& innerRect)

        : v_(
            innerRect.yMin() - outerRect.yMin(),
            outerRect.xMax() - innerRect.xMax(),
            outerRect.yMax() - innerRect.yMax(),
            innerRect.xMin() - outerRect.xMin()) {
    }

    /// Returns the margins as `Vec4f(top, right, bottom, left)`.
    ///
    constexpr const geometry::Vec4f& toVec4f() const {
        return v_;
    }

    /// Returns the top margin.
    ///
    constexpr float top() const {
        return v_[Top];
    }

    /// Sets the top margin.
    ///
    constexpr void setTop(float margin) {
        v_[Top] = margin;
    }

    /// Returns the right margin.
    ///
    constexpr float right() const {
        return v_[Right];
    }

    /// Sets the right margin.
    ///
    constexpr void setRight(float margin) {
        v_[Right] = margin;
    }

    /// Returns the bottom margin.
    ///
    constexpr float bottom() const {
        return v_[Bottom];
    }

    /// Sets the bottom margin.
    ///
    constexpr void setBottom(float margin) {
        v_[Bottom] = margin;
    }

    /// Returns the left margin.
    ///
    constexpr float left() const {
        return v_[Left];
    }

    /// Sets the left margin.
    ///
    constexpr void setLeft(float margin) {
        v_[Left] = margin;
    }

    /// Returns the sum of the left and right margins.
    ///
    constexpr float horizontalSum() const {
        return v_[Left] + v_[Right];
    }

    /// Returns the sum of the top and bottom margins.
    ///
    constexpr float verticalSum() const {
        return v_[Top] + v_[Bottom];
    }

    /// Rounds the value of each margin to the closest integer.
    ///
    void round() {
        v_[Top] = std::round(v_[Top]);
        v_[Right] = std::round(v_[Right]);
        v_[Bottom] = std::round(v_[Bottom]);
        v_[Left] = std::round(v_[Left]);
    }

    /// Returns a copy with each margin rounded to the closest integer.
    ///
    Margins rounded() {
        Margins copy = *this;
        copy.round();
        return copy;
    }

    /// Returns whether all margins of `this` are equal to the margins of `other`.
    ///
    constexpr bool operator==(const Margins& other) {
        return v_ == other.v_;
    }

    /// Returns whether any margin of `this` is different from the corresponding margin in `other`.
    ///
    constexpr bool operator!=(const Margins& other) {
        return v_ != other.v_;
    }

    /// Stretches each margin by the given `offset`.
    ///
    /// Returns a reference to `this`.
    ///
    Margins& operator+=(float offset) {
        v_[Top] += offset;
        v_[Right] += offset;
        v_[Bottom] += offset;
        v_[Left] += offset;
        return *this;
    }

    /// Returns a copy of `this` with each margin stretched by the given `offset`.
    ///
    Margins operator+(float offset) const {
        Margins ret = *this;
        ret += offset;
        return ret;
    }

    /// Stretches each margin by the corresponding margin in `other`.
    ///
    /// Returns a reference to `this`.
    ///
    Margins& operator+=(const Margins& other) {
        v_ += other.v_;
        return *this;
    }

    /// Returns a copy of `m1` with each margin stretched by the corresponding margin in `m2`.
    ///
    friend Margins operator+(const Margins& m1, const Margins& m2) {
        return Margins(m1.v_ + m2.v_);
    }

    /// Returns a copy of `this` with each margin negated.
    ///
    Margins operator-() {
        return Margins(-v_);
    }

    /// Shrinks each margin by the given `offset`.
    ///
    Margins& operator-=(float offset) {
        v_[Top] -= offset;
        v_[Right] -= offset;
        v_[Bottom] -= offset;
        v_[Left] -= offset;
        return *this;
    }

    /// Returns a copy of `this` with each margin shrinked by the given `offset`.
    ///
    Margins operator-(float offset) const {
        Margins ret = *this;
        ret -= offset;
        return ret;
    }

    /// Shrinks each margin by the corresponding margin in `other`.
    ///
    /// Returns a reference to `this`.
    ///
    Margins& operator-=(const Margins& other) {
        v_ -= other.v_;
        return *this;
    }

    /// Returns a copy of `m1` with each margin shrinked by the corresponding margin in `m2`.
    ///
    friend Margins operator-(const Margins& m1, const Margins& m2) {
        return Margins(m1.v_ - m2.v_);
    }

    /// Scales each margin by the given `scale`.
    ///
    Margins& operator*=(float scale) {
        v_ *= scale;
        return *this;
    }

    /// Returns a copy of `margins` with each margin scaled by the given `scale`.
    ///
    friend Margins operator*(const Margins& margins, float scale) {
        return Margins(margins.v_ * scale);
    }

    /// Returns a copy of `margins` with each margin scaled by the given `scale`.
    ///
    friend Margins operator*(float scale, const Margins& margins) {
        return Margins(margins.v_ * scale);
    }

    /// Divides each margin by the given `divisor`.
    ///
    Margins& operator/=(float divisor) {
        v_ /= divisor;
        return *this;
    }

    /// Returns a copy of `margins` with each margin divided by the given `divisor`.
    ///
    friend Margins operator/(const Margins& margins, float divisor) {
        return Margins(margins.v_ / divisor);
    }

private:
    geometry::Vec4f v_;
};

/// Returns a copy of `rect` stretched (offsetted outwards) by `margins`.
///
inline geometry::Rect2f operator+(const geometry::Rect2f& rect, const Margins& margins) {
    return geometry::Rect2f(
        rect.xMin() - margins.left(),
        rect.yMin() - margins.top(),
        rect.xMax() + margins.right(),
        rect.yMax() + margins.bottom());
}

/// Returns a copy of `rect` shrinked (offsetted inwards) by `margins`.
///
/// This operator is convenient to apply padding.
///
inline geometry::Rect2f operator-(const geometry::Rect2f& rect, const Margins& margins) {
    return geometry::Rect2f(
        rect.xMin() + margins.left(),
        rect.yMin() + margins.top(),
        rect.xMax() - margins.right(),
        rect.yMax() - margins.bottom());
}

} // namespace vgc::ui

#endif // VGC_UI_MARGINS_H
