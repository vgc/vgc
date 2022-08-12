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

#ifndef VGC_GEOMETRY_BEZIER_H
#define VGC_GEOMETRY_BEZIER_H

#include <vgc/geometry/api.h>

namespace vgc::geometry {

// clang-format off

/// Returns the position at coordinate \p u of the cubic Bézier curve defined
/// by the four control points \p p0, \p p1, \p p2, and \p p3.
///
/// When u = 0, then the returned position is equal to p0. When u = 1, then the
/// returned position is equal to p3. In the general case, the curve does not
/// pass though p1 and p2.
///
/// The coordinate \p u is typically between [0, 1], but this is not required.
/// Values of u outside the range [0, 1] extrapolate the control points.
///
/// \sa cubicBezierDer(), cubicBezierPosAndDer()
///
template <typename Scalar, typename T>
T cubicBezier(
    const T& p0, const T& p1, const T& p2, const T& p3,
    Scalar u) {

    Scalar v  = 1 - u;
    Scalar u2 = u * u;
    Scalar v2 = v * v;
    Scalar u3 = u2 * u;
    Scalar v3 = v2 * v;

    return       v3      * p0
           + 3 * v2 * u  * p1
           + 3 * v  * u2 * p2
           +          u3 * p3;
}

/// Returns the (non-normalized) derivative at coordinate \p u of the cubic
/// Bézier curve defined by the four control points \p p0, \p p1, \p p2, and \p
/// p3.
///
/// \sa cubicBezier(), cubicBezierPosAndDer()
///
template <typename Scalar, typename T>
T cubicBezierDer(
    const T& p0, const T& p1, const T& p2, const T& p3,
    Scalar u) {

    Scalar v  = 1 - u;
    Scalar u2 = u * u;
    Scalar v2 = v * v;

    return   3 * v2      * (p1 - p0)
           + 6 * v  * u  * (p2 - p1)
           + 3      * u2 * (p3 - p2);
}

/// Returns both the position \p pos and derivative \p der at coordinate \p u
/// of the cubic Bézier curve defined by the four control points \p p0, \p p1,
/// \p p2, and \p p3.
///
/// This function is only very marginally faster than calling cubicBezier() and
/// cubicBezierDer() separately. Therefore, for readability and consistency, we
/// recommend to reserve its usage for cases where performance is critical.
///
/// \sa cubicBezier(), cubicBezierDer()
///
template <typename Scalar, typename T>
T cubicBezierPosAndDer(
    const T& p0, const T& p1, const T& p2, const T& p3,
    Scalar u,
    T& pos,
    T& der) {

    Scalar v  = 1 - u;
    Scalar u2 = u * u;
    Scalar v2 = v * v;
    Scalar u3 = u2 * u;
    Scalar v3 = v2 * v;

    pos =        v3      * p0
           + 3 * v2 * u  * p1
           + 3 * v  * u2 * p2
           +          u3 * p3;

    der =    3 * v2      * (p1 - p0)
           + 6 * v  * u  * (p2 - p1)
           + 3      * u2 * (p3 - p2);
}

// clang-format on

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_BEZIER_H