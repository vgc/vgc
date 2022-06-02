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

#ifndef VGC_GEOMETRY_MAT4D_H
#define VGC_GEOMETRY_MAT4D_H

#include <cmath>

#include <vgc/core/array.h>
#include <vgc/geometry/api.h>
#include <vgc/geometry/vec2d.h>

namespace vgc::geometry {

/// \class vgc::geometry::Mat4d
/// \brief 4x4 matrix using double-precision floating points.
///
/// A Mat4d represents a 4x4 matrix in column-major format.
///
/// The memory size of a Mat4d is exactly 16 * sizeof(double). This will never
/// change in any future version, as this allows to conveniently use this class
/// for data transfer to the GPU (via OpenGL, Metal, etc.).
///
/// Unlike in the Eigen library, VGC has chosen not to distinguish between 4x4
/// matrices and 3D affine transformations in homogeneous coordinates. In other
/// words, if you wish to represent a 3D affine transformation, simply use a
/// Mat4d. Also, you can even use a Mat4d to represent a 2D affine
/// transformation. For example, you can multiply a Mat4d with a Vec2d, which
/// returns the same as multiplying the matrix with the 4D vector [x, y, 0, 1].
///
// VGC_GEOMETRY_API <- Omitted on purpose, otherwise we couldn't define `identity`.
//                     Instead, we manually export functions defined in the .cpp.
class Mat4d
{
public:
    using value_type = double;

    /// Creates an uninitialized `Mat4d`.
    ///
    Mat4d(core::NoInit) {}

    /// Creates a `Mat4d` initialized to the null matrix `Mat3d(0)`.
    ///
    constexpr Mat4d() : Mat4d(0) {}

    /// Creates a Mat4d initialized with the given arguments.
    ///
    constexpr Mat4d(double m11, double m12, double m13, double m14,
                    double m21, double m22, double m23, double m24,
                    double m31, double m32, double m33, double m34,
                    double m41, double m42, double m43, double m44) :

        data_{{m11, m21, m31, m41},
              {m12, m22, m32, m42},
              {m13, m23, m33, m43},
              {m14, m24, m34, m44}}
    {

    }

    /// Creates a diagonal matrix with diagonal elements equal to the given
    /// value. As specific cases, the null matrix is Mat4d(0), and the identity
    /// matrix is Mat4d(1).
    ///
    explicit constexpr Mat4d(double d) :

        data_{{d, 0, 0, 0},
              {0, d, 0, 0},
              {0, 0, d, 0},
              {0, 0, 0, d}}
    {

    }

    /// Defines explicitely all the elements of the matrix
    ///
    Mat4d& setElements(double m11, double m12, double m13, double m14,
                       double m21, double m22, double m23, double m24,
                       double m31, double m32, double m33, double m34,
                       double m41, double m42, double m43, double m44)
    {
        data_[0][0] = m11; data_[0][1] = m21; data_[0][2] = m31; data_[0][3] = m41;
        data_[1][0] = m12; data_[1][1] = m22; data_[1][2] = m32; data_[1][3] = m42;
        data_[2][0] = m13; data_[2][1] = m23; data_[2][2] = m33; data_[2][3] = m43;
        data_[3][0] = m14; data_[3][1] = m24; data_[3][2] = m34; data_[3][3] = m44;
        return *this;
    }

    /// Sets this Mat4d to a diagonal matrix with all diagonal elements equal to
    /// the given value.
    ///
    Mat4d& setToDiagonal(double d)
    {
        return setElements(d, 0, 0, 0,
                           0, d, 0, 0,
                           0, 0, d, 0,
                           0, 0, 0, d);
    }

    /// Sets this Mat4d to the zero matrix.
    ///
    Mat4d& setToZero() { return setToDiagonal(0); }

    /// Sets this Mat4d to the identity matrix.
    ///
    Mat4d& setToIdentity() { return setToDiagonal(1); }

    /// The identity matrix Mat4d(1).
    ///
    static const Mat4d identity;

    /// Accesses the component of the Mat4d the the i-th row and j-th column.
    ///
    const double& operator()(Int i, Int j) const { return data_[j][i]; }

    /// Mutates the component of the Mat4d the the i-th row and j-th column.
    ///
    double& operator()(Int i, Int j) { return data_[j][i]; }

    /// Adds in-place the \p other Mat4d to this Mat4d.
    ///
    Mat4d& operator+=(const Mat4d& other) {
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

    /// Returns the addition of the Mat4d \p m1 and the Mat4d \p m2.
    ///
    friend Mat4d operator+(const Mat4d& m1, const Mat4d& m2) {
        return Mat4d(m1) += m2;
    }

    /// Returns a copy of this Mat4d (unary plus operator).
    ///
    Mat4d operator+() const {
        return *this;
    }

    /// Substracts in-place the \p other Mat4d to this Mat4d.
    ///
    Mat4d& operator-=(const Mat4d& other) {
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

    /// Returns the substraction of the Mat4d \p m1 and the Mat4d \p m2.
    ///
    friend Mat4d operator-(const Mat4d& m1, const Mat4d& m2) {
        return Mat4d(m1) -= m2;
    }

    /// Returns the opposite of this Mat4d (unary minus operator).
    ///
    Mat4d operator-() const {
        return Mat4d(*this) *= -1;
    }

    /// Multiplies in-place the \p other Mat4d to this Mat4d.
    ///
    Mat4d& operator*=(const Mat4d& other) {
        *this = (*this) * other;
        return *this;
    }

    /// Returns the multiplication of the Mat4d \p m1 and the Mat4d \p m2.
    ///
    friend Mat4d operator*(const Mat4d& m1, const Mat4d& m2) {
        Mat4d res;
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
        return res;
    }

    /// Multiplies in-place this Mat4d by the given scalar \p s.
    ///
    Mat4d& operator*=(double s) {
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

    /// Returns the multiplication of this Mat4d by the given scalar \p s.
    ///
    Mat4d operator*(double s) const {
        return Mat4d(*this) *= s;
    }

    /// Returns the multiplication of the scalar \p s with the Mat4d \p m.
    ///
    friend Mat4d operator*(double s, const Mat4d& m) {
        return m * s;
    }

    /// Divides in-place this Mat4d by the given scalar \p s.
    ///
    Mat4d& operator/=(double s) {
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

    /// Returns the division of this Mat4d by the given scalar \p s.
    ///
    Mat4d operator/(double s) const {
        return Mat4d(*this) /= s;
    }

    /// Returns whether the two given Vec2d \p v1 and \p v2 are equal.
    ///
    friend bool operator==(const Mat4d& m1, const Mat4d& m2) {
        return m1.data_[0][0] == m2.data_[0][0] &&
               m1.data_[0][1] == m2.data_[0][1] &&
               m1.data_[0][2] == m2.data_[0][2] &&
               m1.data_[0][3] == m2.data_[0][3] &&
               m1.data_[1][0] == m2.data_[1][0] &&
               m1.data_[1][1] == m2.data_[1][1] &&
               m1.data_[1][2] == m2.data_[1][2] &&
               m1.data_[1][3] == m2.data_[1][3] &&
               m1.data_[2][0] == m2.data_[2][0] &&
               m1.data_[2][1] == m2.data_[2][1] &&
               m1.data_[2][2] == m2.data_[2][2] &&
               m1.data_[2][3] == m2.data_[2][3] &&
               m1.data_[3][0] == m2.data_[3][0] &&
               m1.data_[3][1] == m2.data_[3][1] &&
               m1.data_[3][2] == m2.data_[3][2] &&
               m1.data_[3][3] == m2.data_[3][3];
    }

    /// Returns whether the two given Vec2d \p v1 and \p v2 are different.
    ///
    friend bool operator!=(const Mat4d& m1, const Mat4d& m2) {
        return m1.data_[0][0] != m2.data_[0][0] ||
               m1.data_[0][1] != m2.data_[0][1] ||
               m1.data_[0][2] != m2.data_[0][2] ||
               m1.data_[0][3] != m2.data_[0][3] ||
               m1.data_[1][0] != m2.data_[1][0] ||
               m1.data_[1][1] != m2.data_[1][1] ||
               m1.data_[1][2] != m2.data_[1][2] ||
               m1.data_[1][3] != m2.data_[1][3] ||
               m1.data_[2][0] != m2.data_[2][0] ||
               m1.data_[2][1] != m2.data_[2][1] ||
               m1.data_[2][2] != m2.data_[2][2] ||
               m1.data_[2][3] != m2.data_[2][3] ||
               m1.data_[3][0] != m2.data_[3][0] ||
               m1.data_[3][1] != m2.data_[3][1] ||
               m1.data_[3][2] != m2.data_[3][2] ||
               m1.data_[3][3] != m2.data_[3][3];
    }

    /// Returns the multiplication of this Mat4d by the given Vec2d \p v.
    /// This assumes that the Vec2d represents the Vec4d(x, y, 0, 1) in
    /// homogeneous coordinates, and then only returns the x and y coordinates
    /// of the result.
    ///
    Vec2d operator*(const Vec2d& v) const {
        return Vec2d(
            data_[0][0]*v[0] + data_[1][0]*v[1] +  data_[3][0],
            data_[0][1]*v[0] + data_[1][1]*v[1] +  data_[3][1]);
    }

    /// Returns the inverse of this Mat4d.
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
    Mat4d inverted(bool* isInvertible = nullptr, double epsilon = 0) const;

    /// Right-multiplies this matrix by the translation matrix given
    /// by vx, vy, and vy, that is:
    ///
    /// \verbatim
    /// | 1 0 0 vx |
    /// | 0 1 0 vy |
    /// | 0 0 1 vz |
    /// | 0 0 0 1  |
    /// \endverbatim
    ///
    /// Returns a reference to this Mat4d.
    ///
    Mat4d& translate(double vx, double vy = 0, double vz = 0) {
        Mat4d m(1, 0, 0, vx,
                0, 1, 0, vy,
                0, 0, 1, vz,
                0, 0, 0, 1);
        return (*this) *= m;
    }

    /// Right-multiplies this matrix by the rotation matrix around
    /// the z-axis by \p t radians, that is:
    ///
    /// \verbatim
    /// | cos(t) -sin(t)  0       0 |
    /// | sin(t)  cos(t)  0       0 |
    /// | 0       0       1       0 |
    /// | 0       0       0       1 |
    /// \endverbatim
    ///
    /// Returns a reference to this Mat4d.
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
    Mat4d& rotate(double t, bool orthosnap = true) {
        static double eps = std::numeric_limits<double>::epsilon();
        double c = std::cos(t);
        double s = std::sin(t);
        if (orthosnap && (std::abs(c) < eps || std::abs(s) < eps)) {
            double cr = std::round(c);
            double sr = std::round(s);
            c = cr;
            s = sr;
        }
        Mat4d m(c,-s, 0, 0,
                s, c, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1);
        return (*this) *= m;
    }

    /// Right-multiplies this matrix by the uniform scaling matrix
    /// given by s, that is:
    ///
    /// \verbatim
    /// | s 0 0 0 |
    /// | 0 s 0 0 |
    /// | 0 0 s 0 |
    /// | 0 0 0 1 |
    /// \endverbatim
    ///
    /// Returns a reference to this Mat4d.
    ///
    /// Note: if your 4x4 matrix is not meant to represent a 3D affine
    /// transformation, simply use the following code instead (multiplication
    /// by scalar), which also multiplies the last row and column:
    ///
    /// \code
    /// m *= s;
    /// \endcode
    ///
    Mat4d& scale(double s) {
        Mat4d m(s, 0, 0, 0,
                0, s, 0, 0,
                0, 0, s, 0,
                0, 0, 0, 1);
        return (*this) *= m;
    }

    /// Right-multiplies this matrix by the non-uniform scaling matrix
    /// given by sx, sy, and sz, that is:
    ///
    /// \verbatim
    /// | sx 0  0  0 |
    /// | 0  sy 0  0 |
    /// | 0  0  sz 0 |
    /// | 0  0  0  1 |
    /// \endverbatim
    ///
    /// Returns a reference to this Mat4d.
    ///
    Mat4d& scale(double sx, double sy, double sz = 1) {
        Mat4d m(sx, 0,  0,  0,
                0,  sy, 0,  0,
                0,  0,  sz, 0,
                0,  0,  0,  1);
        return (*this) *= m;
    }

private:
    double data_[4][4];
};

inline constexpr Mat4d Mat4d::identity = Mat4d(1);

/// Alias for vgc::core::Array<vgc::geometry::Mat4d>.
///
using Mat4dArray = core::Array<Mat4d>;

/// Overloads setZero(T& x).
///
/// \sa vgc::core::zero<T>()
///
inline void setZero(Mat4d& m)
{
    m.setToZero();
}

/// Writes the given Mat3d to the output stream.
///
template<typename OStream>
void write(OStream& out, const Mat4d& m)
{
    static const char* s = ", ";
    write(out, '[', m(0,0), s, m(0,1), s, m(0,2), s, m(0,3), s,
                    m(1,0), s, m(1,1), s, m(1,2), s, m(1,3), s,
                    m(2,0), s, m(2,1), s, m(2,2), s, m(2,3), s,
                    m(3,0), s, m(3,1), s, m(3,2), s, m(3,3), ']');
}

} // namespace vgc::geometry

// see https://fmt.dev/latest/api.html#formatting-user-defined-types
template <>
struct fmt::formatter<vgc::geometry::Mat4d> {
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && *it != '}')
            throw format_error("invalid format");
        return it;
    }
    template <typename FormatContext>
    auto format(const vgc::geometry::Mat4d m, FormatContext& ctx) {
        return format_to(ctx.out(),"[{}, {}, {}, {},"
                                   " {}, {}, {}, {},"
                                   " {}, {}, {}, {},"
                                   " {}, {}, {}, {}]",
                         m(0,0), m(0,1), m(0,2), m(0,3),
                         m(1,0), m(1,1), m(1,2), m(1,3),
                         m(2,0), m(2,1), m(2,2), m(2,3),
                         m(3,0), m(3,1), m(3,2), m(3,3));
    }
};

// TODO: parse from string

#endif // VGC_GEOMETRY_MAT4D_H
