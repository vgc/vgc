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
// Instead, edit ./tools/vec2.h, and re-run ./tools/vec2.py.

#ifndef VGC_CORE_VEC2F_H
#define VGC_CORE_VEC2F_H

#include <cmath>

#include <vgc/core/api.h>
#include <vgc/core/array.h>

namespace vgc::core {

/// \class vgc::core::Vec2f
/// \brief 2D vector using single-precision floating points.
///
/// A Vec2f represents either a 2D point (= position), a 2D vector (=
/// difference of positions), a 2D size (= positive position), or a 2D normal
/// (= unit vector). Unlike other libraries, we do not use separate types for
/// all these use cases.
///
/// The memory size of a Vec2f is exactly 2 * sizeof(float). This will never
/// change in any future version, as this allows to conveniently use this class
/// for data transfer to the GPU (via OpenGL, Metal, etc.).
///
// VGC_CORE_API <- Omitted on purpose.
//                 If needed, manually export individual functions.
class Vec2f
{
public:
    using value_type = float;

    /// Creates an uninitialized Vec2f.
    ///
    Vec2f() {}

    /// Creates a Vec2f initialized with the given arguments.
    ///
    constexpr Vec2f(float x, float y) : data_{x, y} {}

    /// Accesses the i-th component of the Vec2f.
    ///
    const float& operator[](Int i) const { return data_[i]; }

    /// Mutates the i-th component of the Vec2f.
    ///
    float& operator[](Int i) { return data_[i]; }

    /// Accesses the first component of the Vec2f.
    ///
    float x() const { return data_[0]; }

    /// Accesses the second component of the Vec2f.
    ///
    float y() const { return data_[1]; }

    /// Mutates the first component of the Vec2f.
    ///
    void setX(float x) { data_[0] = x; }

    /// Mutates the second component of the Vec2f.
    ///
    void setY(float y) { data_[1] = y; }

    /// Adds in-place the \p other Vec2f to this Vec2f.
    ///
    Vec2f& operator+=(const Vec2f& other) {
        data_[0] += other[0];
        data_[1] += other[1];
        return *this;
    }

    /// Returns the addition of the Vec2f \p v1 and the Vec2f \p v2.
    ///
    friend Vec2f operator+(const Vec2f& v1, const Vec2f& v2) {
        return Vec2f(v1) += v2;
    }

    /// Returns a copy of this Vec2f (unary plus operator).
    ///
    Vec2f operator+() const {
        return *this;
    }

    /// Substracts in-place the \p other Vec2f to this Vec2f.
    ///
    Vec2f& operator-=(const Vec2f& other) {
        data_[0] -= other[0];
        data_[1] -= other[1];
        return *this;
    }

    /// Returns the substraction of the Vec2f \p v1 and the Vec2f \p v2.
    ///
    friend Vec2f operator-(const Vec2f& v1, const Vec2f& v2) {
        return Vec2f(v1) -= v2;
    }

    /// Returns the opposite of this Vec2f (unary minus operator).
    ///
    Vec2f operator-() const {
        return Vec2f(-data_[0], -data_[1]);
    }

    /// Multiplies in-place this Vec2f by the given scalar \p s.
    ///
    Vec2f& operator*=(float s) {
        data_[0] *= s;
        data_[1] *= s;
        return *this;
    }

    /// Returns the multiplication of this Vec2f by the given scalar \p s.
    ///
    Vec2f operator*(float s) const {
        return Vec2f(*this) *= s;
    }

    /// Returns the multiplication of the scalar \p s with the Vec2f \p v.
    ///
    friend Vec2f operator*(float s, const Vec2f& v) {
        return v * s;
    }

    /// Divides in-place this Vec2f by the given scalar \p s.
    ///
    Vec2f& operator/=(float s) {
        data_[0] /= s;
        data_[1] /= s;
        return *this;
    }

    /// Returns the division of this Vec2f by the given scalar \p s.
    ///
    Vec2f operator/(float s) const {
        return Vec2f(*this) /= s;
    }

    /// Returns whether the two given Vec2f \p v1 and \p v2 are equal.
    ///
    friend bool operator==(const Vec2f& v1, const Vec2f& v2) {
        return v1.data_[0] == v2.data_[0] &&
               v1.data_[1] == v2.data_[1];
    }

    /// Returns whether the two given Vec2f \p v1 and \p v2 are different.
    ///
    friend bool operator!=(const Vec2f& v1, const Vec2f& v2) {
        return v1.data_[0] != v2.data_[0] ||
               v1.data_[1] != v2.data_[1];
    }

    /// Compares the two Vec2f \p v1 and \p v2 using the lexicographic
    /// order.
    ///
    friend bool operator<(const Vec2f& v1, const Vec2f& v2) {
        return ( (v1.data_[0] < v2.data_[0]) ||
               (!(v2.data_[0] < v1.data_[0]) &&
               ( (v1.data_[1] < v2.data_[1]))));
    }

    /// Compares the two Vec2f \p v1 and \p v2 using the lexicographic
    /// order.
    ///
    friend bool operator<=(const Vec2f& v1, const Vec2f& v2) {
        return !(v2 < v1);
    }

    /// Compares the two Vec2f \p v1 and \p v2 using the lexicographic
    /// order.
    ///
    friend bool operator>(const Vec2f& v1, const Vec2f& v2) {
        return v2 < v1;
    }

    /// Compares the two Vec2f \p v1 and \p v2 using the lexicographic
    /// order.
    ///
    friend bool operator>=(const Vec2f& v1, const Vec2f& v2) {
        return !(v1 < v2);
    }

    /// Returns the Euclidean length of the Vec2f.
    ///
    float length() const {
        return std::sqrt(squaredLength());
    }

    /// Returns the square of the Euclidean length of the Vec2f.
    ///
    /// This function is faster than length(), therefore it is a good idea to
    /// use it whenever you don't need the actual length. For example, if you
    /// need to know which vector has greater length, you can use
    /// v1.squaredLength() < v2.squaredLength().
    ///
    float squaredLength() const {
        return data_[0] * data_[0] + data_[1] * data_[1];
    }

    /// Makes this Vec2f a unit vector by dividing it by length().
    /// If length() < epsilon, this Vec2f is set to (1.0f, 0.0f).
    ///
    Vec2f& normalize() {
        float l = length();
        if (l > epsilon) { // XXX use zero instead of epsilon?
            *this /= l;
        }
        else {
            *this = Vec2f(1.0f, 0.0f);
        }
        return *this;
    }

    /// Returns a normalized copy of this Vec2f.
    ///
    Vec2f normalized() const {
        return Vec2f(*this).normalize();
    }

    /// Rotates this Vec2f by 90° counter-clockwise, assuming a left-handed
    /// coordinate system.
    ///
    Vec2f& orthogonalize() {
        float tmp = data_[0];
        data_[0] = - data_[1];
        data_[1] = tmp;
        return *this;
    }

    /// Returns a copy of this Vec2f rotated 90° counter-clockwise, assuming a
    /// left-handed coordinate system.
    ///
    Vec2f orthogonalized() const {
        return Vec2f(*this).orthogonalize();
    }

    /// Returns the dot product between this Vec2f `a` and the given Vec2f `b`.
    ///
    /// ```cpp
    /// float d = a.dot(b); // equivalent to a[0]*b[0] + a[1]*b[1]
    /// ```
    ///
    /// Note that this is also equal to `a.length() * b.length() * cos(a.angle(b))`.
    ///
    /// \sa det(), angle()
    ///
    float dot(const Vec2f& b) const {
        const Vec2f& a = *this;
        return a[0]*b[0] + a[1]*b[1];
    }

    /// Returns the "determinant" between this Vec2f `a` and the given Vec2f `b`.
    ///
    /// ```cpp
    /// float d = a.det(b); // equivalent to a[0]*b[1] - a[1]*b[0]
    /// ```
    ///
    /// Note that this is equal to:
    /// - `a.length() * b.length() * sin(a.angle(b))`
    /// - the (signed) area of the parallelogram spanned by `a` and `b`
    /// - the Z coordinate of the cross product between `a` and `b`, if `a` and `b`
    ///   are interpreted as 3D vectors with Z = 0.
    ///
    /// Note that `a.det(b)` has the same sign as `a.angle(b)`. See the
    /// documentation of Vec2f::angle() for more information on how to
    /// interpret this sign based on your choice of coordinate system (Y-axis
    /// pointing up or down).
    ///
    /// \sa dot(), angle()
    ///
    float det(const Vec2f& b) const {
        const Vec2f& a = *this;
        return a[0]*b[1] - a[1]*b[0];
    }

    /// Returns the angle, in radians and in the interval (−π, π], between this
    /// Vec2f `a` and the given Vec2f `b`.
    ///
    /// ```cpp
    /// Vec2f a(1, 0);
    /// Vec2f b(1, 1);
    /// float d = a.angle(b); // returns 0.7853981633974483 (approx. π/4 rad = 45 deg)
    /// ```
    ///
    /// This value is computed using the following formula:
    ///
    /// ```cpp
    /// float angle = atan2(a.det(b), a.dot(b));
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
    float angle(const Vec2f& b) const {
        const Vec2f& a = *this;
        return std::atan2(a.det(b), a.dot(b));
    }

    /// Returns whether this Vec2f `a` and the given Vec2f `b` are almost equal
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
    /// float inf = std::numeric_limits<float>::infinity();
    /// Vec2f(inf, inf).isClose(Vec2f(inf, inf));  // true
    /// Vec2f(inf, inf).isClose(Vec2f(inf, -inf)); // false
    /// ```
    ///
    /// If some coordinates are infinite and some others are finite, the
    /// behavior can in some cases be surprising, but actually makes sense:
    ///
    /// ```cpp
    /// Vec2f(inf, inf).isClose(Vec2f(inf, 42)); // false
    /// Vec2f(inf, 42).isClose(Vec2f(inf, 42));  // true
    /// Vec2f(inf, 42).isClose(Vec2f(inf, 43));  // true (yes!)
    /// ```
    ///
    /// Notice how the last one returns true even though `isClose(42, 43)`
    /// returns false. This is because for a sufficiently large x, the distance
    /// between `Vec2f(x, 42)` and `Vec2f(x, 43)`, which is always equal to 1,
    /// is indeed negligible compared to their respective length, which
    /// approaches infinity as x approaches infinity. For example, the
    /// following also returns true:
    ///
    /// ```cpp
    /// Vec2f(1e20f, 42).isClose(Vec2f(1e20f, 43)); // true
    /// ```
    ///
    /// Note that `allClose()` returns false in these cases, which may or may
    /// not be what you need depending on your situation. In case of doubt,
    /// `isClose` is typically the better choice.
    ///
    /// ```cpp
    /// Vec2f(inf, 42).allClose(Vec2f(inf, 43));   // false
    /// Vec2f(1e20f, 42).allClose(Vec2f(1e20f, 43)); // false
    /// ```
    ///
    bool isClose(const Vec2f& b, float relTol = 1e-5f, float absTol = 0.0f) const
    {
        const Vec2f& a = *this;
        float diff2 = a.infdiff_(b).squaredLength();
        if (diff2 == std::numeric_limits<float>::infinity()) {
            return false; // opposite infinities or finite/infinite mismatch
        }
        else {
            float relTol2 = relTol * relTol;
            float absTol2 = absTol * absTol;
            return diff2 <= relTol2 * b.squaredLength() ||
                   diff2 <= relTol2 * a.squaredLength() ||
                   diff2 <= absTol2;
        }
    }

    /// Returns whether all coordinates in this Vec2f `a` are almost equal to
    /// their corresponding coordinate in the given Vec2f `b`, within some
    /// relative tolerance. this function is equivalent to:
    ///
    /// ```cpp
    /// isClose(a[0], b[0], relTol, absTol) && isClose(a[1], b[1], relTol, absTol)
    /// ```
    ///
    /// This is similar to `a.isClose(b)`, but completely decorellates the X
    /// and Y coordinates, which may be preferrable if the two given Vec2f do
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
    /// vgc::core::Vec2f a(1.0f, 0.0f);
    /// vgc::core::Vec2f b(1.0f, 1e-10f);
    /// a.isClose(b);  // true because (b-a).length() <= relTol * a.length()
    /// a.allClose(b); // false because isClose(0.0f, 1e-10f) == false
    /// ```
    ///
    /// In other words, `a` and `b` are relatively close to each others as 2D
    /// points, even though their Y coordinates are not relatively close to
    /// each others.
    ///
    bool allClose(const Vec2f& b, float relTol = 1e-5f, float absTol = 0.0f) const
    {
        const Vec2f& a = *this;
        return core::isClose(a[0], b[0], relTol, absTol) &&
               core::isClose(a[1], b[1], relTol, absTol);
    }

    /// Returns whether the euclidean distance between this Vec2f `a` and the
    /// given Vec2f `b` is smaller or equal than the given absolute tolerance.
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
    /// float inf = std::numeric_limits<float>::infinity();
    /// float absTol = 0.5f;
    /// Vec2f(inf, inf).isNear(Vec2f(inf, inf), absTol);  // true
    /// Vec2f(inf, inf).isNear(Vec2f(inf, -inf), absTol); // false
    /// Vec2f(inf, inf).isNear(Vec2f(inf, 42), absTol);   // false
    /// Vec2f(inf, 42).isNear(Vec2f(inf, 42), absTol);    // true
    /// Vec2f(inf, 42).isNear(Vec2f(inf, 42.4f), absTol);  // true
    /// Vec2f(inf, 42).isNear(Vec2f(inf, 42.6f), absTol);  // false
    /// ```
    ///
    bool isNear(const Vec2f& b, float absTol) const
    {
        const Vec2f& a = *this;
        float diff2 = a.infdiff_(b).squaredLength();
        if (diff2 == std::numeric_limits<float>::infinity()) {
            return false; // opposite infinities or finite/infinite mismatch
        }
        else {
            float absTol2 = absTol * absTol;
            return diff2 <= absTol2;
        }
    }

    /// Returns whether all coordinates in this Vec2f `a` are within some
    /// absolute tolerance of their corresponding coordinate in the given Vec2f
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
    /// Y coordinates, which may be preferrable if the two given Vec2f do not
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
    bool allNear(const Vec2f& b, float absTol) const
    {
        const Vec2f& a = *this;
        return core::isNear(a[0], b[0], absTol) &&
               core::isNear(a[1], b[1], absTol);
    }

private:
    float data_[2];

    Vec2f infdiff_(const Vec2f& b) const
    {
        const Vec2f& a = *this;
        return Vec2f(internal::infdiff(a[0], b[0]),
                     internal::infdiff(a[1], b[1]));
    }
};

/// Alias for vgc::core::Array<vgc::core::Vec2f>.
///
using Vec2fArray = Array<Vec2f>;

/// Overloads setZero(T& x).
///
/// \sa vgc::core::zero<T>()
///
inline void setZero(Vec2f& v)
{
    v[0] = 0.0f;
    v[1] = 0.0f;
}

/// Writes the given Vec2f to the output stream.
///
template<typename OStream>
void write(OStream& out, const Vec2f& v)
{
    write(out, '(', v[0], ", ", v[1], ')');
}

/// Reads a Vec2f from the input stream, and stores it in the given output
/// parameter. Leading whitespaces are allowed. Raises ParseError if the stream
/// does not start with a Vec2f. Raises RangeError if one of its coordinate is
/// outside the representable range of a float.
///
template <typename IStream>
void readTo(Vec2f& v, IStream& in)
{
    skipWhitespaceCharacters(in);
    skipExpectedCharacter(in, '(');
    readTo(v[0], in);
    skipWhitespaceCharacters(in);
    skipExpectedCharacter(in, ',');
    readTo(v[1], in);
    skipWhitespaceCharacters(in);
    skipExpectedCharacter(in, ')');
}

} // namespace vgc::core

// see https://fmt.dev/latest/api.html#formatting-user-defined-types
template <>
struct fmt::formatter<vgc::core::Vec2f> {
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && *it != '}')
            throw format_error("invalid format");
        return it;
    }
    template <typename FormatContext>
    auto format(const vgc::core::Vec2f v, FormatContext& ctx) {
        return format_to(ctx.out(),"({}, {})", v[0], v[1]);
    }
};

#endif // VGC_CORE_VEC2F_H
