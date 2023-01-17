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

// This file was automatically generated, please do not edit directly.
// Instead, edit tools/triangle2x.h then run tools/generate.py.

// clang-format off

#ifndef VGC_GEOMETRY_TRIANGLE2D_H
#define VGC_GEOMETRY_TRIANGLE2D_H

#include <cmath>

#include <vgc/core/array.h>
#include <vgc/geometry/api.h>
#include <vgc/geometry/stride.h>
#include <vgc/geometry/vec2d.h>

namespace vgc::geometry {

/// \class vgc::geometry::Triangle2d
/// \brief 2D triangle using double-precision floating points.
///
/// A Triangle2d represents a triangle, that is, a triplet of points A, B, and
/// C in 2D space.
///
// VGC_GEOMETRY_API <- Omitted on purpose.
//                     If needed, manually export individual functions.
class Triangle2d {
public:
    using ScalarType = double;
    static constexpr Int dimension = 2;

    VGC_WARNING_PUSH
    VGC_WARNING_MSVC_DISABLE(26495) // member variable uninitialized
    /// Creates an uninitialized `Triangle2d`.
    ///
    Triangle2d(core::NoInit) noexcept
        : data_{
            Vec2d(core::noInit),
            Vec2d(core::noInit),
            Vec2d(core::noInit)} {
    }
    VGC_WARNING_POP

    /// Creates a `Triangle2d` initialized to [(0, 0), (0, 0), (0, 0)].
    ///
    constexpr Triangle2d() noexcept
        : data_{Vec2d(), Vec2d(), Vec2d()} {
    }


    /// Creates a `Triangle2d` initialized with the given points.
    ///
    constexpr Triangle2d(const Vec2d& a, const Vec2d& b, const Vec2d& c) noexcept
        : data_{a, b, c} {
    }

    /// Accesses the `i`-th point of this `Triangle2d`, where `i` must be either
    /// `0`, `1`, or `2`, corresponsing respectively to `a()`, `b()`, and `c()`.
    ///
    constexpr const Vec2d& operator[](Int i) const {
        return data_[i];
    }

    /// Mutates the `i`-th point of this `Triangle2d`.
    ///
    constexpr Vec2d& operator[](Int i) {
        return data_[i];
    }

    /// Accesses the first point of this `Triangle2d`.
    ///
    constexpr const Vec2d& a() const {
        return data_[0];
    }

    /// Accesses the second point of this `Triangle2d`.
    ///
    constexpr const Vec2d& b() const {
        return data_[1];
    }

    /// Accesses the third point of this `Triangle2d`.
    ///
    constexpr const Vec2d& c() const {
        return data_[2];
    }

    /// Mutates the first point of this `Triangle2d`.
    ///
    constexpr void setA(const Vec2d& a) {
        data_[0] = a;
    }

    /// Mutates the first point of this `Triangle2d`.
    ///
    constexpr void setA(double x, double y) {
        data_[0] = {x, y};
    }

    /// Mutates the second point of this `Triangle2d`.
    ///
    constexpr void setB(const Vec2d& b) {
        data_[1] = b;
    }

    /// Mutates the second point of this `Triangle2d`.
    ///
    constexpr void setB(double x, double y) {
        data_[1] = {x, y};
    }

    /// Mutates the third point of this `Triangle2d`.
    ///
    constexpr void setC(const Vec2d& c) {
        data_[2] = c;
    }

    /// Mutates the third point of this `Triangle2d`.
    ///
    constexpr void setC(double x, double y) {
        data_[2] = {x, y};
    }

    /// Adds in-place the points of `other` to the points of this `Triangle2d`.
    ///
    constexpr Triangle2d& operator+=(const Triangle2d& other) {
        data_[0] += other[0];
        data_[1] += other[1];
        data_[2] += other[2];
        return *this;
    }

    /// Returns the addition of the two triangles `t1` and `t2`.
    ///
    friend constexpr Triangle2d operator+(const Triangle2d& t1, const Triangle2d& t2) {
        return Triangle2d(t1) += t2;
    }

    /// Returns a copy of this `Triangle2d` (unary plus operator).
    ///
    constexpr Triangle2d operator+() const {
        return *this;
    }

    /// Substracts in-place the points of `other` from the points of this `Triangle2d`.
    ///
    constexpr Triangle2d& operator-=(const Triangle2d& other) {
        data_[0] -= other[0];
        data_[1] -= other[1];
        data_[2] -= other[2];
        return *this;
    }

    /// Returns the substraction of `t1` and `t2`.
    ///
    friend constexpr Triangle2d operator-(const Triangle2d& t1, const Triangle2d& t2) {
        return Triangle2d(t1) -= t2;
    }

    /// Returns the opposite of this `Triangle2d` (unary minus operator).
    ///
    constexpr Triangle2d operator-() const {
        return Triangle2d(-data_[0], -data_[1], -data_[2]);
    }

    /// Multiplies in-place all the points in this `Triangle2d` by
    /// the scalar `s`.
    ///
    constexpr Triangle2d& operator*=(double s) {
        data_[0] *= s;
        data_[1] *= s;
        data_[2] *= s;
        return *this;
    }

    /// Returns the multiplication of all the points in this `Triangle2d` by
    /// the scalar `s`.
    ///
    constexpr Triangle2d operator*(double s) const {
        return Triangle2d(*this) *= s;
    }

    /// Returns the multiplication of the scalar `s` with the all the points
    /// in the triangle `t`.
    ///
    friend constexpr Triangle2d operator*(double s, const Triangle2d& t) {
        return t * s;
    }

    /// Divides in-place the points of this `Triangle2d` by the scalar `s`.
    ///
    constexpr Triangle2d& operator/=(double s) {
        data_[0] /= s;
        data_[1] /= s;
        data_[2] /= s;
        return *this;
    }

    /// Returns the division of the points in this `Triangle2d` by the scalar `s`.
    ///
    constexpr Triangle2d operator/(double s) const {
        return Triangle2d(*this) /= s;
    }

    /// Returns whether `t1` and `t2` are equal.
    ///
    friend constexpr bool operator==(const Triangle2d& t1, const Triangle2d& t2) {
        return t1.data_[0] == t2.data_[0]
            && t1.data_[1] == t2.data_[1]
            && t1.data_[2] == t2.data_[2];
    }

    /// Returns whether `t1` and `t2` are different.
    ///
    friend constexpr bool operator!=(const Triangle2d& t1, const Triangle2d& t2) {
        return t1.data_[0] != t2.data_[0]
            || t1.data_[1] != t2.data_[1]
            || t1.data_[2] != t2.data_[2];
    }

private:
    Vec2d data_[3];
};

/// Alias for `vgc::core::Array<vgc::geometry::Triangle2d>`.
///
using Triangle2dArray = core::Array<Triangle2d>;

/// Allows to iterate over a range of `Triangle2d` stored in a memory buffer of
/// doubles, where consecutive `Triangle2d` elements are separated by a given stride.
///
/// ```cpp
/// FloatArray buffer = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
/// for (const Triangle2& v : Triangle2dSpan(buffer.data(), 2, 6)) {
///     std::cout << v << std::end;
/// }
/// // => prints "[(1, 2), (3, 4), (5, 6)][(7, 8), (9, 10), (11, 12)]"
/// ```
///
using Triangle2dSpan = StrideSpan<double, Triangle2d>;

/// Const version of `Triangle2dSpan`.
///
using Triangle2dConstSpan = StrideSpan<const double, const Triangle2d>;

/// Overloads `setZero(T& x)`.
///
/// \sa `vgc::core::zero<T>()`.
///
inline void setZero(Triangle2d& t) {
    t[0] = Vec2d();
    t[1] = Vec2d();
    t[2] = Vec2d();
}

/// Writes the given `Triangle2d` to the output stream.
///
template<typename OStream>
void write(OStream& out, const Triangle2d& t) {
    write(out, '[', t[0], ", ", t[1], ", ", t[2], ']');
}

/// Reads a `Triangle2d` from the input stream, and stores it in the given output
/// parameter `t`. Leading whitespaces are allowed. Raises `ParseError` if the
/// stream does not start with a `Triangle2d`. Raises `RangeError` if one of its
/// coordinates is outside the representable range of a double.
///
template <typename IStream>
void readTo(Triangle2d& t, IStream& in) {
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
struct fmt::formatter<vgc::geometry::Triangle2d> {
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && *it != '}') {
            throw format_error("invalid format");
        }
        return it;
    }
    template <typename FormatContext>
    auto format(const vgc::geometry::Triangle2d& t, FormatContext& ctx) {
        return format_to(ctx.out(),"([{}, {}, {}])", t[0], t[1], t[2]);
    }
};

#endif // VGC_GEOMETRY_TRIANGLE2D_H
