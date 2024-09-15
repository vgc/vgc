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
// Instead, edit tools/mat4x.h then run tools/generate.py.

#ifndef VGC_GEOMETRY_MAT4_H
#define VGC_GEOMETRY_MAT4_H

#include <cmath>

#include <vgc/core/array.h>
#include <vgc/geometry/api.h>
#include <vgc/geometry/mat.h>
#include <vgc/geometry/mat2.h>
#include <vgc/geometry/mat3.h>
#include <vgc/geometry/stride.h>
#include <vgc/geometry/vec2.h>
#include <vgc/geometry/vec3.h>
#include <vgc/geometry/vec4.h>

namespace vgc::geometry {

/// \class vgc::geometry::Mat4
/// \brief Represents a 4x4 matrix.
///
/// A `Mat4<T>` represents a 4x4 matrix in column-major format.
///
/// The memory size of a `Mat4<T>` is exactly `16 * sizeof(T)`. This will never
/// change in any future version, as this allows to conveniently use this class
/// for data transfer to the GPU (via OpenGL, Metal, etc.).
///
/// Unlike in some libraries, VGC has chosen not to distinguish between 4x4
/// matrices and 3D affine transformations in homogeneous coordinates. In other
/// words, if you wish to represent a 3D affine transformation, simply use a
/// `Mat4<T>`. Also, you can even use a `Mat4<T>` to represent a 2D affine
/// transformation. For example, you can multiply a `Mat4d` with a `Vec2<T>`,
/// which returns the same as multiplying the matrix with the 4D vector
/// `[x, y, 0, 1]`.
///
template<typename T>
class Mat4 {
public:
    using ScalarType = T;
    static constexpr Int dimension = 4;

    VGC_WARNING_PUSH
    VGC_WARNING_MSVC_DISABLE(26495) // member variable uninitialized
    /// Creates an uninitialized `Mat4`.
    ///
    Mat4(core::NoInit) noexcept {
    }
    VGC_WARNING_POP

    /// Creates a `Mat4` initialized to the null matrix `Mat3d(0)`.
    ///
    constexpr Mat4() noexcept
        : Mat4(0) {
    }

    // clang-format off

    /// Creates a `Mat4` initialized with the given arguments.
    ///
    // clang-format off
    constexpr Mat4(
        T m00, T m01, T m02, T m14,
        T m10, T m11, T m12, T m24,
        T m20, T m21, T m22, T m34,
        T m41, T m42, T m43, T m44) noexcept

        : data_{{m00, m10, m20, m41},
                {m01, m11, m21, m42},
                {m02, m12, m22, m43},
                {m14, m24, m34, m44}} {
    }
    // clang-format on

    /// Creates a `Mat4` initialized with the given row vectors.
    ///
    constexpr Mat4(
        const Vec4<T>& v0,
        const Vec4<T>& v1,
        const Vec4<T>& v2,
        const Vec4<T>& v3) noexcept

        : data_{
            {v0[0], v1[0], v2[0], v3[0]},
            {v0[1], v1[1], v2[1], v3[1]},
            {v0[2], v1[2], v2[2], v3[2]},
            {v0[3], v1[3], v2[3], v3[3]}} {
    }

    /// Creates a diagonal matrix with diagonal elements equal to the given
    /// value. As specific cases, the null matrix is `Mat4(0)`, and the
    /// identity matrix is `Mat4(1)`.
    ///
    constexpr explicit Mat4(T d) noexcept
        : data_{
            {d, 0, 0, 0}, //
            {0, d, 0, 0},
            {0, 0, d, 0},
            {0, 0, 0, d}} {
    }

    /// Creates a `Mat4` from another `Mat<4, T>` object by performing a
    /// `static_cast` on each of its elements.
    ///
    template<typename U, VGC_REQUIRES(!std::is_same_v<T, U>)>
    constexpr explicit Mat4(const Mat4<U>& other) noexcept
        : data_{
            {static_cast<T>(other(0, 0)),
             static_cast<T>(other(1, 0)),
             static_cast<T>(other(2, 0)),
             static_cast<T>(other(3, 0))},
            {static_cast<T>(other(0, 1)),
             static_cast<T>(other(1, 1)),
             static_cast<T>(other(2, 1)),
             static_cast<T>(other(3, 1))},
            {static_cast<T>(other(0, 2)),
             static_cast<T>(other(1, 2)),
             static_cast<T>(other(2, 2)),
             static_cast<T>(other(3, 2))},
            {static_cast<T>(other(0, 3)),
             static_cast<T>(other(1, 3)),
             static_cast<T>(other(2, 3)),
             static_cast<T>(other(3, 3))}} {
    }

    /// Creates a `Mat4<T>` from a `Mat2<U>` matrix, assuming the given
    /// matrix represents a 2D linear transformation.
    ///
    /// ```text
    /// |a b|    |a b 0 0|
    /// |c d| -> |c d 0 0|
    ///          |0 0 1 0|
    ///          |0 0 0 1|
    /// ```
    ///
    /// The returned matrix can be used either as a 4D linear transformation
    /// that doesn't modify the third and fourth coordinates, or as a 3D
    /// homogeneous transformation that also happens to be a linear
    /// transformation (no translation or projective components) that also does
    /// not modify the third coordinate.
    ///
    /// If the given matrix is not a `Mat2<T>`, then each of its elements is
    /// converted to `T` by performing a `static_cast`.
    ///
    template<typename U>
    static constexpr Mat4 fromLinear(const Mat2<U>& other) noexcept {
        T a = static_cast<T>(other(0, 0));
        T b = static_cast<T>(other(0, 1));
        T c = static_cast<T>(other(1, 0));
        T d = static_cast<T>(other(1, 1));

        // clang-format off
        return Mat4(a, b, 0, 0,
                    c, d, 0, 0,
                    0, 0, 1, 0,
                    0, 0, 0, 1);
        // clang-format on
    }

    /// Creates a `Mat4<T>` from a `Mat3<U>` matrix, assuming the given
    /// matrix represents a 3D linear transformation.
    ///
    /// ```text
    /// |a b c|    |a b c 0|
    /// |d e f| -> |d e f 0|
    /// |g h i|    |g h i 0|
    ///            |0 0 0 1|
    /// ```
    ///
    /// The returned matrix can be used either as a 4D linear transformation
    /// that doesn't modify the fourth coordinates, or as a 3D homogeneous
    /// transformation that also happens to be a linear transformation (no
    /// translation or projective components).
    ///
    /// If the given matrix is not a `Mat3<T>`, then each of its elements is
    /// converted to `T` by performing a `static_cast`.
    ///
    template<typename U>
    static constexpr Mat4 fromLinear(const Mat3<U>& other) noexcept {
        T a = static_cast<T>(other(0, 0));
        T b = static_cast<T>(other(0, 1));
        T c = static_cast<T>(other(0, 2));
        T d = static_cast<T>(other(1, 0));
        T e = static_cast<T>(other(1, 1));
        T f = static_cast<T>(other(1, 2));
        T g = static_cast<T>(other(2, 0));
        T h = static_cast<T>(other(2, 1));
        T i = static_cast<T>(other(2, 2));
        // clang-format off
        return Mat4(a, b, c, 0,
                    d, e, f, 0,
                    g, h, i, 0,
                    0, 0, 0, 1);
        // clang-format on
    }

    /// Creates a `Mat4<T>` from a `Mat2<U>` matrix, assuming the given matrix
    /// represents a 1D homogeneous transformation (possibly affine or
    /// projective).
    ///
    /// ```text
    /// |a b|    |a 0 0 b|
    /// |c d| -> |0 1 0 0|
    ///          |0 0 1 0|
    ///          |c 0 0 d|
    /// ``
    ///
    /// The returned matrix can be used as a 3D homogeneous transformation
    /// whose linear part does not modify the second and third coordinate.
    ///
    /// If the given matrix is not a `Mat2<T>`, then each of its elements is
    /// converted to `T` by performing a `static_cast`.
    ///
    template<typename U>
    static constexpr Mat4 fromTransform(const Mat2<U>& other) noexcept {
        T a = static_cast<T>(other(0, 0));
        T b = static_cast<T>(other(0, 1));
        T c = static_cast<T>(other(1, 0));
        T d = static_cast<T>(other(1, 1));
        // clang-format off
        return Mat4(a, 0, 0, b,
                    0, 1, 0, 0,
                    0, 0, 1, 0,
                    c, 0, 0, d);
        // clang-format on
    }

    /// Creates a `Mat4<T>` from a `Mat3<U>` matrix, assuming the given matrix
    /// represents a 2D homogeneous transformation (possibly affine or
    /// projective).
    ///
    /// ```text
    /// |a b c|    |a b 0 c|
    /// |d e f| -> |d e 0 f|
    /// |g h i|    |0 0 1 0|
    ///            |g h 0 i|
    /// ``
    ///
    /// The returned matrix can be used as a 3D homogeneous transformation
    /// whose linear part does not modify the third coordinate.
    ///
    /// If the given matrix is not a `Mat3<T>`, then each of its elements is
    /// converted to `T` by performing a `static_cast`.
    ///
    template<typename U>
    static constexpr Mat4 fromTransform(const Mat3<U>& other) noexcept {
        T a = static_cast<T>(other(0, 0));
        T b = static_cast<T>(other(0, 1));
        T c = static_cast<T>(other(0, 2));
        T d = static_cast<T>(other(1, 0));
        T e = static_cast<T>(other(1, 1));
        T f = static_cast<T>(other(1, 2));
        T g = static_cast<T>(other(2, 0));
        T h = static_cast<T>(other(2, 1));
        T i = static_cast<T>(other(2, 2));
        // clang-format off
        return Mat4(a, b, 0, c,
                    d, e, 0, f,
                    0, 0, 1, 0,
                    g, h, 0, i);
        // clang-format on
    }

    /// Modifies all the elements of this matrix.
    ///
    // clang-format off
    Mat4& setElements(
        T m00, T m01, T m02, T m14,
        T m10, T m11, T m12, T m24,
        T m20, T m21, T m22, T m34,
        T m41, T m42, T m43, T m44) {

        data_[0][0] = m00; data_[0][1] = m10; data_[0][2] = m20; data_[0][3] = m41;
        data_[1][0] = m01; data_[1][1] = m11; data_[1][2] = m21; data_[1][3] = m42;
        data_[2][0] = m02; data_[2][1] = m12; data_[2][2] = m22; data_[2][3] = m43;
        data_[3][0] = m14; data_[3][1] = m24; data_[3][2] = m34; data_[3][3] = m44;
        return *this;
    }
    // clang-format on

    /// Sets this matrix to a diagonal matrix with all diagonal elements equal
    /// to the given value.
    ///
    Mat4& setToDiagonal(T d) {
        // clang-format off
        return setElements(
            d, 0, 0, 0,
            0, d, 0, 0,
            0, 0, d, 0,
            0, 0, 0, d);
        // clang-format on
    }

    /// Sets this matrix to the zero matrix.
    ///
    Mat4& setToZero() {
        return setToDiagonal(0);
    }

    /// Sets this matrix to the identity matrix.
    ///
    Mat4& setToIdentity() {
        return setToDiagonal(1);
    }

    /// The identity matrix `Mat4(1)`.
    ///
    static const Mat4 identity;

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
    Mat4& operator+=(const Mat4& other) {
        data_[0][0] += other.data_[0][0];
        data_[0][1] += other.data_[0][1];
        data_[0][2] += other.data_[0][2];
        data_[0][3] += other.data_[0][3];
        data_[1][0] += other.data_[1][0];
        data_[1][1] += other.data_[1][1];
        data_[1][2] += other.data_[1][2];
        data_[1][3] += other.data_[1][3];
        data_[2][0] += other.data_[2][0];
        data_[2][1] += other.data_[2][1];
        data_[2][2] += other.data_[2][2];
        data_[2][3] += other.data_[2][3];
        data_[3][0] += other.data_[3][0];
        data_[3][1] += other.data_[3][1];
        data_[3][2] += other.data_[3][2];
        data_[3][3] += other.data_[3][3];
        return *this;
    }

    /// Returns the addition of the matrix `m1` and the matrix `m2`.
    ///
    friend Mat4 operator+(const Mat4& m1, const Mat4& m2) {
        return Mat4(m1) += m2;
    }

    /// Returns a copy of this matrix (unary plus operator).
    ///
    Mat4 operator+() const {
        return *this;
    }

    /// Substracts in-place the `other` matrix from this matrix.
    ///
    Mat4& operator-=(const Mat4& other) {
        data_[0][0] -= other.data_[0][0];
        data_[0][1] -= other.data_[0][1];
        data_[0][2] -= other.data_[0][2];
        data_[0][3] -= other.data_[0][3];
        data_[1][0] -= other.data_[1][0];
        data_[1][1] -= other.data_[1][1];
        data_[1][2] -= other.data_[1][2];
        data_[1][3] -= other.data_[1][3];
        data_[2][0] -= other.data_[2][0];
        data_[2][1] -= other.data_[2][1];
        data_[2][2] -= other.data_[2][2];
        data_[2][3] -= other.data_[2][3];
        data_[3][0] -= other.data_[3][0];
        data_[3][1] -= other.data_[3][1];
        data_[3][2] -= other.data_[3][2];
        data_[3][3] -= other.data_[3][3];
        return *this;
    }

    /// Returns the substraction of the matrix `m1` and the matrix `m2`.
    ///
    friend Mat4 operator-(const Mat4& m1, const Mat4& m2) {
        return Mat4(m1) -= m2;
    }

    /// Returns the opposite of this matrix (unary minus operator).
    ///
    Mat4 operator-() const {
        return Mat4(*this) *= -1;
    }

    /// Multiplies in-place this matrix with the `other` matrix.
    ///
    Mat4& operator*=(const Mat4& other) {
        *this = (*this) * other;
        return *this;
    }

    /// Returns the multiplication of the matrix `m1` and the matrix `m2`.
    ///
    friend Mat4 operator*(const Mat4& m1, const Mat4& m2) {
        Mat4 res;
        // clang-format off
        res(0,0) = m1(0,0)*m2(0,0) + m1(0,1)*m2(1,0) + m1(0,2)*m2(2,0) + m1(0,3)*m2(3,0);
        res(0,1) = m1(0,0)*m2(0,1) + m1(0,1)*m2(1,1) + m1(0,2)*m2(2,1) + m1(0,3)*m2(3,1);
        res(0,2) = m1(0,0)*m2(0,2) + m1(0,1)*m2(1,2) + m1(0,2)*m2(2,2) + m1(0,3)*m2(3,2);
        res(0,3) = m1(0,0)*m2(0,3) + m1(0,1)*m2(1,3) + m1(0,2)*m2(2,3) + m1(0,3)*m2(3,3);
        res(1,0) = m1(1,0)*m2(0,0) + m1(1,1)*m2(1,0) + m1(1,2)*m2(2,0) + m1(1,3)*m2(3,0);
        res(1,1) = m1(1,0)*m2(0,1) + m1(1,1)*m2(1,1) + m1(1,2)*m2(2,1) + m1(1,3)*m2(3,1);
        res(1,2) = m1(1,0)*m2(0,2) + m1(1,1)*m2(1,2) + m1(1,2)*m2(2,2) + m1(1,3)*m2(3,2);
        res(1,3) = m1(1,0)*m2(0,3) + m1(1,1)*m2(1,3) + m1(1,2)*m2(2,3) + m1(1,3)*m2(3,3);
        res(2,0) = m1(2,0)*m2(0,0) + m1(2,1)*m2(1,0) + m1(2,2)*m2(2,0) + m1(2,3)*m2(3,0);
        res(2,1) = m1(2,0)*m2(0,1) + m1(2,1)*m2(1,1) + m1(2,2)*m2(2,1) + m1(2,3)*m2(3,1);
        res(2,2) = m1(2,0)*m2(0,2) + m1(2,1)*m2(1,2) + m1(2,2)*m2(2,2) + m1(2,3)*m2(3,2);
        res(2,3) = m1(2,0)*m2(0,3) + m1(2,1)*m2(1,3) + m1(2,2)*m2(2,3) + m1(2,3)*m2(3,3);
        res(3,0) = m1(3,0)*m2(0,0) + m1(3,1)*m2(1,0) + m1(3,2)*m2(2,0) + m1(3,3)*m2(3,0);
        res(3,1) = m1(3,0)*m2(0,1) + m1(3,1)*m2(1,1) + m1(3,2)*m2(2,1) + m1(3,3)*m2(3,1);
        res(3,2) = m1(3,0)*m2(0,2) + m1(3,1)*m2(1,2) + m1(3,2)*m2(2,2) + m1(3,3)*m2(3,2);
        res(3,3) = m1(3,0)*m2(0,3) + m1(3,1)*m2(1,3) + m1(3,2)*m2(2,3) + m1(3,3)*m2(3,3);
        // clang-format on
        return res;
    }

    /// Multiplies in-place this matrix by the given scalar `s`.
    ///
    Mat4& operator*=(T s) {
        data_[0][0] *= s;
        data_[0][1] *= s;
        data_[0][2] *= s;
        data_[0][3] *= s;
        data_[1][0] *= s;
        data_[1][1] *= s;
        data_[1][2] *= s;
        data_[1][3] *= s;
        data_[2][0] *= s;
        data_[2][1] *= s;
        data_[2][2] *= s;
        data_[2][3] *= s;
        data_[3][0] *= s;
        data_[3][1] *= s;
        data_[3][2] *= s;
        data_[3][3] *= s;
        return *this;
    }

    /// Returns the multiplication of this matrix by the given scalar `s`.
    ///
    Mat4 operator*(T s) const {
        return Mat4(*this) *= s;
    }

    /// Returns the multiplication of the scalar `s` with the matrix `m`.
    ///
    friend Mat4 operator*(T s, const Mat4& m) {
        return m * s;
    }

    /// Divides in-place this matrix by the given scalar `s`.
    ///
    Mat4& operator/=(T s) {
        data_[0][0] /= s;
        data_[0][1] /= s;
        data_[0][2] /= s;
        data_[0][3] /= s;
        data_[1][0] /= s;
        data_[1][1] /= s;
        data_[1][2] /= s;
        data_[1][3] /= s;
        data_[2][0] /= s;
        data_[2][1] /= s;
        data_[2][2] /= s;
        data_[2][3] /= s;
        data_[3][0] /= s;
        data_[3][1] /= s;
        data_[3][2] /= s;
        data_[3][3] /= s;
        return *this;
    }

    /// Returns the division of this matrix by the given scalar `s`.
    ///
    Mat4 operator/(T s) const {
        return Mat4(*this) /= s;
    }

    /// Returns whether the two given matrices `m1` and `m2` are equal.
    ///
    friend bool operator==(const Mat4& m1, const Mat4& m2) {
        return m1.data_[0][0] == m2.data_[0][0] && m1.data_[0][1] == m2.data_[0][1]
               && m1.data_[0][2] == m2.data_[0][2] && m1.data_[0][3] == m2.data_[0][3]
               && m1.data_[1][0] == m2.data_[1][0] && m1.data_[1][1] == m2.data_[1][1]
               && m1.data_[1][2] == m2.data_[1][2] && m1.data_[1][3] == m2.data_[1][3]
               && m1.data_[2][0] == m2.data_[2][0] && m1.data_[2][1] == m2.data_[2][1]
               && m1.data_[2][2] == m2.data_[2][2] && m1.data_[2][3] == m2.data_[2][3]
               && m1.data_[3][0] == m2.data_[3][0] && m1.data_[3][1] == m2.data_[3][1]
               && m1.data_[3][2] == m2.data_[3][2] && m1.data_[3][3] == m2.data_[3][3];
    }

    /// Returns whether the two given matrices `m1` and `m2` are different.
    ///
    friend bool operator!=(const Mat4& m1, const Mat4& m2) {
        return m1.data_[0][0] != m2.data_[0][0] || m1.data_[0][1] != m2.data_[0][1]
               || m1.data_[0][2] != m2.data_[0][2] || m1.data_[0][3] != m2.data_[0][3]
               || m1.data_[1][0] != m2.data_[1][0] || m1.data_[1][1] != m2.data_[1][1]
               || m1.data_[1][2] != m2.data_[1][2] || m1.data_[1][3] != m2.data_[1][3]
               || m1.data_[2][0] != m2.data_[2][0] || m1.data_[2][1] != m2.data_[2][1]
               || m1.data_[2][2] != m2.data_[2][2] || m1.data_[2][3] != m2.data_[2][3]
               || m1.data_[3][0] != m2.data_[3][0] || m1.data_[3][1] != m2.data_[3][1]
               || m1.data_[3][2] != m2.data_[3][2] || m1.data_[3][3] != m2.data_[3][3];
    }

    /// Returns the multiplication of this `Mat4` by the given `Vec4`.
    ///
    Vec4<T> operator*(const Vec4<T>& v) const {
        // clang-format off
        return Vec4<T>(
            data_[0][0]*v[0] + data_[1][0]*v[1] + data_[2][0]*v[2] + data_[3][0]*v[3],
            data_[0][1]*v[0] + data_[1][1]*v[1] + data_[2][1]*v[2] + data_[3][1]*v[3],
            data_[0][2]*v[0] + data_[1][2]*v[1] + data_[2][2]*v[2] + data_[3][2]*v[3],
            data_[0][3]*v[0] + data_[1][3]*v[1] + data_[2][3]*v[2] + data_[3][3]*v[3]);
        // clang-format on
    }

    /// Returns the result of transforming the given `Vec3` by this `Mat4`
    /// interpreted as a 3D projective transformation.
    ///
    /// This is equivalent to multiplying this `Mat4` by `Vec3(x, y, z, 1)`,
    /// then returning the first three coordinates divided by the fourth
    /// coordinate.
    ///
    Vec3<T> transform(const Vec3<T>& v) const {
        T x = data_[0][0] * v[0] + data_[1][0] * v[1] + data_[2][0] * v[2] + data_[3][0];
        T y = data_[0][1] * v[0] + data_[1][1] * v[1] + data_[2][1] * v[2] + data_[3][1];
        T z = data_[0][2] * v[0] + data_[1][2] * v[1] + data_[2][2] * v[2] + data_[3][2];
        T w = data_[0][3] * v[0] + data_[1][3] * v[1] + data_[2][3] * v[2] + data_[3][3];
        T iw = T(1) / w;
        return Vec3<T>(iw * x, iw * y, iw * z);
    }

    /// Computes the transformation of the given `Vec2` (interpreted as a
    /// `Vec3` with `z = 0`) by this `Mat4` (interpreted as a 3D projective
    /// transformation), and returns the first 2 coordinates.
    ///
    /// See `transform(const Vec3& v)` for details.
    ///
    Vec2<T> transform(const Vec2<T>& v) const {
        T x = data_[0][0] * v[0] + data_[1][0] * v[1] + data_[3][0];
        T y = data_[0][1] * v[0] + data_[1][1] * v[1] + data_[3][1];
        T w = data_[0][3] * v[0] + data_[1][3] * v[1] + data_[3][3];
        T iw = T(1) / w;
        return Vec2<T>(iw * x, iw * y);
    }

    /// Returns the result of transforming the given `Vec3` by this `Mat4`
    /// interpreted as a 3D affine transformation, that is, ignoring the
    /// projective components.
    ///
    /// This is equivalent to multiplying the 3x4 submatrix of this `Mat4` by
    /// `Vec4(x, y, z, 1)`.
    ///
    /// This can be used as a faster version of `transform()` whenever you
    /// know that the last row of the matrix is equal to `[0, 0, 0, 1]`, or
    /// whenever you prefer to behave as if the last row was `[0, 0, 0, 1]`.
    ///
    Vec3<T> transformAffine(const Vec3<T>& v) const {
        T x = data_[0][0] * v[0] + data_[1][0] * v[1] + data_[2][0] * v[2] + data_[3][0];
        T y = data_[0][1] * v[0] + data_[1][1] * v[1] + data_[2][1] * v[2] + data_[3][1];
        T z = data_[0][2] * v[0] + data_[1][2] * v[1] + data_[2][2] * v[2] + data_[3][2];
        return Vec3<T>(x, y, z);
    }

    /// Computes the transformation of the given `Vec2` (interpreted as a
    /// `Vec3` with `z = 0`) by this `Mat4` (interpreted as a 3D affine
    /// transformation, that is, ignoring the projective component), and
    /// returns the first 2 coordinates.
    ///
    /// See `transformAffine(const Vec3& v)` for details.
    ///
    Vec2<T> transformAffine(const Vec2<T>& v) const {
        T x = data_[0][0] * v[0] + data_[1][0] * v[1] + data_[3][0];
        T y = data_[0][1] * v[0] + data_[1][1] * v[1] + data_[3][1];
        return Vec2<T>(x, y);
    }

    /// Returns the result of transforming the given `Vec3` by the linear part
    /// of this `Mat4` interpreted as a 3D projective transformation. That is,
    /// this ignores the translation and projective components, and only
    /// applies the rotation, scale, and shear components.
    ///
    /// This is equivalent to multiplying this matrix by `Vec4(x, y, z, 0)`
    /// and returning only the first three components.
    ///
    /// This is typically used to transform "directions" rather than "points".
    ///
    Vec3<T> transformLinear(const Vec3<T>& v) const {
        T x = data_[0][0] * v[0] + data_[1][0] * v[1] + data_[2][0] * v[2];
        T y = data_[0][1] * v[0] + data_[1][1] * v[1] + data_[2][1] * v[2];
        T z = data_[0][2] * v[0] + data_[1][2] * v[1] + data_[2][2] * v[2];
        return Vec3<T>(x, y, z);
    }

    /// Computes the transformation of the given `Vec2` (interpreted as a
    /// `Vec3` with `z = 0`) by the linear part of this `Mat4` (interpreted
    /// as a 3D projective transformation), and returns the first 2
    /// coordinates.
    ///
    /// See `transformLinear(const Vec3& v)` for details.
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
    Mat4 inverse(bool* isInvertible = nullptr, T epsilon = 0) const;

    /// Right-multiplies this matrix by the translation matrix given
    /// by `vx`, `vy`, and `vy`, that is:
    ///
    /// \verbatim
    /// | 1 0 0 vx |
    /// | 0 1 0 vy |
    /// | 0 0 1 vz |
    /// | 0 0 0 1  |
    /// \endverbatim
    ///
    /// Returns a reference to this matrix.
    ///
    Mat4& translate(T vx, T vy = 0, T vz = 0) {
        data_[3][0] += vx * data_[0][0] + vy * data_[1][0] + vz * data_[2][0];
        data_[3][1] += vx * data_[0][1] + vy * data_[1][1] + vz * data_[2][1];
        data_[3][2] += vx * data_[0][2] + vy * data_[1][2] + vz * data_[2][2];
        data_[3][3] += vx * data_[0][3] + vy * data_[1][3] + vz * data_[2][3];
        return *this;
    }

    /// \overload
    Mat4& translate(const Vec2<T>& v) {
        return translate(v.x(), v.y());
    }

    /// \overload
    Mat4& translate(const Vec3<T>& v) {
        return translate(v.x(), v.y(), v.z());
    }

    /// Right-multiplies this matrix by the rotation matrix around
    /// the z-axis by `t` radians, that is:
    ///
    /// \verbatim
    /// | cos(t) -sin(t)  0       0 |
    /// | sin(t)  cos(t)  0       0 |
    /// | 0       0       1       0 |
    /// | 0       0       0       1 |
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
    /// | 1 -1  0  0 |
    /// | 1  1  0  0 |
    /// | 0  0  1  0 |
    /// | 0  0  0  1 |
    /// \endverbatim
    ///
    Mat4& rotate(T t, bool orthosnap = true) {
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
        Mat4 m(c,-s, 0, 0,
               s, c, 0, 0,
               0, 0, 1, 0,
               0, 0, 0, 1);
        // clang-format on
        return (*this) *= m;
    }

    /// Right-multiplies this matrix by the uniform scaling matrix
    /// given by `s`, that is:
    ///
    /// \verbatim
    /// | s 0 0 0 |
    /// | 0 s 0 0 |
    /// | 0 0 s 0 |
    /// | 0 0 0 1 |
    /// \endverbatim
    ///
    /// Returns a reference to this matrix.
    ///
    /// Note: if your 4x4 matrix is not meant to represent a 3D affine
    /// transformation, simply use the following code instead (multiplication
    /// by scalar), which also multiplies the last row and column:
    ///
    /// ```cpp
    /// m *= s;
    /// ```
    ///
    Mat4& scale(T s) {
        // clang-format off
        Mat4 m(s, 0, 0, 0,
               0, s, 0, 0,
               0, 0, s, 0,
               0, 0, 0, 1);
        // clang-format on
        return (*this) *= m;
    }

    /// Right-multiplies this matrix by the non-uniform scaling matrix
    /// given by `sx`, `sy`, and `sz`, that is:
    ///
    /// \verbatim
    /// | sx 0  0  0 |
    /// | 0  sy 0  0 |
    /// | 0  0  sz 0 |
    /// | 0  0  0  1 |
    /// \endverbatim
    ///
    /// Returns a reference to this matrix.
    ///
    Mat4& scale(T sx, T sy, T sz = 1) {
        // clang-format off
        Mat4 m(sx, 0,  0,  0,
               0,  sy, 0,  0,
               0,  0,  sz, 0,
               0,  0,  0,  1);
        // clang-format on
        return (*this) *= m;
    }

    /// \overload
    Mat4& scale(const Vec3<T>& v) {
        return scale(v.x(), v.y(), v.z());
    }

private:
    T data_[4][4];
};

// This definition must be out-of-class.
// See: https://stackoverflow.com/questions/11928089/
// static-constexpr-member-of-same-type-as-class-being-defined
//
template<typename T>
inline constexpr Mat4<T> Mat4<T>::identity = Mat4<T>(1);

// We define this function out-of-class to keep the name `epsilon` in the
// declaration (and thus documentation), while using `epsilon_` in the
// definition to prevent a warning about hiding core::epsilon.
//
template<typename T>
Mat4<T> Mat4<T>::inverse(bool* isInvertible, T epsilon_) const {

    Mat4 res;

    const auto& d = data_;
    auto& inv = res.data_;

    // clang-format off
    inv[0][0] =   d[1][1]*d[2][2]*d[3][3] - d[1][1]*d[2][3]*d[3][2] - d[2][1]*d[1][2]*d[3][3]
                + d[2][1]*d[1][3]*d[3][2] + d[3][1]*d[1][2]*d[2][3] - d[3][1]*d[1][3]*d[2][2];
    inv[1][0] = - d[1][0]*d[2][2]*d[3][3] + d[1][0]*d[2][3]*d[3][2] + d[2][0]*d[1][2]*d[3][3]
                - d[2][0]*d[1][3]*d[3][2] - d[3][0]*d[1][2]*d[2][3] + d[3][0]*d[1][3]*d[2][2];
    inv[2][0] =   d[1][0]*d[2][1]*d[3][3] - d[1][0]*d[2][3]*d[3][1] - d[2][0]*d[1][1]*d[3][3]
                + d[2][0]*d[1][3]*d[3][1] + d[3][0]*d[1][1]*d[2][3] - d[3][0]*d[1][3]*d[2][1];
    inv[3][0] = - d[1][0]*d[2][1]*d[3][2] + d[1][0]*d[2][2]*d[3][1] + d[2][0]*d[1][1]*d[3][2]
                - d[2][0]*d[1][2]*d[3][1] - d[3][0]*d[1][1]*d[2][2] + d[3][0]*d[1][2]*d[2][1];
    // clang-format on

    T det = d[0][0] * inv[0][0]   //
            + d[0][1] * inv[1][0] //
            + d[0][2] * inv[2][0] //
            + d[0][3] * inv[3][0];

    if (std::abs(det) <= epsilon_) {
        if (isInvertible) {
            *isInvertible = false;
        }
        constexpr T inf = core::infinity<T>;

        // clang-format off
        res.setElements(inf, inf, inf, inf,
                        inf, inf, inf, inf,
                        inf, inf, inf, inf,
                        inf, inf, inf, inf);
        // clang-format on
    }
    else {
        if (isInvertible) {
            *isInvertible = true;
        }
        // clang-format off
        inv[0][1] = - d[0][1]*d[2][2]*d[3][3] + d[0][1]*d[2][3]*d[3][2] + d[2][1]*d[0][2]*d[3][3]
                    - d[2][1]*d[0][3]*d[3][2] - d[3][1]*d[0][2]*d[2][3] + d[3][1]*d[0][3]*d[2][2];
        inv[1][1] =   d[0][0]*d[2][2]*d[3][3] - d[0][0]*d[2][3]*d[3][2] - d[2][0]*d[0][2]*d[3][3]
                    + d[2][0]*d[0][3]*d[3][2] + d[3][0]*d[0][2]*d[2][3] - d[3][0]*d[0][3]*d[2][2];
        inv[2][1] = - d[0][0]*d[2][1]*d[3][3] + d[0][0]*d[2][3]*d[3][1] + d[2][0]*d[0][1]*d[3][3]
                    - d[2][0]*d[0][3]*d[3][1] - d[3][0]*d[0][1]*d[2][3] + d[3][0]*d[0][3]*d[2][1];
        inv[3][1] =   d[0][0]*d[2][1]*d[3][2] - d[0][0]*d[2][2]*d[3][1] - d[2][0]*d[0][1]*d[3][2]
                    + d[2][0]*d[0][2]*d[3][1] + d[3][0]*d[0][1]*d[2][2] - d[3][0]*d[0][2]*d[2][1];
        inv[0][2] =   d[0][1]*d[1][2]*d[3][3] - d[0][1]*d[1][3]*d[3][2] - d[1][1]*d[0][2]*d[3][3]
                    + d[1][1]*d[0][3]*d[3][2] + d[3][1]*d[0][2]*d[1][3] - d[3][1]*d[0][3]*d[1][2];
        inv[1][2] = - d[0][0]*d[1][2]*d[3][3] + d[0][0]*d[1][3]*d[3][2] + d[1][0]*d[0][2]*d[3][3]
                    - d[1][0]*d[0][3]*d[3][2] - d[3][0]*d[0][2]*d[1][3] + d[3][0]*d[0][3]*d[1][2];
        inv[2][2] =   d[0][0]*d[1][1]*d[3][3] - d[0][0]*d[1][3]*d[3][1] - d[1][0]*d[0][1]*d[3][3]
                    + d[1][0]*d[0][3]*d[3][1] + d[3][0]*d[0][1]*d[1][3] - d[3][0]*d[0][3]*d[1][1];
        inv[3][2] = - d[0][0]*d[1][1]*d[3][2] + d[0][0]*d[1][2]*d[3][1] + d[1][0]*d[0][1]*d[3][2]
                    - d[1][0]*d[0][2]*d[3][1] - d[3][0]*d[0][1]*d[1][2] + d[3][0]*d[0][2]*d[1][1];
        inv[0][3] = - d[0][1]*d[1][2]*d[2][3] + d[0][1]*d[1][3]*d[2][2] + d[1][1]*d[0][2]*d[2][3]
                    - d[1][1]*d[0][3]*d[2][2] - d[2][1]*d[0][2]*d[1][3] + d[2][1]*d[0][3]*d[1][2];
        inv[1][3] =   d[0][0]*d[1][2]*d[2][3] - d[0][0]*d[1][3]*d[2][2] - d[1][0]*d[0][2]*d[2][3]
                    + d[1][0]*d[0][3]*d[2][2] + d[2][0]*d[0][2]*d[1][3] - d[2][0]*d[0][3]*d[1][2];
        inv[2][3] = - d[0][0]*d[1][1]*d[2][3] + d[0][0]*d[1][3]*d[2][1] + d[1][0]*d[0][1]*d[2][3]
                    - d[1][0]*d[0][3]*d[2][1] - d[2][0]*d[0][1]*d[1][3] + d[2][0]*d[0][3]*d[1][1];
        inv[3][3] =   d[0][0]*d[1][1]*d[2][2] - d[0][0]*d[1][2]*d[2][1] - d[1][0]*d[0][1]*d[2][2]
                    + d[1][0]*d[0][2]*d[2][1] + d[2][0]*d[0][1]*d[1][2] - d[2][0]*d[0][2]*d[1][1];
        // clang-format on

        res *= T(1) / det;
    }
    return res;
}

/// Alias for `Mat4<float>`.
///
using Mat4f = Mat4<float>;

/// Alias for `Mat4<double>`.
///
using Mat4d = Mat4<double>;

/// Alias for `core::Array<Mat4<T>>`.
///
template<typename T>
using Mat4Array = core::Array<Mat4<T>>;

/// Alias for `core::Array<Mat4f>`.
///
using Mat4fArray = core::Array<Mat4f>;

/// Alias for `core::Array<Mat4d>`.
///
using Mat4dArray = core::Array<Mat4d>;

/// Alias for `core::SharedConstArray<Mat4<T>>`.
///
template<typename T>
using SharedConstMat4Array = core::SharedConstArray<Mat4<T>>;

/// Alias for `core::SharedConstArray<Mat4f>`.
///
using SharedConstMat4fArray = core::SharedConstArray<Mat4f>;

/// Alias for `core::SharedConstArray<Mat4d>`.
///
using SharedConstMat4dArray = core::SharedConstArray<Mat4d>;

/// Allows to iterate over a range of `Mat4<T>` stored in a memory buffer of
/// `T` elements, where consecutive `Mat4<T>` elements are separated by a given
/// stride.
///
template<typename T>
using Mat4Span = StrideSpan<T, Mat4<T>>;

/// Alias for `Mat4Span<float>`.
///
using Mat4fSpan = Mat4Span<float>;

/// Alias for `Mat4Span<double>`.
///
using Mat4dSpan = Mat4Span<float>;

/// Const version of `Mat4Span`.
///
template<typename T>
using Mat4ConstSpan = StrideSpan<const T, const Mat4<T>>;

/// Alias for `Mat4ConstSpan<float>`.
///
using Mat4fConstSpan = Mat4ConstSpan<float>;

/// Alias for `Mat4ConstSpan<double>`.
///
using Mat4dConstSpan = Mat4ConstSpan<double>;

/// Overloads `setZero(T& x)`.
///
/// \sa `vgc::core::zero<T>()`.
///
template<typename T>
inline void setZero(Mat4<T>& m) {
    m.setToZero();
}

/// Writes the given `Mat4` to the output stream.
///
template<typename T, typename OStream>
void write(OStream& out, const Mat4<T>& m) {
    static const char* s = ", ";
    // clang-format off
    write(out,
          "((", m(0,0), s, m(0,1), s, m(0,2), s, m(0,3), "),",
          " (", m(1,0), s, m(1,1), s, m(1,2), s, m(1,3), "),",
          " (", m(2,0), s, m(2,1), s, m(2,2), s, m(2,3), "),",
          " (", m(3,0), s, m(3,1), s, m(3,2), s, m(3,3), "))");
    // clang-format on
}

namespace detail {

template<typename T, typename IStream>
void readToMatRow(Mat4<T>& m, Int i, IStream& in) {
    Vec4<T> v;
    readTo(v, in);
    m(i, 0) = v[0];
    m(i, 1) = v[1];
    m(i, 2) = v[2];
    m(i, 3) = v[3];
}

} // namespace detail

/// Reads a `Mat4` from the input stream, and stores it in the given output
/// parameter `m`. Leading whitespaces are allowed. Raises `ParseError` if the
/// stream does not start with a `Mat4`. Raises `RangeError` if one of its
/// coordinates is outside the representable range of a `T`.
///
template<typename T, typename IStream>
void readTo(Mat4<T>& m, IStream& in) {
    skipWhitespacesAndExpectedCharacter(in, '(');
    detail::readToMatRow(m, 0, in);
    skipWhitespacesAndExpectedCharacter(in, ',');
    detail::readToMatRow(m, 1, in);
    skipWhitespacesAndExpectedCharacter(in, ',');
    detail::readToMatRow(m, 2, in);
    skipWhitespacesAndExpectedCharacter(in, ',');
    detail::readToMatRow(m, 3, in);
    skipExpectedCharacter(in, ')');
}

} // namespace vgc::geometry

template<typename T>
struct fmt::formatter<vgc::geometry::Mat4<T>> {
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && *it != '}') {
            throw format_error("invalid format");
        }
        return it;
    }
    template<typename FormatContext>
    auto format(const vgc::geometry::Mat4<T>& m, FormatContext& ctx) {
        // clang-format off
        return format_to(ctx.out(),
                         "(({}, {}, {}, {}),"
                         " ({}, {}, {}, {}),"
                         " ({}, {}, {}, {}),"
                         " ({}, {}, {}, {}))",
                         m(0,0), m(0,1), m(0,2), m(0,3),
                         m(1,0), m(1,1), m(1,2), m(1,3),
                         m(2,0), m(2,1), m(2,2), m(2,3),
                         m(3,0), m(3,1), m(3,2), m(3,3));
        // clang-format on
    }
};

#endif // VGC_GEOMETRY_MAT4_H
