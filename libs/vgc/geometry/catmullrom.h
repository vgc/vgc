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

#ifndef VGC_GEOMETRY_CATMULLROM_H
#define VGC_GEOMETRY_CATMULLROM_H

#include <vgc/geometry/api.h>

namespace vgc::geometry {

/// Convert four uniform Catmull-Rom control points into the four cubic Bézier
/// control points corresponding to the same cubic curve. The formula is the
/// following:
///
/// \verbatim
/// b0 = c1;
/// b1 = c1 + (c2 - c0) / 6;
/// b2 = c2 - (c3 - c1) / 6;
/// b3 = c2;
/// \endverbatim
///
/// Details:
///
/// The tension parameter k = 1/6 is chosen to ensure that if the
/// Catmull-Rom control points are aligned and uniformly spaced, then
/// the resulting curve is parameterized with constant speed.
///
/// Indeed, a Catmull-Rom curve is generally defined as a sequence of
/// (t[i], c[i]) pairs, and the derivative at p[i] is defined by:
///
/// \verbatim
///            c[i+1] - c[i-1]
///     m[i] = ---------------
///            t[i+1] - t[i-1]
/// \endverbatim
///
/// A *uniform* Catmull-Rom assumes that the "times" or "knot values" t[i] are
/// uniformly spaced, for example: [0, 1, 2, 3, 4, ... ]. The spacing between
/// the t[i]s is chosen to be 1 to match the fact that a Bézier curve is
/// defined for t in [0, 1]; otherwise we'd need an additional factor for the
/// variable subsitution (e.g., if t' = 2*t + 1, then dt' = 2 * dt). With these
/// assumptions, we have t[i+1] - t[i-1] = 2, thus:
///
/// \verbatim
///     m[i] = (c[i+1] - c[i-1]) / 2.
/// \endverbatim
///
/// Now, we recall that for a cubic bezier B(t) defined for t in [0, 1]
/// by the control points b0, b1, b2, b3, we have:
///
/// \verbatim
///     B(t)  = (1-t)^3 b0 + 3(1-t)^2 t b1 + 3(1-t)t^2 b2 + t^3 b3
///     dB/dt = 3(1-t)^2 (b1-b0) + 6(1-t)t(b2-b1) + 3t^2(b3-b2)
/// \endverbatim
///
/// By taking this equation at t = 0, we can deduce that:
///
/// \verbatim
///     b1 = b0 + (1/3) * dB/dt.
/// \endverbatim
///
/// Therefore, if B(t) corresponds to the uniform Catmull-Rom subcurve
/// between c[i] and c[i+1], we have:
///
/// \verbatim
///    b1 = b0 + (1/3) * m[i]
///       = b0 + (1/6) * (c[i+1] - c[i-1])
/// \endverbatim
///
/// Which finishes the explanation why k = 1/6.
///
template<typename T>
void uniformCatmullRomToBezier(
    const T& c0,
    const T& c1,
    const T& c2,
    const T& c3,
    T& b0,
    T& b1,
    T& b2,
    T& b3) {

    const double k = 0.166666666666666667; // = 1/6 up to double precision
    b0 = c1;
    b1 = c1 + k * (c2 - c0);
    b2 = c2 - k * (c3 - c1);
    b3 = c2;
}

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_CATMULLROM_H
