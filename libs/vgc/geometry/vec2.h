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

#ifndef VGC_GEOMETRY_VEC2_H
#define VGC_GEOMETRY_VEC2_H

#include <cmath>

#include <vgc/core/array.h>
#include <vgc/geometry/api.h>
#include <vgc/geometry/stride.h>
#include <vgc/geometry/vec.h>

namespace vgc::geometry {

/// \class vgc::geometry::Vec2
/// \brief Represents a 2D vector.
///
/// A Vec2 represents either a 2D point (= position), a 2D vector (= difference
/// of positions), a 2D size (= positive position), or a 2D normal (= unit
/// vector). Unlike some libraries, we do not provide different types for these
/// different use cases.
///
/// The memory layout of a `Vec2<T>` is exactly the same as:
///
/// ```cpp
/// struct {
///     T x;
///     T y;
/// };
/// ```
///
/// This will never change in any future version, as this allows to
/// conveniently use this class for data transfer to the GPU (via OpenGL,
/// Metal, etc.).
///
template<typename T>
class Vec2 {
public:
    using ScalarType = T;
    static constexpr Int dimension = 2;

    VGC_WARNING_PUSH
    VGC_WARNING_MSVC_DISABLE(26495) // member variable uninitialized
    /// Creates an uninitialized `Vec2`.
    ///
    Vec2(core::NoInit) noexcept {
    }
    VGC_WARNING_POP

    /// Creates a `Vec2` initialized to (0, 0).
    ///
    constexpr Vec2() noexcept
        : data_{0, 0} {
    }

    /// Creates a `Vec2` initialized with the given `x` and `y` coordinates.
    ///
    constexpr Vec2(T x, T y) noexcept
        : data_{x, y} {
    }

    /// Creates a `Vec2<T>` from another `Vec2<U>` object by performing a
    /// `static_cast` on each of its coordinates.
    ///
    /// ```cpp
    /// Vec2d vd(1, 2);
    /// Vec2f vf(vd); // cast from double to float
    /// ```
    ///
    template<typename U, VGC_REQUIRES(!std::is_same_v<U, T>)>
    constexpr explicit Vec2(const Vec2<U>& other) noexcept
        : data_{static_cast<T>(other[0]), static_cast<T>(other[1])} {
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

    /// Adds in-place `other` to this vector.
    ///
    constexpr Vec2& operator+=(const Vec2& other) {
        data_[0] += other[0];
        data_[1] += other[1];
        return *this;
    }

    /// Returns the addition of the two vectors `v1` and `v2`.
    ///
    friend constexpr Vec2 operator+(const Vec2& v1, const Vec2& v2) {
        return Vec2(v1) += v2;
    }

    /// Returns a copy of this vector (unary plus operator).
    ///
    constexpr Vec2 operator+() const {
        return *this;
    }

    /// Substracts in-place `other` from this vector.
    ///
    constexpr Vec2& operator-=(const Vec2& other) {
        data_[0] -= other[0];
        data_[1] -= other[1];
        return *this;
    }

    /// Returns the substraction of `v1` and `v2`.
    ///
    friend constexpr Vec2 operator-(const Vec2& v1, const Vec2& v2) {
        return Vec2(v1) -= v2;
    }

    /// Returns the opposite of this vector (unary minus operator).
    ///
    constexpr Vec2 operator-() const {
        return Vec2(-data_[0], -data_[1]);
    }

    /// Multiplies in-place this vector by the scalar `s`.
    ///
    constexpr Vec2& operator*=(T s) {
        data_[0] *= s;
        data_[1] *= s;
        return *this;
    }

    /// Returns the multiplication of this vector by the scalar `s`.
    ///
    constexpr Vec2 operator*(T s) const {
        return Vec2(*this) *= s;
    }

    /// Returns the multiplication of the scalar `s` with the vector `v`.
    ///
    friend constexpr Vec2 operator*(T s, const Vec2& v) {
        return v * s;
    }

    /// Divides in-place this vector by the scalar `s`.
    ///
    constexpr Vec2& operator/=(T s) {
        data_[0] /= s;
        data_[1] /= s;
        return *this;
    }

    /// Returns the division of this vector by the scalar `s`.
    ///
    constexpr Vec2 operator/(T s) const {
        return Vec2(*this) /= s;
    }

    // clang-format off
    
    /// Returns whether `v1` and `v2` are equal.
    ///
    friend constexpr bool operator==(const Vec2& v1, const Vec2& v2) {
        return v1.data_[0] == v2.data_[0]
            && v1.data_[1] == v2.data_[1];
    }

    /// Returns whether `v1` and `v2` are different.
    ///
    friend constexpr bool operator!=(const Vec2& v1, const Vec2& v2) {
        return v1.data_[0] != v2.data_[0]
            || v1.data_[1] != v2.data_[1];
    }

    /// Compares `v1` and `v2` using lexicographic order on (x, y).
    ///
    friend constexpr bool operator<(const Vec2& v1, const Vec2& v2) {
        return ( (v1.data_[0] < v2.data_[0]) ||
               (!(v2.data_[0] < v1.data_[0]) &&
               ( (v1.data_[1] < v2.data_[1]))));
    }

    // clang-format on

    /// Compares `v1` and `v2` using lexicographic order on (x, y).
    ///
    friend constexpr bool operator<=(const Vec2& v1, const Vec2& v2) {
        return !(v2 < v1);
    }

    /// Compares `v1` and `v2` using lexicographic order on (x, y).
    ///
    friend constexpr bool operator>(const Vec2& v1, const Vec2& v2) {
        return v2 < v1;
    }

    /// Compares `v1` and `v2` using lexicographic order on (x, y).
    ///
    friend constexpr bool operator>=(const Vec2& v1, const Vec2& v2) {
        return !(v1 < v2);
    }

    /// Returns the Euclidean length of this vector, that is,
    /// `sqrt(x*x + y*y)`.
    ///
    /// \sa `squaredLength()`.
    ///
    T length() const {
        return std::sqrt(squaredLength());
    }

    /// Returns the square of the Euclidean length of this vector, that is,
    /// `x*x + y*y`.
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
    /// exactly equal to the null vector `Vec2()`.
    ///
    /// If the vector is considered non-normalizable, then it is set to
    /// `(1, 0)`.
    ///
    /// \sa `length()`, `normalized()`.
    ///
    Vec2& normalize(bool* isNormalizable = nullptr, T epsilon = 0);

    /// Returns a normalized copy of this vector.
    ///
    /// \sa `length()`, `normalize()`.
    ///
    Vec2 normalized(bool* isNormalizable = nullptr, T epsilon = 0) const;

    /// Rotates this vector by 90°, transforming the X-axis unit vector into the
    /// Y-axis unit vector.
    ///
    /// In a top-left origin system (X right, Y down), this means a clockwise
    /// 90° turn.
    ///
    /// In a bottom-left origin system (X right, Y up) this means a
    /// counterclockwise 90° turn.
    ///
    /// ```cpp
    /// Vec2d v(10, 20);
    /// v.orthogonalize(); // => (-20, 10)
    /// ```
    ///
    /// \sa `orthogonalized()`.
    ///
    constexpr Vec2& orthogonalize() {
        T tmp = data_[0];
        data_[0] = -data_[1];
        data_[1] = tmp;
        return *this;
    }

    /// Returns a copy of this vector rotated 90°, transforming the X-axis unit
    /// vector into the Y-axis unit vector.
    ///
    /// ```cpp
    /// Vec2d v1(10, 20);
    /// Vec2d v2 = v1.orthogonalized(); // => v2 == (-20, 10)
    /// ```
    ///
    /// \sa `orthogonalize()`.
    ///
    constexpr Vec2 orthogonalized() const {
        return Vec2(*this).orthogonalize();
    }

    /// Returns the dot product between this vector `a` and the other vector
    /// `b`.
    ///
    /// ```cpp
    /// Vec2d a, b;
    /// double d = a.dot(b);
    /// ```
    ///
    /// This is equivalent to:
    ///
    /// ```cpp
    /// a[0]*b[0] + a[1]*b[1]
    /// ```
    ///
    /// Note that, except for numerical errors, this is also equal to:
    ///
    /// ```cpp
    /// a.length() * b.length() * cos(a.angle(b))
    /// ```
    ///
    /// \sa `det()`, `angle()`.
    ///
    constexpr T dot(const Vec2& b) const {
        const Vec2& a = *this;
        return a[0] * b[0] + a[1] * b[1];
    }

    /// Returns the "2D determinant" between this vector `a` and the given
    /// vector `b`.
    ///
    /// ```cpp
    /// Vec2d a, b;
    /// double d = a.det(b); // equivalent to a[0]*b[1] - a[1]*b[0]
    /// ```
    ///
    /// Note that, except for numerical errors, this is equal to:
    /// - `a.length() * b.length() * sin(a.angle(b))`
    /// - the (signed) area of the parallelogram spanned by `a` and `b`
    /// - the Z coordinate of the cross product between `a` and `b`, if `a` and `b`
    ///   are interpreted as 3D vectors with Z = 0.
    ///
    /// Note that `a.det(b)` has the same sign as `a.angle(b)`. See the
    /// documentation of `angle()` for more information on how to interpret
    /// this sign based on your choice of coordinate system (Y-axis pointing up
    /// or down).
    ///
    /// \sa `dot()`, `angle()`.
    ///
    constexpr T det(const Vec2& b) const {
        const Vec2& a = *this;
        return a[0] * b[1] - a[1] * b[0];
    }

    /// Returns the angle, in radians and in the interval (−π, π], between this
    /// vector `a` and the given vector `b`.
    ///
    /// ```cpp
    /// Vec2d a(1, 0);
    /// Vec2d b(1, 1);
    /// double d = a.angle(b); // returns 0.7853981633974483 (approx. π/4 rad = 45 deg)
    /// ```
    ///
    /// This value is computed using the following formula:
    ///
    /// ```cpp
    /// angle = atan2(a.det(b), a.dot(b));
    /// ```
    ///
    /// It returns an undefined value if either `a` or `b` is zero-length.
    ///
    /// If you are using the following coordinate system (X pointing right and Y
    /// pointing up, like is usual in the fields of mathematics):
    ///
    /// ```
    /// ^ Y
    /// |
    /// |
    /// o-----> X
    /// ```
    ///
    /// then `a.angle(b)` is positive if going from `a` to `b` is a counterclockwise
    /// motion, and negative if going from `a` to `b` is a clockwise motion.
    ///
    /// If instead you are using the following coordinate system (X pointing
    /// right and Y pointing down, like is usual in user interface systems):
    ///
    /// ```
    /// o-----> X
    /// |
    /// |
    /// v Y
    /// ```
    ///
    /// then `a.angle(b)` is positive if going from `a` to `b` is a clockwise
    /// motion, and negative if going from `a` to `b` is a counterclockwise
    /// motion.
    ///
    /// \sa `det()`, `dot()`, `angle()`.
    ///
    T angle(const Vec2& b) const {
        const Vec2& a = *this;
        return std::atan2(a.det(b), a.dot(b));
    }

    /// Returns the angle, in radians and in the interval (−π, π], between the
    /// X axis and this vector `a`.
    ///
    /// ```cpp
    /// Vec2d a(1, 1);
    /// double d = a.angle(); // returns π/4 (= 45 deg)
    /// ```
    ///
    /// This value is computed using the following formula:
    ///
    /// ```cpp
    /// angle = atan2(a[1], a[0]);
    /// ```
    ///
    /// It is equivalent to calling:
    ///
    /// ```cpp
    /// T angle = Vec2(1, 0).angle(a);
    /// ```
    ///
    /// \sa `det()`, `dot()`, `angle(b)`.
    ///
    T angle() const {
        const Vec2& a = *this;
        return std::atan2(a[1], a[0]);
    }

    /// Returns whether this vector `a` and the given vector `b` are almost
    /// equal within some relative tolerance. If all values are finite, this
    /// function is equivalent to:
    ///
    /// ```cpp
    /// (b-a).length() <= max(relTol * max(a.length(), b.length()), absTol)
    /// ```
    ///
    /// If you need a per-coordinate comparison rather than using the euclidean
    /// distance, you should use `allClose()` instead.
    ///
    /// If you need an absolute tolerance (which is especially important if one
    /// of the given vectors could be exactly zero), you should use `isNear()`
    /// or `allNear()` instead. Please refer to the documentation of
    /// `isClose(T, T)` for a general discussion about the differences
    /// between `isClose()` and `isNear()`.
    ///
    /// If any coordinate is NaN, this function returns false. Two coordinates
    /// equal to infinity with the same sign are considered close. Two
    /// coordinates equal to infinity with opposite signs are (obviously) not
    /// considered close.
    ///
    /// ```cpp
    /// double inf = core::DoubleInfinity;
    /// Vec2d(inf, inf).isClose(Vec2d(inf, inf));  // true
    /// Vec2d(inf, inf).isClose(Vec2d(inf, -inf)); // false
    /// ```
    ///
    /// If some coordinates are infinite and some others are finite, the
    /// behavior can in some cases be surprising, but actually makes sense:
    ///
    /// ```cpp
    /// Vec2d(inf, inf).isClose(Vec2d(inf, 42)); // false
    /// Vec2d(inf, 42).isClose(Vec2d(inf, 42));  // true
    /// Vec2d(inf, 42).isClose(Vec2d(inf, 43));  // true (yes!)
    /// ```
    ///
    /// Notice how the last one returns true even though `isClose(42, 43)`
    /// returns false. This is because for a sufficiently large x, the distance
    /// between `Vec2d(x, 42)` and `Vec2d(x, 43)`, which is always equal to 1,
    /// is indeed negligible compared to their respective length, which
    /// approaches infinity as x approaches infinity. For example, the
    /// following also returns true:
    ///
    /// ```cpp
    /// Vec2d(1e20, 42).isClose(Vec2d(1e20, 43)); // true
    /// ```
    ///
    /// Note that `allClose()` returns false in these cases, which may or may
    /// not be what you need depending on your situation. In case of doubt,
    /// `isClose()` is typically the better choice.
    ///
    /// ```cpp
    /// Vec2d(inf, 42).allClose(Vec2c(inf, 43));   // false
    /// Vec2d(1e20, 42).allClose(Vec2d(1e20, 43)); // false
    /// ```
    ///
    bool isClose(
        const Vec2& b,
        T relTol = core::defaultRelativeTolerance<T>,
        T absTol = 0) const {

        const Vec2& a = *this;
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
    /// their corresponding coordinate in the given vector `b`, within some
    /// relative tolerance. this function is equivalent to:
    ///
    /// ```cpp
    /// isClose(a[0], b[0], relTol, absTol) && isClose(a[1], b[1], relTol, absTol)
    /// ```
    ///
    /// This is similar to `a.isClose(b)`, but completely decorellates the X
    /// and Y coordinates, which may be preferrable if the two given vectors do
    /// not represent points/vectors in the euclidean plane, but more abstract
    /// parameters.
    ///
    /// If you need an absolute tolerance (which is especially important if one
    /// of the given vectors could be exactly zero), you should use `isNear()`
    /// or `allNear()` instead.
    ///
    /// Please refer to `isClose(T, T)` for more details on
    /// NaN/infinity handling, and a general discussion about the differences
    /// between `isClose()` and `isNear()`.
    ///
    /// Using `allClose()` is typically faster than `isClose()` since it
    /// doesn't have to compute (squared) distances, but beware that
    /// `allClose()` isn't a true "euclidean proximity" test, and in particular
    /// it is not invariant to rotation of the coordinate system, and will
    /// behave very differently if one of the coordinates is exactly or near
    /// zero:
    ///
    /// ```cpp
    /// Vec2d a(1, 0);
    /// Vec2d b(1, 1e-10);
    /// a.isClose(b);  // true because (b-a).length() <= relTol * a.length()
    /// a.allClose(b); // false because isClose(0, 1e-10) == false
    /// ```
    ///
    /// In other words, `a` and `b` are relatively close to each others as 2D
    /// points, even though their Y coordinates are not relatively close to
    /// each others.
    ///
    bool allClose(
        const Vec2& b,
        T relTol = core::defaultRelativeTolerance<T>,
        T absTol = 0.0f) const {

        const Vec2& a = *this;
        return core::isClose(a[0], b[0], relTol, absTol)
               && core::isClose(a[1], b[1], relTol, absTol);
    }

    /// Returns whether the euclidean distance between this vector `a` and the
    /// given vector `b` is smaller or equal than the given absolute tolerance.
    /// In other words, this returns whether `b` is contained in the disk of
    /// center `a` and radius `absTol`. If all values are finite, this function
    /// is equivalent to:
    ///
    /// ```cpp
    /// (b-a).length() <= absTol
    /// ```
    ///
    /// If you need a per-coordinate comparison rather than using the euclidean
    /// distance, you should use `allNear()` instead.
    ///
    /// If you need a relative tolerance rather than an absolute tolerance, you
    /// should use `isClose()` instead. Please refer to the documentation of
    /// `isClose(T, T)` for a general discussion about the differences
    /// between `isClose()` and `isNear()`.
    ///
    /// If any coordinate is NaN, this function returns false. Two coordinates
    /// equal to infinity with the same sign are considered near. Two
    /// coordinates equal to infinity with opposite signs are (obviously) not
    /// considered near. If some coordinates are infinite and some others are
    /// finite, the behavior is as per intuition:
    ///
    /// ```cpp
    /// double inf = core::DoubleInfinity;
    /// double absTol = 0.5;
    /// Vec2d(inf, inf).isNear(Vec2d(inf, inf), absTol);  // true
    /// Vec2d(inf, inf).isNear(Vec2d(inf, -inf), absTol); // false
    /// Vec2d(inf, inf).isNear(Vec2d(inf, 42), absTol);   // false
    /// Vec2d(inf, 42).isNear(Vec2d(inf, 42), absTol);    // true
    /// Vec2d(inf, 42).isNear(Vec2d(inf, 42.4), absTol);  // true
    /// Vec2d(inf, 42).isNear(Vec2d(inf, 42.6), absTol);  // false
    /// ```
    ///
    bool isNear(const Vec2& b, T absTol) const {
        const Vec2& a = *this;
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
    /// absolute tolerance of their corresponding coordinate in the given
    /// vector `b`. this function is equivalent to:
    ///
    /// ```cpp
    /// isNear(a[0], b[0], absTol) && isNear(a[1], b[1], absTol)
    /// ```
    ///
    /// Which, if all coordinates are finite, is equivalent to:
    ///
    /// ```cpp
    /// abs(a[0]-b[0]) <= absTol && abs(a[1]-b[1]) <= absTol
    /// ```
    ///
    /// This is similar to `a.isNear(b)`, but completely decorellates the X and
    /// Y coordinates, which may be preferrable if the two given vectors do not
    /// represent points/vectors in the euclidean plane, but more abstract
    /// parameters.
    ///
    /// If you need a relative tolerance rather than an absolute tolerance, you
    /// should use `isClose()` or `allClose()` instead.
    ///
    /// Please refer to `core::isClose()` for more details on NaN/infinity
    /// handling, and a discussion about the differences between `isClose()` and
    /// `isNear()`.
    ///
    /// Using `allNear()` is typically faster than `isNear()`, since it doesn't
    /// have to compute (squared) distances. However, beware that `allNear()`
    /// isn't a true euclidean proximity test, and in particular it isn't
    /// invariant to rotation of the coordinate system, which may or not matter
    /// depending on the situation.
    ///
    /// A good use case for `allNear()` is to determine whether the size of a
    /// rectangle (e.g., the size of a widget) has changed, in which case a
    /// true euclidean test isn't really meaningful anyway, and the performance
    /// gain of using `allNear()` can be useful.
    ///
    bool allNear(const Vec2& b, T absTol) const {
        const Vec2& a = *this;
        return core::isNear(a[0], b[0], absTol) && core::isNear(a[1], b[1], absTol);
    }

private:
    T data_[2];

    Vec2 infdiff_(const Vec2& b) const {
        const Vec2& a = *this;
        return Vec2(core::detail::infdiff(a[0], b[0]), core::detail::infdiff(a[1], b[1]));
    }
};

// We define this function out-of-class to keep the name `epsilon` in the
// declaration (and thus documentation), while using `epsilon_` in the
// definition to prevent a warning about hiding core::epsilon.
//
// Note: Currently, (inf, 42).normalized() returns (nan, 0). Indeed,
// we have v.length() = inf, and (inf/inf, 42/inf) = (nan, 0).
//
// We could instead do special handling of infinite values such that:
//
// ( inf, 42).normalized() -> ( 1,  0)
// (-inf, 42).normalized() -> (-1,  0)
// (42,  inf).normalized() -> ( 0,  1)
// (42, -inf).normalized() -> ( 0, -1)
//
// ( inf,  inf).normalized() -> ( u,  u)  with u = sqrt(2) / 2
// ( inf, -inf).normalized() -> ( u, -u)
// (-inf,  inf).normalized() -> (-u,  u)
// (-inf, -inf).normalized() -> (-u, -u)
//
// We could also have special handling of nan values such that:
//
// (nan,  42).normalized() -> (nan, nan)
// (nan, inf).normalized() -> (0, 1) or (nan, nan)
// (nan,   0).normalized() -> (1, 0)
//
// Perhaps such special handling could be optional, with a NanPolicy
// argument controling whether to do special handling to avoid Nans.
//
template<typename T>
inline Vec2<T>& Vec2<T>::normalize(bool* isNormalizable, T epsilon_) {
    T l2 = squaredLength();
    if (l2 <= epsilon_ * epsilon_) {
        if (isNormalizable) {
            *isNormalizable = false;
        }
        *this = Vec2(1, 0);
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
inline Vec2<T> Vec2<T>::normalized(bool* isNormalizable, T epsilon_) const {
    return Vec2(*this).normalize(isNormalizable, epsilon_);
}

/// Alias for `Vec2<float>`.
///
using Vec2f = Vec2<float>;

/// Alias for `Vec2<double>`.
///
using Vec2d = Vec2<double>;

/// Alias for `core::Array<Vec2<T>>`.
///
template<typename T>
using Vec2Array = core::Array<Vec2<T>>;

/// Alias for `core::Array<Vec2f>`.
///
using Vec2fArray = core::Array<Vec2f>;

/// Alias for `core::Array<Vec2d>`.
///
using Vec2dArray = core::Array<Vec2d>;

/// Alias for `core::SharedConstArray<Vec2<T>>`.
///
template<typename T>
using SharedConstVec2Array = core::SharedConstArray<Vec2<T>>;

/// Alias for `core::SharedConstArray<Vec2f>`.
///
using SharedConstVec2fArray = core::SharedConstArray<Vec2f>;

/// Alias for `core::SharedConstArray<Vec2d>`.
///
using SharedConstVec2dArray = core::SharedConstArray<Vec2d>;

/// Allows to iterate over a range of `Vec2` elements stored in a memory buffer
/// of Ts, where consecutive `Vec2` elements are separated by a given stride.
///
/// ```cpp
/// FloatArray buffer = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
/// for (const Vec2d& v : Vec2dSpan(buffer.data(), 2, 5)) {
///     std::cout << v << std::end;
/// }
/// // => prints "(1, 2)(6, 7)"
/// ```
///
template<typename T>
using Vec2Span = StrideSpan<T, Vec2<T>>;

/// Alias for `Vec2Span<float>`.
///
using Vec2fSpan = Vec2Span<float>;

/// Alias for `Vec2Span<double>`.
///
using Vec2dSpan = Vec2Span<double>;

/// Const version of `Vec2Span`.
///
template<typename T>
using Vec2ConstSpan = StrideSpan<const T, const Vec2<T>>;

/// Alias for `Vec2ConstSpan<float>`.
///
using Vec2fConstSpan = Vec2ConstSpan<float>;

/// Alias for `Vec2ConstSpan<double>`.
///
using Vec2dConstSpan = Vec2ConstSpan<double>;

/// Overloads `setZero(T& x)`.
///
/// \sa `core::zero<T>()`.
///
template<typename T>
inline void setZero(Vec2<T>& v) {
    v[0] = 0;
    v[1] = 0;
}

/// Writes the given `Vec2` to the output stream.
///
template<typename T, typename OStream>
void write(OStream& out, const Vec2<T>& v) {
    write(out, '(', v[0], ", ", v[1], ')');
}

/// Reads a `Vec2` from the input stream, and stores it in the given output
/// parameter `v`. Leading whitespaces are allowed. Raises `ParseError` if the
/// stream does not start with a `Vec2`. Raises `RangeError` if one of its
/// coordinates is outside the representable range of a T.
///
template<typename T, typename IStream>
void readTo(Vec2<T>& v, IStream& in) {
    skipWhitespacesAndExpectedCharacter(in, '(');
    readTo(v[0], in);
    skipWhitespacesAndExpectedCharacter(in, ',');
    readTo(v[1], in);
    skipWhitespacesAndExpectedCharacter(in, ')');
}

} // namespace vgc::geometry

template<typename T>
struct fmt::formatter<vgc::geometry::Vec2<T>> {
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && *it != '}') {
            throw format_error("invalid format");
        }
        return it;
    }
    template<typename FormatContext>
    auto format(const vgc::geometry::Vec2<T>& v, FormatContext& ctx) {
        return format_to(ctx.out(), "({}, {})", v[0], v[1]);
    }
};

#endif // VGC_GEOMETRY_VEC2_H
