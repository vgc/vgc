// Copyright 2021 The VGC Developers
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

#ifndef VGC_GEOMETRY_VEC3_H
#define VGC_GEOMETRY_VEC3_H

#include <cmath>

#include <vgc/core/array.h>
#include <vgc/geometry/api.h>
#include <vgc/geometry/stride.h>
#include <vgc/geometry/vec.h>

namespace vgc::geometry {

/// \class vgc::geometry::Vec3
/// \brief Represents a 3D vector.
///
/// A Vec3 represents either a 3D point (= position), a 3D vector (= difference
/// of positions), a 3D size (= positive position), or a 3D normal (= unit
/// vector). Unlike some libraries, we do not provide different types for these
/// different use cases.
///
/// The memory layout of a `Vec3<T>` is exactly the same as:
///
/// ```cpp
/// struct {
///     T x;
///     T y;
///     T z;
/// };
/// ```
///
/// This will never change in any future version, as this allows to
/// conveniently use this class for data transfer to the GPU (via OpenGL,
/// Metal, etc.).
///
template<typename T>
class Vec3 {
public:
    using ScalarType = T;
    static constexpr Int dimension = 3;

    VGC_WARNING_PUSH
    VGC_WARNING_MSVC_DISABLE(26495) // member variable uninitialized
    /// Creates an uninitialized `Vec3`.
    ///
    Vec3(core::NoInit) noexcept {
    }
    VGC_WARNING_POP

    /// Creates a `Vec3` initialized to (0, 0, 0).
    ///
    constexpr Vec3() noexcept
        : data_{0, 0, 0} {
    }

    /// Creates a `Vec3` initialized with the given `x`, `y`, and `z`
    /// coordinates.
    ///
    constexpr Vec3(T x, T y, T z) noexcept
        : data_{x, y, z} {
    }

    /// Creates a `Vec3<T>` from another `Vec3<U>` object by performing a
    /// `static_cast` on each of its coordinates.
    ///
    /// ```cpp
    /// Vec3d vd(1, 2, 3);
    /// Vec3f vf(vd); // cast from double to float
    /// ```
    ///
    template<typename U, VGC_REQUIRES(!std::is_same_v<U, T>)>
    constexpr explicit Vec3(const Vec3<U>& other) noexcept
        : data_{
            static_cast<T>(other[0]),
            static_cast<T>(other[1]),
            static_cast<T>(other[2])} {
    }

    /// Returns a pointer to the underlying array of components.
    ///
    const T* data() const {
        return data_;
    }

    /// Returns a pointer to the underlying array of components.
    ///
    T* data() {
        return data_;
    }

    /// Accesses the `i`-th coordinate of this vector.
    ///
    constexpr const T& operator[](Int i) const {
        return data_[i];
    }

    /// Mutates the `i`-th coordinate of this vector.
    ///
    constexpr T& operator[](Int i) {
        return data_[i];
    }

    /// Accesses the first coordinate of this vector.
    ///
    constexpr T x() const {
        return data_[0];
    }

    /// Accesses the second coordinate of this vector.
    ///
    constexpr T y() const {
        return data_[1];
    }

    /// Accesses the third coordinate of this vector.
    ///
    constexpr T z() const {
        return data_[2];
    }

    /// Mutates the first coordinate of this vector.
    ///
    constexpr void setX(T x) {
        data_[0] = x;
    }

    /// Mutates the second coordinate of this vector.
    ///
    constexpr void setY(T y) {
        data_[1] = y;
    }

    /// Mutates the third coordinate of this vector.
    ///
    constexpr void setZ(T z) {
        data_[2] = z;
    }

    /// Adds in-place `other` to this vector.
    ///
    constexpr Vec3& operator+=(const Vec3& other) {
        data_[0] += other[0];
        data_[1] += other[1];
        data_[2] += other[2];
        return *this;
    }

    /// Returns the addition of the two vectors `v1` and `v2`.
    ///
    friend constexpr Vec3 operator+(const Vec3& v1, const Vec3& v2) {
        return Vec3(v1) += v2;
    }

    /// Returns a copy of this vector (unary plus operator).
    ///
    constexpr Vec3 operator+() const {
        return *this;
    }

    /// Substracts in-place `other` from this vector.
    ///
    constexpr Vec3& operator-=(const Vec3& other) {
        data_[0] -= other[0];
        data_[1] -= other[1];
        data_[2] -= other[2];
        return *this;
    }

    /// Returns the substraction of `v1` and `v2`.
    ///
    friend constexpr Vec3 operator-(const Vec3& v1, const Vec3& v2) {
        return Vec3(v1) -= v2;
    }

    /// Returns the opposite of this vector (unary minus operator).
    ///
    constexpr Vec3 operator-() const {
        return Vec3(-data_[0], -data_[1], -data_[2]);
    }

    /// Multiplies in-place this vector by the scalar `s`.
    ///
    constexpr Vec3& operator*=(T s) {
        data_[0] *= s;
        data_[1] *= s;
        data_[2] *= s;
        return *this;
    }

    /// Returns the multiplication of this vector by the scalar `s`.
    ///
    constexpr Vec3 operator*(T s) const {
        return Vec3(*this) *= s;
    }

    /// Returns the multiplication of the scalar `s` with the vector `v`.
    ///
    friend constexpr Vec3 operator*(T s, const Vec3& v) {
        return v * s;
    }

    /// Divides in-place this vector by the scalar `s`.
    ///
    constexpr Vec3& operator/=(T s) {
        data_[0] /= s;
        data_[1] /= s;
        data_[2] /= s;
        return *this;
    }

    /// Returns the division of this vector by the scalar `s`.
    ///
    constexpr Vec3 operator/(T s) const {
        return Vec3(*this) /= s;
    }

    /// Returns whether `v1` and `v2` are equal.
    ///
    friend constexpr bool operator==(const Vec3& v1, const Vec3& v2) {
        return v1.data_[0] == v2.data_[0]    //
               && v1.data_[1] == v2.data_[1] //
               && v1.data_[2] == v2.data_[2];
    }

    /// Returns whether `v1` and `v2` are different.
    ///
    friend constexpr bool operator!=(const Vec3& v1, const Vec3& v2) {
        return v1.data_[0] != v2.data_[0]    //
               || v1.data_[1] != v2.data_[1] //
               || v1.data_[2] != v2.data_[2];
    }

    /// Compares `v1` and `v2` using lexicographic order on (x, y, z).
    ///
    friend constexpr bool operator<(const Vec3& v1, const Vec3& v2) {
        // clang-format off
        return ( (v1.data_[0] < v2.data_[0]) ||
               (!(v2.data_[0] < v1.data_[0]) &&
               ( (v1.data_[1] < v2.data_[1]) ||
               (!(v2.data_[1] < v1.data_[1]) &&
               ( (v1.data_[2] < v2.data_[2]))))));
        // clang-format on
    }

    /// Compares `v1` and `v2` using lexicographic order on (x, y, z).
    ///
    friend constexpr bool operator<=(const Vec3& v1, const Vec3& v2) {
        return !(v2 < v1);
    }

    /// Compares `v1` and `v2` using lexicographic order on (x, y, z).
    ///
    friend constexpr bool operator>(const Vec3& v1, const Vec3& v2) {
        return v2 < v1;
    }

    /// Compares `v1` and `v2` using lexicographic order on (x, y, z).
    ///
    friend constexpr bool operator>=(const Vec3& v1, const Vec3& v2) {
        return !(v1 < v2);
    }

    /// Returns the Euclidean length of this vector, that is,
    /// `sqrt(x*x + y*y + z*z)`.
    ///
    /// \sa `squaredLength()`.
    ///
    T length() const {
        return std::sqrt(squaredLength());
    }

    /// Returns the square of the Euclidean length of this vector, that is,
    /// `x*x + y*y + z*z`.
    ///
    /// This function is faster than `length()`, therefore it is a good idea to
    /// use it whenever you don't need the actual length. For example, if you
    /// need to know which vector has greater length, you can use
    /// `v1.squaredLength() < v2.squaredLength()`.
    ///
    /// \sa `length()`.
    ///
    constexpr T squaredLength() const {
        return this->dot(*this);
    }

    /// Makes this vector a unit vector by dividing it by its length.
    ///
    /// If provided, `isNormalizable` is set to either `true` or `false`
    /// depending on whether the vector was considered normalizable.
    ///
    /// The vector is considered non-normalizable whenever its length is less
    /// or equal than the given `epsilon`. An appropriate epsilon is
    /// context-dependent, and therefore zero is used as default, which means
    /// that the vector is considered non-normalizable if and only if it is
    /// exactly equal to the null vector `Vec3()`.
    ///
    /// If the vector is considered non-normalizable, then it is set to
    /// `(1, 0, 0)`.
    ///
    /// \sa `length()`, `normalized()`.
    ///
    Vec3& normalize(bool* isNormalizable = nullptr, T epsilon = 0);

    /// Returns a normalized copy of this vector.
    ///
    /// \sa `length()`, `normalize()`.
    ///
    Vec3 normalized(bool* isNormalizable = nullptr, T epsilon = 0) const;

    /// Returns the dot product between this vector `a` and the other vector
    /// `b`.
    ///
    /// ```cpp
    /// Vec3d a, b;
    /// double d = a.dot(b);
    /// ```
    ///
    /// This is equivalent to:
    ///
    /// ```cpp
    /// a[0]*b[0] + a[1]*b[1] + a[2]*b[2]
    /// ```
    ///
    /// Note that, except for numerical errors, this is also equal to:
    ///
    /// ```cpp
    /// a.length() * b.length() * cos(a.angle(b))
    /// ```
    ///
    /// \sa `angle()`, `cross()`.
    ///
    constexpr T dot(const Vec3& b) const {
        const Vec3& a = *this;
        return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    }

    /// Returns the cross product between this vector `a` and the other vector
    /// `b`.
    ///
    /// ```cpp
    /// Vec3d a, b;
    /// double d = a.cross(b);
    /// ```
    ///
    /// \sa `angle()`, `dot()`.
    ///
    constexpr Vec3 cross(const Vec3& b) const {
        const Vec3& a = *this;
        return Vec3(
            a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0]);
    }

    /// Returns the angle, in radians and in the interval [0, π], between this
    /// vector `a` and the other vector `b`.
    ///
    /// ```cpp
    /// Vec3d a, b;
    /// double angle = a.angle(b);
    /// ```
    ///
    /// This value is computed using the following formula:
    ///
    /// ```cpp
    /// angle = atan2(a.cross(b).length(), a.dot(b));
    /// ```
    ///
    /// It returns an undefined value if either `a` or `b` is zero-length.
    ///
    /// \sa `cross()`, `dot()`, `length()`.
    ///
    T angle(const Vec3& b) const {
        const Vec3& a = *this;
        return std::atan2(a.cross(b).length(), a.dot(b));
    }

    /// Returns whether this vector `a` and the other vector `b` are almost
    /// equal within some relative tolerance. See `Vec2::isClose()` for
    /// details.
    ///
    bool isClose(
        const Vec3& b,
        T relTol = core::defaultRelativeTolerance<T>,
        T absTol = 0) const {

        const Vec3& a = *this;
        T diff2 = a.infdiff_(b).squaredLength();
        if (diff2 == std::numeric_limits<T>::infinity()) {
            return false; // opposite infinities or finite/infinite mismatch
        }
        else {
            T relTol2 = relTol * relTol;
            T absTol2 = absTol * absTol;
            return diff2 <= relTol2 * b.squaredLength()    //
                   || diff2 <= relTol2 * a.squaredLength() //
                   || diff2 <= absTol2;
        }
    }

    /// Returns whether all coordinates in this vector `a` are almost equal to
    /// their corresponding coordinate in the other vector `b`, within some
    /// relative tolerance. See `Vec2::allClose()` for details.
    ///
    bool allClose(
        const Vec3& b,
        T relTol = core::defaultRelativeTolerance<T>,
        T absTol = 0.0f) const {

        const Vec3& a = *this;
        return core::isClose(a[0], b[0], relTol, absTol)
               && core::isClose(a[1], b[1], relTol, absTol)
               && core::isClose(a[2], b[2], relTol, absTol);
    }

    /// Returns whether the euclidean distance between this vector `a` and the
    /// other vector `b` is smaller or equal than the given absolute tolerance.
    /// See `Vec2::isNear()` for details.
    ///
    bool isNear(const Vec3& b, T absTol) const {
        const Vec3& a = *this;
        T diff2 = a.infdiff_(b).squaredLength();
        if (diff2 == std::numeric_limits<T>::infinity()) {
            return false; // opposite infinities or finite/infinite mismatch
        }
        else {
            T absTol2 = absTol * absTol;
            return diff2 <= absTol2;
        }
    }

    /// Returns whether all coordinates in this vector `a` are within some
    /// absolute tolerance of their corresponding coordinate in the other
    /// vector `b`. See `Vec2::allNear()` for details.
    ///
    bool allNear(const Vec3& b, T absTol) const {
        const Vec3& a = *this;
        return core::isNear(a[0], b[0], absTol)    //
               && core::isNear(a[1], b[1], absTol) //
               && core::isNear(a[2], b[2], absTol);
    }

private:
    T data_[3];

    Vec3 infdiff_(const Vec3& b) const {
        const Vec3& a = *this;
        return Vec3(
            core::detail::infdiff(a[0], b[0]),
            core::detail::infdiff(a[1], b[1]),
            core::detail::infdiff(a[2], b[2]));
    }
};

// See Vec2 for detailed comments
template<typename T>
inline Vec3<T>& Vec3<T>::normalize(bool* isNormalizable, T epsilon_) {
    T l2 = squaredLength();
    if (l2 <= epsilon_ * epsilon_) {
        if (isNormalizable) {
            *isNormalizable = false;
        }
        *this = Vec3(1, 0, 0);
    }
    else {
        if (isNormalizable) {
            *isNormalizable = true;
        }
        T l = std::sqrt(l2);
        *this /= l;
    }
    return *this;
}

template<typename T>
inline Vec3<T> Vec3<T>::normalized(bool* isNormalizable, T epsilon_) const {
    return Vec3(*this).normalize(isNormalizable, epsilon_);
}

/// Alias for `Vec3<float>`.
///
using Vec3f = Vec3<float>;

/// Alias for `Vec3<double>`.
///
using Vec3d = Vec3<double>;

/// Alias for `core::Array<Vec3<T>>`.
///
template<typename T>
using Vec3Array = core::Array<Vec3<T>>;

/// Alias for `core::Array<Vec3f>`.
///
using Vec3fArray = core::Array<Vec3f>;

/// Alias for `core::Array<Vec3d>`.
///
using Vec3dArray = core::Array<Vec3d>;

/// Alias for `core::SharedConstArray<Vec3<T>>`.
///
template<typename T>
using SharedConstVec3Array = core::SharedConstArray<Vec3<T>>;

/// Alias for `core::SharedConstArray<Vec3f>`.
///
using SharedConstVec3fArray = core::SharedConstArray<Vec3f>;

/// Alias for `core::SharedConstArray<Vec3d>`.
///
using SharedConstVec3dArray = core::SharedConstArray<Vec3d>;

/// Allows to iterate over a range of `Vec3` elements stored in a memory buffer
/// of Ts, where consecutive `Vec3` elements are separated by a given stride.
///
/// See `Vec2Span` for details.
///
template<typename T>
using Vec3Span = StrideSpan<T, Vec3<T>>;

/// Alias for `Vec3Span<float>`.
///
using Vec3fSpan = Vec3Span<float>;

/// Alias for `Vec3Span<double>`.
///
using Vec3dSpan = Vec3Span<double>;

/// Const version of `Vec3Span`.
///
template<typename T>
using Vec3ConstSpan = StrideSpan<const T, const Vec3<T>>;

/// Alias for `Vec3ConstSpan<float>`.
///
using Vec3fConstSpan = Vec3ConstSpan<float>;

/// Alias for `Vec3ConstSpan<double>`.
///
using Vec3dConstSpan = Vec3ConstSpan<double>;

/// Overloads `setZero(T& x)`.
///
/// \sa `core::zero<T>()`.
///
template<typename T>
inline void setZero(Vec3<T>& v) {
    v[0] = 0;
    v[1] = 0;
    v[2] = 0;
}

/// Writes the given `Vec3` to the output stream.
///
template<typename T, typename OStream>
void write(OStream& out, const Vec3<T>& v) {
    write(out, '(', v[0], ", ", v[1], ", ", v[2], ')');
}

/// Reads a `Vec3` from the input stream, and stores it in the given output
/// parameter `v`. Leading whitespaces are allowed. Raises `ParseError` if the
/// stream does not start with a `Vec3`. Raises `RangeError` if one of its
/// coordinates is outside the representable range of a T.
///
template<typename T, typename IStream>
void readTo(Vec3<T>& v, IStream& in) {
    skipWhitespacesAndExpectedCharacter(in, '(');
    readTo(v[0], in);
    skipWhitespacesAndExpectedCharacter(in, ',');
    readTo(v[1], in);
    skipWhitespacesAndExpectedCharacter(in, ',');
    readTo(v[2], in);
    skipWhitespacesAndExpectedCharacter(in, ')');
}

} // namespace vgc::geometry

template<typename T>
struct fmt::formatter<vgc::geometry::Vec3<T>> {
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && *it != '}') {
            throw format_error("invalid format");
        }
        return it;
    }
    template<typename FormatContext>
    auto format(const vgc::geometry::Vec3<T>& v, FormatContext& ctx) {
        return format_to(ctx.out(), "({}, {}, {})", v[0], v[1], v[2]);
    }
};

#endif // VGC_GEOMETRY_VEC3_H
