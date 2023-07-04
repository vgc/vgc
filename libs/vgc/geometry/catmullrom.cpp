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

#include <vgc/geometry/catmullrom.h>

#include <vgc/geometry/bezier.h>
#include <vgc/geometry/curve.h>

namespace vgc::geometry {

namespace {

Vec2d segmentStartPositionOpenUnchecked(
    core::ConstSpan<Vec2d> knotPositions,
    Int segmentIndex) {

    return knotPositions.getUnchecked(segmentIndex);
}

Vec2d segmentEndPositionOpenUnchecked(
    core::ConstSpan<Vec2d> knotPositions,
    Int segmentIndex) {

    return knotPositions.getUnchecked(segmentIndex + 1);
}

Vec2d segmentStartPositionClosedUnchecked(
    core::ConstSpan<Vec2d> knotPositions,
    Int segmentIndex) {

    return knotPositions.getUnchecked(segmentIndex);
}

Vec2d segmentEndPositionClosedUnchecked(
    core::ConstSpan<Vec2d> knotPositions,
    Int segmentIndex) {

    if (segmentIndex == knotPositions.length()) {
        return knotPositions.getUnchecked(0);
    }
    else {
        return knotPositions.getUnchecked(segmentIndex + 1);
    }
}

} // namespace

Int CatmullRomSplineStroke2d::numKnots_() const {
    return positions_.length();
}

bool CatmullRomSplineStroke2d::isZeroLengthSegment_(Int segmentIndex) const {
    return chordLengths_[segmentIndex] == 0;
}

Vec2d CatmullRomSplineStroke2d::evalNonZeroCenterline(Int segmentIndex, double u) const {
    auto bezier = segmentToBezier(segmentIndex);
    return bezier.eval(u);
}

Vec2d CatmullRomSplineStroke2d::evalNonZeroCenterline(
    Int segmentIndex,
    double u,
    Vec2d& dp) const {

    auto bezier = segmentToBezier(segmentIndex);
    return bezier.eval(u, dp);
}

StrokeSampleEx2d CatmullRomSplineStroke2d::evalNonZero(Int segmentIndex, double u) const {
    if (isWidthConstant_) {
        CubicBezier2d centerlineBezier = segmentToBezier(segmentIndex);
        double hw = 0.5 * widths_[0];
        Vec2d dp(core::noInit);
        Vec2d p = centerlineBezier.eval(u, dp);
        double speed = dp.length();
        return StrokeSampleEx2d(p, dp / speed, hw, speed, segmentIndex, u);
    }
    else {
        CubicBezier2d halfwidthsBezier(core::noInit);
        CubicBezier2d centerlineBezier = segmentToBezier(segmentIndex, halfwidthsBezier);
        Vec2d dp(core::noInit);
        Vec2d p = centerlineBezier.eval(u, dp);
        double speed = dp.length();
        Vec2d hw = halfwidthsBezier.eval(u);
        return StrokeSampleEx2d(p, dp / speed, hw, speed, segmentIndex, u);
    }
}

void CatmullRomSplineStroke2d::sampleNonZeroSegment(
    StrokeSampleEx2dArray& out,
    Int segmentIndex,
    const CurveSamplingParameters& params) const {

    detail::AdaptiveStrokeSampler sampler = {};

    if (isWidthConstant_) {
        CubicBezier2d centerlineBezier = segmentToBezier(segmentIndex);
        double hw = 0.5 * widths_[0];
        sampler.sample(
            [&, hw](double u) -> StrokeSampleEx2d {
                Vec2d dp(core::noInit);
                Vec2d p = centerlineBezier.eval(u, dp);
                double speed = dp.length();
                return StrokeSampleEx2d(p, dp / speed, hw, speed, segmentIndex, u);
            },
            params,
            out);
    }
    else {
        CubicBezier2d halfwidthsBezier(core::noInit);
        CubicBezier2d centerlineBezier = segmentToBezier(segmentIndex, halfwidthsBezier);
        sampler.sample(
            [&](double u) -> StrokeSampleEx2d {
                Vec2d dp(core::noInit);
                Vec2d p = centerlineBezier.eval(u, dp);
                double speed = dp.length();
                Vec2d hw = halfwidthsBezier.eval(u);
                return StrokeSampleEx2d(p, dp / speed, hw, speed, segmentIndex, u);
            },
            params,
            out);
    }
}

StrokeSampleEx2d CatmullRomSplineStroke2d::zeroLengthStrokeSample() const {
    return StrokeSampleEx2d(
        positions().first(), Vec2d(0, 1), 0.5 /*constantHalfwidth_*/, 0, 0);
}

// Currently assumes first derivative at endpoint is non null.
// TODO: Support null derivatives (using limit analysis).
// TODO: Support different halfwidth on both sides.
std::array<Vec2d, 2>
CatmullRomSplineStroke2d::computeOffsetLineTangentsAtSegmentEndpoint_(
    Int segmentIndex,
    Int endpointIndex) const {

    CubicBezier2d halfwidthBezier(core::noInit);
    CubicBezier2d positionsBezier = segmentToBezier(segmentIndex, halfwidthBezier);

    const std::array<Vec2d, 4>& positions = positionsBezier.controlPoints();
    const std::array<Vec2d, 4>& halfwidths = halfwidthBezier.controlPoints();

    Vec2d p = core::noInit;
    Vec2d dp = core::noInit;
    Vec2d ddp = core::noInit;
    Vec2d w = core::noInit;
    Vec2d dw = core::noInit;
    if (endpointIndex) {
        p = positions[3];
        dp = 3 * (positions[3] - positions[2]);
        ddp = 6 * (positions[3] - 2 * positions[2] + positions[1]);
        w = halfwidths[3];
        dw = 3 * (halfwidths[3] - halfwidths[2]);
    }
    else {
        p = positions[0];
        dp = 3 * (positions[1] - positions[0]);
        ddp = 6 * (positions[2] - 2 * positions[1] + positions[0]);
        w = halfwidths[0];
        dw = 3 * (halfwidths[1] - halfwidths[0]);
    }

    double dpl = dp.length();
    Vec2d n = dp.orthogonalized() / dpl;
    Vec2d dn = dp * (ddp.det(dp)) / (dpl * dpl * dpl);

    Vec2d offset0 = dn * w[0] + n * dw[0];
    Vec2d offset1 = -(dn * w[1] + n * dw[1]);
    return {(dp + offset0).normalized(), (dp + offset1).normalized()};
}

namespace {

std::array<Int, 4> computeKnotIndices_(bool isClosed, Int numKnots, Int segmentIndex) {
    // Ensure we have a valid segment between two control points
    const Int numSegments = isClosed ? numKnots : (numKnots ? numKnots - 1 : 0);
    VGC_ASSERT(segmentIndex >= 0);
    VGC_ASSERT(segmentIndex < numSegments);

    // Get indices of points used by the Catmull-Rom interpolation, handle
    // wrapping for closed curves and boundary for open curves.
    std::array<Int, 4> indices = {
        segmentIndex - 1, segmentIndex, segmentIndex + 1, segmentIndex + 2};
    if (isClosed) {
        if (indices[0] < 0) {
            indices[0] = numKnots - 1;
        }
        if (indices[2] > numKnots - 1) {
            indices[2] = 0;
            indices[3] = 1;
        }
        if (indices[3] > numKnots - 1) {
            indices[3] = 0;
        }
    }
    else {
        if (indices[0] < 0) {
            indices[0] = 0;
        }
        if (indices[2] > numKnots - 1) {
            indices[2] = numKnots - 1;
            indices[3] = numKnots - 1;
        }
        else if (indices[3] > numKnots - 1) {
            indices[3] = numKnots - 1;
        }
    }
    return indices;
}

enum class SegmentType {
    None = 0,
    Corner = 1,
    AfterCorner = 2,
    BeforeCorner = 3,
    BetweenCorners = 4,
};

SegmentType computeSegmentCenterlineCubicBezier_(
    CubicBezier2d& bezier,
    CatmullRomSplineParameterization parameterization,
    core::ConstSpan<Vec2d> knotPositions,
    core::ConstSpan<double> chordLengths,
    const std::array<Int, 4>& knotIndices,
    std::array<double, 3>& fixedChordLengths) {

    std::array<Vec2d, 4> knots = {
        knotPositions.getUnchecked(knotIndices[0]),
        knotPositions.getUnchecked(knotIndices[1]),
        knotPositions.getUnchecked(knotIndices[2]),
        knotPositions.getUnchecked(knotIndices[3])};

    fixedChordLengths = {
        chordLengths.getUnchecked(knotIndices[0]),
        chordLengths.getUnchecked(knotIndices[1]),
        chordLengths.getUnchecked(knotIndices[2])};

    std::array<Vec2d, 3> chords = {
        knots[1] - knots[0], knots[2] - knots[1], knots[3] - knots[2]};

    // Aliases
    const Vec2d p0p1 = chords[0];
    const Vec2d p1p2 = chords[1];
    const Vec2d p2p3 = chords[2];
    const double d01 = fixedChordLengths[0];
    const double d12 = fixedChordLengths[1];
    const double d23 = fixedChordLengths[2];

    // Handle "corner knots", defined as:
    // 1. Two consecutive equal points, or
    // 2. The first/last knot of an open curve
    //
    SegmentType result = SegmentType::None;
    bool isAfterCorner = (d01 == 0);
    bool isCorner = (d12 == 0);
    bool isBeforeCorner = (d23 == 0);
    if (isCorner) {
        bezier = CubicBezier2d(knots);
        return SegmentType::Corner;
    }
    else if (isAfterCorner) {
        if (isBeforeCorner) {
            // (d01 == 0) && (d12 > 0) && (d23 == 0)
            //
            // Linear parametrization
            double u = 1.0 / 3;
            double v = (1 - u);
            bezier = CubicBezier2d(
                knots[1],
                v * knots[1] + u * knots[2],
                u * knots[1] + v * knots[2],
                knots[2]);
            return SegmentType::BetweenCorners;
        }
        else {
            // (d01 == 0) && (d12 > 0) && (d23 > 0)
            //
            // Creates an imaginary control point p0 that would extrapolate the
            // curve, defined as:
            //
            //        p1    p2
            //         o----o         distance(p0, p1)  == distance(p1, p2)
            //        '      `        angle(p0, p1, p2) == angle(p1, p2, p3)
            //       o        `       w1 - w0           == w2 - w1
            //    p0           `
            //                  o p3
            //
            // Similarly to using "mirror tangents", this prevents ugly
            // inflexion points that would happen by keeping p0 = p1, as
            // illustrated here: https://github.com/vgc/vgc/pull/1341
            //
            Vec2d d = p2p3 / d23;                    // unit vector to reflect
            Vec2d n = (p1p2 / d12).orthogonalized(); // unit axis of reflexion
            Vec2d q = 2 * d.dot(n) * n - d;          // refection of d along n
            knots[0] = knots[1] + d12 * q;
            fixedChordLengths[0] = d12;
            result = SegmentType::AfterCorner;
        }
    }
    else if (isBeforeCorner) {
        // (d01 > 0) && (d12 > 0) && (d23 == 0)
        //
        // Similar as AfterCorner case above.
        Vec2d d = -p0p1 / d01;
        Vec2d n = (p1p2 / d12).orthogonalized();
        Vec2d q = 2 * d.dot(n) * n - d;
        knots[3] = knots[2] + d12 * q;
        fixedChordLengths[2] = d12;
        result = SegmentType::BeforeCorner;
    }

    switch (parameterization) {
    case CatmullRomSplineParameterization::Uniform: {
        bezier = uniformCatmullRomToBezier<Vec2d>(knots);
        break;
    }
    case CatmullRomSplineParameterization::Centripetal: {
        bezier = centripetalCatmullRomToBezier<Vec2d>(knots, fixedChordLengths);
        break;
    }
    }

    return result;
}

void computeSegmentHalfwidthsCubicBezier_(
    CubicBezier2d& bezier,
    core::ConstSpan<double> knotWidths,
    const std::array<Int, 4>& knotIndices,
    const std::array<Vec2d, 4>& centerlineControlPoints,
    const std::array<double, 3>& fixedChordLengths,
    SegmentType segmentType) {

    std::array<double, 4> hws = {
        0.5 * knotWidths.getUnchecked(knotIndices[0]),
        0.5 * knotWidths.getUnchecked(knotIndices[1]),
        0.5 * knotWidths.getUnchecked(knotIndices[2]),
        0.5 * knotWidths.getUnchecked(knotIndices[3])};

    std::array<Vec2d, 4> knots = {
        Vec2d(hws[0], hws[0]),
        Vec2d(hws[1], hws[1]),
        Vec2d(hws[2], hws[2]),
        Vec2d(hws[3], hws[3])};

    // Aliases
    const double d01 = fixedChordLengths[0];
    const double d12 = fixedChordLengths[1];
    const double d23 = fixedChordLengths[2];

    std::array<Vec2d, 2> fixedNeighborKnots;

    // Handle "corner knots", defined as:
    // 1. Two consecutive equal points, or
    // 2. The first/last knot of an open curve
    //
    switch (segmentType) {
    case SegmentType::None: {
        fixedNeighborKnots[0] = knots[0];
        fixedNeighborKnots[1] = knots[3];
        break;
    }
    case SegmentType::BetweenCorners:
    case SegmentType::Corner: {
        double u = 1.0 / 3;
        double v = (1 - u);
        bezier = CubicBezier2d(
            knots[1], //
            v * knots[1] + u * knots[2],
            u * knots[1] + v * knots[2],
            knots[2]);
        // Fast return.
        return;
    }
    case SegmentType::AfterCorner: {
        // Imaginary control point, see `initPositions_()`.
        fixedNeighborKnots[0] = 2 * knots[1] - knots[2];
        fixedNeighborKnots[1] = knots[3];
        break;
    }
    case SegmentType::BeforeCorner: {
        // Imaginary control point, see `initPositions_()`.
        fixedNeighborKnots[0] = knots[0];
        fixedNeighborKnots[1] = 2 * knots[2] - knots[1];
        break;
    }
    }

    // Compute Bézier control points for halfwidths such that on both sides of
    // each knot we have the same desired dw/ds.
    //
    double d012 = d01 + d12;
    double d123 = d12 + d23;
    // desired dw/ds at start/end
    Vec2d dhw_ds_1 = (knots[2] - fixedNeighborKnots[0]) / d012;
    Vec2d dhw_ds_2 = (fixedNeighborKnots[1] - knots[1]) / d123;
    // 1/3 of ds/du at start/end
    double ds_du_1 = (centerlineControlPoints[1] - centerlineControlPoints[0]).length();
    double ds_du_2 = (centerlineControlPoints[3] - centerlineControlPoints[2]).length();
    // w1 - w0 = 1/3 of dw/du at start; w3 - w2 = 1/3 of dw/du at end
    Vec2d hw1 = knots[1] + dhw_ds_1 * ds_du_1;
    Vec2d hw2 = knots[2] - dhw_ds_2 * ds_du_2;

    bezier = CubicBezier2d(knots[1], hw1, hw2, knots[2]);
}

} // namespace

CubicBezier2d CatmullRomSplineStroke2d::segmentToBezier(Int segmentIndex) const {
    CubicBezier2d centerlineBezier(core::noInit);

    std::array<Int, 4> knotIndices =
        computeKnotIndices_(isClosed(), positions().length(), segmentIndex);

    std::array<double, 3> fixedChordLengths;
    computeSegmentCenterlineCubicBezier_(
        centerlineBezier,
        parametrization_,
        positions(),
        chordLengths(),
        knotIndices,
        fixedChordLengths);

    return centerlineBezier;
}

CubicBezier2d CatmullRomSplineStroke2d::segmentToBezier(
    Int segmentIndex,
    CubicBezier2d& halfwidths) const {

    CubicBezier2d centerlineBezier(core::noInit);

    std::array<Int, 4> knotIndices =
        computeKnotIndices_(isClosed(), positions().length(), segmentIndex);

    std::array<double, 3> fixedChordLengths;
    SegmentType segmentType = computeSegmentCenterlineCubicBezier_(
        centerlineBezier,
        parametrization_,
        positions(),
        chordLengths(),
        knotIndices,
        fixedChordLengths);

    if (isWidthConstant_) {
        double chw = 0.5 * constantWidth();
        Vec2d cp(chw, chw);
        halfwidths = CubicBezier2d(cp, cp, cp, cp);
    }
    else {
        computeSegmentHalfwidthsCubicBezier_(
            halfwidths,
            widths(),
            knotIndices,
            centerlineBezier.controlPoints(),
            fixedChordLengths,
            segmentType);
    }

    return centerlineBezier;
}

void CatmullRomSplineStroke2d::computeChordLengths_() {
    const auto& positions = this->positions();
    const Vec2d* p = positions.data();
    Int n = positions.length();
    chordLengths_.resizeNoInit(n);
    double* chordLengths = chordLengths_.data();
    if (n > 0) {
        for (Int i = 0; i < n - 1; ++i) {
            chordLengths[i] = (p[i + 1] - p[i]).length();
        }
        // We compute the closure even if the spline is not closed.
        chordLengths[n - 1] = (p[n - 1] - p[0]).length();
    }
}

} // namespace vgc::geometry
