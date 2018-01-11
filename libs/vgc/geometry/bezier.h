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

#ifndef VGC_GEOMETRY_BEZIER_H
#define VGC_GEOMETRY_BEZIER_H

#include <vgc/geometry/api.h>

namespace vgc {
namespace geometry {

/// Returns the position \p pos and (non-normalized) derivative \p der at
/// coordinate \p u of the cubic Bézier curve defined by the four control
/// points \p p0, \p p1, \p p2, and \p p3.
///
/// When u = 0, then the returned position is equal to p0. When u = 1, then the
/// returned position is equal to p3. In the general case, the curve does not
/// pass though p1 and p2.
///
/// The coordinate \p u is typically between [0, 1], but any real number is valid
/// input, extrapolating the control points if u s outside the range [0, 1].
///
/// The optional output parameters \p pos and \p der are not evaluated if null,
/// saving computation time if you don't need them. However, if you do need both,
/// then it is slightly faster to evaluate them in one call.
///
template <typename Scalar, typename T>
void evalCubicBezier(
    const T& p0, const T& p1,  const T& p2,  const T& p3, Scalar u,
    T* pos = nullptr, T* der = nullptr)
{
    // Get local parameterization
    Scalar v  = 1 - u;
    Scalar u2 = u * u;
    Scalar v2 = v * v;

    // Evaluate position
    if (pos) {
        Scalar u3 = u2 * u;
        Scalar v3 = v2 * v;

        *pos =   (1 * v3 * 1 ) * p0
               + (3 * v2 * u ) * p1
               + (3 * v  * u2) * p2
               + (1 * 1  * u3) * p3;
    }

    // Evaluate derivative
    if (der) {
        *der =   (3 * v2 * 1 ) * (p1 - p0)
               + (6 * v  * u ) * (p2 - p1)
               + (3 * 1  * u2) * (p3 - p2);
    }
}

/// Convenient wrapper around evalCubicBezier() returning only the position.
///
template <typename Scalar, typename T>
T evalCubicBezierPosition(
    const T& p0, const T& p1,  const T& p2,  const T& p3, Scalar u)
{
    T res;
    evalCubicBezier(p0, p1, p2, p3, u, &res, nullptr);
}

/// Convenient wrapper around evalCubicBezier() returning only the derivative.
///
template <typename Scalar, typename T>
T evalCubicBezierDerivative(
    const T& p0, const T& p1,  const T& p2,  const T& p3, Scalar u)
{
    T res;
    evalCubicBezier(p0, p1, p2, p3, u, nullptr, &res);
}

} // namespace geometry
} // namespace vgc

#endif // VGC_GEOMETRY_BEZIER_H
