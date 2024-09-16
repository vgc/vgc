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

#ifndef VGC_GEOMETRY_MAT3_H
#define VGC_GEOMETRY_MAT3_H

#include <cmath>

#include <vgc/core/array.h>
#include <vgc/geometry/api.h>
#include <vgc/geometry/mat.h>
#include <vgc/geometry/mat2.h>
#include <vgc/geometry/stride.h>
#include <vgc/geometry/vec2.h>
#include <vgc/geometry/vec3.h>

namespace vgc::geometry {

/// \class vgc::geometry::Mat3
/// \brief Represents a 3x3 matrix.
///
/// A `Mat3<T>` represents a 3x3 matrix in column-major format.
///
/// The memory size of a `Mat3<T>` is exactly `9 * sizeof(T)`. This will never
/// change in any future version, as this allows to conveniently use this class
/// for data transfer to the GPU (via OpenGL, Metal, etc.).
///
/// Unlike some libraries, VGC has chosen not to distinguish between 3x3
/// matrices and 2D affine transformations in homogeneous coordinates. In other
/// words, if you wish to represent a 2D affine transformation, simply use a
/// `Mat3<T>`.
///
template<typename T>
class Mat3 {
public:
    using ScalarType = T;
    static constexpr Int dimension = 3;

    VGC_WARNING_PUSH
    VGC_WARNING_MSVC_DISABLE(26495) // member variable uninitialized
    /// Creates an uninitialized `Mat3`.
    ///
    Mat3(core::NoInit) noexcept {
    }
    VGC_WARNING_POP

    /// Creates a `Mat3` initialized to the null matrix `Mat3(0)`.
    ///
    constexpr Mat3() noexcept
        : Mat3(0) {
    }

    /// Creates a `Mat3` initialized with the given arguments.
    ///
    // clang-format off
    constexpr Mat3(
        T m00, T m01, T m02,
        T m10, T m11, T m12,
        T m20, T m21, T m22) noexcept

        : data_{{m00, m10, m20},
                {m01, m11, m21},
                {m02, m12, m22}} {
    }
    // clang-format on

    /// Creates a `Mat3` initialized with the given row vectors.
    ///
    constexpr Mat3(const Vec3<T>& v0, const Vec3<T>& v1, const Vec3<T>& v2) noexcept
        : data_{
            {v0[0], v1[0], v2[0]}, //
            {v0[1], v1[1], v2[1]},
            {v0[2], v1[2], v2[2]}} {
    }

    /// Creates a diagonal matrix with diagonal elements equal to the given
    /// value. As specific cases, the null matrix is ` Mat3(0)`, and the
    /// identity matrix is `Mat3(1)`.
    ///
    constexpr explicit Mat3(T d) noexcept
        : data_{
            {d, 0, 0}, //
            {0, d, 0},
            {0, 0, d}} {
    }

    /// Creates a `Mat3` from another `Mat<3, T>` object by performing a
    /// `static_cast` on each of its elements.
    ///
    template<typename U, VGC_REQUIRES(!std::is_same_v<T, U>)>
    constexpr explicit Mat3(const Mat3<U>& other) noexcept
        : data_{
            {static_cast<T>(other(0, 0)),
             static_cast<T>(other(1, 0)),
             static_cast<T>(other(2, 0))},
            {static_cast<T>(other(0, 1)),
             static_cast<T>(other(1, 1)),
             static_cast<T>(other(2, 1))},
            {static_cast<T>(other(0, 2)),
             static_cast<T>(other(1, 2)),
             static_cast<T>(other(2, 2))}} {
    }

    /// Creates a `Mat3<T>` from a `Mat2<U>` matrix, assuming the given
    /// matrix represents a 2D linear transformation.
    ///
    /// ```text
    /// |a b|    |a b 0|
    /// |c d| -> |c d 0|
    ///          |0 0 1|
    /// ```
    ///
    /// The returned matrix can be used either as a 3D linear transformation
    /// that doesn't modify the third coordinate, or as a 2D homogeneous
    /// transformation that also happens to be a linear transformation (no
    /// translation or projective components).
    ///
    /// If the given matrix is not a `Mat2<T>`, then each of its elements is
    /// converted to `T` by performing a `static_cast`.
    ///
    template<typename U>
    static constexpr Mat3 fromLinear(const Mat2<U>& other) noexcept {
        T a = static_cast<T>(other(0, 0));
        T b = static_cast<T>(other(0, 1));
        T c = static_cast<T>(other(1, 0));
        T d = static_cast<T>(other(1, 1));
        // clang-format off
        return Mat3(a, b, 0,
                    c, d, 0,
                    0, 0, 1);
        // clang-format on
    }

    /// Creates a `Mat3<T>` from a `Mat2<U>` matrix, assuming the given matrix
    /// represents a 1D homogeneous transformation (possibly affine or
    /// projective).
    ///
    /// ```text
    /// |a b|    |a 0 b|
    /// |c d| -> |0 1 0|
    ///          |c 0 d|
    /// ``
    ///
    /// The returned matrix can be used as a 2D homogeneous transformation
    /// whose linear part does not modify the second coordinate.
    ///
    /// If the given matrix is not a `Mat2<T>`, then each of its elements is
    /// converted to `T` by performing a `static_cast`.
    ///
    template<typename U>
    static constexpr Mat3 fromTransform(const Mat2<U>& other) noexcept {
        T a = static_cast<T>(other(0, 0));
        T b = static_cast<T>(other(0, 1));
        T c = static_cast<T>(other(1, 0));
        T d = static_cast<T>(other(1, 1));
        // clang-format off
        return Mat3(a, 0, b,
                    0, 1, 0,
                    c, 0, d);
        // clang-format on
    }

    /// Modifies all the elements of this matrix.
    ///
    // clang-format off
    Mat3& setElements(
        T m00, T m01, T m02,
        T m10, T m11, T m12,
        T m20, T m21, T m22) {

        data_[0][0] = m00; data_[0][1] = m10; data_[0][2] = m20;
        data_[1][0] = m01; data_[1][1] = m11; data_[1][2] = m21;
        data_[2][0] = m02; data_[2][1] = m12; data_[2][2] = m22;
        return *this;
    }
    // clang-format on

    /// Sets this matrix to a diagonal matrix with all diagonal elements equal
    /// to the given value.
    ///
    Mat3& setToDiagonal(T d) {
        // clang-format off
        return setElements(
            d, 0, 0,
            0, d, 0,
            0, 0, d);
        // clang-format on
    }

    /// Sets this matrix to the zero matrix.
    ///
    Mat3& setToZero() {
        return setToDiagonal(0);
    }

    /// Sets this matrix to the identity matrix.
    ///
    Mat3& setToIdentity() {
        return setToDiagonal(1);
    }

    /// The identity matrix `Mat3(1)`.
    ///
    static const Mat3 identity;

    /// Returns a pointer to the underlying (colum-major ordered) array of
    /// components.
    ///
    const T* data() const {
        return data_[0];
    }

    /// Returns a pointer to the underlying (colum-major ordered) array of
    /// components.
    ///
    T* data() {
        return data_[0];
    }

    /// Accesses the component of the matrix at the `i`-th row and `j`-th
    /// column.
    ///
    const T& operator()(Int i, Int j) const {
        return data_[j][i];
    }

    /// Mutates the component of the matrix at the `i`-th row and `j`-th column.
    ///
    T& operator()(Int i, Int j) {
        return data_[j][i];
    }

    /// Adds in-place the `other` matrix to this matrix.
    ///
    Mat3& operator+=(const Mat3& other) {
        data_[0][0] += other.data_[0][0];
        data_[0][1] += other.data_[0][1];
        data_[0][2] += other.data_[0][2];
        data_[1][0] += other.data_[1][0];
        data_[1][1] += other.data_[1][1];
        data_[1][2] += other.data_[1][2];
        data_[2][0] += other.data_[2][0];
        data_[2][1] += other.data_[2][1];
        data_[2][2] += other.data_[2][2];
        return *this;
    }

    /// Returns the addition of the matrix `m1` and the matrix `m2`.
    ///
    friend Mat3 operator+(const Mat3& m1, const Mat3& m2) {
        return Mat3(m1) += m2;
    }

    /// Returns a copy of this matrix (unary plus operator).
    ///
    Mat3 operator+() const {
        return *this;
    }

    /// Substracts in-place the `other` matrix from this matrix.
    ///
    Mat3& operator-=(const Mat3& other) {
        data_[0][0] -= other.data_[0][0];
        data_[0][1] -= other.data_[0][1];
        data_[0][2] -= other.data_[0][2];
        data_[1][0] -= other.data_[1][0];
        data_[1][1] -= other.data_[1][1];
        data_[1][2] -= other.data_[1][2];
        data_[2][0] -= other.data_[2][0];
        data_[2][1] -= other.data_[2][1];
        data_[2][2] -= other.data_[2][2];
        return *this;
    }

    /// Returns the substraction of the matrix `m1` and the matrix `m2`.
    ///
    friend Mat3 operator-(const Mat3& m1, const Mat3& m2) {
        return Mat3(m1) -= m2;
    }

    /// Returns the opposite of this matrix (unary minus operator).
    ///
    Mat3 operator-() const {
        return Mat3(*this) *= -1;
    }

    /// Multiplies in-place this matrix with the `other` matrix.
    ///
    Mat3& operator*=(const Mat3& other) {
        *this = (*this) * other;
        return *this;
    }

    /// Returns the multiplication of the matrix `m1` and the matrix `m2`.
    ///
    friend Mat3 operator*(const Mat3& m1, const Mat3& m2) {
        Mat3 res;
        res(0, 0) = m1(0, 0) * m2(0, 0) + m1(0, 1) * m2(1, 0) + m1(0, 2) * m2(2, 0);
        res(0, 1) = m1(0, 0) * m2(0, 1) + m1(0, 1) * m2(1, 1) + m1(0, 2) * m2(2, 1);
        res(0, 2) = m1(0, 0) * m2(0, 2) + m1(0, 1) * m2(1, 2) + m1(0, 2) * m2(2, 2);
        res(1, 0) = m1(1, 0) * m2(0, 0) + m1(1, 1) * m2(1, 0) + m1(1, 2) * m2(2, 0);
        res(1, 1) = m1(1, 0) * m2(0, 1) + m1(1, 1) * m2(1, 1) + m1(1, 2) * m2(2, 1);
        res(1, 2) = m1(1, 0) * m2(0, 2) + m1(1, 1) * m2(1, 2) + m1(1, 2) * m2(2, 2);
        res(2, 0) = m1(2, 0) * m2(0, 0) + m1(2, 1) * m2(1, 0) + m1(2, 2) * m2(2, 0);
        res(2, 1) = m1(2, 0) * m2(0, 1) + m1(2, 1) * m2(1, 1) + m1(2, 2) * m2(2, 1);
        res(2, 2) = m1(2, 0) * m2(0, 2) + m1(2, 1) * m2(1, 2) + m1(2, 2) * m2(2, 2);
        return res;
    }

    /// Multiplies in-place this matrix by the given scalar `s`.
    ///
    Mat3& operator*=(T s) {
        data_[0][0] *= s;
        data_[0][1] *= s;
        data_[0][2] *= s;
        data_[1][0] *= s;
        data_[1][1] *= s;
        data_[1][2] *= s;
        data_[2][0] *= s;
        data_[2][1] *= s;
        data_[2][2] *= s;
        return *this;
    }

    /// Returns the multiplication of this matrix by the given scalar `s`.
    ///
    Mat3 operator*(T s) const {
        return Mat3(*this) *= s;
    }

    /// Returns the multiplication of the scalar `s` with the matrix `m`.
    ///
    friend Mat3 operator*(T s, const Mat3& m) {
        return m * s;
    }

    /// Divides in-place this matrix by the given scalar `s`.
    ///
    Mat3& operator/=(T s) {
        data_[0][0] /= s;
        data_[0][1] /= s;
        data_[0][2] /= s;
        data_[1][0] /= s;
        data_[1][1] /= s;
        data_[1][2] /= s;
        data_[2][0] /= s;
        data_[2][1] /= s;
        data_[2][2] /= s;
        return *this;
    }

    /// Returns the division of this matrix by the given scalar `s`.
    ///
    Mat3 operator/(T s) const {
        return Mat3(*this) /= s;
    }

    /// Returns whether the two given matrices `m1` and `m2` are equal.
    ///
    friend bool operator==(const Mat3& m1, const Mat3& m2) {
        return m1.data_[0][0] == m2.data_[0][0] && m1.data_[0][1] == m2.data_[0][1]
               && m1.data_[0][2] == m2.data_[0][2] && m1.data_[1][0] == m2.data_[1][0]
               && m1.data_[1][1] == m2.data_[1][1] && m1.data_[1][2] == m2.data_[1][2]
               && m1.data_[2][0] == m2.data_[2][0] && m1.data_[2][1] == m2.data_[2][1]
               && m1.data_[2][2] == m2.data_[2][2];
    }

    /// Returns whether the two given matrices `m1` and `m2` are different.
    ///
    friend bool operator!=(const Mat3& m1, const Mat3& m2) {
        return m1.data_[0][0] != m2.data_[0][0] || m1.data_[0][1] != m2.data_[0][1]
               || m1.data_[0][2] != m2.data_[0][2] || m1.data_[1][0] != m2.data_[1][0]
               || m1.data_[1][1] != m2.data_[1][1] || m1.data_[1][2] != m2.data_[1][2]
               || m1.data_[2][0] != m2.data_[2][0] || m1.data_[2][1] != m2.data_[2][1]
               || m1.data_[2][2] != m2.data_[2][2];
    }

    /// Returns the multiplication of this `Mat3` by the given `Vec3`.
    ///
    Vec3<T> operator*(const Vec3<T>& v) const {
        return Vec3<T>(
            data_[0][0] * v[0] + data_[1][0] * v[1] + data_[2][0] * v[2],
            data_[0][1] * v[0] + data_[1][1] * v[1] + data_[2][1] * v[2],
            data_[0][2] * v[0] + data_[1][2] * v[1] + data_[2][2] * v[2]);
    }

    /// Returns the result of transforming the given `Vec2` by this `Mat3`
    /// interpreted as a 2D projective transformation.
    ///
    /// This is equivalent to multiplying this `Mat3` by `Vec3(x, y, 1)`,
    /// then returning the first two coordinates divided by the third
    /// coordinate.
    ///
    Vec2<T> transform(const Vec2<T>& v) const {
        T x = data_[0][0] * v[0] + data_[1][0] * v[1] + data_[2][0];
        T y = data_[0][1] * v[0] + data_[1][1] * v[1] + data_[2][1];
        T w = data_[0][2] * v[0] + data_[1][2] * v[1] + data_[2][2];
        T iw = T(1) / w;
        return Vec2<T>(iw * x, iw * y);
    }

    /// Returns the result of transforming the given `Vec2` by this `Mat3`
    /// interpreted as a 2D affine transformation, that is, ignoring the
    /// projective components.
    ///
    /// This is equivalent to multiplying the 2x3 submatrix of this `Mat3` by
    /// `Vec3(x, y, 1)`.
    ///
    /// This can be used as a faster version of `transform()` whenever you
    /// know that the last row of the matrix is equal to `[0, 0, 1]`, or
    /// whenever you prefer to behave as if the last row was `[0, 0, 1]`.
    ///
    Vec2<T> transformAffine(const Vec2<T>& v) const {
        T x = data_[0][0] * v[0] + data_[1][0] * v[1] + data_[2][0];
        T y = data_[0][1] * v[0] + data_[1][1] * v[1] + data_[2][1];
        return Vec2<T>(x, y);
    }

    /// Returns the result of transforming the given `Vec2` by the linear part
    /// of this `Mat3` interpreted as a 2D projective transformation. That is,
    /// this ignores the translation and projective components, and only
    /// applies the rotation, scale, and shear components.
    ///
    /// This is equivalent to multiplying this matrix by `Vec3(x, y, 0)` and
    /// returning only the first two components.
    ///
    /// This is typically used to transform "directions" rather than "points".
    ///
    Vec2<T> transformLinear(const Vec2<T>& v) const {
        T x = data_[0][0] * v[0] + data_[1][0] * v[1];
        T y = data_[0][1] * v[0] + data_[1][1] * v[1];
        return Vec2<T>(x, y);
    }

    /// Returns the inverse of this matrix.
    ///
    /// If provided, `isInvertible` is set to either `true` or `false`
    /// depending on whether the matrix was considered invertible or not.
    ///
    /// The matrix is considered non-invertible whenever its determinant is
    /// less or equal than the provided `epsilon`. An appropriate epsilon is
    /// context-dependent, and therefore zero is used as default, which means
    /// that the matrix is considered non-invertible if and only if its
    /// determinant is exactly zero (example: the null matrix).
    ///
    /// If the matrix is considered non-invertible, then the returned matrix
    /// has all its elements set to `core::infinity<T>`.
    ///
    Mat3 inverse(bool* isInvertible = nullptr, T epsilon = 0) const;

    /// Right-multiplies this matrix by the translation matrix given
    /// by `vx` and `vy`, that is:
    ///
    /// \verbatim
    /// | 1 0 vx |
    /// | 0 1 vy |
    /// | 0 0 1  |
    /// \endverbatim
    ///
    /// Returns a reference to this matrix.
    ///
    Mat3& translate(T vx, T vy = 0) {
        data_[2][0] += vx * data_[0][0] + vy * data_[1][0];
        data_[2][1] += vx * data_[0][1] + vy * data_[1][1];
        data_[2][2] += vx * data_[0][2] + vy * data_[1][2];
        return *this;
    }

    /// \overload
    Mat3& translate(const Vec2<T>& v) {
        return translate(v.x(), v.y());
    }

    /// Right-multiplies this matrix by the rotation matrix around
    /// the z-axis by `t` radians, that is:
    ///
    /// \verbatim
    /// | cos(t) -sin(t)  0 |
    /// | sin(t)  cos(t)  0 |
    /// | 0       0       1 |
    /// \endverbatim
    ///
    /// Returns a reference to this matrix.
    ///
    /// If `orthosnap` is true (the default), then rotations which are
    /// extremely close to a multiple of 90° are snapped to this exact multiple
    /// of 90°. This ensures that if you call `rotate(pi / 2)`, you get exactly
    /// the following matrix:
    ///
    /// \verbatim
    /// | 1 -1  0 |
    /// | 1  1  0 |
    /// | 0  0  1 |
    /// \endverbatim
    ///
    Mat3& rotate(T t, bool orthosnap = true) {
        static T eps = std::numeric_limits<T>::epsilon();
        T c = std::cos(t);
        T s = std::sin(t);
        if (orthosnap && (std::abs(c) < eps || std::abs(s) < eps)) {
            T cr = std::round(c);
            T sr = std::round(s);
            c = cr;
            s = sr;
        }
        // clang-format off
        Mat3 m(c,-s, 0,
               s, c, 0,
               0, 0, 1);
        // clang-format on
        return (*this) *= m;
    }

    /// Right-multiplies this matrix by the uniform scaling matrix
    /// given by `s`, that is:
    ///
    /// \verbatim
    /// | s 0 0 |
    /// | 0 s 0 |
    /// | 0 0 1 |
    /// \endverbatim
    ///
    /// Returns a reference to this matrix.
    ///
    /// Note: if your 3x3 matrix is not meant to represent a 2D affine
    /// transformation, simply use the following code instead (multiplication
    /// by scalar), which also multiplies the last row and column:
    ///
    /// ```cpp
    /// m *= s;
    /// ```
    ///
    Mat3& scale(T s) {
        // clang-format off
        Mat3 m(s, 0, 0,
               0, s, 0,
               0, 0, 1);
        // clang-format on
        return (*this) *= m;
    }

    /// Right-multiplies this matrix by the non-uniform scaling matrix
    /// given by `sx` and `sy`, that is:
    ///
    /// \verbatim
    /// | sx 0  0 |
    /// | 0  sy 0 |
    /// | 0  0  1 |
    /// \endverbatim
    ///
    /// Returns a reference to this Mat3.
    ///
    Mat3& scale(T sx, T sy) {
        // clang-format off
        Mat3 m(sx, 0,  0,
               0,  sy, 0,
               0,  0,  1);
        // clang-format on
        return (*this) *= m;
    }

    /// \overload
    Mat3& scale(const Vec2<T>& v) {
        return scale(v.x(), v.y());
    }

private:
    T data_[3][3];
};

// This definition must be out-of-class.
// See: https://stackoverflow.com/questions/11928089/
// static-constexpr-member-of-same-type-as-class-being-defined
//
template<typename T>
inline constexpr Mat3<T> Mat3<T>::identity = Mat3<T>(1);

template<typename T>
Mat3<T> Mat3<T>::inverse(bool* isInvertible, T epsilon_) const {

    Mat3 res;

    const auto& d = data_;
    auto& inv = res.data_;

    // clang-format off
    inv[0][0] =   d[1][1]*d[2][2] - d[2][1]*d[1][2];
    inv[1][0] = - d[1][0]*d[2][2] + d[2][0]*d[1][2];
    inv[2][0] =   d[1][0]*d[2][1] - d[2][0]*d[1][1];
    // clang-format on

    T det = d[0][0] * inv[0][0] + d[0][1] * inv[1][0] + d[0][2] * inv[2][0];

    if (std::abs(det) <= epsilon_) {
        if (isInvertible) {
            *isInvertible = false;
        }
        constexpr T inf = core::infinity<T>;
        // clang-format off
        res.setElements(inf, inf, inf,
                        inf, inf, inf,
                        inf, inf, inf);
        // clang-format on
    }
    else {
        if (isInvertible) {
            *isInvertible = true;
        }
        // clang-format off
        inv[0][1] = - d[0][1]*d[2][2] + d[2][1]*d[0][2];
        inv[1][1] =   d[0][0]*d[2][2] - d[2][0]*d[0][2];
        inv[2][1] = - d[0][0]*d[2][1] + d[2][0]*d[0][1];
        inv[0][2] =   d[0][1]*d[1][2] - d[1][1]*d[0][2];
        inv[1][2] = - d[0][0]*d[1][2] + d[1][0]*d[0][2];
        inv[2][2] =   d[0][0]*d[1][1] - d[1][0]*d[0][1];
        // clang-format on

        res *= T(1) / det;
    }
    return res;
}

/// Alias for `Mat3<float>`.
///
using Mat3f = Mat3<float>;

/// Alias for `Mat3<double>`.
///
using Mat3d = Mat3<double>;

/// Alias for `core::Array<Mat3<T>>`.
///
template<typename T>
using Mat3Array = core::Array<Mat3<T>>;

/// Alias for `core::Array<Mat3f>`.
///
using Mat3fArray = core::Array<Mat3f>;

/// Alias for `core::Array<Mat3d>`.
///
using Mat3dArray = core::Array<Mat3d>;

/// Alias for `core::SharedConstArray<Mat3<T>>`.
///
template<typename T>
using SharedConstMat3Array = core::SharedConstArray<Mat3<T>>;

/// Alias for `core::SharedConstArray<Mat3f>`.
///
using SharedConstMat3fArray = core::SharedConstArray<Mat3f>;

/// Alias for `core::SharedConstArray<Mat3d>`.
///
using SharedConstMat3dArray = core::SharedConstArray<Mat3d>;

/// Allows to iterate over a range of `Mat3<T>` stored in a memory buffer of
/// `T` elements, where consecutive `Mat3<T>` elements are separated by a given
/// stride.
///
template<typename T>
using Mat3Span = StrideSpan<T, Mat3<T>>;

/// Alias for `Mat3Span<float>`.
///
using Mat3fSpan = Mat3Span<float>;

/// Alias for `Mat3Span<double>`.
///
using Mat3dSpan = Mat3Span<float>;

/// Const version of `Mat3Span`.
///
template<typename T>
using Mat3ConstSpan = StrideSpan<const T, const Mat3<T>>;

/// Alias for `Mat3ConstSpan<float>`.
///
using Mat3fConstSpan = Mat3ConstSpan<float>;

/// Alias for `Mat3ConstSpan<double>`.
///
using Mat3dConstSpan = Mat3ConstSpan<double>;

/// Overloads `setZero(T& x)`.
///
/// \sa `vgc::core::zero<T>()`.
///
template<typename T>
inline void setZero(Mat3<T>& m) {
    m.setToZero();
}

/// Writes the given `Mat3` to the output stream.
///
template<typename T, typename OStream>
void write(OStream& out, const Mat3<T>& m) {
    static const char* s = ", ";
    // clang-format off
    write(out,
          "((", m(0,0), s, m(0,1), s, m(0,2), "),",
          " (", m(1,0), s, m(1,1), s, m(1,2), "),",
          " (", m(2,0), s, m(2,1), s, m(2,2), "))");
    // clang-format on
}

namespace detail {

template<typename T, typename IStream>
void readToMatRow(Mat3<T>& m, Int i, IStream& in) {
    Vec3<T> v;
    readTo(v, in);
    m(i, 0) = v[0];
    m(i, 1) = v[1];
    m(i, 2) = v[2];
}

} // namespace detail

/// Reads a `Mat3` from the input stream, and stores it in the given output
/// parameter `m`. Leading whitespaces are allowed. Raises `ParseError` if the
/// stream does not start with a `Mat2d`. Raises `RangeError` if one of its
/// coordinates is outside the representable range of a `T`.
///
template<typename T, typename IStream>
void readTo(Mat3<T>& m, IStream& in) {
    skipWhitespacesAndExpectedCharacter(in, '(');
    detail::readToMatRow(m, 0, in);
    skipWhitespacesAndExpectedCharacter(in, ',');
    detail::readToMatRow(m, 1, in);
    skipWhitespacesAndExpectedCharacter(in, ',');
    detail::readToMatRow(m, 2, in);
    skipExpectedCharacter(in, ')');
}

} // namespace vgc::geometry

template<typename T>
struct fmt::formatter<vgc::geometry::Mat3<T>> {
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && *it != '}') {
            throw format_error("invalid format");
        }
        return it;
    }
    template<typename FormatContext>
    auto format(const vgc::geometry::Mat3<T>& m, FormatContext& ctx) {
        // clang-format off
        return format_to(ctx.out(),
                         "(({}, {}, {}),"
                         " ({}, {}, {}),"
                         " ({}, {}, {}))",
                         m(0,0), m(0,1), m(0,2),
                         m(1,0), m(1,1), m(1,2),
                         m(2,0), m(2,1), m(2,2));
        // clang-format on
    }
};

#endif // VGC_GEOMETRY_MAT3_H
