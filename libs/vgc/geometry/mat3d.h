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
// Instead, edit tools/mat3x.h then run tools/generate.py.

#ifndef VGC_GEOMETRY_MAT3D_H
#define VGC_GEOMETRY_MAT3D_H

#include <cmath>

#include <vgc/core/array.h>
#include <vgc/geometry/api.h>
#include <vgc/geometry/mat.h>
#include <vgc/geometry/stride.h>
#include <vgc/geometry/vec2d.h>
#include <vgc/geometry/vec3d.h>

namespace vgc::geometry {

/// \class vgc::geometry::Mat3d
/// \brief 3x3 matrix using double-precision floating points.
///
/// A Mat3d represents a 3x3 matrix in column-major format.
///
/// The memory size of a Mat3d is exactly 9 * sizeof(double). This will never
/// change in any future version, as this allows to conveniently use this class
/// for data transfer to the GPU (via OpenGL, Metal, etc.).
///
/// Unlike in the Eigen library, VGC has chosen not to distinguish between 3x3
/// matrices and 2D affine transformations in homogeneous coordinates. In other
/// words, if you wish to represent a 2D affine transformation, simply use a
/// Mat3d.
///
// VGC_GEOMETRY_API <- Omitted on purpose, otherwise we couldn't define `identity`.
//                     Instead, we manually export functions defined in the .cpp.
class Mat3d
{
public:
    using ScalarType = double;
    static constexpr Int dimension = 3;

    /// Creates an uninitialized `Mat3d`.
    ///
    Mat3d(core::NoInit) {}

    /// Creates a `Mat3d` initialized to the null matrix `Mat3d(0)`.
    ///
    constexpr Mat3d() : Mat3d(0) {}

    /// Creates a Mat3d initialized with the given arguments.
    ///
    constexpr Mat3d(double m11, double m12, double m13,
                    double m21, double m22, double m23,
                    double m31, double m32, double m33)
        : data_{{m11, m21, m31},
                {m12, m22, m32},
                {m13, m23, m33}} {}

    /// Creates a diagonal matrix with diagonal elements equal to the given
    /// value. As specific cases, the null matrix is Mat3d(0), and the identity
    /// matrix is Mat3d(1).
    ///
    explicit constexpr Mat3d(double d)
        : data_{{d, 0, 0},
                {0, d, 0},
                {0, 0, d}} {}

    /// Creates a `Mat3d` from another `Mat<3, T>` object by performing a
    /// `static_cast` on each of its elements.
    ///
    template<typename TMat3, VGC_REQUIRES(
                 isMat<TMat3> &&
                 TMat3::dimension == 3 &&
                 !std::is_same_v<TMat3, Mat3d>)>
    explicit constexpr Mat3d(const TMat3& other)
        : data_{{static_cast<double>(other(0, 0)),
                 static_cast<double>(other(1, 0)),
                 static_cast<double>(other(2, 0))},
                {static_cast<double>(other(0, 1)),
                 static_cast<double>(other(1, 1)),
                 static_cast<double>(other(2, 1))},
                {static_cast<double>(other(0, 2)),
                 static_cast<double>(other(1, 2)),
                 static_cast<double>(other(2, 2))}} {}

    /// Defines explicitely all the elements of the matrix
    ///
    Mat3d& setElements(double m11, double m12, double m13,
                       double m21, double m22, double m23,
                       double m31, double m32, double m33) {
        data_[0][0] = m11; data_[0][1] = m21; data_[0][2] = m31;
        data_[1][0] = m12; data_[1][1] = m22; data_[1][2] = m32;
        data_[2][0] = m13; data_[2][1] = m23; data_[2][2] = m33;
        return *this;
    }

    /// Sets this Mat3d to a diagonal matrix with all diagonal elements equal to
    /// the given value.
    ///
    Mat3d& setToDiagonal(double d) {
        return setElements(d, 0, 0,
                           0, d, 0,
                           0, 0, d);
    }

    /// Sets this Mat3d to the zero matrix.
    ///
    Mat3d& setToZero() { return setToDiagonal(0); }

    /// Sets this Mat3d to the identity matrix.
    ///
    Mat3d& setToIdentity() { return setToDiagonal(1); }

    /// The identity matrix Mat3d(1).
    ///
    static const Mat3d identity;

    /// Returns a pointer to the underlying (colum-major ordered) array of components.
    ///
    const double* data() const { return data_[0]; }

    /// Returns a pointer to the underlying (colum-major ordered) array of components.
    ///
    double* data() { return data_[0]; }

    /// Accesses the component of the Mat3d the the i-th row and j-th column.
    ///
    const double& operator()(Int i, Int j) const { return data_[j][i]; }

    /// Mutates the component of the Mat3d the the i-th row and j-th column.
    ///
    double& operator()(Int i, Int j) { return data_[j][i]; }

    /// Adds in-place the \p other Mat3d to this Mat3d.
    ///
    Mat3d& operator+=(const Mat3d& other) {
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

    /// Returns the addition of the Mat3d \p m1 and the Mat3d \p m2.
    ///
    friend Mat3d operator+(const Mat3d& m1, const Mat3d& m2) {
        return Mat3d(m1) += m2;
    }

    /// Returns a copy of this Mat3d (unary plus operator).
    ///
    Mat3d operator+() const {
        return *this;
    }

    /// Substracts in-place the \p other Mat3d to this Mat3d.
    ///
    Mat3d& operator-=(const Mat3d& other) {
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

    /// Returns the substraction of the Mat3d \p m1 and the Mat3d \p m2.
    ///
    friend Mat3d operator-(const Mat3d& m1, const Mat3d& m2) {
        return Mat3d(m1) -= m2;
    }

    /// Returns the opposite of this Mat3d (unary minus operator).
    ///
    Mat3d operator-() const {
        return Mat3d(*this) *= -1;
    }

    /// Multiplies in-place the \p other Mat3d to this Mat3d.
    ///
    Mat3d& operator*=(const Mat3d& other) {
        *this = (*this) * other;
        return *this;
    }

    /// Returns the multiplication of the Mat3d \p m1 and the Mat3d \p m2.
    ///
    friend Mat3d operator*(const Mat3d& m1, const Mat3d& m2) {
        Mat3d res;
        res(0,0) = m1(0,0)*m2(0,0) + m1(0,1)*m2(1,0) + m1(0,2)*m2(2,0);
        res(0,1) = m1(0,0)*m2(0,1) + m1(0,1)*m2(1,1) + m1(0,2)*m2(2,1);
        res(0,2) = m1(0,0)*m2(0,2) + m1(0,1)*m2(1,2) + m1(0,2)*m2(2,2);
        res(1,0) = m1(1,0)*m2(0,0) + m1(1,1)*m2(1,0) + m1(1,2)*m2(2,0);
        res(1,1) = m1(1,0)*m2(0,1) + m1(1,1)*m2(1,1) + m1(1,2)*m2(2,1);
        res(1,2) = m1(1,0)*m2(0,2) + m1(1,1)*m2(1,2) + m1(1,2)*m2(2,2);
        res(2,0) = m1(2,0)*m2(0,0) + m1(2,1)*m2(1,0) + m1(2,2)*m2(2,0);
        res(2,1) = m1(2,0)*m2(0,1) + m1(2,1)*m2(1,1) + m1(2,2)*m2(2,1);
        res(2,2) = m1(2,0)*m2(0,2) + m1(2,1)*m2(1,2) + m1(2,2)*m2(2,2);
        return res;
    }

    /// Multiplies in-place this Mat3d by the given scalar \p s.
    ///
    Mat3d& operator*=(double s) {
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

    /// Returns the multiplication of this Mat3d by the given scalar \p s.
    ///
    Mat3d operator*(double s) const {
        return Mat3d(*this) *= s;
    }

    /// Returns the multiplication of the scalar \p s with the Mat3d \p m.
    ///
    friend Mat3d operator*(double s, const Mat3d& m) {
        return m * s;
    }

    /// Divides in-place this Mat3d by the given scalar \p s.
    ///
    Mat3d& operator/=(double s) {
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

    /// Returns the division of this Mat3d by the given scalar \p s.
    ///
    Mat3d operator/(double s) const {
        return Mat3d(*this) /= s;
    }

    /// Returns whether the two given Mat3d \p m1 and \p m2 are equal.
    ///
    friend bool operator==(const Mat3d& m1, const Mat3d& m2) {
        return m1.data_[0][0] == m2.data_[0][0] &&
               m1.data_[0][1] == m2.data_[0][1] &&
               m1.data_[0][2] == m2.data_[0][2] &&
               m1.data_[1][0] == m2.data_[1][0] &&
               m1.data_[1][1] == m2.data_[1][1] &&
               m1.data_[1][2] == m2.data_[1][2] &&
               m1.data_[2][0] == m2.data_[2][0] &&
               m1.data_[2][1] == m2.data_[2][1] &&
               m1.data_[2][2] == m2.data_[2][2];
    }

    /// Returns whether the two given Mat3d \p m1 and \p m2 are different.
    ///
    friend bool operator!=(const Mat3d& m1, const Mat3d& m2) {
        return m1.data_[0][0] != m2.data_[0][0] ||
               m1.data_[0][1] != m2.data_[0][1] ||
               m1.data_[0][2] != m2.data_[0][2] ||
               m1.data_[1][0] != m2.data_[1][0] ||
               m1.data_[1][1] != m2.data_[1][1] ||
               m1.data_[1][2] != m2.data_[1][2] ||
               m1.data_[2][0] != m2.data_[2][0] ||
               m1.data_[2][1] != m2.data_[2][1] ||
               m1.data_[2][2] != m2.data_[2][2];
    }

    /// Returns the multiplication of this Mat3d by the given Vec3d \p v.
    ///
    Vec3d operator*(const Vec3d& v) const {
        return Vec3d(
            data_[0][0]*v[0] + data_[1][0]*v[1] + data_[2][0]*v[2],
            data_[0][1]*v[0] + data_[1][1]*v[1] + data_[2][1]*v[2],
            data_[0][2]*v[0] + data_[1][2]*v[1] + data_[2][2]*v[2]);
    }

    /// Returns the result of transforming the given `Vec2d` by this `Mat3d`
    /// interpreted as a 2D projective transformation.
    ///
    /// This is equivalent to multiplying this `Mat3d` by `Vec3d(x, y, 1)`,
    /// then returning the first two coordinates divided by the third
    /// coordinate.
    ///
    Vec2d transformPoint(const Vec2d& v) const {
        double x = data_[0][0]*v[0] + data_[1][0]*v[1] + data_[2][0];
        double y = data_[0][1]*v[0] + data_[1][1]*v[1] + data_[2][1];
        double w = data_[0][2]*v[0] + data_[1][2]*v[1] + data_[2][2];
        double iw = 1.0 / w;
        return Vec2d(iw * x, iw * y);
    }

    /// Returns the result of transforming the given `Vec2d` by this `Mat3d`
    /// interpreted as a 2D affine transformation, that is, ignoring the
    /// projective components.
    ///
    /// This is equivalent to multiplying the 2x3 submatrix of this `Mat3d` by
    /// `Vec3d(x, y, 1)`.
    ///
    /// This can be used as a faster version of `transformPoint()` whenever you
    /// know that the last row of the matrix is equal to `[0, 0, 1]`, or
    /// whenever you prefer to behave as if the last row was `[0, 0, 1]`.
    ///
    Vec2d transformPointAffine(const Vec2d& v) const {
        double x = data_[0][0]*v[0] + data_[1][0]*v[1] + data_[2][0];
        double y = data_[0][1]*v[0] + data_[1][1]*v[1] + data_[2][1];
        return Vec2d(x, y);
    }

    /// Returns the inverse of this Mat3d.
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
    /// has all its elements set to std::numeric_limits<double>::infinity().
    ///
    VGC_GEOMETRY_API
    Mat3d inverted(bool* isInvertible = nullptr, double epsilon = 0) const;

    /// Right-multiplies this matrix by the translation matrix given
    /// by vx and vy, that is:
    ///
    /// \verbatim
    /// | 1 0 vx |
    /// | 0 1 vy |
    /// | 0 0 1  |
    /// \endverbatim
    ///
    /// Returns a reference to this Mat3d.
    ///
    Mat3d& translate(double vx, double vy = 0) {
        data_[2][0] += vx*data_[0][0] + vy*data_[1][0];
        data_[2][1] += vx*data_[0][1] + vy*data_[1][1];
        data_[2][2] += vx*data_[0][2] + vy*data_[1][2];
        return *this;
    }

    /// Overloads `Mat3d::translate(double, double)`.
    ///
    Mat3d& translate(const Vec2d& v) {
        return translate(v.x(), v.y());
    }

    /// Right-multiplies this matrix by the rotation matrix around
    /// the z-axis by \p t radians, that is:
    ///
    /// \verbatim
    /// | cos(t) -sin(t)  0 |
    /// | sin(t)  cos(t)  0 |
    /// | 0       0       1 |
    /// \endverbatim
    ///
    /// Returns a reference to this Mat3d.
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
    Mat3d& rotate(double t, bool orthosnap = true) {
        static double eps = std::numeric_limits<double>::epsilon();
        double c = std::cos(t);
        double s = std::sin(t);
        if (orthosnap && (std::abs(c) < eps || std::abs(s) < eps)) {
            double cr = std::round(c);
            double sr = std::round(s);
            c = cr;
            s = sr;
        }
        Mat3d m(c,-s, 0,
                s, c, 0,
                0, 0, 1);
        return (*this) *= m;
    }

    /// Right-multiplies this matrix by the uniform scaling matrix
    /// given by s, that is:
    ///
    /// \verbatim
    /// | s 0 0 |
    /// | 0 s 0 |
    /// | 0 0 1 |
    /// \endverbatim
    ///
    /// Returns a reference to this Mat3d.
    ///
    /// Note: if your 3x3 matrix is not meant to represent a 2D affine
    /// transformation, simply use the following code instead (multiplication
    /// by scalar), which also multiplies the last row and column:
    ///
    /// \code
    /// m *= s;
    /// \endcode
    ///
    Mat3d& scale(double s) {
        Mat3d m(s, 0, 0,
                0, s, 0,
                0, 0, 1);
        return (*this) *= m;
    }

    /// Right-multiplies this matrix by the non-uniform scaling matrix
    /// given by sx and sy, that is:
    ///
    /// \verbatim
    /// | sx 0  0 |
    /// | 0  sy 0 |
    /// | 0  0  1 |
    /// \endverbatim
    ///
    /// Returns a reference to this Mat3d.
    ///
    Mat3d& scale(double sx, double sy) {
        Mat3d m(sx, 0,  0,
                0,  sy, 0,
                0,  0,  1);
        return (*this) *= m;
    }

    /// Overloads `Mat3d::scale(double, double)`.
    ///
    Mat3d& scale(const Vec2d& v) {
        return scale(v.x(), v.y());
    }

private:
    double data_[3][3];
};

inline constexpr Mat3d Mat3d::identity = Mat3d(1);

/// Alias for vgc::core::Array<vgc::geometry::Mat3d>.
///
using Mat3dArray = core::Array<Mat3d>;

/// Allows to iterate over a range of `Mat3d` stored in a memory buffer of
/// doubles, where consecutive `Mat3d` elements are separated by a given stride.
///
using Mat3dSpan = StrideSpan<double, Mat3d>;

/// Const version of `Mat3dSpan`.
///
using Mat3dConstSpan = StrideSpan<const double, const Mat3d>;

/// Overloads setZero(T& x).
///
/// \sa vgc::core::zero<T>()
///
inline void setZero(Mat3d& m) {
    m.setToZero();
}

/// Writes the given Mat3d to the output stream.
///
template<typename OStream>
void write(OStream& out, const Mat3d& m) {
    static const char* s = ", ";
    write(out, '[', m(0,0), s, m(0,1), s, m(0,2), s,
                    m(1,0), s, m(1,1), s, m(1,2), s,
                    m(2,0), s, m(2,1), s, m(2,2), ']');
}

} // namespace vgc::geometry

// see https://fmt.dev/latest/api.html#formatting-user-defined-types
template <>
struct fmt::formatter<vgc::geometry::Mat3d> {
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && *it != '}')
            throw format_error("invalid format");
        return it;
    }
    template <typename FormatContext>
    auto format(const vgc::geometry::Mat3d m, FormatContext& ctx) {
        return format_to(ctx.out(),"[{}, {}, {},"
                                   " {}, {}, {},"
                                   " {}, {}, {}]",
                         m(0,0), m(0,1), m(0,2),
                         m(1,0), m(1,1), m(1,2),
                         m(2,0), m(2,1), m(2,2));
    }
};

// TODO: parse from string

#endif // VGC_GEOMETRY_MAT3D_H
