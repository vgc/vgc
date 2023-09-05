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

#include <vgc/geometry/yuksel.h>

#include <vgc/geometry/bezier.h>
#include <vgc/geometry/stroke.h>

namespace vgc::geometry {

Vec2d YukselSplineStroke2d::evalNonZeroCenterline(Int segmentIndex, double u) const {
    YukselBezierSegment2d centerlineSegment = segmentEvaluator(segmentIndex);
    return centerlineSegment.eval(u);
}

Vec2d YukselSplineStroke2d::evalNonZeroCenterline(Int segmentIndex, double u, Vec2d& dp)
    const {

    YukselBezierSegment2d centerlineSegment = segmentEvaluator(segmentIndex);
    return centerlineSegment.eval(u, dp);
}

StrokeSampleEx2d YukselSplineStroke2d::evalNonZero(Int segmentIndex, double u) const {
    if (hasConstantWidth()) {
        YukselBezierSegment2d centerlineSegment = segmentEvaluator(segmentIndex);
        double hw = 0.5 * constantWidth();
        Vec2d tangent = core::noInit;
        double speed;
        Vec2d p = centerlineSegment.eval(u, tangent, speed);
        Vec2d normal = tangent.orthogonalized() / speed;
        return StrokeSampleEx2d(p, tangent, normal, hw, speed, segmentIndex, u);
    }
    else {
        CubicBezier2d halfwidthsSegment(core::noInit);
        YukselBezierSegment2d centerlineSegment =
            segmentEvaluator(segmentIndex, halfwidthsSegment);
        Vec2d tangent = core::noInit;
        double speed;
        Vec2d p = centerlineSegment.eval(u, tangent, speed);
        Vec2d hw = halfwidthsSegment.eval(u);
        Vec2d normal = tangent.orthogonalized() / speed;
        return StrokeSampleEx2d(p, tangent, normal, hw, speed, segmentIndex, u);
    }
}

void YukselSplineStroke2d::sampleNonZeroSegment(
    StrokeSampleEx2dArray& out,
    Int segmentIndex,
    const CurveSamplingParameters& params,
    detail::AdaptiveStrokeSampler& sampler) const {

    if (hasConstantWidth()) {
        YukselBezierSegment2d centerlineSegment = segmentEvaluator(segmentIndex);
        double hw = 0.5 * constantWidth();
        sampler.sample(
            [&, hw](double u) -> StrokeSampleEx2d {
                Vec2d tangent = core::noInit;
                double speed;
                Vec2d p = centerlineSegment.eval(u, tangent, speed);
                Vec2d normal = tangent.orthogonalized() / speed;
                return StrokeSampleEx2d(p, tangent, normal, hw, speed, segmentIndex, u);
            },
            params,
            out);
    }
    else {
        CubicBezier2d halfwidthsSegment(core::noInit);
        YukselBezierSegment2d centerlineSegment =
            segmentEvaluator(segmentIndex, halfwidthsSegment);
        sampler.sample(
            [&](double u) -> StrokeSampleEx2d {
                Vec2d tangent = core::noInit;
                double speed;
                Vec2d p = centerlineSegment.eval(u, tangent, speed);
                Vec2d hw = halfwidthsSegment.eval(u);
                Vec2d normal = tangent.orthogonalized() / speed;
                return StrokeSampleEx2d(p, tangent, normal, hw, speed, segmentIndex, u);
            },
            params,
            out);
    }
}

StrokeSampleEx2d YukselSplineStroke2d::zeroLengthStrokeSample() const {
    return StrokeSampleEx2d(
        positions().first(),
        Vec2d(0, 1),
        Vec2d(-1, 0),
        0.5 /*constantHalfwidth_*/,
        0 /*speed*/,
        0,
        0);
}

namespace {

struct QuadraticParameters {
    Vec2d bi;
    double ti;
};

double computeTi_(const Vec2d& knot0, const Vec2d& knot1, const Vec2d& knot2) {
    // With the corner mechanism we can assume either [knot0, knot1] or [knot1, knot2]
    // is non zero-lengh.

    //--------------------------------
    // chord-length ratio (not great)
    //--------------------------------
    //double l01 = (knot1 - knot0).length();
    //double l02 = l01 + (knot2 - knot1).length();
    //if (l02 == 0) {
    //    return 0.5;
    //}
    //return l01 / l02;

    //----------------------------
    // max curvature on endpoints
    //----------------------------
    // For now we use the exact formula but a numeric method may be as precise and faster.
    // See "High-Performance Polynomial Root Finding for Graphics" by Cem Yuksel.
    Vec2d v02 = knot2 - knot0;
    Vec2d v10 = knot0 - knot1;
    double a = v02.dot(v02);
    double b = 3 * v02.dot(v10);
    double c = (3 * knot0 - 2 * knot1 - knot2).dot(v10);
    double d = -v10.dot(v10);
    // Solving `ax³ + bx² + cx + d = 0` in [0, 1]
    // https://en.wikipedia.org/wiki/Cubic_equation
    if (a == 0) {
        // knot0 == knot2
        return 0.5;
    }
    double p = (3 * a * c - b * b) / (3 * a * a);
    double q = (2 * b * b * b - 9 * a * b * c + 27 * a * a * d) / (27 * a * a * a);
    double discriminant = q * q / 4 + p * p * p / 27;
    double t = 0;
    if (discriminant >= 0) {
        // 1 real root
        double r = std::sqrt(discriminant);
        return std::cbrt(-q / 2 + r) + std::cbrt(-q / 2 - r) - b / (3 * a);
    }
    else {
        // 3 real roots
        for (int i = 0; i < 3; ++i) {
            double cc = std::cos(
                1.0 / 3 * std::acos(3.0 * q / (2 * p) * std::sqrt(-3.0 / p))
                - 2.0 * core::pi * i / 3);
            t = 2 * std::sqrt(-p / 3) * cc - b / (3 * a);
            if (t >= 0 && t <= 1) {
                return t;
            }
        }
    }
    // fallback
    return 0.5;
}

Vec2d computeBi_(const Vec2d& knot0, const Vec2d& knot1, const Vec2d& knot2, double ti) {
    if (ti <= 0) {
        return knot0;
    }
    if (ti >= 1) {
        return knot1;
    }
    double qi = 1 - ti;
    double c = 1 / (2.0 * qi * ti);
    return c * (knot1 - qi * qi * knot0 - ti * ti * knot2);
}

std::array<Int, 4> computeKnotIndices_(bool isClosed, Int numKnots, Int segmentIndex) {
    // Ensure we have a valid segment between two control points
    const Int numSegments = isClosed ? numKnots : (numKnots ? numKnots - 1 : 0);
    VGC_ASSERT(segmentIndex >= 0);
    VGC_ASSERT(segmentIndex < numSegments);

    // Get indices of points used by the interpolation, handle
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

SegmentType computeSegmentCenterlineYukselSegment_(
    YukselBezierSegment2d& segment,
    core::ConstSpan<Vec2d> knotPositions,
    core::ConstSpan<double> chordLengths,
    const std::array<Int, 4>& knotIndices,
    std::array<double, 3>& fixedChordLengths) {

    std::array<Vec2d, 4> knots = {
        knotPositions.getUnchecked(knotIndices[0]),
        knotPositions.getUnchecked(knotIndices[1]),
        knotPositions.getUnchecked(knotIndices[2]),
        knotPositions.getUnchecked(knotIndices[3])};

    double cl0 = chordLengths.getUnchecked(knotIndices[0]);
    double cl1 = chordLengths.getUnchecked(knotIndices[1]);
    double cl2 = chordLengths.getUnchecked(knotIndices[2]);

    fixedChordLengths = {
        knotIndices[0] != knotIndices[1] ? cl0 : 0,
        cl1,
        knotIndices[2] != knotIndices[3] ? cl2 : 0};

    // Aliases
    const Vec2d p1p2 = knots[2] - knots[1];
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
        segment = YukselBezierSegment2d(knots, knots[1], 1, knots[2], 0);
        return SegmentType::Corner;
    }
    else if (isAfterCorner) {
        if (isBeforeCorner) {
            // (d01 == 0) && (d12 > 0) && (d23 == 0)
            //
            // Linear parameterization
            Vec2d mid = 0.5 * (knots[1] + knots[2]);
            segment = YukselBezierSegment2d(knots, mid, 0.5, mid, 0.5);
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
            Vec2d p2p3 = knots[3] - knots[2];
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
        Vec2d p0p1 = knots[1] - knots[0];
        Vec2d d = -p0p1 / d01;
        Vec2d n = (p1p2 / d12).orthogonalized();
        Vec2d q = 2 * d.dot(n) * n - d;
        knots[3] = knots[2] + d12 * q;
        fixedChordLengths[2] = d12;
        result = SegmentType::BeforeCorner;
    }

    double ti0 = computeTi_(knots[0], knots[1], knots[2]);
    Vec2d bi0 = computeBi_(knots[0], knots[1], knots[2], ti0);
    double ti1 = computeTi_(knots[1], knots[2], knots[3]);
    Vec2d bi1 = computeBi_(knots[1], knots[2], knots[3], ti1);

    segment = YukselBezierSegment2d(knots, bi0, ti0, bi1, ti1);
    return result;
}

void computeSegmentHalfwidthsCubicBezier_(
    CubicBezier2d& bezier,
    core::ConstSpan<double> knotWidths,
    const std::array<Int, 4>& knotIndices,
    const YukselBezierSegment2d& centerlineSegment,
    const std::array<double, 3>& chordLengths,
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

    // Handle "corner knots", defined as:
    // 1. Two consecutive equal points, or
    // 2. The first/last knot of an open curve
    //
    switch (segmentType) {
    case SegmentType::None: {
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
        knots[0] = 2 * knots[1] - knots[2];
        break;
    }
    case SegmentType::BeforeCorner: {
        // Imaginary control point, see `initPositions_()`.
        knots[3] = 2 * knots[2] - knots[1];
        break;
    }
    }

    // Compute Bézier control points for halfwidths such that on both sides of
    // each knot we have the same desired dw/ds.
    //
    // desired dw/ds at start/end
    Vec2d dhw_ds_1 = (knots[2] - knots[0]) / (chordLengths[0] + chordLengths[1]);
    Vec2d dhw_ds_2 = (knots[3] - knots[1]) / (chordLengths[1] + chordLengths[2]);
    // 1/3 of ds/du at start/end
    double ds_du_1 = (1.0 / 3) * (centerlineSegment.startDerivative()).length();
    double ds_du_2 = (1.0 / 3) * (centerlineSegment.endDerivative()).length();
    // w1 - w0 = 1/3 of dw/du at start; w3 - w2 = 1/3 of dw/du at end
    Vec2d hw1 = knots[1] + dhw_ds_1 * ds_du_1;
    Vec2d hw2 = knots[2] - dhw_ds_2 * ds_du_2;

    bezier = CubicBezier2d(knots[1], hw1, hw2, knots[2]);
}

} // namespace

YukselBezierSegment2d YukselSplineStroke2d::segmentEvaluator(Int segmentIndex) const {
    YukselBezierSegment2d centerlineSegment(core::noInit);

    std::array<Int, 4> knotIndices =
        computeKnotIndices_(isClosed(), positions().length(), segmentIndex);

    std::array<double, 3> segChordLengths;
    computeSegmentCenterlineYukselSegment_(
        centerlineSegment, positions(), chordLengths(), knotIndices, segChordLengths);

    return centerlineSegment;
}

YukselBezierSegment2d YukselSplineStroke2d::segmentEvaluator(
    Int segmentIndex,
    CubicBezier2d& halfwidths) const {

    YukselBezierSegment2d centerlineSegment(core::noInit);

    std::array<Int, 4> knotIndices =
        computeKnotIndices_(isClosed(), positions().length(), segmentIndex);

    std::array<double, 3> segChordLengths;
    SegmentType segmentType = computeSegmentCenterlineYukselSegment_(
        centerlineSegment, positions(), chordLengths(), knotIndices, segChordLengths);

    if (hasConstantWidth()) {
        double chw = 0.5 * constantWidth();
        Vec2d cp(chw, chw);
        halfwidths = CubicBezier2d(cp, cp, cp, cp);
    }
    else {
        computeSegmentHalfwidthsCubicBezier_(
            halfwidths,
            widths(),
            knotIndices,
            centerlineSegment,
            segChordLengths,
            segmentType);
    }

    return centerlineSegment;
}

const StrokeModelInfo& YukselSplineStroke2d::modelInfo_() const {
    static StrokeModelInfo info = StrokeModelInfo(core::StringId("YukselSpline"), 500);
    return info;
}

std::unique_ptr<AbstractStroke2d> YukselSplineStroke2d::cloneEmpty_() const {
    return std::make_unique<YukselSplineStroke2d>(isClosed());
}

std::unique_ptr<AbstractStroke2d> YukselSplineStroke2d::clone_() const {
    return std::make_unique<YukselSplineStroke2d>(*this);
}

bool YukselSplineStroke2d::copyAssign_(const AbstractStroke2d* other_) {
    auto other = dynamic_cast<const YukselSplineStroke2d*>(other_);
    if (!other) {
        return false;
    }
    *this = *other;
    return true;
}

bool YukselSplineStroke2d::moveAssign_(AbstractStroke2d* other_) {
    auto other = dynamic_cast<YukselSplineStroke2d*>(other_);
    if (!other) {
        return false;
    }
    *this = std::move(*other);
    return true;
}

namespace {

StrokeEndInfo computeSegmentEndInfo(
    const YukselBezierSegment2d& centerlineSegment,
    const CubicBezier2d& halfwidthsSegment,
    Int endIndex) {

    Vec2d tangent = core::noInit;
    double speed;
    Vec2d ddp = core::noInit;
    Vec2d p = centerlineSegment.eval(endIndex ? 1. : 0., tangent, speed, ddp);
    Vec2d dp = tangent * speed;
    Vec2d dw = core::noInit;
    // TODO: can be optimized if necessary.
    Vec2d w = halfwidthsSegment.eval(endIndex == 0 ? 0 : 1, dw);

    StrokeEndInfo result(p, tangent, w);

    if (speed == 0) {
        if (dw == Vec2d()) {
            result.setOffsetLineTangents({tangent, tangent});
        }
        else {
            Vec2d normal = tangent.orthogonalized();
            result.setOffsetLineTangents({normal, -normal});
        }
    }
    else {
        Vec2d n = tangent.orthogonalized();
        Vec2d dn = tangent * (ddp.det(dp)) / (speed * speed);

        Vec2d offset0 = dn * w[0] + n * dw[0];
        Vec2d offset1 = -(dn * w[1] + n * dw[1]);

        result.setOffsetLineTangents(
            {(dp + offset0).normalized(), (dp + offset1).normalized()});
    }

    return result;
}

} // namespace

StrokeBoundaryInfo YukselSplineStroke2d::computeBoundaryInfo_() const {

    Int n = this->numSegments();

    CubicBezier2d halfwidthsSegment0(core::noInit);
    YukselBezierSegment2d centerlineSegment0 = segmentEvaluator(0, halfwidthsSegment0);
    CubicBezier2d halfwidthsSegment1(core::noInit);
    YukselBezierSegment2d centerlineSegment1 =
        segmentEvaluator(n - 1, halfwidthsSegment1);

    StrokeBoundaryInfo result;
    result[0] = computeSegmentEndInfo(centerlineSegment0, halfwidthsSegment0, 0);
    result[1] = computeSegmentEndInfo(centerlineSegment1, halfwidthsSegment1, 1);
    return result;
}

void YukselSplineStroke2d::updateCache_(
    const core::Array<SegmentComputeData>& /*baseComputeDataArray*/) const {
    // no custom cache data
}

} // namespace vgc::geometry
