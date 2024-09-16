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

#ifndef VGC_GEOMETRY_TRIANGLE2_H
#define VGC_GEOMETRY_TRIANGLE2_H

#include <vgc/core/array.h>
#include <vgc/geometry/api.h>
#include <vgc/geometry/stride.h>
#include <vgc/geometry/vec2.h>

namespace vgc::geometry {

/// \class vgc::geometry::Triangle2
/// \brief Represents a 2D triangle.
///
/// A `Triangle2` represents a 2D triangle, that is, a triplet of points A, B,
/// and C in 2D space.
///
template<typename T>
class Triangle2 {
public:
    using ScalarType = T;
    static constexpr Int dimension = 2;

    VGC_WARNING_PUSH
    VGC_WARNING_MSVC_DISABLE(26495) // member variable uninitialized
    /// Creates an uninitialized `Triangle2`.
    ///
    Triangle2(core::NoInit) noexcept
        : data_{
            Vec2<T>(core::noInit),
            Vec2<T>(core::noInit),
            Vec2<T>(core::noInit)} {
    }
    VGC_WARNING_POP

    /// Creates a `Triangle2` initialized to [(0, 0), (0, 0), (0, 0)].
    ///
    constexpr Triangle2() noexcept
        : data_{Vec2<T>(), Vec2<T>(), Vec2<T>()} {
    }


    /// Creates a `Triangle2` initialized with the given points.
    ///
    constexpr Triangle2(const Vec2<T>& a, const Vec2<T>& b, const Vec2<T>& c) noexcept
        : data_{a, b, c} {
    }

    /// Accesses the `i`-th point of this triangle, where `i` must be either
    /// `0`, `1`, or `2`, corresponsing respectively to `a()`, `b()`, and `c()`.
    ///
    constexpr const Vec2<T>& operator[](Int i) const {
        return data_[i];
    }

    /// Mutates the `i`-th point of this triangle.
    ///
    constexpr Vec2<T>& operator[](Int i) {
        return data_[i];
    }

    /// Accesses the first point of this triangle.
    ///
    constexpr const Vec2<T>& a() const {
        return data_[0];
    }

    /// Accesses the second point of this triangle.
    ///
    constexpr const Vec2<T>& b() const {
        return data_[1];
    }

    /// Accesses the third point of this triangle.
    ///
    constexpr const Vec2<T>& c() const {
        return data_[2];
    }

    /// Mutates the first point of this triangle.
    ///
    constexpr void setA(const Vec2<T>& a) {
        data_[0] = a;
    }

    /// Mutates the first point of this triangle.
    ///
    constexpr void setA(T x, T y) {
        data_[0] = {x, y};
    }

    /// Mutates the second point of this triangle.
    ///
    constexpr void setB(const Vec2<T>& b) {
        data_[1] = b;
    }

    /// Mutates the second point of this triangle.
    ///
    constexpr void setB(T x, T y) {
        data_[1] = {x, y};
    }

    /// Mutates the third point of this triangle.
    ///
    constexpr void setC(const Vec2<T>& c) {
        data_[2] = c;
    }

    /// Mutates the third point of this triangle.
    ///
    constexpr void setC(T x, T y) {
        data_[2] = {x, y};
    }

    /// Adds in-place the points of the `other` triangle to the points of this
    /// triangle.
    ///
    constexpr Triangle2& operator+=(const Triangle2& other) {
        data_[0] += other[0];
        data_[1] += other[1];
        data_[2] += other[2];
        return *this;
    }

    /// Returns the addition of the two triangles `t1` and `t2`.
    ///
    friend constexpr Triangle2 operator+(const Triangle2& t1, const Triangle2& t2) {
        return Triangle2(t1) += t2;
    }

    /// Returns a copy of this triangle (unary plus operator).
    ///
    constexpr Triangle2 operator+() const {
        return *this;
    }

    /// Substracts in-place the points of the `other` triangle from the points
    /// of this triangle.
    ///
    constexpr Triangle2& operator-=(const Triangle2& other) {
        data_[0] -= other[0];
        data_[1] -= other[1];
        data_[2] -= other[2];
        return *this;
    }

    /// Returns the substraction of two triangles `t1` and `t2`.
    ///
    friend constexpr Triangle2 operator-(const Triangle2& t1, const Triangle2& t2) {
        return Triangle2(t1) -= t2;
    }

    /// Returns the opposite of this triangle (unary minus operator).
    ///
    constexpr Triangle2 operator-() const {
        return Triangle2(-data_[0], -data_[1], -data_[2]);
    }

    /// Multiplies in-place all the points in this triangle by
    /// the scalar `s`.
    ///
    constexpr Triangle2& operator*=(T s) {
        data_[0] *= s;
        data_[1] *= s;
        data_[2] *= s;
        return *this;
    }

    /// Returns the multiplication of all the points in this triangle by
    /// the scalar `s`.
    ///
    constexpr Triangle2 operator*(T s) const {
        return Triangle2(*this) *= s;
    }

    /// Returns the multiplication of the scalar `s` with the all the points
    /// in the triangle `t`.
    ///
    friend constexpr Triangle2 operator*(T s, const Triangle2& t) {
        return t * s;
    }

    /// Divides in-place the points of this triangle by the scalar `s`.
    ///
    constexpr Triangle2& operator/=(T s) {
        data_[0] /= s;
        data_[1] /= s;
        data_[2] /= s;
        return *this;
    }

    /// Returns the division of the points in this triangle by the scalar `s`.
    ///
    constexpr Triangle2 operator/(T s) const {
        return Triangle2(*this) /= s;
    }

    /// Returns whether the triangles `t1` and `t2` are equal.
    ///
    friend constexpr bool operator==(const Triangle2& t1, const Triangle2& t2) {
        return t1.data_[0] == t2.data_[0]
            && t1.data_[1] == t2.data_[1]
            && t1.data_[2] == t2.data_[2];
    }

    /// Returns whether the triangles `t1` and `t2` are different.
    ///
    friend constexpr bool operator!=(const Triangle2& t1, const Triangle2& t2) {
        return t1.data_[0] != t2.data_[0]
            || t1.data_[1] != t2.data_[1]
            || t1.data_[2] != t2.data_[2];
    }

    /// Returns whether the given `point` is inside this triangle (borders
    /// included).
    ///
    bool contains(const Vec2<T>& point) const {

        // The three sides of the triangle
        Vec2<T> v0 = data_[1] - data_[0];
        Vec2<T> v1 = data_[2] - data_[1];
        Vec2<T> v2 = data_[0] - data_[2];

        T det_ = v0.det(v1);
        if (det_ == 0) {
            // Degenerate cases:
            // - All points are equals
            // - Two of the points are equals
            // - The three points are different but aligned

            // Find longest triangle side, store result in (p2, v2, l2)
            T l0 = v0.squaredLength();
            T l1 = v1.squaredLength();
            T l2 = v2.squaredLength();
            Vec2<T> p2 = data_[2];
            if (l0 > l2) {
                p2 = data_[0];
                v2 = v0;
                l2 = l0;
            }
            if (l1 > l2) {
                p2 = data_[1];
                v2 = v1;
                l2 = l1;
            }

            if (l2 == 0) {
                // Case where the triangle is a point.
                return point == p2;
            }
            else {
                // Case where the triangle is a line segment.
                T det2 = v2.det(point - p2);
                T dot2 = v2.dot(point - p2);
                return (det2 == 0 && 0 <= dot2 && dot2 <= l2);
            }
        }
        else {
            // Normal case: the triangle has non-zero area
            T det0 = v0.det(point - data_[0]);
            T det1 = v1.det(point - data_[1]);
            T det2 = v2.det(point - data_[2]);
            bool hasPos = (det0 > 0) || (det1 > 0) || (det2 > 0);
            bool hasNeg = (det0 < 0) || (det1 < 0) || (det2 < 0);
            return !(hasPos && hasNeg);
        }
    }

private:
    Vec2<T> data_[3];
};

/// Alias for `Triangle2<float>`.
///
using Triangle2f = Triangle2<float>;

/// Alias for `Triangle2<double>`.
///
using Triangle2d = Triangle2<double>;

/// Alias for `core::Array<Triangle2<T>>`.
///
template<typename T>
using Triangle2Array = core::Array<Triangle2<T>>;

/// Alias for `core::Array<Triangle2f>`.
///
using Triangle2fArray = core::Array<Triangle2f>;

/// Alias for `core::Array<Triangle2d>`.
///
using Triangle2dArray = core::Array<Triangle2d>;

/// Allows to iterate over a range of `Triangle2` elements stored in a memory
/// buffer of `T` elements, where consecutive `Triangle2` elements are
/// separated by a given stride.
///
/// ```cpp
/// core::DoubleArray buffer = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
/// for (const Triangle2d& v : Triangle2dSpan(buffer.data(), 2, 6)) {
///     std::cout << v << std::end;
/// }
/// // => prints "((1, 2), (3, 4), (5, 6))((7, 8), (9, 10), (11, 12))"
/// ```
///
template<typename T>
using Triangle2Span = StrideSpan<T, Triangle2<T>>;

/// Alias for `Triangle2Span<float>`.
///
using Triangle2fSpan = Triangle2Span<float>;

/// Alias for `Triangle2Span<double>`.
///
using Triangle2dSpan = Triangle2Span<double>;

/// Const version of `Triangle2Span`.
///
template<typename T>
using Triangle2ConstSpan = StrideSpan<const T, const Triangle2<T>>;

/// Alias for `Triangle2ConstSpan<float>`.
///
using Triangle2fConstSpan = Triangle2ConstSpan<float>;

/// Alias for `Triangle2ConstSpan<double>`.
///
using Triangle2dConstSpan = Triangle2ConstSpan<double>;

/// Overloads `setZero(T& x)`.
///
/// \sa `core::zero<T>()`.
///
template<typename T>
inline void setZero(Triangle2<T>& t) {
    t = Triangle2<T>();
}

/// Writes the given `Triangle2` to the output stream.
///
template<typename T, typename OStream>
void write(OStream& out, const Triangle2<T>& t) {
    write(out, '(', t[0], ", ", t[1], ", ", t[2], ')');
}

/// Reads a `Triangle2` from the input stream, and stores it in the given output
/// parameter `t`. Leading whitespaces are allowed. Raises `ParseError` if the
/// stream does not start with a `Triangle2`. Raises `RangeError` if one of its
/// coordinates is outside the representable range of a `T`.
///
template<typename T, typename IStream>
void readTo(Triangle2<T>& t, IStream& in) {
    skipWhitespacesAndExpectedCharacter(in, '(');
    readTo(t[0], in);
    skipWhitespacesAndExpectedCharacter(in, ',');
    readTo(t[1], in);
    skipWhitespacesAndExpectedCharacter(in, ',');
    readTo(t[2], in);
    skipWhitespacesAndExpectedCharacter(in, ')');
}

} // namespace vgc::geometry

template<typename T>
struct fmt::formatter<vgc::geometry::Triangle2<T>> {
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && *it != '}') {
            throw format_error("invalid format");
        }
        return it;
    }
    template<typename FormatContext>
    auto format(const vgc::geometry::Triangle2<T>& t, FormatContext& ctx) {
        return format_to(ctx.out(), "({}, {}, {})", t[0], t[1], t[2]);
    }
};

#endif // VGC_GEOMETRY_TRIANGLE2_H
