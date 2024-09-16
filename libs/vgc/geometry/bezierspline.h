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

#ifndef VGC_GEOMETRY_BEZIERSPLINE_H
#define VGC_GEOMETRY_BEZIERSPLINE_H

#include <vgc/core/arithmetic.h>
#include <vgc/core/array.h>
#include <vgc/core/assert.h>
#include <vgc/geometry/api.h>
#include <vgc/geometry/traits.h>
#include <vgc/geometry/vec2.h>
#include <vgc/geometry/vec3.h>
#include <vgc/geometry/vec4.h>

namespace vgc::geometry {

/// \class vgc::geometry::BezierSpline
/// \brief Represents a piecewise-cubic continuous curve represented via
///        Bezier control points.
///
/// This class is low-level. You may prefer to use the Curves2d class
/// instead.
///
/// All the Bezier control points (both knots and tangents) are stored
/// contiguously into a single array of size 3n+1,
/// where n is the number of cubic segments.
///
/// For convenience, we allow clients to directly access and mutate the
/// data using the method data(). However, this means that clients may
/// potentially mess it up and give it a size which is not 3n+1. For
/// safety, BezierSpline handles this gracefully: it simply ignores the last
/// few points if they are meaningless. Here is the relation between
/// data().size(), numControlPoints(), and numSegments():
///
/// \verbatim
/// data().size()    numControlPoints()     numSegments()
///      0                  0                   0
///      1                  0                   0
///      2                  0                   0
///      3                  0                   0
///      4                  4                   1
///      5                  4                   1
///      6                  4                   1
///      7                  7                   2
///      8                  7                   2
///      9                  7                   2
/// \endverbatim
///
template<typename Point, typename T = scalarType<Point>>
class BezierSpline {
public:
    using ScalarType = T;
    static constexpr Int dimension = geometry::dimension<Point>;

    /// Creates an empty BezierSpline.
    ///
    BezierSpline() {
    }

    /// Accesses the underlying data.
    ///
    const core::Array<Point>& data() const {
        return data_;
    }

    /// Mutates the underlying data.
    /// Clients should ensure that the size stays of the form 3n+1.
    ///
    core::Array<Point>& data() {
        return data_;
    }

    /// Returns the number of cubic segments of the BezierSpline.
    ///
    Int numSegments() const {
        if (data_.size() > 0) {
            return (data_.size() - 1) / 3;
        }
        else {
            return 0;
        }
    }

    /// Returns the number of control points of the BezierSpline.
    /// This is smaller than data().size() in case data().size()
    /// is not of the form 3*n + 1.
    ///
    Int numControlPoints() const {
        return 1 + 3 * numSegments();
    }

    /// Returns whether the spline is empty, that is, whether
    /// numSegments() == numControlPoints() == 0.
    ///
    bool isEmpty() const {
        return numSegments();
    }

    /// Evaluates a non-empty spline given u in [0, 1] and writes
    /// the result in the optional output parameters \p pos and \p der.
    /// It is undefined behaviour to evaluate an empty spline.
    ///
    void eval(T u, Point* pos = nullptr, Point* der = nullptr) const {

        VGC_ASSERT(numSegments() > 0);

        // Continuously map from [0, 1] to [0, n]
        Int n = numSegments();
        u *= n;

        // Get which segment to evaluate in [0 .. n-1]
        Int segmentIndex = core::clamp(core::ifloor<Int>(u), 0, n - 1);

        // Get local parameterization
        T t = u - segmentIndex;
        T t2 = t * t;
        T t3 = t2 * t;
        T s = 1 - t;
        T s2 = s * s;
        T s3 = s2 * s;

        // Get corresponding control points
        const Point& p0 = data_[segmentIndex];
        const Point& p1 = data_[segmentIndex + 1];
        const Point& p2 = data_[segmentIndex + 2];
        const Point& p3 = data_[segmentIndex + 3];

        // Evaluate
        // clang-format off
        if (pos) {
            *pos =   s3          * p0
                   + 3 * s2 * t  * p1
                   + 3 * s  * t2 * p2
                   + t3          * p3;
        }
        if (der) {
            *der =   s2         * (p1 - p0)
                   + 2 * s * t  * (p2 - p1)
                   + t2 * p2    * (p3 - p2);
        }
        // clang-format on
    }

    /// Returns the position of a non-empty spline given u in [0, 1].
    /// It is undefined behaviour to evaluate an empty spline.
    /// For performance, use eval() if you need both the position
    /// and the derivative.
    ///
    Point position(T u) const {
        T res;
        eval(u, &res);
        return res;
    }

    /// Returns the position of a non-empty spline given u in [0, 1].
    /// It is undefined behaviour to evaluate an empty spline.
    /// For performance, use eval() if you need both the position
    /// and the derivative.
    ///
    Point derivative(T u) const {
        T res;
        eval(u, nullptr, &res);
        return res;
    }

private:
    core::Array<Point> data_;
};

using BezierSpline1f = BezierSpline<float>;
using BezierSpline1d = BezierSpline<double>;

using BezierSpline2f = BezierSpline<Vec2f>;
using BezierSpline2d = BezierSpline<Vec2d>;

using BezierSpline3f = BezierSpline<Vec3f>;
using BezierSpline3d = BezierSpline<Vec3d>;

using BezierSpline4f = BezierSpline<Vec4f>;
using BezierSpline4d = BezierSpline<Vec4d>;

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_BEZIERSPLINE_H
