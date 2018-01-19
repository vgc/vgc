// Copyright 2017 The VGC Developers
// See the COPYRIGHT file at the top-level directory of this distribution
// and at https://github.com/vgc.io/vgc/blob/master/COPYRIGHT
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

#ifndef VGC_GEOMETRY_MAT4D_H
#define VGC_GEOMETRY_MAT4D_H

#include <vgc/geometry/api.h>
#include <vgc/geometry/vec2d.h>

namespace vgc {
namespace geometry {

/// \class vgc::geometry::Mat4d
/// \brief 4x4 matrix using double-precision floating points.
///
/// A class that stores a 4x4 matrix in column-major format. This ensures
/// direct compatibility with OpenGL. The memory size a Mat4d is exactly 16 *
/// sizeof(double). This will never change in any future version, as this
/// allows to conveniently use this class for data transfer to OpenGL.
///
class VGC_GEOMETRY_API Mat4d
{
public:
    /// Creates an uninitialized Mat4d.
    ///
    Mat4d() {}

    /// Creates a Mat4d initialized with the given arguments.
    ///
    Mat4d(double m11, double m12, double m13, double m14,
          double m21, double m22, double m23, double m24,
          double m31, double m32, double m33, double m34,
          double m41, double m42, double m43, double m44)
    {
        data_[0][0] = m11; data_[0][1] = m21; data_[0][2] = m31; data_[0][3] = m41;
        data_[1][0] = m12; data_[1][1] = m22; data_[1][2] = m32; data_[1][3] = m42;
        data_[2][0] = m13; data_[2][1] = m23; data_[2][2] = m33; data_[2][3] = m43;
        data_[3][0] = m14; data_[3][1] = m24; data_[3][2] = m34; data_[3][3] = m44;
    }

    /// Accesses the component of the Mat4d the the i-th row and j-th column.
    ///
    const double& operator()(int i, int j) const { return data_[j][i]; }

    /// Mutates the component of the Mat4d the the i-th row and j-th column.
    ///
    double& operator()(int i, int j) { return data_[j][i]; }

    /// Adds in-place the \p other Vec2d to this Vec2d.
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
    Mat4d inverse() const;

private:
    double data_[4][4];
};

} // namespace geometry
} // namespace vgc

#endif // VGC_GEOMETRY_MAT4D_H
