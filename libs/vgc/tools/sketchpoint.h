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

#ifndef VGC_TOOLS_SKETCHPOINT_H
#define VGC_TOOLS_SKETCHPOINT_H

#include <vgc/core/array.h>
#include <vgc/geometry/vec2d.h>
#include <vgc/tools/api.h>

namespace vgc::tools {

/// \class vgc::tools::SketchPoint
/// \brief Stores data about one sample when sketching
///
class VGC_TOOLS_API SketchPoint {
public:
    /// Creates a zero-initialized `SketchPoint`.
    ///
    constexpr SketchPoint() noexcept
        : position_()
        , pressure_(0)
        , timestamp_(0)
        , width_(0)
        , s_(0) {
    }

    VGC_WARNING_PUSH
    VGC_WARNING_MSVC_DISABLE(26495) // member variable uninitialized

    /// Creates an uninitialized `SketchPoint`.
    ///
    SketchPoint(core::NoInit) noexcept
        : position_(core::noInit) {
    }

    VGC_WARNING_POP

    /// Creates a `SketchPoint` initialized with the given values.
    ///
    constexpr SketchPoint(
        const geometry::Vec2d& position,
        double pressure,
        double timestamp,
        double width,
        double s = 0) noexcept

        : position_(position)
        , pressure_(pressure)
        , timestamp_(timestamp)
        , width_(width)
        , s_(s) {
    }

    /// Returns the position of this `SketchPoint`.
    ///
    constexpr const geometry::Vec2d& position() const {
        return position_;
    }

    /// Sets the position of this `SketchPoint`.
    ///
    constexpr void setPosition(const geometry::Vec2d& position) {
        position_ = position;
    }

    /// Returns the pressure of this `SketchPoint`.
    ///
    constexpr double pressure() const {
        return pressure_;
    }

    /// Sets the pressure of this `SketchPoint`.
    ///
    constexpr void setPressure(double pressure) {
        pressure_ = pressure;
    }

    /// Returns the timestamp of this `SketchPoint`.
    ///
    constexpr double timestamp() const {
        return timestamp_;
    }

    /// Sets the timestamp of this `SketchPoint`.
    ///
    constexpr void setTimestamp(double timestamp) {
        timestamp_ = timestamp;
    }

    /// Returns the width of this `SketchPoint`.
    ///
    constexpr double width() const {
        return width_;
    }

    /// Sets the width of this `SketchPoint`.
    ///
    constexpr void setWidth(double width) {
        width_ = width;
    }

    /// Returns the cumulative chordal distance from the first point to this
    /// point.
    ///
    constexpr double s() const {
        return s_;
    }

    /// Sets the cumulative chordal distance from the first point to this
    /// point.
    ///
    constexpr void setS(double s) {
        s_ = s;
    }

    /// Adds in-place `other` to this `SketchPoint`.
    ///
    constexpr SketchPoint& operator+=(const SketchPoint& other) {
        position_ += other.position_;
        pressure_ += other.pressure_;
        timestamp_ += other.timestamp_;
        width_ += other.width_;
        s_ += other.s_;
        return *this;
    }

    /// Returns the addition of the two points `p1` and `p2`.
    ///
    /// This adds all components (position, pressure, timestamp, width, and s),
    /// which is useful to compute linear combinations, e.g.: `0.5 * (p1 + p2)`
    ///
    friend constexpr SketchPoint operator+(const SketchPoint& p1, const SketchPoint& p2) {
        return SketchPoint(p1) += p2;
    }

    /// Returns a copy of this `SketchPoint` (unary plus operator).
    ///
    constexpr SketchPoint operator+() const {
        return *this;
    }

    /// Substracts in-place `other` from this `SketchPoint`.
    ///
    constexpr SketchPoint& operator-=(const SketchPoint& other) {
        position_ -= other.position_;
        pressure_ -= other.pressure_;
        timestamp_ -= other.timestamp_;
        width_ -= other.width_;
        s_ -= other.s_;
        return *this;
    }

    /// Returns the substraction of `p1` and `p2`.
    ///
    friend constexpr SketchPoint operator-(const SketchPoint& p1, const SketchPoint& p2) {
        return SketchPoint(p1) -= p2;
    }

    /// Returns the opposite of this `SketchPoint` (unary minus operator).
    ///
    constexpr SketchPoint operator-() const {
        return SketchPoint(-position_, -pressure_, -timestamp_, -width_, -s_);
    }

    /// Multiplies in-place this `SketchPoint` by the scalar `s`.
    ///
    constexpr SketchPoint& operator*=(double s) {
        position_ *= s;
        pressure_ *= s;
        timestamp_ *= s;
        width_ *= s;
        s_ *= s;
        return *this;
    }

    /// Returns the multiplication of this `SketchPoint` by the scalar `s`.
    ///
    constexpr SketchPoint operator*(double s) const {
        return SketchPoint(*this) *= s;
    }

    /// Returns the multiplication of the scalar `s` with the vector `v`.
    ///
    /// This multiplies all components (position, pressure, timestamp, width, and s),
    /// which is useful to compute linear combinations, e.g.: `0.5 * (p1 + p2)`
    ///
    friend constexpr SketchPoint operator*(double s, const SketchPoint& p) {
        return p * s;
    }

    /// Divides in-place this `SketchPoint` by the scalar `s`.
    ///
    constexpr SketchPoint& operator/=(double s) {
        position_ /= s;
        pressure_ /= s;
        timestamp_ /= s;
        width_ /= s;
        s_ /= s;
        return *this;
    }

    /// Returns the division of this `SketchPoint` by the scalar `s`.
    ///
    constexpr SketchPoint operator/(double s) const {
        return SketchPoint(*this) /= s;
    }

    /// Returns whether `p1` and `p2` are equal, that is, if all their
    /// components are equal (position, pressure, timestamp, width, and s).
    ///
    friend constexpr bool operator==(const SketchPoint& p1, const SketchPoint& p2) {
        return p1.position_ == p2.position_      //
               && p1.pressure_ == p2.pressure_   //
               && p1.timestamp_ == p2.timestamp_ //
               && p1.width_ == p2.width_         //
               && p1.s_ == p2.s_;
    }

    /// Returns whether `p1` and `p2` are different.
    ///
    friend constexpr bool operator!=(const SketchPoint& p1, const SketchPoint& p2) {
        return !(p1 == p2);
    }

    /// Compares the timestamps of `p1` and `p2`.
    ///
    /// Note that because `<` compares only the timestamps, while `==` tests for equality
    /// of all components (position, pressure, timestamp, width, and s), the following
    /// can all be true at the same time:
    ///
    /// - !(p1 < p2)
    /// - !(p2 < p1)
    /// - p1 != p2
    ///
    friend constexpr bool operator<(const SketchPoint& p1, const SketchPoint& p2) {
        return p1.timestamp_ < p2.timestamp_;
    }

    /// Compares the timestamps of `p1` and `p2`.
    ///
    friend constexpr bool operator<=(const SketchPoint& p1, const SketchPoint& p2) {
        return !(p2 < p1);
    }

    /// Compares the timestamps of `p1` and `p2`.
    ///
    friend constexpr bool operator>(const SketchPoint& p1, const SketchPoint& p2) {
        return p2 < p1;
    }

    /// Compares the timestamps of `p1` and `p2`.
    ///
    friend constexpr bool operator>=(const SketchPoint& p1, const SketchPoint& p2) {
        return !(p1 < p2);
    }

private:
    geometry::Vec2d position_;
    double pressure_;
    double timestamp_;
    double width_;
    double s_;
};

using SketchPointArray = core::Array<SketchPoint>;

} // namespace vgc::tools

#endif // VGC_TOOLS_SKETCHPOINT_H
