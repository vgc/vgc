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

// This file is used to generate all the variants of this class.
// You must manually run generate.py after any modification.

// clang-format off

#ifndef VGC_GEOMETRY_MAT2X_H
#define VGC_GEOMETRY_MAT2X_H

#include <cmath>

#include <vgc/core/array.h>
#include <vgc/geometry/api.h>
#include <vgc/geometry/mat.h>
#include <vgc/geometry/stride.h>
#include "vec2x.h"

namespace vgc::geometry {

/// \class vgc::geometry::Mat2x
/// \brief 2x2 matrix using %scalar-type-description%.
///
/// A Mat2x represents a 2x2 matrix in column-major format.
///
/// The memory size of a Mat2x is exactly 4 * sizeof(float). This will never
/// change in any future version, as this allows to conveniently use this class
/// for data transfer to the GPU (via OpenGL, Metal, etc.).
///
// VGC_GEOMETRY_API <- Omitted on purpose, otherwise we couldn't define `identity`.
//                     Instead, we manually export functions defined in the .cpp.
class Mat2x {
public:
    using ScalarType = float;
    static constexpr Int dimension = 2;

    VGC_WARNING_PUSH
    VGC_WARNING_MSVC_DISABLE(26495) // member variable uninitialized
    /// Creates an uninitialized `Mat2x`.
    ///
    Mat2x(core::NoInit) noexcept {
    }
    VGC_WARNING_POP

    /// Creates a `Mat2x` initialized to the null matrix `Mat2x(0)`.
    ///
    constexpr Mat2x() noexcept
        : Mat2x(0) {
    }

    /// Creates a Mat2x initialized with the given arguments.
    ///
    constexpr Mat2x(
        float m11, float m12,
        float m21, float m22) noexcept

        : data_{{m11, m21},
                {m12, m22}} {
    }

    /// Creates a diagonal matrix with diagonal elements equal to the given
    /// value. As specific cases, the null matrix is Mat2x(0), and the identity
    /// matrix is Mat2x(1).
    ///
    explicit constexpr Mat2x(float d) noexcept
        : data_{{d, 0},
                {0, d}} {
    }

    /// Creates a `Mat2x` from another `Mat<2, T>` object by performing a
    /// `static_cast` on each of its elements.
    ///
    template<typename TMat2,
        VGC_REQUIRES(
            isMat<TMat2>
         && TMat2::dimension == 2
         && !std::is_same_v<TMat2, Mat2x>)>
    explicit constexpr Mat2x(const TMat2& other) noexcept
        : data_{{static_cast<float>(other(0, 0)),
                 static_cast<float>(other(1, 0))},
                {static_cast<float>(other(0, 1)),
                 static_cast<float>(other(1, 1))}} {
    }

    /// Defines explicitely all the elements of the matrix
    ///
    Mat2x& setElements(
        float m11, float m12,
        float m21, float m22) {

        data_[0][0] = m11; data_[0][1] = m21;
        data_[1][0] = m12; data_[1][1] = m22;
        return *this;
    }

    /// Sets this Mat2x to a diagonal matrix with all diagonal elements equal to
    /// the given value.
    ///
    Mat2x& setToDiagonal(float d) {
        return setElements(
            d, 0,
            0, d);
    }

    /// Sets this Mat2x to the zero matrix.
    ///
    Mat2x& setToZero() {
        return setToDiagonal(0);
    }

    /// Sets this Mat2x to the identity matrix.
    ///
    Mat2x& setToIdentity() {
        return setToDiagonal(1);
    }

    /// The identity matrix Mat2x(1).
    ///
    static const Mat2x identity;

    /// Accesses the component of the Mat2x at the `i`-th row and `j`-th column.
    ///
    const float& operator()(Int i, Int j) const {
        return data_[j][i];
    }

    /// Mutates the component of the Mat2x at the `i`-th row and `j`-th column.
    ///
    float& operator()(Int i, Int j) {
        return data_[j][i];
    }

    /// Adds in-place the \p other Mat2x to this Mat2x.
    ///
    Mat2x& operator+=(const Mat2x& other) {
        data_[0][0] += other.data_[0][0];
        data_[0][1] += other.data_[0][1];
        data_[1][0] += other.data_[1][0];
        data_[1][1] += other.data_[1][1];
        return *this;
    }

    /// Returns the addition of the Mat2x \p m1 and the Mat2x \p m2.
    ///
    friend Mat2x operator+(const Mat2x& m1, const Mat2x& m2) {
        return Mat2x(m1) += m2;
    }

    /// Returns a copy of this Mat2x (unary plus operator).
    ///
    Mat2x operator+() const {
        return *this;
    }

    /// Substracts in-place the \p other Mat2x to this Mat2x.
    ///
    Mat2x& operator-=(const Mat2x& other) {
        data_[0][0] -= other.data_[0][0];
        data_[0][1] -= other.data_[0][1];
        data_[1][0] -= other.data_[1][0];
        data_[1][1] -= other.data_[1][1];
        return *this;
    }

    /// Returns the substraction of the Mat2x \p m1 and the Mat2x \p m2.
    ///
    friend Mat2x operator-(const Mat2x& m1, const Mat2x& m2) {
        return Mat2x(m1) -= m2;
    }

    /// Returns the opposite of this Mat2x (unary minus operator).
    ///
    Mat2x operator-() const {
        return Mat2x(*this) *= -1;
    }

    /// Multiplies in-place the \p other Mat2x to this Mat2x.
    ///
    Mat2x& operator*=(const Mat2x& other) {
        *this = (*this) * other;
        return *this;
    }

    /// Returns the multiplication of the Mat2x \p m1 and the Mat2x \p m2.
    ///
    friend Mat2x operator*(const Mat2x& m1, const Mat2x& m2) {
        Mat2x res;
        res(0,0) = m1(0,0)*m2(0,0) + m1(0,1)*m2(1,0);
        res(0,1) = m1(0,0)*m2(0,1) + m1(0,1)*m2(1,1);
        res(1,0) = m1(1,0)*m2(0,0) + m1(1,1)*m2(1,0);
        res(1,1) = m1(1,0)*m2(0,1) + m1(1,1)*m2(1,1);
        return res;
    }

    /// Multiplies in-place this Mat2x by the given scalar \p s.
    ///
    Mat2x& operator*=(float s) {
        data_[0][0] *= s;
        data_[0][1] *= s;
        data_[1][0] *= s;
        data_[1][1] *= s;
        return *this;
    }

    /// Returns the multiplication of this Mat2x by the given scalar \p s.
    ///
    Mat2x operator*(float s) const {
        return Mat2x(*this) *= s;
    }

    /// Returns the multiplication of the scalar \p s with the Mat2x \p m.
    ///
    friend Mat2x operator*(float s, const Mat2x& m) {
        return m * s;
    }

    /// Divides in-place this Mat2x by the given scalar \p s.
    ///
    Mat2x& operator/=(float s) {
        data_[0][0] /= s;
        data_[0][1] /= s;
        data_[1][0] /= s;
        data_[1][1] /= s;
        return *this;
    }

    /// Returns the division of this Mat2x by the given scalar \p s.
    ///
    Mat2x operator/(float s) const {
        return Mat2x(*this) /= s;
    }

    /// Returns whether the two given Vec2x \p v1 and \p v2 are equal.
    ///
    friend bool operator==(const Mat2x& m1, const Mat2x& m2) {
        return m1.data_[0][0] == m2.data_[0][0]
            && m1.data_[0][1] == m2.data_[0][1]
            && m1.data_[1][0] == m2.data_[1][0]
            && m1.data_[1][1] == m2.data_[1][1];
    }

    /// Returns whether the two given Vec2x \p v1 and \p v2 are different.
    ///
    friend bool operator!=(const Mat2x& m1, const Mat2x& m2) {
        return m1.data_[0][0] != m2.data_[0][0]
            || m1.data_[0][1] != m2.data_[0][1]
            || m1.data_[1][0] != m2.data_[1][0]
            || m1.data_[1][1] != m2.data_[1][1];
    }

    /// Returns the multiplication of this Mat2x by the given Vec2x \p v.
    ///
    Vec2x operator*(const Vec2x& v) const {
        return Vec2x(
            data_[0][0]*v[0] + data_[1][0]*v[1],
            data_[0][1]*v[0] + data_[1][1]*v[1]);
    }

    /// Returns the result of transforming the given `float` by this `Mat2x`
    /// interpreted as a 1D projective transformation.
    ///
    /// This is equivalent to multiplying this `Mat2x` by `Vec2x(x, 1)`,
    /// then returning the first coordinate divided by the second
    /// coordinate.
    ///
    /// ```cpp
    /// Mat2x m(a, b,
    ///         c, d);
    /// float x;
    /// float y = m.transformPoint(x); // y = (ax + b) / (cx + d)
    /// ```
    ///
    float transformPoint(float x) const {
        float x_ = data_[0][0]*x + data_[1][0];
        float w_ = data_[0][1]*x + data_[1][1];
        return x_ / w_;
    }

    /// Returns the result of transforming the given `float` by this `Mat2x`
    /// interpreted as a 1D affine transformation, that is, ignoring the
    /// projective components.
    ///
    /// This is equivalent to multiplying the 1x2 submatrix of this `Mat2x` by
    /// `Vec2x(x, 1)`.
    ///
    /// ```cpp
    /// Mat2x m(a, b,
    ///         c, d);
    /// float x;
    /// float y = m.transformPointAffine(x); // y = ax + b
    /// ```
    ///
    /// This can be used as a faster version of `transformPoint()` whenever you
    /// know that the last row of the matrix is equal to `[0, 1]`, or
    /// whenever you prefer to behave as if the last row was `[0, 1]`.
    ///
    float transformPointAffine(float x) const {
        return data_[0][0]*x + data_[1][0];
    }

    /// Returns the inverse of this Mat2x.
    ///
    /// If provided, isInvertible is set to either true or false depending on
    /// whether the matrix was considered invertible or not.
    ///
    /// The matrix is considered non-invertible whenever its determinant is
    /// less or equal than the provided epsilon. An appropriate epsilon is
    /// context-dependent, and therefore zero is used as default, which means
    /// that the matrix is considered non-invertible if and only if its
    /// determinant is exactly zero (example: the null matrix).
    ///
    /// If the matrix is considered non-invertible, then the returned matrix
    /// has all its elements set to std::numeric_limits<float>::infinity().
    ///
    VGC_GEOMETRY_API
    Mat2x inverted(bool* isInvertible = nullptr, float epsilon = 0) const;

private:
    float data_[2][2];
};

inline constexpr Mat2x Mat2x::identity = Mat2x(1);

/// Alias for vgc::core::Array<vgc::geometry::Mat2x>.
///
using Mat2xArray = core::Array<Mat2x>;

/// Allows to iterate over a range of `Mat2x` stored in a memory buffer of
/// floats, where consecutive `Mat2x` elements are separated by a given stride.
///
using Mat2xSpan = StrideSpan<float, Mat2x>;

/// Const version of `Mat2xSpan`.
///
using Mat2xConstSpan = StrideSpan<const float, const Mat2x>;

/// Overloads setZero(T& x).
///
/// \sa vgc::core::zero<T>()
///
inline void setZero(Mat2x& m) {
    m.setToZero();
}

/// Writes the given Mat2x to the output stream.
///
template<typename OStream>
void write(OStream& out, const Mat2x& m) {
    static const char* s = ", ";
    write(out, '[', m(0,0), s, m(0,1), s,
                    m(1,0), s, m(1,1), ']');
}

} // namespace vgc::geometry

// see https://fmt.dev/latest/api.html#formatting-user-defined-types
template <>
struct fmt::formatter<vgc::geometry::Mat2x> {
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && *it != '}') {
            throw format_error("invalid format");
        }
        return it;
    }
    template <typename FormatContext>
    auto format(const vgc::geometry::Mat2x& m, FormatContext& ctx) {
        return format_to(ctx.out(),"[{}, {},"
                                   " {}, {}]",
                         m(0,0), m(0,1),
                         m(1,0), m(1,1));
    }
};

// TODO: parse from string

#endif // VGC_GEOMETRY_MAT2X_H
