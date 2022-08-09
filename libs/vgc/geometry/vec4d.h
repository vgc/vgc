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
// Instead, edit tools/vec4x.h then run tools/generate.py.

#ifndef VGC_GEOMETRY_VEC4D_H
#define VGC_GEOMETRY_VEC4D_H

#include <cmath>

#include <vgc/core/array.h>
#include <vgc/geometry/api.h>
#include <vgc/geometry/stride.h>
#include <vgc/geometry/vec.h>

namespace vgc::geometry {

/// \class vgc::geometry::Vec4d
/// \brief 2D vector using double-precision floating points.
///
/// A Vec4d typically represents either a 4D point (= position), a 4D vector (=
/// difference of positions), a 3D vector in homogenous coordinates, or any
/// cases where you need to store 4 contiguous doubles.
///
/// The memory size of a `Vec4d` is exactly `4 * sizeof(double)`. This will never
/// change in any future version, as this allows to conveniently use this class
/// for data transfer to the GPU (via OpenGL, Metal, etc.).
///
// VGC_GEOMETRY_API <- Omitted on purpose.
//                     If needed, manually export individual functions.
class Vec4d
{
public:
    using ScalarType = double;
    static constexpr Int dimension = 4;

    /// Creates an uninitialized `Vec4d`.
    ///
    Vec4d(core::NoInit) {}

    /// Creates a `Vec4d` initialized to (0, 0, 0, 0).
    ///
    constexpr Vec4d() : data_{0, 0, 0, 0} {}


    /// Creates a `Vec4d` initialized with the given `x`, `y`, `z`, `w` coordinates.
    ///
    constexpr Vec4d(double x, double y, double z, double w) : data_{x, y, z, w} {}

    /// Creates a `Vec4d` from another `Vec<2, T>` object by performing a
    /// `static_cast` on each of its coordinates.
    ///
    /// ```cpp
    /// vgc::geometrt::Vec4d vd(1, 2);
    /// vgc::geometrt::Vec4 vf(vd); // cast from double to double
    /// ```
    ///
    template<typename TVec4, VGC_REQUIRES(
                 isVec<TVec4> &&
                 TVec4::dimension == 4 &&
                 !std::is_same_v<TVec4, Vec4d>)>
    explicit constexpr Vec4d(const TVec4& other)
        : data_{static_cast<double>(other[0]),
                static_cast<double>(other[1]),
                static_cast<double>(other[2]),
                static_cast<double>(other[3])} {}

    /// Returns a pointer to the underlying array of components.
    ///
    const double* data() const { return data_; }

    /// Returns a pointer to the underlying array of components.
    ///
    double* data() { return data_; }

    /// Accesses the `i`-th coordinate of this `Vec4d`.
    ///
    constexpr const double& operator[](Int i) const { return data_[i]; }

    /// Mutates the `i`-th coordinate of this `Vec4d`.
    ///
    constexpr double& operator[](Int i) { return data_[i]; }

    /// Accesses the first coordinate of this `Vec4d`.
    ///
    constexpr double x() const { return data_[0]; }

    /// Accesses the second coordinate of this `Vec4d`.
    ///
    constexpr double y() const { return data_[1]; }

    /// Accesses the third coordinate of this `Vec4d`.
    ///
    constexpr double z() const { return data_[2]; }

    /// Accesses the fourth coordinate of this `Vec4d`.
    ///
    constexpr double w() const { return data_[3]; }

    /// Mutates the first coordinate of this `Vec4d`.
    ///
    constexpr void setX(double x) { data_[0] = x; }

    /// Mutates the second coordinate of this `Vec4d`.
    ///
    constexpr void setY(double y) { data_[1] = y; }

    /// Mutates the third coordinate of this `Vec4d`.
    ///
    constexpr void setZ(double z) { data_[2] = z; }

    /// Mutates the fourth coordinate of this `Vec4d`.
    ///
    constexpr void setW(double w) { data_[3] = w; }

    /// Adds in-place `other` to this `Vec4d`.
    ///
    constexpr Vec4d& operator+=(const Vec4d& other) {
        data_[0] += other[0];
        data_[1] += other[1];
        data_[2] += other[2];
        data_[3] += other[3];
        return *this;
    }

    /// Returns the addition of the two vectors `v1` and `v2`.
    ///
    friend constexpr Vec4d operator+(const Vec4d& v1, const Vec4d& v2) {
        return Vec4d(v1) += v2;
    }

    /// Returns a copy of this `Vec4d` (unary plus operator).
    ///
    constexpr Vec4d operator+() const {
        return *this;
    }

    /// Substracts in-place `other` from this `Vec4d`.
    ///
    constexpr Vec4d& operator-=(const Vec4d& other) {
        data_[0] -= other[0];
        data_[1] -= other[1];
        data_[2] -= other[2];
        data_[3] -= other[3];
        return *this;
    }

    /// Returns the substraction of `v1` and `v2`.
    ///
    friend constexpr Vec4d operator-(const Vec4d& v1, const Vec4d& v2) {
        return Vec4d(v1) -= v2;
    }

    /// Returns the opposite of this `Vec4d` (unary minus operator).
    ///
    constexpr Vec4d operator-() const {
        return Vec4d(-data_[0], -data_[1], -data_[2], -data_[3]);
    }

    /// Multiplies in-place this `Vec4d` by the scalar `s`.
    ///
    constexpr Vec4d& operator*=(double s) {
        data_[0] *= s;
        data_[1] *= s;
        data_[2] *= s;
        data_[3] *= s;
        return *this;
    }

    /// Returns the multiplication of this `Vec4d` by the scalar `s`.
    ///
    constexpr Vec4d operator*(double s) const {
        return Vec4d(*this) *= s;
    }

    /// Returns the multiplication of the scalar `s` with the vector `v`.
    ///
    friend constexpr Vec4d operator*(double s, const Vec4d& v) {
        return v * s;
    }

    /// Divides in-place this `Vec4d` by the scalar `s`.
    ///
    constexpr Vec4d& operator/=(double s) {
        data_[0] /= s;
        data_[1] /= s;
        data_[2] /= s;
        data_[3] /= s;
        return *this;
    }

    /// Returns the division of this `Vec4d` by the scalar `s`.
    ///
    constexpr Vec4d operator/(double s) const {
        return Vec4d(*this) /= s;
    }

    /// Returns whether `v1` and `v2` are equal.
    ///
    friend constexpr bool operator==(const Vec4d& v1, const Vec4d& v2) {
        return v1.data_[0] == v2.data_[0] &&
               v1.data_[1] == v2.data_[1] &&
               v1.data_[2] == v2.data_[2] &&
               v1.data_[3] == v2.data_[3];
    }

    /// Returns whether `v1` and `v2` are different.
    ///
    friend constexpr bool operator!=(const Vec4d& v1, const Vec4d& v2) {
        return v1.data_[0] != v2.data_[0] ||
               v1.data_[1] != v2.data_[1] ||
               v1.data_[2] != v2.data_[2] ||
               v1.data_[3] != v2.data_[3];
    }

    /// Compares `v1` and `v2` using the lexicographic
    /// order.
    ///
    friend constexpr bool operator<(const Vec4d& v1, const Vec4d& v2) {
        return (v1.data_[0] < v2.data_[0]) || (
                !(v2.data_[0] < v1.data_[0]) && (
                   (v1.data_[1] < v2.data_[1]) || (
                    !(v2.data_[1] < v1.data_[1]) && (
                       (v1.data_[2] < v2.data_[2]) || (
                        !(v2.data_[2] < v1.data_[2]) && (
                            v1.data_[3] < v2.data_[3]
                         )
                       )
                     )
                   )
                 )
               );
    }

    /// Compares `v1` and `v2` using the lexicographic
    /// order.
    ///
    friend constexpr bool operator<=(const Vec4d& v1, const Vec4d& v2) {
        return !(v2 < v1);
    }

    /// Compares the `v1` and `v2` using the lexicographic
    /// order.
    ///
    friend constexpr bool operator>(const Vec4d& v1, const Vec4d& v2) {
        return v2 < v1;
    }

    /// Compares `v1` and `v2` using the lexicographic
    /// order.
    ///
    friend constexpr bool operator>=(const Vec4d& v1, const Vec4d& v2) {
        return !(v1 < v2);
    }

    /// Returns the Euclidean length of this `Vec4d`.
    ///
    double length() const {
        return std::sqrt(squaredLength());
    }

    /// Returns the square of the Euclidean length of this `Vec4d`.
    ///
    /// This function is faster than `length()`, therefore it is a good idea to
    /// use it whenever you don't need the actual length. For example, if you
    /// need to know which vector has greater length, you can use
    /// `v1.squaredLength() < v2.squaredLength()`.
    ///
    constexpr double squaredLength() const {
        return data_[0] * data_[0] +
               data_[1] * data_[1] +
               data_[2] * data_[2] +
               data_[3] * data_[3];
    }

    /// Makes this `Vec4d` a unit vector by dividing it by its length.
    ///
    /// If provided, `isNormalizable` is set to either `true` or `false`
    /// depending on whether the vector was considered normalizable.
    ///
    /// The vector is considered non-normalizable whenever its length is less
    /// or equal than the given `epsilon`. An appropriate epsilon is
    /// context-dependent, and therefore zero is used as default, which means
    /// that the vector is considered non-normalizable if and only if it is
    /// exactly equal to the null vector `Vec4d()`.
    ///
    /// If the vector is considered non-normalizable, then it is set to
    /// `(1.0, 0.0, 0.0, 0.0)`.
    ///
    /// \sa `length()`.
    ///
    Vec4d& normalize(bool* isNormalizable = nullptr, double epsilon = 0.0);

    /// Returns a normalized copy of this Vec4d.
    ///
    Vec4d normalized(bool* isNormalizable = nullptr, double epsilon = 0.0) const;

    /// Returns the dot product between this Vec4d `a` and the given Vec4d `b`.
    ///
    /// ```cpp
    /// double d = a.dot(b);
    /// ```
    ///
    constexpr double dot(const Vec4d& b) const {
        const Vec4d& a = *this;
        return a[0]*b[0] + a[1]*b[1] + a[2]*b[2] + a[3]*b[3];
    }

    /// Returns the angle, in radians and in the interval [0, π], between this
    /// Vec4d `a` and the given Vec4d `b`.
    ///
    /// This value is computed using the following formula:
    ///
    /// ```cpp
    /// double angle = acos(a.dot(b) / (a.length() * b.length()));
    /// ```
    ///
    /// It returns an undefined value if either `a` or `b` is zero-length.
    ///
    double angle(const Vec4d& b) const {
        const Vec4d& a = *this;
        return std::acos(a.dot(b) / (a.length() * b.length()));
    }

    /// Returns whether this Vec4d `a` and the given Vec4d `b` are almost equal
    /// within some relative tolerance. See `Vec2d::isClose()` for details.
    ///
    bool isClose(const Vec4d& b, double relTol = 1e-9, double absTol = 0.0) const {
        const Vec4d& a = *this;
        double diff2 = a.infdiff_(b).squaredLength();
        if (diff2 == std::numeric_limits<double>::infinity()) {
            return false; // opposite infinities or finite/infinite mismatch
        }
        else {
            double relTol2 = relTol * relTol;
            double absTol2 = absTol * absTol;
            return diff2 <= relTol2 * b.squaredLength() ||
                   diff2 <= relTol2 * a.squaredLength() ||
                   diff2 <= absTol2;
        }
    }

    /// Returns whether all coordinates in this Vec4d `a` are almost equal to
    /// their corresponding coordinate in the given Vec4d `b`, within some
    /// relative tolerance. See `Vec2d::allClose()` for details.
    ///
    bool allClose(const Vec4d& b, double relTol = 1e-9, double absTol = 0.0) const {
        const Vec4d& a = *this;
        return core::isClose(a[0], b[0], relTol, absTol) &&
               core::isClose(a[1], b[1], relTol, absTol) &&
               core::isClose(a[2], b[2], relTol, absTol) &&
               core::isClose(a[3], b[3], relTol, absTol);
    }

    /// Returns whether the euclidean distance between this Vec4d `a` and the
    /// given Vec4d `b` is smaller or equal than the given absolute tolerance.
    /// See `Vec2d::isNear()` for details.
    ///
    bool isNear(const Vec4d& b, double absTol) const {
        const Vec4d& a = *this;
        double diff2 = a.infdiff_(b).squaredLength();
        if (diff2 == std::numeric_limits<double>::infinity()) {
            return false; // opposite infinities or finite/infinite mismatch
        }
        else {
            double absTol2 = absTol * absTol;
            return diff2 <= absTol2;
        }
    }

    /// Returns whether all coordinates in this Vec4d `a` are within some
    /// absolute tolerance of their corresponding coordinate in the given Vec4d
    /// `b`. See `Vec2d::allNear()` for details.
    ///
    bool allNear(const Vec4d& b, double absTol) const {
        const Vec4d& a = *this;
        return core::isNear(a[0], b[0], absTol) &&
               core::isNear(a[1], b[1], absTol) &&
               core::isNear(a[2], b[2], absTol) &&
               core::isNear(a[3], b[3], absTol);
    }

private:
    double data_[4];

    Vec4d infdiff_(const Vec4d& b) const
    {
        const Vec4d& a = *this;
        return Vec4d(core::internal::infdiff(a[0], b[0]),
                     core::internal::infdiff(a[1], b[1]),
                     core::internal::infdiff(a[2], b[2]),
                     core::internal::infdiff(a[3], b[3]));
    }
};

// See Vec2d for detailed comments
inline Vec4d& Vec4d::normalize(bool* isNormalizable, double epsilon_) {
    double l2 = squaredLength();
    if (l2 <= epsilon_*epsilon_) {
        if (isNormalizable) {
            *isNormalizable = false;
        }
        *this = Vec4d(1.0, 0.0, 0.0, 0.0);
    }
    else {
        if (isNormalizable) {
            *isNormalizable = true;
        }
        double l = std::sqrt(l2);
        *this /= l;
    }
    return *this;
}

inline Vec4d Vec4d::normalized(bool* isNormalizable, double epsilon_) const {
    return Vec4d(*this).normalize(isNormalizable, epsilon_);
}

/// Alias for `vgc::core::Array<vgc::geometry::Vec4d>`.
///
using Vec4dArray = core::Array<Vec4d>;

/// Allows to iterate over a range of `Vec4d` stored in a memory buffer of
/// doubles, where consecutive `Vec4d` elements are separated by a given stride.
///
using Vec4dSpan = StrideSpan<double, Vec4d>;

/// Const version of `Vec4dSpan`.
///
using Vec4dConstSpan = StrideSpan<const double, const Vec4d>;

/// Overloads `setZero(T& x)`.
///
/// \sa `vgc::core::zero<T>()`.
///
inline void setZero(Vec4d& v) {
    v[0] = 0.0;
    v[1] = 0.0;
    v[2] = 0.0;
    v[3] = 0.0;
}

/// Writes the given `Vec4d` to the output stream.
///
template<typename OStream>
void write(OStream& out, const Vec4d& v) {
    write(out, '(', v[0], ", ", v[1], ", ", v[2], ", ", v[3], ')');
}

/// Reads a `Vec4d` from the input stream, and stores it in the given output
/// parameter `v`. Leading whitespaces are allowed. Raises `ParseError` if the
/// stream does not start with a `Vec4d`. Raises `RangeError` if one of its
/// coordinates is outside the representable range of a double.
///
template <typename IStream>
void readTo(Vec4d& v, IStream& in) {
    skipWhitespaceCharacters(in);
    skipExpectedCharacter(in, '(');
    readTo(v[0], in);
    skipWhitespaceCharacters(in);
    skipExpectedCharacter(in, ',');
    readTo(v[1], in);
    skipWhitespaceCharacters(in);
    skipExpectedCharacter(in, ',');
    readTo(v[2], in);
    skipWhitespaceCharacters(in);
    skipExpectedCharacter(in, ',');
    readTo(v[3], in);
    skipWhitespaceCharacters(in);
    skipExpectedCharacter(in, ')');
}

} // namespace vgc::geometry

// see https://fmt.dev/latest/api.html#formatting-user-defined-types
template <>
struct fmt::formatter<vgc::geometry::Vec4d> {
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && *it != '}')
            throw format_error("invalid format");
        return it;
    }
    template <typename FormatContext>
    auto format(const vgc::geometry::Vec4d v, FormatContext& ctx) {
        return format_to(ctx.out(),"({}, {}, {}, {})", v[0], v[1], v[2], v[3]);
    }
};

#endif // VGC_GEOMETRY_VEC4D_H
