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

#include <vgc/core/span.h>
#include <vgc/geometry/api.h>
#include <vgc/geometry/vec2d.h>

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

    constexpr double k = 1.0 / 6; // = 1/6 up to double precision
    b0 = c1;
    b1 = c1 + k * (c2 - c0);
    b2 = c2 - k * (c3 - c1);
    b3 = c2;
}

/// Overload of `uniformCatmullRomToBezier` expecting a pointer to a contiguous sequence
/// of 4 control points in input and output.
///
template<typename T>
void uniformCatmullRomToBezier(const T* inFourPoints, T* outFourPoints) {

    constexpr double k = 1.0 / 6; // = 1/6 up to double precision
    outFourPoints[0] = inFourPoints[1];
    outFourPoints[1] = inFourPoints[1] + k * (inFourPoints[2] - inFourPoints[0]);
    outFourPoints[2] = inFourPoints[2] - k * (inFourPoints[3] - inFourPoints[1]);
    outFourPoints[3] = inFourPoints[2];
}

/// Variant of `uniformCatmullRomToBezier` expecting a pointer to a contiguous sequence
/// of 4 control points and performs the operation in-place.
///
template<typename T>
void uniformCatmullRomToBezierInPlace(T* inoutFourPoints) {

    constexpr double k = 1.0 / 6; // = 1/6 up to double precision
    T p1 = inoutFourPoints[1] + k * (inoutFourPoints[2] - inoutFourPoints[0]);
    T p2 = inoutFourPoints[2] - k * (inoutFourPoints[3] - inoutFourPoints[1]);
    inoutFourPoints[0] = inoutFourPoints[1];
    inoutFourPoints[3] = inoutFourPoints[2];
    inoutFourPoints[1] = p1;
    inoutFourPoints[2] = p2;
}

/// Variant of `uniformCatmullRomToBezier` expecting a pointer to a contiguous sequence
/// of 4 control points and performs the operation in-place.
/// Also the tangents are capped to prevent p0p1 and p2p3 from intersecting.
///
inline void uniformCatmullRomToBezierCappedInPlace(Vec2d* inoutFourPoints) {

    constexpr double k = 1.0 / 6; // = 1/6 up to double precision
    const double maxMagnitude =
        k * 2 * (inoutFourPoints[2] - inoutFourPoints[1]).length();
    Vec2d t0 = inoutFourPoints[2] - inoutFourPoints[0];
    Vec2d t1 = inoutFourPoints[3] - inoutFourPoints[1];
    const double kd0 = k * t0.length();
    const double kd1 = k * t1.length();
    Vec2d pt0 = inoutFourPoints[1] + t0.normalized() * (std::min)(maxMagnitude, kd0);
    Vec2d pt1 = inoutFourPoints[2] - t1.normalized() * (std::min)(maxMagnitude, kd1);
    inoutFourPoints[0] = inoutFourPoints[1];
    inoutFourPoints[3] = inoutFourPoints[2];
    inoutFourPoints[1] = pt0;
    inoutFourPoints[2] = pt1;
}

/// Convert four control points of a Catmull-Rom with centripetal paremetrization
/// into the four cubic Bézier control points corresponding to the segment of the
/// curve interpolating between the second and third points.
///
/// See http://www.cemyuksel.com/research/catmullrom_param/catmullrom.pdf
///
template<typename T>
void centripetalCatmullRomToBezier(
    core::ConstSpan<T, 4> points,
    core::Span<T, 4> outPoints) {

    std::array<double, 3> lengths = {
        (points[1] - points[0]).length(),
        (points[2] - points[1]).length(),
        (points[3] - points[2]).length()};

    centripetalCatmullRomToBezier<T>(points, lengths, outPoints);
}

/// Overload of `centripetalCatmullRomToBezier`.
///
template<typename T>
void centripetalCatmullRomToBezier(
    core::ConstSpan<T, 4> points,
    core::ConstSpan<double, 3> lengths,
    core::Span<T, 4> outPoints) {

    std::array<double, 3> sqrtLengths = {
        std::sqrt(lengths[0]), std::sqrt(lengths[1]), std::sqrt(lengths[2])};

    centripetalCatmullRomToBezier<T>(points, lengths, sqrtLengths, outPoints);
}

/// Overload of `centripetalCatmullRomToBezier`.
///
template<typename T>
void centripetalCatmullRomToBezier(
    core::ConstSpan<T, 4> points,
    core::ConstSpan<double, 3> lengths,
    core::ConstSpan<double, 3> sqrtLengths,
    core::Span<T, 4> outPoints) {

    double d1 = lengths[0];
    double d2 = lengths[1];
    double d3 = lengths[2];
    double d1a = sqrtLengths[0];
    double d2a = sqrtLengths[1];
    double d3a = sqrtLengths[2];

    T p1 = points[1];
    if (d1a > 0) {
        double c1 = 2 * d1 + 3 * d1a * d2a + d2;
        double c2 = 3 * d1a * (d1a + d2a);
        p1 = (d1 * points[2] - d2 * points[0] + c1 * points[1]) / c2;
    }

    T p2 = points[2];
    if (d3a > 0) {
        double c1 = 2 * d3 + 3 * d2a * d3a + d2;
        double c2 = 3 * d3a * (d2a + d3a);
        p2 = (d3 * points[1] - d2 * points[3] + c1 * points[2]) / c2;
    }

    outPoints[0] = points[1];
    outPoints[3] = points[2];
    outPoints[1] = p1;
    outPoints[2] = p2;
}

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_CATMULLROM_H
