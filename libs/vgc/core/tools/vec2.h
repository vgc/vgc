// Copyright 2020 The VGC Developers
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

// This file is a "template" (not in the C++ sense) used to generate all the
// Vec2 variants. Please refer to vec2.py for more info.

#ifndef VGC_CORE_VEC2X_H
#define VGC_CORE_VEC2X_H

#include <cmath>
#include <string>

#include <vgc/core/api.h>
#include <vgc/core/arithmetic.h>
#include <vgc/core/format.h>
#include <vgc/core/parse.h>

namespace vgc {
namespace core {

/// \class vgc::core::Vec2x
/// \brief 2D vector using %SCALAR_DESCRIPTION%.
///
/// A Vec2x represents either a 2D point (= position), a 2D vector (=
/// difference of positions), a 2D size (= positive position), or a 2D normal
/// (= unit vector). Unlike other libraries, we do not use separate types for
/// all these use cases.
///
/// The memory size a Vec2x is exactly 2 * sizeof(float). This will never
/// change in any future version, as this allows to conveniently use this class
/// for data transfer to the GPU (via OpenGL, Metal, etc.).
///
class VGC_CORE_API Vec2x
{
public:
    using value_type = float;

    /// Creates an uninitialized Vec2x.
    ///
    Vec2x() {}

    /// Creates a Vec2x initialized with the given arguments.
    ///
    Vec2x(float x, float y) : data_{x, y} {}

    /// Accesses the i-th component of the Vec2x.
    ///
    const float& operator[](int i) const { return data_[i]; }

    /// Mutates the i-th component of the Vec2x.
    ///
    float& operator[](int i) { return data_[i]; }

    /// Accesses the first component of the Vec2x.
    ///
    float x() const { return data_[0]; }

    /// Accesses the second component of the Vec2x.
    ///
    float y() const { return data_[1]; }

    /// Mutates the first component of the Vec2x.
    ///
    void setX(float x) { data_[0] = x; }

    /// Mutates the second component of the Vec2x.
    ///
    void setY(float y) { data_[1] = y; }

    /// Adds in-place the \p other Vec2x to this Vec2x.
    ///
    Vec2x& operator+=(const Vec2x& other) {
        data_[0] += other[0];
        data_[1] += other[1];
        return *this;
    }

    /// Returns the addition of the Vec2x \p v1 and the Vec2x \p v2.
    ///
    friend Vec2x operator+(const Vec2x& v1, const Vec2x& v2) {
        return Vec2x(v1) += v2;
    }

    /// Returns a copy of this Vec2x (unary plus operator).
    ///
    Vec2x operator+() const {
        return *this;
    }

    /// Substracts in-place the \p other Vec2x to this Vec2x.
    ///
    Vec2x& operator-=(const Vec2x& other) {
        data_[0] -= other[0];
        data_[1] -= other[1];
        return *this;
    }

    /// Returns the substraction of the Vec2x \p v1 and the Vec2x \p v2.
    ///
    friend Vec2x operator-(const Vec2x& v1, const Vec2x& v2) {
        return Vec2x(v1) -= v2;
    }

    /// Returns the opposite of this Vec2x (unary minus operator).
    ///
    Vec2x operator-() const {
        return Vec2x(-data_[0], -data_[1]);
    }

    /// Multiplies in-place this Vec2x by the given scalar \p s.
    ///
    Vec2x& operator*=(float s) {
        data_[0] *= s;
        data_[1] *= s;
        return *this;
    }

    /// Returns the multiplication of this Vec2x by the given scalar \p s.
    ///
    Vec2x operator*(float s) const {
        return Vec2x(*this) *= s;
    }

    /// Returns the multiplication of the scalar \p s with the Vec2x \p v.
    ///
    friend Vec2x operator*(float s, const Vec2x& v) {
        return v * s;
    }

    /// Divides in-place this Vec2x by the given scalar \p s.
    ///
    Vec2x& operator/=(float s) {
        data_[0] /= s;
        data_[1] /= s;
        return *this;
    }

    /// Returns the division of this Vec2x by the given scalar \p s.
    ///
    Vec2x operator/(float s) const {
        return Vec2x(*this) /= s;
    }

    /// Returns whether the two given Vec2x \p v1 and \p v2 are equal.
    ///
    friend bool operator==(const Vec2x& v1, const Vec2x& v2) {
        return v1.data_[0] == v2.data_[0] &&
               v1.data_[1] == v2.data_[1];
    }

    /// Returns whether the two given Vec2x \p v1 and \p v2 are different.
    ///
    friend bool operator!=(const Vec2x& v1, const Vec2x& v2) {
        return v1.data_[0] != v2.data_[0] ||
               v1.data_[1] != v2.data_[1];
    }

    /// Compares the two Vec2x \p v1 and \p v2 using the lexicographic
    /// order.
    ///
    friend bool operator<(const Vec2x& v1, const Vec2x& v2) {
        return ( (v1.data_[0] < v2.data_[0]) ||
               (!(v2.data_[0] < v1.data_[0]) &&
               ( (v1.data_[1] < v2.data_[1]))));
    }

    /// Compares the two Vec2x \p v1 and \p v2 using the lexicographic
    /// order.
    ///
    friend bool operator<=(const Vec2x& v1, const Vec2x& v2) {
        return !(v2 < v1);
    }

    /// Compares the two Vec2x \p v1 and \p v2 using the lexicographic
    /// order.
    ///
    friend bool operator>(const Vec2x& v1, const Vec2x& v2) {
        return v2 < v1;
    }

    /// Compares the two Vec2x \p v1 and \p v2 using the lexicographic
    /// order.
    ///
    friend bool operator>=(const Vec2x& v1, const Vec2x& v2) {
        return !(v1 < v2);
    }

    /// Returns the Euclidean length of the Vec2x.
    ///
    float length() const {
        return std::sqrt(squaredLength());
    }

    /// Returns the square of the Euclidean length of the Vec2x.
    ///
    /// This function is faster than length(), therefore it is a good idea to
    /// use it whenever you don't need the actual length. For example, if you
    /// need to know which vector has greater length, you can use
    /// v1.squaredLength() < v2.squaredLength().
    ///
    float squaredLength() const {
        return data_[0] * data_[0] + data_[1] * data_[1];
    }

    /// Makes this Vec2x a unit vector by dividing it by length().
    /// If length() < epsilon, this Vec2x is set to (1.0f, 0.0f).
    ///
    Vec2x& normalize() {
        float l = length();
        if (l > epsilon) { // XXX use zero instead of epsilon?
            *this /= l;
        }
        else {
            *this = Vec2x(1.0f, 0.0f);
        }
        return *this;
    }

    /// Returns a normalized copy of this Vec2x.
    ///
    Vec2x normalized() const {
        return Vec2x(*this).normalize();
    }

    /// Rotates this Vec2x by 90° counter-clockwise, assuming a left-handed
    /// coordinate system.
    ///
    Vec2x& orthogonalize() {
        float tmp = data_[0];
        data_[0] = - data_[1];
        data_[1] = tmp;
        return *this;
    }

    /// Returns a copy of this Vec2x rotated 90° counter-clockwise, assuming a
    /// left-handed coordinate system.
    ///
    Vec2x orthogonalized() const {
        return Vec2x(*this).orthogonalize();
    }

    /// Returns the dot product between this Vec2x `a` and the given Vec2x `b`.
    ///
    /// ```cpp
    /// float d = a.dot(b); // equivalent to a[0]*b[0] + a[1]*b[1]
    /// ```
    ///
    float dot(const Vec2x& b) {
        const Vec2x& a = *this;
        return a[0]*b[0] + a[1]*b[1];
    }

    /// Returns whether this Vec2x `a` and the given Vec2x `b` are almost equal
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
    /// Vec2x(inf, inf).isClose(Vec2x(inf, inf));  // true
    /// Vec2x(inf, inf).isClose(Vec2x(inf, -inf)); // false
    /// ```
    ///
    /// If some coordinates are infinite and some others are finite, the
    /// behavior can in some cases be surprising, but actually makes sense:
    ///
    /// ```cpp
    /// Vec2x(inf, inf).isClose(Vec2x(inf, 42)); // false
    /// Vec2x(inf, 42).isClose(Vec2x(inf, 42));  // true
    /// Vec2x(inf, 42).isClose(Vec2x(inf, 43));  // true (yes!)
    /// ```
    ///
    /// Notice how the last one returns true even though `isClose(42, 43)`
    /// returns false. This is because for a sufficiently large x, the distance
    /// between `Vec2x(x, 42)` and `Vec2x(x, 43)`, which is always equal to 1,
    /// is indeed negligible compared to their respective length, which
    /// approaches infinity as x approaches infinity. For example, the
    /// following also returns true:
    ///
    /// ```cpp
    /// Vec2x(1e20f, 42).isClose(Vec2x(1e20f, 43)); // true
    /// ```
    ///
    /// Note that `allClose()` returns false in these cases, which may or may
    /// not be what you need depending on your situation. In case of doubt,
    /// `isClose` is typically the better choice.
    ///
    /// ```cpp
    /// Vec2x(inf, 42).allClose(Vec2x(inf, 43));   // false
    /// Vec2x(1e20f, 42).allClose(Vec2x(1e20f, 43)); // false
    /// ```
    ///
    bool isClose(const Vec2x& b, float relTol = 1e-5f, float absTol = 0.0f) const
    {
        const Vec2x& a = *this;
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

    /// Returns whether all coordinates in this Vec2x `a` are almost equal to
    /// their corresponding coordinate in the given Vec2x `b`, within some
    /// relative tolerance. this function is equivalent to:
    ///
    /// ```cpp
    /// isClose(a[0], b[0], relTol, absTol) && isClose(a[1], b[1], relTol, absTol)
    /// ```
    ///
    /// This is similar to `a.isClose(b)`, but completely decorellates the X
    /// and Y coordinates, which may be preferrable if the two given Vec2x do
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
    /// vgc::core::Vec2x a(1.0f, 0.0f);
    /// vgc::core::Vec2x b(1.0f, 1e-10f);
    /// a.isClose(b);  // true because (b-a).length() <= relTol * a.length()
    /// a.allClose(b); // false because isClose(0.0f, 1e-10f) == false
    /// ```
    ///
    /// In other words, `a` and `b` are relatively close to each others as 2D
    /// points, even though their Y coordinates are not relatively close to
    /// each others.
    ///
    bool allClose(const Vec2x& b, float relTol = 1e-5f, float absTol = 0.0f) const
    {
        const Vec2x& a = *this;
        return core::isClose(a[0], b[0], relTol, absTol) &&
               core::isClose(a[1], b[1], relTol, absTol);
    }

    /// Returns whether the euclidean distance between this Vec2x `a` and the
    /// given Vec2x `b` is smaller or equal than the given absolute tolerance.
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
    /// Vec2x(inf, inf).isNear(Vec2x(inf, inf), absTol);  // true
    /// Vec2x(inf, inf).isNear(Vec2x(inf, -inf), absTol); // false
    /// Vec2x(inf, inf).isNear(Vec2x(inf, 42), absTol);   // false
    /// Vec2x(inf, 42).isNear(Vec2x(inf, 42), absTol);    // true
    /// Vec2x(inf, 42).isNear(Vec2x(inf, 42.4f), absTol);  // true
    /// Vec2x(inf, 42).isNear(Vec2x(inf, 42.6f), absTol);  // false
    /// ```
    ///
    bool isNear(const Vec2x& b, float absTol) const
    {
        const Vec2x& a = *this;
        float diff2 = a.infdiff_(b).squaredLength();
        if (diff2 == std::numeric_limits<float>::infinity()) {
            return false; // opposite infinities or finite/infinite mismatch
        }
        else {
            float absTol2 = absTol * absTol;
            return diff2 <= absTol2;
        }
    }

    /// Returns whether all coordinates in this Vec2x `a` are within some
    /// absolute tolerance of their corresponding coordinate in the given Vec2x
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
    /// Y coordinates, which may be preferrable if the two given Vec2x do not
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
    bool allNear(const Vec2x& b, float absTol) const
    {
        const Vec2x& a = *this;
        return core::isNear(a[0], b[0], absTol) &&
               core::isNear(a[1], b[1], absTol);
    }

private:
    float data_[2];

    Vec2x infdiff_(const Vec2x& b) const
    {
        const Vec2x& a = *this;
        return Vec2x(internal::infdiff(a[0], b[0]),
                     internal::infdiff(a[1], b[1]));
    }
};

/// Overloads setZero(T& x).
///
/// \sa vgc::core::zero<T>()
///
inline void setZero(Vec2x& v)
{
    v[0] = 0.0f;
    v[1] = 0.0f;
}

/// Writes the given Vec2x to the output stream.
///
template<typename OStream>
void write(OStream& out, const Vec2x& v)
{
    write(out, '(', v[0], ", ", v[1], ')');
}

/// Reads a Vec2x from the input stream, and stores it in the given output
/// parameter. Leading whitespaces are allowed. Raises ParseError if the stream
/// does not start with a Vec2x. Raises RangeError if one of its coordinate is
/// outside the representable range of a float.
///
template <typename IStream>
void readTo(Vec2x& v, IStream& in)
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

} // namespace core
} // namespace vgc

#endif // VGC_CORE_VEC2X_H
