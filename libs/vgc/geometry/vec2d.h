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

// This file was automatically generated, please do not edit directly.
// Instead, edit tools/vec2x.h then run tools/generate.py.

// clang-format off

#ifndef VGC_GEOMETRY_VEC2D_H
#define VGC_GEOMETRY_VEC2D_H

#include <cmath>

#include <vgc/core/array.h>
#include <vgc/geometry/api.h>
#include <vgc/geometry/stride.h>
#include <vgc/geometry/vec.h>

namespace vgc::geometry {

/// \class vgc::geometry::Vec2d
/// \brief 2D vector using double-precision floating points.
///
/// A Vec2d represents either a 2D point (= position), a 2D vector (=
/// difference of positions), a 2D size (= positive position), or a 2D normal
/// (= unit vector). Unlike other libraries, we do not use separate types for
/// all these use cases.
///
/// The memory size of a `Vec2d` is exactly `2 * sizeof(double)`. This will never
/// change in any future version, as this allows to conveniently use this class
/// for data transfer to the GPU (via OpenGL, Metal, etc.).
///
// VGC_GEOMETRY_API <- Omitted on purpose.
//                     If needed, manually export individual functions.
class Vec2d {
public:
    using ScalarType = double;
    static constexpr Int dimension = 2;

    VGC_WARNING_PUSH
    VGC_WARNING_MSVC_DISABLE(26495) // member variable uninitialized
    /// Creates an uninitialized `Vec2d`.
    ///
    Vec2d(core::NoInit) noexcept {
    }
    VGC_WARNING_POP

    /// Creates a `Vec2d` initialized to (0, 0).
    ///
    constexpr Vec2d() noexcept
        : data_{0, 0} {
    }


    /// Creates a `Vec2d` initialized with the given `x` and `y` coordinates.
    ///
    constexpr Vec2d(double x, double y) noexcept
        : data_{x, y} {
    }

    /// Creates a `Vec2d` from another `Vec<2, T>` object by performing a
    /// `static_cast` on each of its coordinates.
    ///
    /// ```cpp
    /// vgc::geometrt::Vec2d vd(1, 2);
    /// vgc::geometrt::Vec2 vf(vd); // cast from double to double
    /// ```
    ///
    template<typename TVec2,
        VGC_REQUIRES(
            isVec<TVec2>
         && TVec2::dimension == 2
         && !std::is_same_v<TVec2, Vec2d>)>
    explicit constexpr Vec2d(const TVec2& other) noexcept
        : data_{static_cast<double>(other[0]),
                static_cast<double>(other[1])} {
    }

    /// Accesses the `i`-th coordinate of this `Vec2d`.
    ///
    constexpr const double& operator[](Int i) const {
        return data_[i];
    }

    /// Mutates the `i`-th coordinate of this `Vec2d`.
    ///
    constexpr double& operator[](Int i) {
        return data_[i];
    }

    /// Accesses the first coordinate of this `Vec2d`.
    ///
    constexpr double x() const {
        return data_[0];
    }

    /// Accesses the second coordinate of this `Vec2d`.
    ///
    constexpr double y() const {
        return data_[1];
    }

    /// Mutates the first coordinate of this `Vec2d`.
    ///
    constexpr void setX(double x) {
        data_[0] = x;
    }

    /// Mutates the second coordinate of this `Vec2d`.
    ///
    constexpr void setY(double y) {
        data_[1] = y;
    }

    /// Adds in-place `other` to this `Vec2d`.
    ///
    constexpr Vec2d& operator+=(const Vec2d& other) {
        data_[0] += other[0];
        data_[1] += other[1];
        return *this;
    }

    /// Returns the addition of the two vectors `v1` and `v2`.
    ///
    friend constexpr Vec2d operator+(const Vec2d& v1, const Vec2d& v2) {
        return Vec2d(v1) += v2;
    }

    /// Returns a copy of this `Vec2d` (unary plus operator).
    ///
    constexpr Vec2d operator+() const {
        return *this;
    }

    /// Substracts in-place `other` from this `Vec2d`.
    ///
    constexpr Vec2d& operator-=(const Vec2d& other) {
        data_[0] -= other[0];
        data_[1] -= other[1];
        return *this;
    }

    /// Returns the substraction of `v1` and `v2`.
    ///
    friend constexpr Vec2d operator-(const Vec2d& v1, const Vec2d& v2) {
        return Vec2d(v1) -= v2;
    }

    /// Returns the opposite of this `Vec2d` (unary minus operator).
    ///
    constexpr Vec2d operator-() const {
        return Vec2d(-data_[0], -data_[1]);
    }

    /// Multiplies in-place this `Vec2d` by the scalar `s`.
    ///
    constexpr Vec2d& operator*=(double s) {
        data_[0] *= s;
        data_[1] *= s;
        return *this;
    }

    /// Returns the multiplication of this `Vec2d` by the scalar `s`.
    ///
    constexpr Vec2d operator*(double s) const {
        return Vec2d(*this) *= s;
    }

    /// Returns the multiplication of the scalar `s` with the vector `v`.
    ///
    friend constexpr Vec2d operator*(double s, const Vec2d& v) {
        return v * s;
    }

    /// Divides in-place this `Vec2d` by the scalar `s`.
    ///
    constexpr Vec2d& operator/=(double s) {
        data_[0] /= s;
        data_[1] /= s;
        return *this;
    }

    /// Returns the division of this `Vec2d` by the scalar `s`.
    ///
    constexpr Vec2d operator/(double s) const {
        return Vec2d(*this) /= s;
    }

    /// Returns whether `v1` and `v2` are equal.
    ///
    friend constexpr bool operator==(const Vec2d& v1, const Vec2d& v2) {
        return v1.data_[0] == v2.data_[0]
            && v1.data_[1] == v2.data_[1];
    }

    /// Returns whether `v1` and `v2` are different.
    ///
    friend constexpr bool operator!=(const Vec2d& v1, const Vec2d& v2) {
        return v1.data_[0] != v2.data_[0]
            || v1.data_[1] != v2.data_[1];
    }

    /// Compares `v1` and `v2` using the lexicographic
    /// order.
    ///
    friend constexpr bool operator<(const Vec2d& v1, const Vec2d& v2) {
        return ( (v1.data_[0] < v2.data_[0]) ||
               (!(v2.data_[0] < v1.data_[0]) &&
               ( (v1.data_[1] < v2.data_[1]))));
    }

    /// Compares `v1` and `v2` using the lexicographic
    /// order.
    ///
    friend constexpr bool operator<=(const Vec2d& v1, const Vec2d& v2) {
        return !(v2 < v1);
    }

    /// Compares the `v1` and `v2` using the lexicographic
    /// order.
    ///
    friend constexpr bool operator>(const Vec2d& v1, const Vec2d& v2) {
        return v2 < v1;
    }

    /// Compares `v1` and `v2` using the lexicographic
    /// order.
    ///
    friend constexpr bool operator>=(const Vec2d& v1, const Vec2d& v2) {
        return !(v1 < v2);
    }

    /// Returns the Euclidean length of this `Vec2d`.
    ///
    double length() const {
        return std::sqrt(squaredLength());
    }

    /// Returns the square of the Euclidean length of this `Vec2d`.
    ///
    /// This function is faster than `length()`, therefore it is a good idea to
    /// use it whenever you don't need the actual length. For example, if you
    /// need to know which vector has greater length, you can use
    /// `v1.squaredLength() < v2.squaredLength()`.
    ///
    constexpr double squaredLength() const {
        return data_[0] * data_[0] + data_[1] * data_[1];
    }

    /// Makes this `Vec2d` a unit vector by dividing it by its length.
    ///
    /// If provided, `isNormalizable` is set to either `true` or `false`
    /// depending on whether the vector was considered normalizable.
    ///
    /// The vector is considered non-normalizable whenever its length is less
    /// or equal than the given `epsilon`. An appropriate epsilon is
    /// context-dependent, and therefore zero is used as default, which means
    /// that the vector is considered non-normalizable if and only if it is
    /// exactly equal to the null vector `Vec2d()`.
    ///
    /// If the vector is considered non-normalizable, then it is set to
    /// `(1.0, 0.0)`.
    ///
    /// \sa `length()`.
    ///
    Vec2d& normalize(bool* isNormalizable = nullptr, double epsilon = 0.0);

    /// Returns a normalized copy of this Vec2d.
    ///
    Vec2d normalized(bool* isNormalizable = nullptr, double epsilon = 0.0) const;

    /// Rotates this Vec2d by 90° counter-clockwise, assuming a left-handed
    /// coordinate system.
    ///
    constexpr Vec2d& orthogonalize() {
        double tmp = data_[0];
        data_[0] = - data_[1];
        data_[1] = tmp;
        return *this;
    }

    /// Returns a copy of this Vec2d rotated 90° counter-clockwise, assuming a
    /// left-handed coordinate system.
    ///
    constexpr Vec2d orthogonalized() const {
        return Vec2d(*this).orthogonalize();
    }

    /// Returns the dot product between this Vec2d `a` and the given Vec2d `b`.
    ///
    /// ```cpp
    /// double d = a.dot(b); // equivalent to a[0]*b[0] + a[1]*b[1]
    /// ```
    ///
    /// Note that this is also equal to `a.length() * b.length() * cos(a.angle(b))`.
    ///
    /// \sa det(), angle()
    ///
    constexpr double dot(const Vec2d& b) const {
        const Vec2d& a = *this;
        return a[0]*b[0] + a[1]*b[1];
    }

    /// Returns the "determinant" between this Vec2d `a` and the given Vec2d `b`.
    ///
    /// ```cpp
    /// double d = a.det(b); // equivalent to a[0]*b[1] - a[1]*b[0]
    /// ```
    ///
    /// Note that this is equal to:
    /// - `a.length() * b.length() * sin(a.angle(b))`
    /// - the (signed) area of the parallelogram spanned by `a` and `b`
    /// - the Z coordinate of the cross product between `a` and `b`, if `a` and `b`
    ///   are interpreted as 3D vectors with Z = 0.
    ///
    /// Note that `a.det(b)` has the same sign as `a.angle(b)`. See the
    /// documentation of Vec2d::angle() for more information on how to
    /// interpret this sign based on your choice of coordinate system (Y-axis
    /// pointing up or down).
    ///
    /// \sa dot(), angle()
    ///
    constexpr double det(const Vec2d& b) const {
        const Vec2d& a = *this;
        return a[0]*b[1] - a[1]*b[0];
    }

    /// Returns the angle, in radians and in the interval (−π, π], between this
    /// Vec2d `a` and the given Vec2d `b`.
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
    /// double angle = atan2(a.det(b), a.dot(b));
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
    /// then a.angle(b) is positive if going from a to b is a counterclockwise
    /// motion, and negative if going from a to b is a clockwise motion.
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
    /// then a.angle(b) is positive if going from a to b is a clockwise motion,
    /// and negative if going from a to b is a counterclockwise motion.
    ///
    /// \sa det(), dot()
    ///
    double angle(const Vec2d& b) const {
        const Vec2d& a = *this;
        return std::atan2(a.det(b), a.dot(b));
    }

    /// Returns whether this Vec2d `a` and the given Vec2d `b` are almost equal
    /// within some relative tolerance. If all values are finite, this function
    /// is equivalent to:
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
    /// `isClose(float, float)` for a general discussion about the differences
    /// between `isClose()` and `isNear()`.
    ///
    /// If any coordinate is NaN, this function returns false. Two coordinates
    /// equal to infinity with the same sign are considered close. Two
    /// coordinates equal to infinity with opposite signs are (obviously) not
    /// considered close.
    ///
    /// ```cpp
    /// double inf = std::numeric_limits<double>::infinity();
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
    /// `isClose` is typically the better choice.
    ///
    /// ```cpp
    /// Vec2d(inf, 42).allClose(Vec2d(inf, 43));   // false
    /// Vec2d(1e20, 42).allClose(Vec2d(1e20, 43)); // false
    /// ```
    ///
    bool isClose(const Vec2d& b, double relTol = 1e-9, double absTol = 0.0) const {
        const Vec2d& a = *this;
        double diff2 = a.infdiff_(b).squaredLength();
        if (diff2 == std::numeric_limits<double>::infinity()) {
            return false; // opposite infinities or finite/infinite mismatch
        }
        else {
            double relTol2 = relTol * relTol;
            double absTol2 = absTol * absTol;
            return diff2 <= relTol2 * b.squaredLength()
                || diff2 <= relTol2 * a.squaredLength()
                || diff2 <= absTol2;
        }
    }

    /// Returns whether all coordinates in this Vec2d `a` are almost equal to
    /// their corresponding coordinate in the given Vec2d `b`, within some
    /// relative tolerance. this function is equivalent to:
    ///
    /// ```cpp
    /// isClose(a[0], b[0], relTol, absTol) && isClose(a[1], b[1], relTol, absTol)
    /// ```
    ///
    /// This is similar to `a.isClose(b)`, but completely decorellates the X
    /// and Y coordinates, which may be preferrable if the two given Vec2d do
    /// not represent points/vectors in the euclidean plane, but more abstract
    /// parameters.
    ///
    /// If you need an absolute tolerance (which is especially important if one
    /// of the given vectors could be exactly zero), you should use `isNear()`
    /// or `allNear()` instead.
    ///
    /// Please refer to `isClose(float, float)` for more details on
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
    /// vgc::core::Vec2d a(1.0, 0.0);
    /// vgc::core::Vec2d b(1.0, 1e-10);
    /// a.isClose(b);  // true because (b-a).length() <= relTol * a.length()
    /// a.allClose(b); // false because isClose(0.0, 1e-10) == false
    /// ```
    ///
    /// In other words, `a` and `b` are relatively close to each others as 2D
    /// points, even though their Y coordinates are not relatively close to
    /// each others.
    ///
    bool allClose(const Vec2d& b, double relTol = 1e-9, double absTol = 0.0) const {
        const Vec2d& a = *this;
        return core::isClose(a[0], b[0], relTol, absTol)
            && core::isClose(a[1], b[1], relTol, absTol);
    }

    /// Returns whether the euclidean distance between this Vec2d `a` and the
    /// given Vec2d `b` is smaller or equal than the given absolute tolerance.
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
    /// `isClose(float, float)` for a general discussion about the differences
    /// between `isClose()` and `isNear()`.
    ///
    /// If any coordinate is NaN, this function returns false. Two coordinates
    /// equal to infinity with the same sign are considered near. Two
    /// coordinates equal to infinity with opposite signs are (obviously) not
    /// considered near. If some coordinates are infinite and some others are
    /// finite, the behavior is as per intuition:
    ///
    /// ```cpp
    /// double inf = std::numeric_limits<double>::infinity();
    /// double absTol = 0.5;
    /// Vec2d(inf, inf).isNear(Vec2d(inf, inf), absTol);  // true
    /// Vec2d(inf, inf).isNear(Vec2d(inf, -inf), absTol); // false
    /// Vec2d(inf, inf).isNear(Vec2d(inf, 42), absTol);   // false
    /// Vec2d(inf, 42).isNear(Vec2d(inf, 42), absTol);    // true
    /// Vec2d(inf, 42).isNear(Vec2d(inf, 42.4), absTol);  // true
    /// Vec2d(inf, 42).isNear(Vec2d(inf, 42.6), absTol);  // false
    /// ```
    ///
    bool isNear(const Vec2d& b, double absTol) const {
        const Vec2d& a = *this;
        double diff2 = a.infdiff_(b).squaredLength();
        if (diff2 == std::numeric_limits<double>::infinity()) {
            return false; // opposite infinities or finite/infinite mismatch
        }
        else {
            double absTol2 = absTol * absTol;
            return diff2 <= absTol2;
        }
    }

    /// Returns whether all coordinates in this Vec2d `a` are within some
    /// absolute tolerance of their corresponding coordinate in the given Vec2d
    /// `b`. this function is equivalent to:
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
    /// Y coordinates, which may be preferrable if the two given Vec2d do not
    /// represent points/vectors in the euclidean plane, but more abstract
    /// parameters.
    ///
    /// If you need a relative tolerance rather than an absolute tolerance, you
    /// should use `isClose()` or `allClose()` instead.
    ///
    /// Please refer to `isClose(float, float)` for more details on NaN/infinity
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
    bool allNear(const Vec2d& b, double absTol) const {
        const Vec2d& a = *this;
        return core::isNear(a[0], b[0], absTol)
            && core::isNear(a[1], b[1], absTol);
    }

private:
    double data_[2];

    Vec2d infdiff_(const Vec2d& b) const {
        const Vec2d& a = *this;
        return Vec2d(
            core::detail::infdiff(a[0], b[0]),
            core::detail::infdiff(a[1], b[1]));
    }
};

// We define this function out-of-class to keep the name `epsilon` in the
// declaration (and thus documentation), while using `epsilon_` in the
// definition to prevent a warning about hiding vgc::core::epsilon.
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
inline Vec2d& Vec2d::normalize(bool* isNormalizable, double epsilon_) {
    double l2 = squaredLength();
    if (l2 <= epsilon_*epsilon_) {
        if (isNormalizable) {
            *isNormalizable = false;
        }
        *this = Vec2d(1.0, 0.0);
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

inline Vec2d Vec2d::normalized(bool* isNormalizable, double epsilon_) const {
    return Vec2d(*this).normalize(isNormalizable, epsilon_);
}

/// Alias for `vgc::core::Array<vgc::geometry::Vec2d>`.
///
using Vec2dArray = core::Array<Vec2d>;

/// Alias for `vgc::core::SharedConstArray<vgc::geometry::Vec2d>`.
///
using SharedConstVec2dArray = core::SharedConstArray<Vec2d>;

/// Allows to iterate over a range of `Vec2d` stored in a memory buffer of
/// doubles, where consecutive `Vec2d` elements are separated by a given stride.
///
/// ```cpp
/// FloatArray buffer = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
/// for (const Vec2& v : Vec2dSpan(buffer.data(), 2, 5)) {
///     std::cout << v << std::end;
/// }
/// // => prints "(1, 2)(6, 7)"
/// ```
///
using Vec2dSpan = StrideSpan<double, Vec2d>;

/// Const version of `Vec2dSpan`.
///
using Vec2dConstSpan = StrideSpan<const double, const Vec2d>;

/// Overloads `setZero(T& x)`.
///
/// \sa `vgc::core::zero<T>()`.
///
inline void setZero(Vec2d& v) {
    v[0] = 0.0;
    v[1] = 0.0;
}

/// Writes the given `Vec2d` to the output stream.
///
template<typename OStream>
void write(OStream& out, const Vec2d& v) {
    write(out, '(', v[0], ", ", v[1], ')');
}

/// Reads a `Vec2d` from the input stream, and stores it in the given output
/// parameter `v`. Leading whitespaces are allowed. Raises `ParseError` if the
/// stream does not start with a `Vec2d`. Raises `RangeError` if one of its
/// coordinates is outside the representable range of a double.
///
template <typename IStream>
void readTo(Vec2d& v, IStream& in) {
    skipWhitespaceCharacters(in);
    skipExpectedCharacter(in, '(');
    readTo(v[0], in);
    skipWhitespaceCharacters(in);
    skipExpectedCharacter(in, ',');
    readTo(v[1], in);
    skipWhitespaceCharacters(in);
    skipExpectedCharacter(in, ')');
}

} // namespace vgc::geometry

// see https://fmt.dev/latest/api.html#formatting-user-defined-types
template <>
struct fmt::formatter<vgc::geometry::Vec2d> {
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && *it != '}') {
            throw format_error("invalid format");
        }
        return it;
    }
    template <typename FormatContext>
    auto format(const vgc::geometry::Vec2d& v, FormatContext& ctx) {
        return format_to(ctx.out(),"({}, {})", v[0], v[1]);
    }
};

#endif // VGC_GEOMETRY_VEC2D_H
