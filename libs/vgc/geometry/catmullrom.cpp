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

namespace vgc::geometry {

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
    if (hasConstantWidth()) {
        CubicBezier2d centerlineBezier = segmentToBezier(segmentIndex);
        CubicBezier1d tu = segmentToNormalReparametrization(segmentIndex);
        double hw = 0.5 * constantWidth();
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

    if (hasConstantWidth()) {
        CubicBezier2d centerlineBezier = segmentToBezier(segmentIndex);
        CubicBezier1d tu = segmentToNormalReparametrization(segmentIndex);
        double hw = 0.5 * constantWidth();
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

CubicBezier2d CatmullRomSplineStroke2d::segmentToBezier(Int segmentIndex) const {
    CubicBezier2d centerlineBezier(core::noInit);

    updateCache();

    Int numSegments = this->numSegments();
    detail::checkSegmentIndexIsValid(segmentIndex, numSegments);

    Int numKnots = this->numKnots();
    bool isClosed = this->isClosed();

    Int i0 = segmentIndex;
    Int i3 = segmentIndex + 1;
    if (i3 >= numKnots) {
        i3 = isClosed ? 0 : segmentIndex;
    }
    Int j = i0 * 2;

    const Vec2d* p = positions().data();
    const Vec2d* cp = centerlineControlPoints_.data();
    return CubicBezier2d(p[i0], cp[j], cp[j + 1], p[i3]);
}

CubicBezier2d CatmullRomSplineStroke2d::segmentToBezier(
    Int segmentIndex,
    CubicBezier2d& halfwidths) const {

    updateCache();

    Int numSegments = this->numSegments();
    detail::checkSegmentIndexIsValid(segmentIndex, numSegments);

    Int numKnots = this->numKnots();
    bool isClosed = this->isClosed();

    Int i0 = segmentIndex;
    Int i3 = segmentIndex + 1;
    if (i3 >= numKnots) {
        i3 = isClosed ? 0 : segmentIndex;
    }
    Int j = i0 * 2;

    if (hasConstantWidth()) {
        double chw = 0.5 * constantWidth();
        Vec2d cp(chw, chw);
        halfwidths = CubicBezier2d(cp, cp, cp, cp);
    }
    else {
        const double* w = widths().data();
        const Vec2d* chw = halfwidthsControlPoints_.data();
        double hw0 = 0.5 * w[i0];
        double hw3 = 0.5 * w[i3];
        halfwidths = CubicBezier2d(Vec2d(hw0, hw0), chw[j], chw[j + 1], Vec2d(hw3, hw3));
    }

    const Vec2d* p = positions().data();
    const Vec2d* cp = centerlineControlPoints_.data();
    return CubicBezier2d(p[i0], cp[j], cp[j + 1], p[i3]);
}

CubicBezier1d
CatmullRomSplineStroke2d::segmentToNormalReparametrization(Int segmentIndex) const {

    updateCache();

    Int numSegments = this->numSegments();
    detail::checkSegmentIndexIsValid(segmentIndex, numSegments);

    Int i = segmentIndex;

    const Vec2d* cp = normalReparametrizationControlValues_.data();
    return CubicBezier1d(0, cp[i][0], cp[i][1], 1);
}

const StrokeModelInfo& CatmullRomSplineStroke2d::modelInfo_() const {
    static StrokeModelInfo info = StrokeModelInfo(core::StringId("CatmullRom"), 1000);
    return info;
}

std::unique_ptr<AbstractStroke2d> CatmullRomSplineStroke2d::cloneEmpty_() const {
    return std::make_unique<CatmullRomSplineStroke2d>(parameterization_, isClosed());
}

std::unique_ptr<AbstractStroke2d> CatmullRomSplineStroke2d::clone_() const {
    return std::make_unique<CatmullRomSplineStroke2d>(*this);
}

bool CatmullRomSplineStroke2d::copyAssign_(const AbstractStroke2d* other_) {
    auto other = dynamic_cast<const CatmullRomSplineStroke2d*>(other_);
    if (!other) {
        return false;
    }
    *this = *other;
    return true;
}

bool CatmullRomSplineStroke2d::moveAssign_(AbstractStroke2d* other_) {
    auto other = dynamic_cast<CatmullRomSplineStroke2d*>(other_);
    if (!other) {
        return false;
    }
    *this = std::move(*other);
    return true;
}

namespace {

struct PointData {
    Vec2d p = core::noInit;
    Vec2d dp = core::noInit;
    Vec2d ddp = core::noInit;
    double speed;
    double curvature;
};

PointData
computeSegmentEndPointData(const CubicBezier2d& centerlineBezier, Int endIndex) {

    const std::array<Vec2d, 4>& positions = centerlineBezier.controlPoints();

    PointData result;

    Vec2d dp = core::noInit;
    Vec2d ddp = core::noInit;
    if (endIndex) {
        result.p = positions[3];
        dp = 3 * (positions[3] - positions[2]);
        result.dp = dp;
        ddp = 6 * (positions[3] - 2 * positions[2] + positions[1]);
        result.ddp = ddp;
    }
    else {
        result.p = positions[0];
        dp = 3 * (positions[1] - positions[0]);
        result.dp = dp;
        ddp = 6 * (positions[2] - 2 * positions[1] + positions[0]);
        result.ddp = ddp;
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

struct WidthData {
    Vec2d w = core::noInit;
    Vec2d dw = core::noInit;
};

WidthData
computeSegmentEndWidthData(const CubicBezier2d& halfwidthsBezier, Int endIndex) {

    const std::array<Vec2d, 4>& halfwidths = halfwidthsBezier.controlPoints();

    WidthData result;

    if (endIndex) {
        result.w = halfwidths[3];
        result.dw = 3 * (halfwidths[3] - halfwidths[2]);
    }
    else {
        result.w = halfwidths[0];
        result.dw = 3 * (halfwidths[1] - halfwidths[0]);
    }

    return result;
}

// Currently assumes first derivative at endpoint is non null.
// TODO: Support null derivatives (using limit analysis).
// TODO: Support different halfwidth on both sides.
std::array<Vec2d, 2>
computeOffsetLineTangents(const PointData& pData, const WidthData& wData) {
    Vec2d dp = pData.dp;
    double dpl = pData.speed;
    Vec2d n = dp.orthogonalized() / dpl;
    Vec2d dn = dp * pData.curvature;
    Vec2d offset0 = dn * wData.w[0] + n * wData.dw[0];
    Vec2d offset1 = -(dn * wData.w[1] + n * wData.dw[1]);
    Vec2d t0 = (dp + offset0).normalized();
    Vec2d t1 = (dp + offset1).normalized();
    if (!std::isnormal(t0.x()) || !std::isnormal(t0.y())) {
        t0 = Vec2d();
    }
    if (!std::isnormal(t1.x()) || !std::isnormal(t1.y())) {
        t1 = Vec2d();
    }
    return {t0, t1};
}

} // namespace

StrokeBoundaryInfo CatmullRomSplineStroke2d::computeBoundaryInfo_() const {

    StrokeBoundaryInfo result;

    Int n = this->numSegments();
    if (n > 0) {
        CubicBezier2d halfwidthBezier0(core::noInit);
        CubicBezier2d positionsBezier0 = segmentToBezier(0, halfwidthBezier0);
        CubicBezier2d halfwidthBezier1(core::noInit);
        CubicBezier2d positionsBezier1 = segmentToBezier(n - 1, halfwidthBezier1);

        PointData pData0 = computeSegmentEndPointData(positionsBezier0, 0);
        PointData pData1 = computeSegmentEndPointData(positionsBezier1, 1);
        WidthData wData0 = computeSegmentEndWidthData(halfwidthBezier0, 0);
        WidthData wData1 = computeSegmentEndWidthData(halfwidthBezier1, 1);

        result[0] = StrokeEndInfo(
            pData0.p, pData0.speed != 0 ? (pData0.dp / pData0.speed) : Vec2d(0, 1), wData0.w);
        result[0].setOffsetLineTangents(computeOffsetLineTangents(pData0, wData0));
        result[1] = StrokeEndInfo(
            pData1.p, pData1.speed != 0 ? (pData1.dp / pData1.speed) : Vec2d(0, 1), wData1.w);
        result[1].setOffsetLineTangents(computeOffsetLineTangents(pData1, wData1));
    }
    else if (this->numKnots() > 0) {
        Vec2d p = positions().first();
        double hw = 0.5 * (hasConstantWidth() ? constantWidth() : widths().first());
        result[0] = StrokeEndInfo(p, Vec2d(0, 1), Vec2d(hw, hw));
        result[1] = StrokeEndInfo(p, Vec2d(0, 1), Vec2d(hw, hw));
    }
    return result;
}

namespace {

struct CatmullRomSegmentComputeData {
    std::array<double, 3> imaginaryChordLengths;
    CubicBezier2d centerlineBezier;
    std::array<PointData, 2> endPointDataPair;
    std::array<WidthData, 2> endWidthDataPair;
};

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

void forceOffsetLineTangentContinuityUsingHalfwidths(
    Vec2dArray& outHalfwidthsControlPoints,
    const core::Array<CatmullRomSegmentComputeData>& computeDataArray,
    const core::Array<CurveSegmentType>& segmentTypes,
    bool isClosed) {

    // Here we try to make offset line tangent continuous at start knot.
    Int numSegments = segmentTypes.length();
    for (Int i = 0; i < numSegments; ++i) {
        const CatmullRomSegmentComputeData& computeData = computeDataArray[i];

        CurveSegmentType segmentType = segmentTypes[i];

        if ((isClosed || i > 1)
            && (segmentType == CurveSegmentType::Simple
                || segmentType == CurveSegmentType::BeforeCorner)) {
            Int previousSegmentIndex = i - 1;
            if (previousSegmentIndex < 0) {
                previousSegmentIndex = numSegments - 1;
            }
            const CatmullRomSegmentComputeData& previousSegmentComputeData =
                computeDataArray[previousSegmentIndex];

            // basically we want to change a single width control point.

            const PointData& pd0 = previousSegmentComputeData.endPointDataPair[1];
            const PointData& pd1 = computeData.endPointDataPair[0];
            const WidthData& wd0 = previousSegmentComputeData.endWidthDataPair[1];
            const WidthData& wd1 = computeData.endWidthDataPair[0];

            if (std::abs(pd0.curvature) < std::abs(pd1.curvature)) {
                // use previous segment offset line end tangents as
                // current segment offset line start tangents.

                double speed0 = pd0.speed;
                double k0 = pd0.curvature;
                double tc00 = (1 + k0 * wd0.w[0]) * speed0;
                double tc01 = (1 - k0 * wd0.w[1]) * speed0;
                double nc00 = wd0.dw[0];
                double nc01 = -wd0.dw[1];
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

                Vec2d hws1 = wd1.w;
                double speed1 = pd1.speed;
                double k1 = pd1.curvature;
                double hw110 = +do00_du * (1 + k1 * hws1[0]) * speed1 / 3 + hws1[0];
                double hw111 = -do01_du * (1 - k1 * hws1[1]) * speed1 / 3 + hws1[1];

                Int j = i * 2;
                outHalfwidthsControlPoints[j] = Vec2d(hw110, hw111);
            }
            else {
                // use current segment offset line start tangents as
                // previous segment offset line end tangents.

                double speed1 = pd1.speed;
                double k1 = pd1.curvature;
                double tc10 = (1 + k1 * wd1.w[0]) * speed1;
                double tc11 = (1 - k1 * wd1.w[1]) * speed1;
                double nc10 = wd1.dw[0];
                double nc11 = -wd1.dw[1];
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

                Vec2d hws0 = wd0.w;
                double speed0 = pd0.speed;
                double k0 = pd0.curvature;
                double hw020 = -do10_du * (1 + k0 * hws0[0]) * speed0 / 3 + hws0[0];
                double hw021 = +do11_du * (1 - k0 * hws0[1]) * speed0 / 3 + hws0[1];

                Int j = previousSegmentIndex * 2;
                outHalfwidthsControlPoints[j + 1] = Vec2d(hw020, hw021);
            }
        }
    }
}

} // namespace

void CatmullRomSplineStroke2d::updateCache_(
    const core::Array<SegmentComputeData>& baseComputeDataArray) const {

    const auto& positions = this->positions();
    const auto& widths = this->widths();
    Int numSegments = this->numSegments();
    bool isClosed = this->isClosed();

    core::Array<CatmullRomSegmentComputeData> computeDataArray(numSegments, core::noInit);

    centerlineControlPoints_.resizeNoInit(numSegments * 2);
    halfwidthsControlPoints_.resizeNoInit(numSegments * 2);
    normalReparametrizationControlValues_.resizeNoInit(numSegments);

    // todo: Indices are special only for a few cases, we could make a special loop
    //       instead of doing special case logic on indices at each iteration.
    //       At least, computeData allows for not doing it twice.

    for (Int i = 0; i < numSegments; ++i) {
        const SegmentComputeData& baseComputeData = baseComputeDataArray[i];
        CatmullRomSegmentComputeData& computeData = computeDataArray[i];

        std::array<Vec2d, 4> posKnots =
            detail::getElementsUnchecked(positions, baseComputeData.knotIndices);

        CurveSegmentType segmentType = segmentTypes()[i];

        CubicBezier2d centerlineBezier = computeSegmentCenterlineCubicBezier(
            parameterization_,
            posKnots,
            baseComputeData.chords,
            baseComputeData.chordLengths,
            segmentType,
            computeData.imaginaryChordLengths);
        computeData.centerlineBezier = centerlineBezier;

        computeData.endPointDataPair[0] = computeSegmentEndPointData(centerlineBezier, 0);
        computeData.endPointDataPair[1] = computeSegmentEndPointData(centerlineBezier, 1);

        Int j = i * 2;
        centerlineControlPoints_.getUnchecked(j) = centerlineBezier.controlPoint1();
        centerlineControlPoints_.getUnchecked(j + 1) = centerlineBezier.controlPoint2();
    }

    CubicBezier2d halfwidthsBezier(core::noInit);

    for (Int i = 0; i < numSegments; ++i) {
        const SegmentComputeData& baseComputeData = baseComputeDataArray[i];
        CatmullRomSegmentComputeData& computeData = computeDataArray[i];

        std::array<double, 4> widthKnots =
            detail::getElementsUnchecked(widths, baseComputeData.knotIndices);

        CurveSegmentType segmentType = segmentTypes()[i];

        const CubicBezier2d& centerlineBezier = computeData.centerlineBezier;
        halfwidthsBezier = computeSegmentHalfwidthsCubicBezier(
            widthKnots,
            centerlineBezier.controlPoints(),
            computeData.imaginaryChordLengths,
            segmentType);

        computeData.endWidthDataPair[0] = computeSegmentEndWidthData(halfwidthsBezier, 0);
        computeData.endWidthDataPair[1] = computeSegmentEndWidthData(halfwidthsBezier, 1);

        Int j = i * 2;
        halfwidthsControlPoints_.getUnchecked(j) = halfwidthsBezier.controlPoint1();
        halfwidthsControlPoints_.getUnchecked(j + 1) = halfwidthsBezier.controlPoint2();
    }

    // Here we relax normals to make their derivative continuous.
    constexpr bool enableRelaxedNormals = true;
    for (Int i = 0; i < numSegments; ++i) {
        CatmullRomSegmentComputeData& computeData = computeDataArray[i];

        CurveSegmentType segmentType = segmentTypes()[i];

        normalReparametrizationControlValues_[i] = Vec2d(1. / 3, 2. / 3);

        if (enableRelaxedNormals && (isClosed || i > 1)
            && (segmentType == CurveSegmentType::Simple
                || segmentType == CurveSegmentType::BeforeCorner)) {

            Int previousSegmentIndex = i - 1;
            if (previousSegmentIndex < 0) {
                previousSegmentIndex = numSegments - 1;
            }
            CatmullRomSegmentComputeData& previousSegmentComputeData =
                computeDataArray[previousSegmentIndex];

            PointData& ed0 = previousSegmentComputeData.endPointDataPair[1];
            PointData& ed1 = computeData.endPointDataPair[0];

            double np0 = std::abs(ed0.curvature);
            double np1 = std::abs(ed1.curvature);

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
        forceOffsetLineTangentContinuityUsingHalfwidths(
            halfwidthsControlPoints_, computeDataArray, segmentTypes(), isClosed);
    }
}

} // namespace vgc::geometry
