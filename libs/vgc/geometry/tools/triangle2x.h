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

// This file is used to generate all the variants of this class.
// You must manually run generate.py after any modification.

// clang-format off

#ifndef VGC_GEOMETRY_TRIANGLE2X_H
#define VGC_GEOMETRY_TRIANGLE2X_H

#include <cmath>

#include <vgc/core/array.h>
#include <vgc/geometry/api.h>
#include <vgc/geometry/stride.h>
#include "vec2x.h"

namespace vgc::geometry {

/// \class vgc::geometry::Triangle2x
/// \brief 2D triangle using %scalar-type-description%.
///
/// A Triangle2x represents a triangle, that is, a triplet of points A, B, and
/// C in 2D space.
///
// VGC_GEOMETRY_API <- Omitted on purpose.
//                     If needed, manually export individual functions.
class Triangle2x {
public:
    using ScalarType = float;
    static constexpr Int dimension = 2;

    VGC_WARNING_PUSH
    VGC_WARNING_MSVC_DISABLE(26495) // member variable uninitialized
    /// Creates an uninitialized `Triangle2x`.
    ///
    Triangle2x(core::NoInit) noexcept
        : data_{
            Vec2x(core::noInit),
            Vec2x(core::noInit),
            Vec2x(core::noInit)} {
    }
    VGC_WARNING_POP

    /// Creates a `Triangle2x` initialized to [(0, 0), (0, 0), (0, 0)].
    ///
    constexpr Triangle2x() noexcept
        : data_{Vec2x(), Vec2x(), Vec2x()} {
    }


    /// Creates a `Triangle2x` initialized with the given points.
    ///
    constexpr Triangle2x(const Vec2x& a, const Vec2x& b, const Vec2x& c) noexcept
        : data_{a, b, c} {
    }

    /// Accesses the `i`-th point of this `Triangle2x`, where `i` must be either
    /// `0`, `1`, or `2`, corresponsing respectively to `a()`, `b()`, and `c()`.
    ///
    constexpr const Vec2x& operator[](Int i) const {
        return data_[i];
    }

    /// Mutates the `i`-th point of this `Triangle2x`.
    ///
    constexpr Vec2x& operator[](Int i) {
        return data_[i];
    }

    /// Accesses the first point of this `Triangle2x`.
    ///
    constexpr const Vec2x& a() const {
        return data_[0];
    }

    /// Accesses the second point of this `Triangle2x`.
    ///
    constexpr const Vec2x& b() const {
        return data_[1];
    }

    /// Accesses the third point of this `Triangle2x`.
    ///
    constexpr const Vec2x& c() const {
        return data_[2];
    }

    /// Mutates the first point of this `Triangle2x`.
    ///
    constexpr void setA(const Vec2x& a) {
        data_[0] = a;
    }

    /// Mutates the first point of this `Triangle2x`.
    ///
    constexpr void setA(float x, float y) {
        data_[0] = {x, y};
    }

    /// Mutates the second point of this `Triangle2x`.
    ///
    constexpr void setB(const Vec2x& b) {
        data_[1] = b;
    }

    /// Mutates the second point of this `Triangle2x`.
    ///
    constexpr void setB(float x, float y) {
        data_[1] = {x, y};
    }

    /// Mutates the third point of this `Triangle2x`.
    ///
    constexpr void setC(const Vec2x& c) {
        data_[2] = c;
    }

    /// Mutates the third point of this `Triangle2x`.
    ///
    constexpr void setC(float x, float y) {
        data_[2] = {x, y};
    }

    /// Adds in-place the points of `other` to the points of this `Triangle2x`.
    ///
    constexpr Triangle2x& operator+=(const Triangle2x& other) {
        data_[0] += other[0];
        data_[1] += other[1];
        data_[2] += other[2];
        return *this;
    }

    /// Returns the addition of the two triangles `t1` and `t2`.
    ///
    friend constexpr Triangle2x operator+(const Triangle2x& t1, const Triangle2x& t2) {
        return Triangle2x(t1) += t2;
    }

    /// Returns a copy of this `Triangle2x` (unary plus operator).
    ///
    constexpr Triangle2x operator+() const {
        return *this;
    }

    /// Substracts in-place the points of `other` from the points of this `Triangle2x`.
    ///
    constexpr Triangle2x& operator-=(const Triangle2x& other) {
        data_[0] -= other[0];
        data_[1] -= other[1];
        data_[2] -= other[2];
        return *this;
    }

    /// Returns the substraction of `t1` and `t2`.
    ///
    friend constexpr Triangle2x operator-(const Triangle2x& t1, const Triangle2x& t2) {
        return Triangle2x(t1) -= t2;
    }

    /// Returns the opposite of this `Triangle2x` (unary minus operator).
    ///
    constexpr Triangle2x operator-() const {
        return Triangle2x(-data_[0], -data_[1], -data_[2]);
    }

    /// Multiplies in-place all the points in this `Triangle2x` by
    /// the scalar `s`.
    ///
    constexpr Triangle2x& operator*=(float s) {
        data_[0] *= s;
        data_[1] *= s;
        data_[2] *= s;
        return *this;
    }

    /// Returns the multiplication of all the points in this `Triangle2x` by
    /// the scalar `s`.
    ///
    constexpr Triangle2x operator*(float s) const {
        return Triangle2x(*this) *= s;
    }

    /// Returns the multiplication of the scalar `s` with the all the points
    /// in the triangle `t`.
    ///
    friend constexpr Triangle2x operator*(float s, const Triangle2x& t) {
        return t * s;
    }

    /// Divides in-place the points of this `Triangle2x` by the scalar `s`.
    ///
    constexpr Triangle2x& operator/=(float s) {
        data_[0] /= s;
        data_[1] /= s;
        data_[2] /= s;
        return *this;
    }

    /// Returns the division of the points in this `Triangle2x` by the scalar `s`.
    ///
    constexpr Triangle2x operator/(float s) const {
        return Triangle2x(*this) /= s;
    }

    /// Returns whether `t1` and `t2` are equal.
    ///
    friend constexpr bool operator==(const Triangle2x& t1, const Triangle2x& t2) {
        return t1.data_[0] == t2.data_[0]
            && t1.data_[1] == t2.data_[1]
            && t1.data_[2] == t2.data_[2];
    }

    /// Returns whether `t1` and `t2` are different.
    ///
    friend constexpr bool operator!=(const Triangle2x& t1, const Triangle2x& t2) {
        return t1.data_[0] != t2.data_[0]
            || t1.data_[1] != t2.data_[1]
            || t1.data_[2] != t2.data_[2];
    }

private:
    Vec2x data_[3];
};

/// Alias for `vgc::core::Array<vgc::geometry::Triangle2x>`.
///
using Triangle2xArray = core::Array<Triangle2x>;

/// Allows to iterate over a range of `Triangle2x` stored in a memory buffer of
/// floats, where consecutive `Triangle2x` elements are separated by a given stride.
///
/// ```cpp
/// FloatArray buffer = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
/// for (const Triangle2f& v : Triangle2xSpan(buffer.data(), 2, 6)) {
///     std::cout << v << std::end;
/// }
/// // => prints "[(1, 2), (3, 4), (5, 6)][(7, 8), (9, 10), (11, 12)]"
/// ```
///
using Triangle2xSpan = StrideSpan<float, Triangle2x>;

/// Const version of `Triangle2xSpan`.
///
using Triangle2xConstSpan = StrideSpan<const float, const Triangle2x>;

/// Overloads `setZero(T& x)`.
///
/// \sa `vgc::core::zero<T>()`.
///
inline void setZero(Triangle2x& t) {
    t[0] = Vec2x();
    t[1] = Vec2x();
    t[2] = Vec2x();
}

/// Writes the given `Triangle2x` to the output stream.
///
template<typename OStream>
void write(OStream& out, const Triangle2x& t) {
    write(out, '[', t[0], ", ", t[1], ", ", t[2], ']');
}

/// Reads a `Triangle2x` from the input stream, and stores it in the given output
/// parameter `t`. Leading whitespaces are allowed. Raises `ParseError` if the
/// stream does not start with a `Triangle2x`. Raises `RangeError` if one of its
/// coordinates is outside the representable range of a float.
///
template <typename IStream>
void readTo(Triangle2x& t, IStream& in) {
    skipWhitespaceCharacters(in);
    skipExpectedCharacter(in, '[');
    readTo(t[0], in);
    skipWhitespaceCharacters(in);
    skipExpectedCharacter(in, ',');
    readTo(t[1], in);
    skipWhitespaceCharacters(in);
    skipExpectedCharacter(in, ',');
    readTo(t[2], in);
    skipWhitespaceCharacters(in);
    skipExpectedCharacter(in, ']');
}

} // namespace vgc::geometry

// see https://fmt.dev/latest/api.html#formatting-user-defined-types
template <>
struct fmt::formatter<vgc::geometry::Triangle2x> {
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && *it != '}') {
            throw format_error("invalid format");
        }
        return it;
    }
    template <typename FormatContext>
    auto format(const vgc::geometry::Triangle2x& t, FormatContext& ctx) {
        return format_to(ctx.out(),"([{}, {}, {}])", t[0], t[1], t[2]);
    }
};

#endif // VGC_GEOMETRY_TRIANGLE2X_H
