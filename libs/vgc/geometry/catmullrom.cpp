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

Int CatmullRomSplineStroke2d::numKnots_() const {
    return positions_.length();
}

bool CatmullRomSplineStroke2d::isZeroLengthSegment_(Int segmentIndex) const {
    computeCache_();
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
        CubicBezier1d tu = segmentToNormalReparametrization(segmentIndex);
        double hw = 0.5 * widths_[0];
        Vec2d dp(core::noInit);
        Vec2d p = centerlineBezier.eval(u, dp);
        double t = tu.eval(u);
        Vec2d dp2 = centerlineBezier.evalDerivative(t);
        double speed = dp.length();
        return StrokeSampleEx2d(
            p, dp / speed, dp2.normalized().orthogonalized(), hw, speed, segmentIndex, u);
    }
    else {
        CubicBezier2d halfwidthsBezier(core::noInit);
        CubicBezier2d centerlineBezier = segmentToBezier(segmentIndex, halfwidthsBezier);
        CubicBezier1d tu = segmentToNormalReparametrization(segmentIndex);
        Vec2d dp(core::noInit);
        Vec2d p = centerlineBezier.eval(u, dp);
        double t = tu.eval(u);
        Vec2d dp2 = centerlineBezier.evalDerivative(t);
        double speed = dp.length();
        Vec2d hw = halfwidthsBezier.eval(u);
        return StrokeSampleEx2d(
            p, dp / speed, dp2.normalized().orthogonalized(), hw, speed, segmentIndex, u);
    }
}

void CatmullRomSplineStroke2d::sampleNonZeroSegment(
    StrokeSampleEx2dArray& out,
    Int segmentIndex,
    const CurveSamplingParameters& params,
    detail::AdaptiveStrokeSampler& sampler) const {

    if (isWidthConstant_) {
        CubicBezier2d centerlineBezier = segmentToBezier(segmentIndex);
        CubicBezier1d tu = segmentToNormalReparametrization(segmentIndex);
        double hw = 0.5 * widths_[0];
        sampler.sample(
            [&, hw](double u) -> StrokeSampleEx2d {
                Vec2d dp(core::noInit);
                Vec2d p = centerlineBezier.eval(u, dp);
                double t = tu.eval(u);
                Vec2d dp2 = centerlineBezier.evalDerivative(t);
                double speed = dp.length();
                return StrokeSampleEx2d(
                    p,
                    dp / speed,
                    dp2.normalized().orthogonalized(),
                    hw,
                    speed,
                    segmentIndex,
                    u);
            },
            params,
            out);
    }
    else {
        CubicBezier2d halfwidthsBezier(core::noInit);
        CubicBezier2d centerlineBezier = segmentToBezier(segmentIndex, halfwidthsBezier);
        CubicBezier1d tu = segmentToNormalReparametrization(segmentIndex);
        sampler.sample(
            [&](double u) -> StrokeSampleEx2d {
                Vec2d dp(core::noInit);
                Vec2d p = centerlineBezier.eval(u, dp);
                double t = tu.eval(u);
                Vec2d dp2 = centerlineBezier.evalDerivative(t);
                double speed = dp.length();
                Vec2d hw = halfwidthsBezier.eval(u);
                return StrokeSampleEx2d(
                    p,
                    dp / speed,
                    dp2.normalized().orthogonalized(),
                    hw,
                    speed,
                    segmentIndex,
                    u);
            },
            params,
            out);
    }
}

StrokeSampleEx2d CatmullRomSplineStroke2d::zeroLengthStrokeSample() const {
    return StrokeSampleEx2d(
        positions().first(), Vec2d(0, 1), Vec2d(-1, 0), 0.5 /*constantHalfwidth_*/, 0, 0);
}

namespace {

struct PointData {
    Vec2d p = core::noInit;
    Vec2d dp = core::noInit;
    Vec2d ddp = core::noInit;
    Vec2d w = core::noInit;
    Vec2d dw = core::noInit;
    double speed;
    double curvature;
};

PointData computeSegmentEndpointData(
    const CubicBezier2d& centerlineBezier,
    const CubicBezier2d& halfwidthsBezier,
    Int endpointIndex) {

    const std::array<Vec2d, 4>& positions = centerlineBezier.controlPoints();
    const std::array<Vec2d, 4>& halfwidths = halfwidthsBezier.controlPoints();

    PointData result;

    Vec2d dp = core::noInit;
    Vec2d ddp = core::noInit;
    if (endpointIndex) {
        result.p = positions[3];
        dp = 3 * (positions[3] - positions[2]);
        result.dp = dp;
        ddp = 6 * (positions[3] - 2 * positions[2] + positions[1]);
        result.ddp = ddp;
        result.w = halfwidths[3];
        result.dw = 3 * (halfwidths[3] - halfwidths[2]);
    }
    else {
        result.p = positions[0];
        dp = 3 * (positions[1] - positions[0]);
        result.dp = dp;
        ddp = 6 * (positions[2] - 2 * positions[1] + positions[0]);
        result.ddp = ddp;
        result.w = halfwidths[0];
        result.dw = 3 * (halfwidths[1] - halfwidths[0]);
    }

    // todo: ensure speed is non zero
    double speed = dp.length();
    result.speed = speed;
    if (speed == 0) {
        result.curvature = core::DoubleInfinity;
    }
    else {
        result.curvature = ddp.det(dp) / (speed * speed * speed);
    }

    return result;
}

// Currently assumes first derivative at endpoint is non null.
// TODO: Support null derivatives (using limit analysis).
// TODO: Support different halfwidth on both sides.
std::array<Vec2d, 2> computeOffsetLineTangents(const PointData& data) {

    Vec2d dp = data.dp;
    double dpl = data.speed;
    Vec2d n = dp.orthogonalized() / dpl;
    Vec2d dn = dp * data.curvature;

    Vec2d offset0 = dn * data.w[0] + n * data.dw[0];
    Vec2d offset1 = -(dn * data.w[1] + n * data.dw[1]);
    return {(dp + offset0).normalized(), (dp + offset1).normalized()};
}

} // namespace

std::array<Vec2d, 2>
CatmullRomSplineStroke2d::computeOffsetLineTangentsAtSegmentEndpoint_(
    Int segmentIndex,
    Int endpointIndex) const {

    CubicBezier2d halfwidthBezier(core::noInit);
    CubicBezier2d positionsBezier = segmentToBezier(segmentIndex, halfwidthBezier);

    PointData data =
        computeSegmentEndpointData(positionsBezier, halfwidthBezier, endpointIndex);

    return computeOffsetLineTangents(data);
}

namespace {

struct SegmentComputeData {
    std::array<Int, 4> knotIndices;
    std::array<double, 3> imaginaryChordLengths;
    CubicBezier2d centerlineBezier;
    std::array<PointData, 2> endpointDataPair;
};

Vec2dArray computeChords(core::ConstSpan<Vec2d> knotPositions) {
    const Vec2d* p = knotPositions.data();
    Int n = knotPositions.length();
    Vec2dArray chords(n, core::noInit);
    if (n > 0) {
        for (Int i = 0; i < n - 1; ++i) {
            chords[i] = p[i + 1] - p[i];
        }
        // Last chord is the closure.
        chords[n - 1] = p[n - 1] - p[0];
    }
    return chords;
}

void computeLengths(const Vec2dArray& vectors, core::DoubleArray& outLengths) {
    Int n = vectors.length();
    outLengths.resizeNoInit(n);
    for (Int i = 0; i < n; ++i) {
        outLengths[i] = vectors[i].length();
    }
}

void checkSegmentIndexIsValid(Int segmentIndex, Int numSegments) {
    VGC_ASSERT(segmentIndex >= 0);
    VGC_ASSERT(segmentIndex < numSegments);
}

// Assumes segmentIndex is valid.
void computeSegmentKnotAndChordIndices(
    Int numKnots,
    bool isClosed,
    Int segmentIndex,
    std::array<Int, 4>& outKnotIndices,
    std::array<Int, 3>& outChordIndices) {

    // Get indices of points used by the Catmull-Rom interpolation, handle
    // wrapping for closed curves and boundary for open curves.
    outKnotIndices = {segmentIndex - 1, segmentIndex, segmentIndex + 1, segmentIndex + 2};
    outChordIndices = {segmentIndex - 1, segmentIndex, segmentIndex + 1};

    if (isClosed) {
        if (outKnotIndices[0] < 0) {
            outKnotIndices[0] = numKnots - 1;
            outChordIndices[0] = numKnots - 1;
        }
        if (outKnotIndices[2] > numKnots - 1) {
            outKnotIndices[2] = 0;
            outChordIndices[2] = 0;
            outKnotIndices[3] = 1;
        }
        if (outKnotIndices[3] > numKnots - 1) {
            outKnotIndices[3] = 0;
        }
    }
    else {
        Int zeroLengthChordIndex = numKnots - 1;
        if (outKnotIndices[0] < 0) {
            outKnotIndices[0] = 0;
            outChordIndices[0] = zeroLengthChordIndex;
        }
        if (outKnotIndices[2] > numKnots - 1) {
            outKnotIndices[2] = numKnots - 1;
            outChordIndices[2] = zeroLengthChordIndex;
            outKnotIndices[3] = numKnots - 1;
        }
        else if (outKnotIndices[3] > numKnots - 1) {
            outKnotIndices[3] = numKnots - 1;
        }
    }
}

template<std::size_t I, typename... T, std::size_t... Is>
constexpr std::tuple<std::tuple_element_t<I + Is, std::tuple<T...>>...>
SubPackAsTuple_(std::index_sequence<Is...>) {
}

template<typename T, size_t n, size_t... Is>
std::array<T, n> getElementsUnchecked_(
    const core::Array<T>& arr,
    const std::array<Int, n>& indices,
    std::index_sequence<Is...>) {

    return std::array<T, n>{arr.getUnchecked(indices[Is])...};
}

template<typename T, size_t n>
std::array<T, n>
getElementsUnchecked(const core::Array<T>& arr, const std::array<Int, n>& indices) {
    return getElementsUnchecked_(arr, indices, std::make_index_sequence<n>());
}

CurveSegmentType
computeSegmentTypeFromChordLengths(const std::array<double, 3>& segmentChordLengths) {
    if (segmentChordLengths[1] == 0) {
        return CurveSegmentType::Corner;
    }
    bool isAfterCorner = (segmentChordLengths[0] == 0);
    bool isBeforeCorner = (segmentChordLengths[2] == 0);
    if (isAfterCorner) {
        if (isBeforeCorner) {
            return CurveSegmentType::BetweenCorners;
        }
        return CurveSegmentType::AfterCorner;
    }
    else if (isBeforeCorner) {
        return CurveSegmentType::BeforeCorner;
    }
    else {
        return CurveSegmentType::Simple;
    }
}

CubicBezier2d computeSegmentCenterlineCubicBezier(
    CatmullRomSplineParameterization parameterization,
    const std::array<Vec2d, 4>& knots,
    const std::array<Vec2d, 3>& chords,
    const std::array<double, 3>& chordLengths,
    CurveSegmentType segmentType,
    std::array<double, 3>& outImaginaryChordLengths) {

    CubicBezier2d result(core::noInit);

    // Aliases
    const Vec2d p0p1 = chords[0];
    const Vec2d p1p2 = chords[1];
    const Vec2d p2p3 = chords[2];
    const double d01 = chordLengths[0];
    const double d12 = chordLengths[1];
    const double d23 = chordLengths[2];

    std::array<Vec2d, 4> imaginaryKnots = knots;
    outImaginaryChordLengths = chordLengths;

    // Handle "corner knots", defined as:
    // 1. Two consecutive equal points, or
    // 2. The first/last knot of an open curve
    //
    switch (segmentType) {
    case CurveSegmentType::Simple: {
        break;
    }
    case CurveSegmentType::Corner: {
        result = CubicBezier2d(knots);
        return result;
    }
    case CurveSegmentType::AfterCorner: {
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
        imaginaryKnots[0] = knots[1] + d12 * q;
        outImaginaryChordLengths[0] = d12;
        break;
    }
    case CurveSegmentType::BeforeCorner: {
        // (d01 > 0) && (d12 > 0) && (d23 == 0)
        //
        // Similar as AfterCorner case above.
        Vec2d d = -p0p1 / d01;
        Vec2d n = (p1p2 / d12).orthogonalized();
        Vec2d q = 2 * d.dot(n) * n - d;
        imaginaryKnots[3] = knots[2] + d12 * q;
        outImaginaryChordLengths[2] = d12;
        break;
    }
    case CurveSegmentType::BetweenCorners: {
        // (d01 == 0) && (d12 > 0) && (d23 == 0)
        //
        // Linear parameterization
        double u = 1.0 / 3;
        double v = (1 - u);
        result = CubicBezier2d(
            knots[1], v * knots[1] + u * knots[2], u * knots[1] + v * knots[2], knots[2]);
        return result;
    }
    }

    switch (parameterization) {
    case CatmullRomSplineParameterization::Uniform: {
        result = uniformCatmullRomToBezier<Vec2d>(imaginaryKnots);
        break;
    }
    case CatmullRomSplineParameterization::Centripetal: {
        result = centripetalCatmullRomToBezier<Vec2d>(
            imaginaryKnots, outImaginaryChordLengths);
        break;
    }
    case CatmullRomSplineParameterization::Chordal: {
        result =
            chordalCatmullRomToBezier<Vec2d>(imaginaryKnots, outImaginaryChordLengths);
        break;
    }
    }

    // Test: fix tangent lengths using OGH
    // (Optimized Geometric Hermite)
    //
    constexpr bool enableOGHSpeeds = true;
    if (enableOGHSpeeds) {
        const std::array<Vec2d, 4>& cps = result.controlPoints();
        Vec2d ab = cps[3] - cps[0];
        Vec2d v0 = (cps[1] - cps[0]).normalized();
        Vec2d v1 = (cps[3] - cps[2]).normalized();

        double abDotV0 = ab.dot(v0);
        double abDotV1 = ab.dot(v1);
        double v0v1 = v0.dot(v1);

        double den = 4 - v0v1 * v0v1;
        double a0 = std::abs((6 * abDotV0 - 3 * abDotV1 * v0v1) / den);
        double a1 = std::abs((6 * abDotV1 - 3 * abDotV0 * v0v1) / den);

        std::array<Vec2d, 4> newCps = {
            cps[0], cps[0] + v0 * a0 / 3, cps[3] - v1 * a1 / 3, cps[3]};

        result = CubicBezier2d(newCps);
    }

    return result;
}

CubicBezier2d computeSegmentHalfwidthsCubicBezier(
    const std::array<double, 4>& knots,
    const std::array<Vec2d, 4>& centerlineControlPoints,
    const std::array<double, 3>& imaginaryChordLengths,
    CurveSegmentType segmentType) {

    CubicBezier2d result(core::noInit);

    std::array<double, 4> hws = {
        0.5 * knots[0], 0.5 * knots[1], 0.5 * knots[2], 0.5 * knots[3]};

    std::array<Vec2d, 4> hwKnots = {
        Vec2d(hws[0], hws[0]),
        Vec2d(hws[1], hws[1]),
        Vec2d(hws[2], hws[2]),
        Vec2d(hws[3], hws[3])};

    // Aliases
    const double d01 = imaginaryChordLengths[0];
    const double d12 = imaginaryChordLengths[1];
    const double d23 = imaginaryChordLengths[2];

    std::array<Vec2d, 2> imaginaryHwKnots = {hwKnots[0], hwKnots[3]};

    // Handle "corner knots", defined as:
    // 1. Two consecutive equal points, or
    // 2. The first/last knot of an open curve
    //
    switch (segmentType) {
    case CurveSegmentType::Simple: {
        break;
    }
    case CurveSegmentType::BetweenCorners:
    case CurveSegmentType::Corner: {
        double u = 1.0 / 3;
        double v = (1 - u);
        result = CubicBezier2d(
            hwKnots[1], //
            v * hwKnots[1] + u * hwKnots[2],
            u * hwKnots[1] + v * hwKnots[2],
            hwKnots[2]);
        // Fast return.
        return result;
    }
    case CurveSegmentType::AfterCorner: {
        // Imaginary control point, see `initPositions_()`.
        imaginaryHwKnots[0] = 2 * hwKnots[1] - hwKnots[2];
        break;
    }
    case CurveSegmentType::BeforeCorner: {
        // Imaginary control point, see `initPositions_()`.
        imaginaryHwKnots[1] = 2 * hwKnots[2] - hwKnots[1];
        break;
    }
    }

    // Compute Bézier control points for halfwidths such that on both sides of
    // each knot we have the same desired dw/ds.
    //
    double d012 = d01 + d12;
    double d123 = d12 + d23;
    // desired dw/ds at start/end
    Vec2d dhw_ds_1 = (hwKnots[2] - imaginaryHwKnots[0]) / d012;
    Vec2d dhw_ds_2 = (imaginaryHwKnots[1] - hwKnots[1]) / d123;
    // 1/3 of ds/du at start/end
    double ds_du_1 = (centerlineControlPoints[1] - centerlineControlPoints[0]).length();
    double ds_du_2 = (centerlineControlPoints[3] - centerlineControlPoints[2]).length();
    // w1 - w0 = 1/3 of dw/du at start; w3 - w2 = 1/3 of dw/du at end
    Vec2d hw1 = hwKnots[1] + dhw_ds_1 * ds_du_1;
    Vec2d hw2 = hwKnots[2] - dhw_ds_2 * ds_du_2;

    result = CubicBezier2d(hwKnots[1], hw1, hw2, hwKnots[2]);
    return result;
}

} // namespace

CubicBezier2d CatmullRomSplineStroke2d::segmentToBezier(Int segmentIndex) const {
    CubicBezier2d centerlineBezier(core::noInit);

    computeCache_();

    Int numSegments = this->numSegments();
    checkSegmentIndexIsValid(segmentIndex, numSegments);

    Int numKnots = this->numKnots();
    bool isClosed = this->isClosed();

    Int i0 = segmentIndex;
    Int i3 = segmentIndex + 1;
    if (i3 >= numKnots) {
        i3 = isClosed ? 0 : segmentIndex;
    }
    Int j = i0 * 2;

    const Vec2d* p = positions_.data();
    const Vec2d* cp = centerlineControlPoints_.data();
    return CubicBezier2d(p[i0], cp[j], cp[j + 1], p[i3]);
}

CubicBezier2d CatmullRomSplineStroke2d::segmentToBezier(
    Int segmentIndex,
    CubicBezier2d& halfwidths) const {

    computeCache_();

    Int numSegments = this->numSegments();
    checkSegmentIndexIsValid(segmentIndex, numSegments);

    Int numKnots = this->numKnots();
    bool isClosed = this->isClosed();

    Int i0 = segmentIndex;
    Int i3 = segmentIndex + 1;
    if (i3 >= numKnots) {
        i3 = isClosed ? 0 : segmentIndex;
    }
    Int j = i0 * 2;

    if (isWidthConstant_) {
        double chw = 0.5 * constantWidth();
        Vec2d cp(chw, chw);
        halfwidths = CubicBezier2d(cp, cp, cp, cp);
    }
    else {
        const double* w = widths_.data();
        const Vec2d* chw = halfwidthsControlPoints_.data();
        double hw0 = 0.5 * w[i0];
        double hw3 = 0.5 * w[i3];
        halfwidths = CubicBezier2d(Vec2d(hw0, hw0), chw[j], chw[j + 1], Vec2d(hw3, hw3));
    }

    const Vec2d* p = positions_.data();
    const Vec2d* cp = centerlineControlPoints_.data();
    return CubicBezier2d(p[i0], cp[j], cp[j + 1], p[i3]);
}

CubicBezier1d
CatmullRomSplineStroke2d::segmentToNormalReparametrization(Int segmentIndex) const {

    computeCache_();

    Int numSegments = this->numSegments();
    checkSegmentIndexIsValid(segmentIndex, numSegments);

    Int i = segmentIndex;

    const Vec2d* cp = normalReparametrizationControlValues_.data();
    return CubicBezier1d(0, cp[i][0], cp[i][1], 1);
}

void CatmullRomSplineStroke2d::computeCache_() const {

    if (!isCacheDirty_) {
        return;
    }

    const auto& positions = this->positions();
    const auto& widths = this->widths();
    Int numKnots = positions.length();
    Int numSegments = this->numSegments();
    bool isClosed = this->isClosed();

    Vec2dArray chords = computeChords(positions);
    if (!isClosed) {
        chords.last() = Vec2d();
    }

    bool updateSegmentTypes = false;
    if (chordLengths_.isEmpty()) {
        computeLengths(chords, chordLengths_);
        segmentTypes_.resizeNoInit(numSegments);
        updateSegmentTypes = true;
    }

    centerlineControlPoints_.resizeNoInit(numSegments * 2);
    halfwidthsControlPoints_.resizeNoInit(numSegments * 2);
    normalReparametrizationControlValues_.resizeNoInit(numSegments);

    core::Array<SegmentComputeData> computeDataArray(numSegments, core::noInit);

    // todo: Indices are special only for a few cases, we could make a special loop
    //       instead of doing special case logic on indices at each iteration.
    //       At least, computeData allows for not doing it twice.

    for (Int i = 0; i < numSegments; ++i) {
        SegmentComputeData& computeData = computeDataArray[i];

        std::array<Int, 4> segKnotIndices;
        std::array<Int, 3> segChordIndices;
        computeSegmentKnotAndChordIndices(
            numKnots, isClosed, i, segKnotIndices, segChordIndices);
        computeData.knotIndices = segKnotIndices;

        std::array<Vec2d, 4> segPosKnots =
            getElementsUnchecked(positions, segKnotIndices);

        std::array<Vec2d, 3> segChords = getElementsUnchecked(chords, segChordIndices);
        std::array<double, 3> segChordLengths =
            getElementsUnchecked(chordLengths_, segChordIndices);

        CurveSegmentType segmentType;
        if (updateSegmentTypes) {
            segmentType = computeSegmentTypeFromChordLengths(segChordLengths);
            segmentTypes_.getUnchecked(i) = segmentType;
        }
        else {
            segmentType = segmentTypes_[i];
        }

        CubicBezier2d centerlineBezier = computeSegmentCenterlineCubicBezier(
            parameterization_,
            segPosKnots,
            segChords,
            segChordLengths,
            segmentType,
            computeData.imaginaryChordLengths);
        computeData.centerlineBezier = centerlineBezier;

        Int j = i * 2;
        centerlineControlPoints_.getUnchecked(j) = centerlineBezier.controlPoint1();
        centerlineControlPoints_.getUnchecked(j + 1) = centerlineBezier.controlPoint2();
    }

    CubicBezier2d halfwidthsBezier(core::noInit);

    for (Int i = 0; i < numSegments; ++i) {
        SegmentComputeData& computeData = computeDataArray[i];

        std::array<double, 4> segWidthKnots =
            getElementsUnchecked(widths, computeData.knotIndices);

        CurveSegmentType segmentType = segmentTypes_[i];

        const CubicBezier2d& centerlineBezier = computeData.centerlineBezier;
        halfwidthsBezier = computeSegmentHalfwidthsCubicBezier(
            segWidthKnots,
            centerlineBezier.controlPoints(),
            computeData.imaginaryChordLengths,
            segmentType);

        computeData.endpointDataPair[0] =
            computeSegmentEndpointData(centerlineBezier, halfwidthsBezier, 0);
        computeData.endpointDataPair[1] =
            computeSegmentEndpointData(centerlineBezier, halfwidthsBezier, 1);

        Int j = i * 2;
        halfwidthsControlPoints_.getUnchecked(j) = halfwidthsBezier.controlPoint1();
        halfwidthsControlPoints_.getUnchecked(j + 1) = halfwidthsBezier.controlPoint2();
    }

    // Here we relax tangents to make their derivative continuous.
    constexpr bool enableRelaxedNormals = true;
    for (Int i = 0; i < numSegments; ++i) {
        SegmentComputeData& computeData = computeDataArray[i];

        CurveSegmentType segmentType = segmentTypes_[i];

        normalReparametrizationControlValues_[i] = Vec2d(1. / 3, 2. / 3);

        if (enableRelaxedNormals && (isClosed || i > 1)
            && (segmentType == CurveSegmentType::Simple
                || segmentType == CurveSegmentType::BeforeCorner)) {

            Int previousSegmentIndex = i - 1;
            if (previousSegmentIndex < 0) {
                previousSegmentIndex = numSegments - 1;
            }
            SegmentComputeData& previousSegmentComputeData =
                computeDataArray[previousSegmentIndex];

            PointData& ed0 = previousSegmentComputeData.endpointDataPair[1];
            PointData& ed1 = computeData.endpointDataPair[0];

            double np0 = std::abs(ed0.curvature /* * ed0.speed*/);
            double np1 = std::abs(ed1.curvature /* * ed1.speed*/);

            Int i0 = previousSegmentIndex;
            Int i1 = i;

            if (ed0.curvature * ed1.curvature < 0) {
                // Signs of curvature are different, it is impossible to make
                // k1 match k2 with a positive coefficient.
                // Thus we force both sides to have a fake curvature of 0
                // (fake C1 inflexion point).
                normalReparametrizationControlValues_[i0][1] = 1;
                normalReparametrizationControlValues_[i1][0] = 0;
            }
            else if (np0 < np1) {
                double dt = np0 / np1;
                normalReparametrizationControlValues_[i1][0] = dt / 3;
            }
            else if (np1 < np0) {
                double dt = np1 / np0;
                normalReparametrizationControlValues_[i0][1] = 1.0 - dt / 3;
            }
        }
    }

    constexpr bool enableOffsetLineTangentContinuity = false;
    if (enableOffsetLineTangentContinuity) {
        // Here we try to make offset line tangent continuous at start knot.
        for (Int i = 0; i < numSegments; ++i) {
            SegmentComputeData& computeData = computeDataArray[i];

            CurveSegmentType segmentType = segmentTypes_[i];

            if ((isClosed || i > 1)
                && (segmentType == CurveSegmentType::Simple
                    || segmentType == CurveSegmentType::BeforeCorner)) {
                Int previousSegmentIndex = i - 1;
                if (previousSegmentIndex < 0) {
                    previousSegmentIndex = numSegments - 1;
                }
                SegmentComputeData& previousSegmentComputeData =
                    computeDataArray[previousSegmentIndex];

                // basically we want to change a single width control point.

                PointData& ed0 = previousSegmentComputeData.endpointDataPair[1];
                PointData& ed1 = computeData.endpointDataPair[0];

                if (std::abs(ed0.curvature) < std::abs(ed1.curvature)) {
                    // use previous segment offset line end tangents as
                    // current segment offset line start tangents.

                    double speed0 = ed0.speed;
                    double k0 = ed0.curvature;
                    double tc00 = (1 + k0 * ed0.w[0]) * speed0;
                    double tc01 = (1 - k0 * ed0.w[1]) * speed0;
                    double nc00 = ed0.dw[0];
                    double nc01 = -ed0.dw[1];
                    double do00_du = nc00 / tc00;
                    double do01_du = nc01 / tc01;

                    // do10_du = 3 *  (hw110 - hw100) / ((1 + k1 * hw100) * speed1)
                    // do11_du = 3 * -(hw111 - hw101) / ((1 - k1 * hw101) * speed1)
                    //
                    // hw110 - hw100 =  do10_du * (1 + k1 * hw100) * speed1 / 3
                    // hw111 - hw101 = -do11_du * (1 - k1 * hw101) * speed1 / 3
                    //
                    // hw110 =  do10_du * (1 + k1 * hw100) * speed1 / 3 + hw100
                    // hw111 = -do11_du * (1 - k1 * hw101) * speed1 / 3 + hw101
                    //
                    // we want do10_du == do00_du and do11_du == do01_du

                    double hw10 = 0.5 * widths[i];
                    Vec2d hws10(hw10, hw10);
                    double speed1 = ed1.speed;
                    double k1 = ed1.curvature;
                    double hw110 = +do00_du * (1 + k1 * hws10[0]) * speed1 / 3 + hws10[0];
                    double hw111 = -do01_du * (1 - k1 * hws10[1]) * speed1 / 3 + hws10[1];

                    Int j = i * 2;
                    halfwidthsControlPoints_[j] = Vec2d(hw110, hw111);
                }
                else {
                    // use current segment offset line start tangents as
                    // previous segment offset line end tangents.

                    double speed1 = ed1.speed;
                    double k1 = ed1.curvature;
                    double tc10 = (1 + k1 * ed1.w[0]) * speed1;
                    double tc11 = (1 - k1 * ed1.w[1]) * speed1;
                    double nc10 = ed1.dw[0];
                    double nc11 = -ed1.dw[1];
                    double do10_du = nc10 / tc10;
                    double do11_du = nc11 / tc11;

                    // do00_du = 3 * -(hw020 - hw030) / ((1 + k0 * hw030) * speed0)
                    // do01_du = 3 *  (hw021 - hw031) / ((1 - k0 * hw031) * speed0)
                    //
                    // hw020 - hw030 = -do00_du * (1 + k0 * hw030) * speed0 / 3
                    // hw021 - hw031 =  do01_du * (1 - k0 * hw031) * speed0 / 3
                    //
                    // hw020 = -do00_du * (1 + k0 * hw030) * speed0 / 3 + hw030
                    // hw021 =  do01_du * (1 - k0 * hw031) * speed0 / 3 + hw031
                    //
                    // we want do00_du == do10_du and do01_du == do11_du

                    double hw03 = 0.5 * widths[i];
                    Vec2d hws03(hw03, hw03);
                    double speed0 = ed0.speed;
                    double k0 = ed0.curvature;
                    double hw020 = -do10_du * (1 + k0 * hws03[0]) * speed0 / 3 + hws03[0];
                    double hw021 = +do11_du * (1 - k0 * hws03[1]) * speed0 / 3 + hws03[1];

                    Int j = previousSegmentIndex * 2;
                    halfwidthsControlPoints_[j + 1] = Vec2d(hw020, hw021);
                }
            }
        }
    }

    isCacheDirty_ = false;
}

} // namespace vgc::geometry
