// Copyright 2023 The VGC Developers
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
#include <vgc/geometry/bezier.h>
#include <vgc/geometry/curve.h>
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
CubicBezier<T, double> uniformCatmullRomToBezier(core::ConstSpan<T, 4> points) {
    std::array<T, 4> controlPoints;
    uniformCatmullRomToBezier(points.data(), controlPoints.data());
    return CubicBezier<T, double>(controlPoints);
}

/// Overload of `uniformCatmullRomToBezier` with in/out parameter per point.
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
    T p1 = inFourPoints[1] + k * (inFourPoints[2] - inFourPoints[0]);
    T p2 = inFourPoints[2] - k * (inFourPoints[3] - inFourPoints[1]);
    outFourPoints[0] = inFourPoints[1]; // These two lines must be done first in order
    outFourPoints[3] = inFourPoints[2]; // to support inFourPoints == outFourPoints
    outFourPoints[1] = p1;
    outFourPoints[2] = p2;
}

/// Variant of `uniformCatmullRomToBezier` that caps the tangents
/// to prevent p0p1 and p2p3 from intersecting.
///
template<typename T>
void uniformCatmullRomToBezierCapped(const T* inFourPoints, T* outFourPoints) {
    constexpr double k = 1.0 / 6; // = 1/6 up to double precision
    T t0 = inFourPoints[2] - inFourPoints[0];
    T t1 = inFourPoints[3] - inFourPoints[1];
    const double d0 = t0.length();
    const double d1 = t1.length();
    const double maxMagnitude = k * 2 * (inFourPoints[2] - inFourPoints[1]).length();
    T tangent0 = {};
    T tangent1 = {};
    if (d0 > 0) {
        tangent0 = (t0 / d0) * (std::min)(maxMagnitude, k * d0);
    }
    if (d1 > 0) {
        tangent1 = (t1 / d1) * (std::min)(maxMagnitude, k * d1);
    }
    T pt0 = inFourPoints[1] + tangent0;
    T pt1 = inFourPoints[2] - tangent1;
    outFourPoints[0] = inFourPoints[1]; // These two lines must be done first in order
    outFourPoints[3] = inFourPoints[2]; // to support inFourPoints == outFourPoints
    outFourPoints[1] = pt0;
    outFourPoints[2] = pt1;
}

/// Convert four control points of a Catmull-Rom with centripetal paremetrization
/// into the four cubic Bézier control points corresponding to the segment of the
/// curve interpolating between the second and third points.
///
/// See http://www.cemyuksel.com/research/catmullrom_param/catmullrom.pdf
///
template<typename T>
CubicBezier<T, double> centripetalCatmullRomToBezier(core::ConstSpan<T, 4> points) {

    const T* points_ = points.data();
    std::array<double, 3> lengths = {
        (points_[1] - points_[0]).length(),
        (points_[2] - points_[1]).length(),
        (points_[3] - points_[2]).length()};

    return centripetalCatmullRomToBezier<T>(points, lengths);
}

/// Overload of `centripetalCatmullRomToBezier()` that accepts pre-computed lengths.
///
template<typename T>
CubicBezier<T, double> centripetalCatmullRomToBezier(
    core::ConstSpan<T, 4> points,
    core::ConstSpan<double, 3> lengths) {

    const double* lengths_ = lengths.data();
    std::array<double, 3> sqrtLengths = {
        std::sqrt(lengths_[0]), std::sqrt(lengths_[1]), std::sqrt(lengths_[2])};

    return centripetalCatmullRomToBezier<T>(points, lengths, sqrtLengths);
}

/// Overload of `centripetalCatmullRomToBezier()` that accepts pre-computed lengths
/// and square roots of lengths.
///
template<typename T>
CubicBezier<T, double> centripetalCatmullRomToBezier(
    core::ConstSpan<T, 4> points,
    core::ConstSpan<double, 3> lengths,
    core::ConstSpan<double, 3> sqrtLengths) {

    std::array<T, 4> controlPoints;
    centripetalCatmullRomToBezier(
        points.data(), lengths.data(), sqrtLengths.data(), controlPoints.data());
    return CubicBezier<T, double>(controlPoints);
}

/// Overload of `centripetalCatmullRomToBezier()` that accepts pre-computed
/// lengths, square roots of lengths, and returns the results via an output
/// parameter.
///
template<typename T>
void centripetalCatmullRomToBezier(
    const T* points,
    const double* lengths,
    const double* sqrtLengths,
    T* outPoints) {

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

enum CatmullRomSplineParameterization {
    Uniform,
    Centripetal
};

// TODO: immutable version with ConstShared storage
//       & move these classes in a new set of .h/.cpp
class VGC_GEOMETRY_API CatmullRomSplineStroke2d : public AbstractStroke2d {
public:
    CatmullRomSplineStroke2d(
        CatmullRomSplineParameterization parametrization,
        bool isClosed)

        : AbstractStroke2d(isClosed)
        , parametrization_(parametrization) {
    }

    CatmullRomSplineStroke2d(
        CatmullRomSplineParameterization parametrization,
        bool isClosed,
        double constantWidth)

        : AbstractStroke2d(isClosed)
        , widths_(1, constantWidth)
        , isWidthConstant_(true)
        , parametrization_(parametrization) {
    }

    template<typename TRangePositions, typename TRangeWidths>
    CatmullRomSplineStroke2d(
        CatmullRomSplineParameterization parametrization,
        bool isClosed,
        bool isWidthConstant,
        TRangePositions&& positions,
        TRangeWidths&& widths)

        : AbstractStroke2d(isClosed)
        , positions_(std::forward<TRangePositions>(positions))
        , widths_(std::forward<TRangeWidths>(widths))
        , isWidthConstant_(isWidthConstant)
        , parametrization_(parametrization) {

        computeChordLengths_();
    }

    const core::Array<Vec2d>& positions() const {
        return positions_;
    }

    core::Array<Vec2d>&& movePositions() {
        return std::move(positions_);
    }

    template<typename TRange>
    void setPositions(TRange&& positions) {
        positions_ = std::forward<TRange>(positions);
        computeChordLengths_();
    }

    const core::Array<double>& widths() const {
        return widths_;
    }

    // TODO: make data class and startEdit() endEdit()
    core::Array<double>&& moveWidths() {
        return std::move(widths_);
    }

    template<typename TRange>
    void setWidths(TRange&& widths) {
        widths_ = std::forward<TRange>(widths);
    }

    void setConstantWidth(double width) {
        isWidthConstant_ = true;
        widths_.resize(1);
        widths_[0] = width;
    }

    bool isWidthConstant() const {
        return isWidthConstant_;
    }

    const core::Array<double>& chordLengths() const {
        return chordLengths_;
    }

protected:
    Int numKnots_() const override;

    bool isZeroLengthSegment_(Int segmentIndex) const override;

    Vec2d evalNonZeroCenterline(Int segmentIndex, double u) const override;

    Vec2d evalNonZeroCenterline(Int segmentIndex, double u, Vec2d& dp) const override;

    StrokeSampleEx2d evalNonZero(Int segmentIndex, double u) const override;

    void sampleNonZeroSegment(
        StrokeSampleEx2dArray& out,
        Int segmentIndex,
        const CurveSamplingParameters& params) const override;

    StrokeSampleEx2d zeroLengthStrokeSample() const override;

    std::array<Vec2d, 2> computeOffsetLineTangentsAtSegmentEndpoint_(
        Int segmentIndex,
        Int endpointIndex) const override;

    CubicBezier2d segmentToBezier(Int segmentIndex) const;
    CubicBezier2d segmentToBezier(Int segmentIndex, CubicBezier2d& halfwidths) const;

    double constantWidth() const {
        return widths_[0];
    }

private:
    core::Array<Vec2d> positions_;
    core::Array<double> widths_;
    core::Array<double> chordLengths_;
    bool isWidthConstant_ = false;
    CatmullRomSplineParameterization parametrization_;

    void computeChordLengths_();
};

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_CATMULLROM_H
