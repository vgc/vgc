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

#ifndef VGC_GEOMETRY_MAT2_H
#define VGC_GEOMETRY_MAT2_H

#include <cmath>

#include <vgc/core/array.h>
#include <vgc/geometry/api.h>
#include <vgc/geometry/mat.h>
#include <vgc/geometry/stride.h>
#include <vgc/geometry/vec2.h>

namespace vgc::geometry {

/// \class vgc::geometry::Mat2
/// \brief Represents a 2x2 matrix.
///
/// A `Mat2<T>` represents a 2x2 matrix in column-major format.
///
/// The memory size of a `Mat2<T>` is exactly `4 * sizeof(T)`. This will never
/// change in any future version, as this allows to conveniently use this class
/// for data transfer to the GPU (via OpenGL, Metal, etc.).
///
template<typename T>
class Mat2 {
public:
    using ScalarType = T;
    static constexpr Int dimension = 2;

    VGC_WARNING_PUSH
    VGC_WARNING_MSVC_DISABLE(26495) // member variable uninitialized
    /// Creates an uninitialized `Mat2`.
    ///
    Mat2(core::NoInit) noexcept {
    }
    VGC_WARNING_POP

    /// Creates a `Mat2` initialized to the null matrix `Mat2(0)`.
    ///
    constexpr Mat2() noexcept
        : Mat2(0) {
    }

    /// Creates a `Mat2` initialized with the given arguments.
    ///
    // clang-format off
    constexpr Mat2(
        T m00, T m01,
        T m10, T m11) noexcept

        : data_{{m00, m10},
                {m01, m11}} {
    }
    // clang-format on

    /// Creates a `Mat2` initialized with the given row vectors.
    ///
    constexpr Mat2(const Vec2<T>& v0, const Vec2<T>& v1) noexcept
        : data_{
            {v0[0], v1[0]}, //
            {v0[1], v1[1]}} {
    }

    /// Creates a diagonal matrix with diagonal elements equal to the given
    /// value. As specific cases, the null matrix is `Mat2(0)`, and the
    /// identity matrix is `Mat2(1)`.
    ///
    constexpr explicit Mat2(T d) noexcept
        : data_{
            {d, 0}, //
            {0, d}} {
    }

    /// Creates a `Mat2<T>` from another `Mat2<U>` object by performing a
    /// `static_cast` on each of its elements.
    ///
    template<typename U, VGC_REQUIRES(!std::is_same_v<T, U>)>
    constexpr explicit Mat2(const Mat2<U>& other) noexcept
        : data_{
            {static_cast<T>(other(0, 0)), static_cast<T>(other(1, 0))},
            {static_cast<T>(other(0, 1)), static_cast<T>(other(1, 1))}} {
    }

    /// Modifies all the elements of this matrix.
    ///
    // clang-format off
    Mat2& setElements(
        T m00, T m01,
        T m10, T m11) {

        data_[0][0] = m00; data_[0][1] = m10;
        data_[1][0] = m01; data_[1][1] = m11;
        return *this;
    }
    // clang-format on

    /// Sets this matrix to a diagonal matrix with all diagonal elements equal
    /// to the given value.
    ///
    Mat2& setToDiagonal(T d) {
        // clang-format off
        return setElements(
            d, 0,
            0, d);
        // clang-format on
    }

    /// Sets this matrix to the zero matrix.
    ///
    Mat2& setToZero() {
        return setToDiagonal(0);
    }

    /// Sets this matrix to the identity matrix.
    ///
    Mat2& setToIdentity() {
        return setToDiagonal(1);
    }

    /// The identity matrix `Mat2(1)`.
    ///
    static const Mat2 identity;

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
    Mat2& operator+=(const Mat2& other) {
        data_[0][0] += other.data_[0][0];
        data_[0][1] += other.data_[0][1];
        data_[1][0] += other.data_[1][0];
        data_[1][1] += other.data_[1][1];
        return *this;
    }

    /// Returns the addition of the matrix `m1` and the matrix `m2`.
    ///
    friend Mat2 operator+(const Mat2& m1, const Mat2& m2) {
        return Mat2(m1) += m2;
    }

    /// Returns a copy of this matrix (unary plus operator).
    ///
    Mat2 operator+() const {
        return *this;
    }

    /// Substracts in-place the `other` matrix from this matrix.
    ///
    Mat2& operator-=(const Mat2& other) {
        data_[0][0] -= other.data_[0][0];
        data_[0][1] -= other.data_[0][1];
        data_[1][0] -= other.data_[1][0];
        data_[1][1] -= other.data_[1][1];
        return *this;
    }

    /// Returns the substraction of the matrix `m1` and the matrix `m2`.
    ///
    friend Mat2 operator-(const Mat2& m1, const Mat2& m2) {
        return Mat2(m1) -= m2;
    }

    /// Returns the opposite of this matrix (unary minus operator).
    ///
    Mat2 operator-() const {
        return Mat2(*this) *= -1;
    }

    /// Multiplies in-place this matrix with the `other` matrix.
    ///
    Mat2& operator*=(const Mat2& other) {
        *this = (*this) * other;
        return *this;
    }

    /// Returns the multiplication of the matrix `m1` and the matrix `m2`.
    ///
    friend Mat2 operator*(const Mat2& m1, const Mat2& m2) {
        Mat2 res;
        res(0, 0) = m1(0, 0) * m2(0, 0) + m1(0, 1) * m2(1, 0);
        res(0, 1) = m1(0, 0) * m2(0, 1) + m1(0, 1) * m2(1, 1);
        res(1, 0) = m1(1, 0) * m2(0, 0) + m1(1, 1) * m2(1, 0);
        res(1, 1) = m1(1, 0) * m2(0, 1) + m1(1, 1) * m2(1, 1);
        return res;
    }

    /// Multiplies in-place this matrix by the given scalar `s`.
    ///
    Mat2& operator*=(T s) {
        data_[0][0] *= s;
        data_[0][1] *= s;
        data_[1][0] *= s;
        data_[1][1] *= s;
        return *this;
    }

    /// Returns the multiplication of this matrix by the given scalar `s`.
    ///
    Mat2 operator*(T s) const {
        return Mat2(*this) *= s;
    }

    /// Returns the multiplication of the scalar `s` with the matrix `m`.
    ///
    friend Mat2 operator*(T s, const Mat2& m) {
        return m * s;
    }

    /// Divides in-place this matrix by the given scalar `s`.
    ///
    Mat2& operator/=(T s) {
        data_[0][0] /= s;
        data_[0][1] /= s;
        data_[1][0] /= s;
        data_[1][1] /= s;
        return *this;
    }

    /// Returns the division of this matrix by the given scalar `s`.
    ///
    Mat2 operator/(T s) const {
        return Mat2(*this) /= s;
    }

    /// Returns whether the two given matrices `m1` and `m2` are equal.
    ///
    friend bool operator==(const Mat2& m1, const Mat2& m2) {
        return m1.data_[0][0] == m2.data_[0][0] && m1.data_[0][1] == m2.data_[0][1]
               && m1.data_[1][0] == m2.data_[1][0] && m1.data_[1][1] == m2.data_[1][1];
    }

    /// Returns whether the two given matrices `m1` and `m2` are different.
    ///
    friend bool operator!=(const Mat2& m1, const Mat2& m2) {
        return m1.data_[0][0] != m2.data_[0][0] || m1.data_[0][1] != m2.data_[0][1]
               || m1.data_[1][0] != m2.data_[1][0] || m1.data_[1][1] != m2.data_[1][1];
    }

    /// Returns the multiplication of this `Mat2` by the given `Vec2`.
    ///
    Vec2<T> operator*(const Vec2<T>& v) const {
        return Vec2<T>(
            data_[0][0] * v[0] + data_[1][0] * v[1],
            data_[0][1] * v[0] + data_[1][1] * v[1]);
    }

    /// Returns the result of transforming the given scalar `x` by this `Mat2`
    /// interpreted as a 1D projective transformation.
    ///
    /// This is equivalent to multiplying this `Mat2` by `Vec2(x, 1)`,
    /// then returning the first coordinate divided by the second
    /// coordinate.
    ///
    /// ```cpp
    /// Mat2d m(a, b,
    ///         c, d);
    /// double x;
    /// double y = m.transform(x); // y = (ax + b) / (cx + d)
    /// ```
    ///
    T transform(T x) const {
        T x_ = data_[0][0] * x + data_[1][0];
        T w_ = data_[0][1] * x + data_[1][1];
        return x_ / w_;
    }

    /// Returns the result of transforming the given scalar `x` by this `Mat2`
    /// interpreted as a 1D affine transformation, that is, ignoring the
    /// projective components.
    ///
    /// This is equivalent to multiplying the 1x2 submatrix of this `Mat2` by
    /// `Vec2(x, 1)`.
    ///
    /// ```cpp
    /// Mat2d m(a, b,
    ///         c, d);
    /// double x;
    /// double y = m.transformAffine(x); // y = ax + b
    /// ```
    ///
    /// This can be used as a faster version of `transform()` whenever you
    /// know that the last row of the matrix is equal to `[0, 1]`, or
    /// whenever you prefer to behave as if the last row was `[0, 1]`.
    ///
    T transformAffine(T x) const {
        return data_[0][0] * x + data_[1][0];
    }

    /// Returns the result of transforming the given scalar `x` by the linear
    /// part of this `Mat2` interpreted as a 1D projective transformation. That
    /// is, this ignores the translation and projective components, and only
    /// applies the scale component.
    ///
    /// This is equivalent to multiplying this matrix by `Vec2(x, 0)` and returning
    /// only the first component.
    ///
    /// ```cpp
    /// Mat2d m(a, b,
    ///         c, d);
    /// double x;
    /// double y = m.transformLinear(x); // y = ax
    /// ```
    ///
    /// This is typically used to transform "directions" rather than "points".
    ///
    T transformLinear(T x) const {
        return data_[0][0] * x;
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
    Mat2 inverse(bool* isInvertible = nullptr, T epsilon = 0) const;

private:
    T data_[2][2];
};

// This definition must be out-of-class.
// See: https://stackoverflow.com/questions/11928089/
// static-constexpr-member-of-same-type-as-class-being-defined
//
template<typename T>
inline constexpr Mat2<T> Mat2<T>::identity = Mat2<T>(1);

template<typename T>
Mat2<T> Mat2<T>::inverse(bool* isInvertible, T epsilon_) const {

    Mat2 res;

    const auto& d = data_;
    auto& inv = res.data_;

    inv[0][0] = d[1][1];
    inv[1][0] = -d[1][0];

    T det = d[0][0] * inv[0][0] + d[0][1] * inv[1][0];

    if (std::abs(det) <= epsilon_) {
        if (isInvertible) {
            *isInvertible = false;
        }
        constexpr T inf = core::infinity<T>;
        res.setElements(inf, inf, inf, inf);
    }
    else {
        if (isInvertible) {
            *isInvertible = true;
        }

        inv[0][1] = -d[0][1];
        inv[1][1] = d[0][0];

        res *= T(1) / det;
    }
    return res;
}

/// Alias for `Mat2<float>`.
///
using Mat2f = Mat2<float>;

/// Alias for `Mat2<double>`.
///
using Mat2d = Mat2<double>;

/// Alias for `core::Array<Mat2<T>>`.
///
template<typename T>
using Mat2Array = core::Array<Mat2<T>>;

/// Alias for `core::Array<Mat2f>`.
///
using Mat2fArray = core::Array<Mat2f>;

/// Alias for `core::Array<Mat2d>`.
///
using Mat2dArray = core::Array<Mat2d>;

/// Alias for `core::SharedConstArray<Mat2<T>>`.
///
template<typename T>
using SharedConstMat2Array = core::SharedConstArray<Mat2<T>>;

/// Alias for `core::SharedConstArray<Mat2f>`.
///
using SharedConstMat2fArray = core::SharedConstArray<Mat2f>;

/// Alias for `core::SharedConstArray<Mat2d>`.
///
using SharedConstMat2dArray = core::SharedConstArray<Mat2d>;

/// Allows to iterate over a range of `Mat2<T>` stored in a memory buffer of
/// `T` elements, where consecutive `Mat2<T>` elements are separated by a given
/// stride.
///
template<typename T>
using Mat2Span = StrideSpan<T, Mat2<T>>;

/// Alias for `Mat2Span<float>`.
///
using Mat2fSpan = Mat2Span<float>;

/// Alias for `Mat2Span<double>`.
///
using Mat2dSpan = Mat2Span<float>;

/// Const version of `Mat2Span`.
///
template<typename T>
using Mat2ConstSpan = StrideSpan<const T, const Mat2<T>>;

/// Alias for `Mat2ConstSpan<float>`.
///
using Mat2fConstSpan = Mat2ConstSpan<float>;

/// Alias for `Mat2ConstSpan<double>`.
///
using Mat2dConstSpan = Mat2ConstSpan<double>;

/// Overloads `setZero(T& x)`.
///
/// \sa `vgc::core::zero<T>()`.
///
template<typename T>
inline void setZero(Mat2<T>& m) {
    m.setToZero();
}

/// Writes the given `Mat2` to the output stream.
///
template<typename T, typename OStream>
void write(OStream& out, const Mat2<T>& m) {
    static const char* s = ", ";
    // clang-format off
    write(out,
          "((", m(0,0), s, m(0,1), "),",
          " (", m(1,0), s, m(1,1), "))");
    // clang-format on
}

namespace detail {

template<typename T, typename IStream>
void readToMatRow(Mat2<T>& m, Int i, IStream& in) {
    Vec2<T> v;
    readTo(v, in);
    m(i, 0) = v[0];
    m(i, 1) = v[1];
}

} // namespace detail

/// Reads a `Mat2` from the input stream, and stores it in the given output
/// parameter `m`. Leading whitespaces are allowed. Raises `ParseError` if the
/// stream does not start with a `Mat2`. Raises `RangeError` if one of its
/// coordinates is outside the representable range of a `T`.
///
template<typename T, typename IStream>
void readTo(Mat2<T>& m, IStream& in) {
    skipWhitespacesAndExpectedCharacter(in, '(');
    detail::readToMatRow(m, 0, in);
    skipWhitespacesAndExpectedCharacter(in, ',');
    detail::readToMatRow(m, 1, in);
    skipExpectedCharacter(in, ')');
}

} // namespace vgc::geometry

template<typename T>
struct fmt::formatter<vgc::geometry::Mat2<T>> {
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && *it != '}') {
            throw format_error("invalid format");
        }
        return it;
    }
    template<typename FormatContext>
    auto format(const vgc::geometry::Mat2<T>& m, FormatContext& ctx) {
        // clang-format off
        return format_to(ctx.out(),
                         "(({}, {}),"
                         " ({}, {}))",
                         m(0,0), m(0,1),
                         m(1,0), m(1,1));
        // clang-format on
    }
};

#endif // VGC_GEOMETRY_MAT2_H
