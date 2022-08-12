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

#include <cassert>
#include <cmath>
#include <vector>
#include <vgc/core/algorithm.h>
#include <vgc/geometry/api.h>

namespace vgc::geometry {

/// \class vgc::geometry::BezierSpline
/// \brief Represents a piecewise-cubic continuous curve represented via
///        Bezier control points.
///
/// This class is low-level. You may prefer to use the Curve class
/// instead.
///
/// All the Bezier control points (both knots and tangents) are stored
/// contiguously into a single std::vector<T> of size 3n+1,
/// where n is the number of cubic segments.
///
/// For convenience, we allow clients to directly access and mutate the
/// data using the method data(). However, this means that clients may
/// potentially mess it up and give it a size which is not 3n+1. For
/// safety, BezierSpline handles this gracefully: it simply ignores the last
/// few points if they are meaningless. Here is the relation between
/// data().size(), numControlPoints(), and numSegments():
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
/// Scalar must be float or double (default is double).
/// The following operators on T must be implemented:
/// \code
/// T& operator+(const T&, const T&);
/// T& operator*(Scalar, const T&);
/// \endcode
///
template<typename T, typename Scalar = double>
class BezierSpline {
public:
    /// Creates an empty BezierSpline.
    ///
    BezierSpline() {
    }

    /// Accesses the underlying data.
    ///
    const std::vector<T>& data() const {
        return data_;
    }

    /// Mutates the underlying data.
    /// Clients should ensure that the size stays of the form 3n+1.
    ///
    std::vector<T>& data() {
        return data_;
    }

    /// Returns the number of cubic segments of the BezierSpline.
    ///
    int numSegments() const {
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
    int numControlPoints() const {
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
    void eval(Scalar u, T* pos = nullptr, T* der = nullptr) const {
        assert(!isEmpty());

        // Continuously map from [0, 1] to [0, n]
        int n = numSegments();
        u *= n;

        // Get which segment to evaluate in [0 .. n-1]
        int segmentIndex = core::clamp((int)std::floor(u), 0, n - 1);

        // Get local parameterization
        Scalar t = u - segmentIndex;
        Scalar t2 = t * t;
        Scalar t3 = t2 * t;
        Scalar s = 1 - t;
        Scalar s2 = s * s;
        Scalar s3 = s2 * s;

        // Get corresponding control points
        const T& p0 = data_[segmentIndex];
        const T& p1 = data_[segmentIndex + 1];
        const T& p2 = data_[segmentIndex + 2];
        const T& p3 = data_[segmentIndex + 3];

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
    T position(Scalar u) const {
        T res;
        eval(u, &res);
        return res;
    }

    /// Returns the position of a non-empty spline given u in [0, 1].
    /// It is undefined behaviour to evaluate an empty spline.
    /// For performance, use eval() if you need both the position
    /// and the derivative.
    ///
    T derivative(Scalar u) const {
        T res;
        eval(u, nullptr, &res);
        return res;
    }

private:
    std::vector<T> data_;
};

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_BEZIERSPLINE_H
