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

#ifndef VGC_GEOMETRY_VEC2D_H
#define VGC_GEOMETRY_VEC2D_H

#include <cmath>
#include <vgc/geometry/api.h>
#include <vgc/geometry/epsilon.h>

namespace vgc {
namespace geometry {

/// \class vgc::geometry::Vec2d
/// \brief 2D vector using double-precision floating points.
///
/// The memory size a Vec2d is exactly 2 * sizeof(double). This will never
/// change in any future version, as this allows to conveniently use this class
/// for data transfer to OpenGL.
///
/// Like in the Eigen library, VGC has chosen not to distinguish between points
/// and vectors. In other words, if you wish to represent a 2D point, simply use a Vec2d.
///
class VGC_GEOMETRY_API Vec2d
{
public:
    /// Creates an uninitialized Vec2d.
    ///
    Vec2d() {}

    /// Creates a Vec2d initialized with the given arguments.
    ///
    Vec2d(double x, double y) : data_{x, y} {}

    /// Accesses the i-th component of the Vec2d.
    ///
    const double& operator[](int i) const { return data_[i]; }

    /// Mutates the i-th component of the Vec2d.
    ///
    double& operator[](int i) { return data_[i]; }

    /// Accesses the first component of the Vec2d.
    ///
    double x() const { return data_[0]; }

    /// Accesses the second component of the Vec2d.
    ///
    double y() const { return data_[1]; }

    /// Mutates the first component of the Vec2d.
    ///
    void setX(double x) { data_[0] = x; }

    /// Mutates the second component of the Vec2d.
    ///
    void setY(double y) { data_[1] = y; }

    /// Adds in-place the \p other Vec2d to this Vec2d.
    ///
    Vec2d& operator+=(const Vec2d& other) {
        data_[0] += other[0];
        data_[1] += other[1];
        return *this;
    }

    /// Returns the addition of the Vec2d \p v1 and the Vec2d \p v2.
    ///
    friend Vec2d operator+(const Vec2d& v1, const Vec2d& v2) {
        return Vec2d(v1) += v2;
    }

    /// Substracts in-place the \p other Vec2d to this Vec2d.
    ///
    Vec2d& operator-=(const Vec2d& other) {
        data_[0] -= other[0];
        data_[1] -= other[1];
        return *this;
    }

    /// Returns the substraction of the Vec2d \p v1 and the Vec2d \p v2.
    ///
    friend Vec2d operator-(const Vec2d& v1, const Vec2d& v2) {
        return Vec2d(v1) -= v2;
    }

    /// Multiplies in-place this Vec2d by the given scalar \p s.
    ///
    Vec2d& operator*=(double s) {
        data_[0] *= s;
        data_[1] *= s;
        return *this;
    }

    /// Returns the multiplication of this Vec2d by the given scalar \p s.
    ///
    Vec2d operator*(double s) const {
        return Vec2d(*this) *= s;
    }

    /// Returns the multiplication of the scalar \p s with the Vec2d \p v.
    ///
    friend Vec2d operator*(double s, const Vec2d& v) {
        return v * s;
    }

    /// Divides in-place this Vec2d by the given scalar \p s.
    ///
    Vec2d& operator/=(double s) {
        data_[0] /= s;
        data_[1] /= s;
        return *this;
    }

    /// Returns the division of this Vec2d by the given scalar \p s.
    ///
    Vec2d operator/(double s) const {
        return Vec2d(*this) /= s;
    }

    /// Returns the Euclidean length of the Vec2d.
    ///
    double length() const {
        return std::sqrt(squaredLength());
    }

    /// Returns the square of the Euclidean length of the Vec2d.
    ///
    /// This function is faster than length(), therefore it is a good idea to
    /// use it whenever you don't need the actual length. For example, if you
    /// need to know which vector has greater length, you can use
    /// v1.squaredLength() < v2.squaredLength().
    ///
    double squaredLength() const {
        return data_[0] * data_[0] + data_[1] * data_[1];
    }

    /// Makes this Vec2d a unit vector by dividing it by length().
    /// If length() < epsilon, this Vec2d is set to (1.0, 0.0).
    ///
    Vec2d& normalize() {
        double l = length();
        if (l > epsilon) {
            *this /= l;
        }
        else {
            *this = Vec2d(1.0, 0.0);
        }
        return *this;
    }

    /// Returns a normalized copy of this Vec2d.
    ///
    Vec2d normalized() const {
        return Vec2d(*this).normalize();
    }

    /// Rotates this Vec2d by 90° counter-clockwise, assuming a left-handed
    /// coordinate system.
    ///
    Vec2d& orthogonalize() {
        double tmp = data_[0];
        data_[0] = - data_[1];
        data_[1] = tmp;
        return *this;
    }

    /// Returns a copy of this Vec2d rotated 90° counter-clockwise, assuming a
    /// left-handed coordinate system.
    ///
    Vec2d orthogonalized() const {
        return Vec2d(*this).orthogonalize();
    }

private:
    double data_[2];
};

} // namespace geometry
} // namespace vgc

#endif // VGC_GEOMETRY_VEC2D_H
