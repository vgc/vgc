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

#include <vgc/geometry/curve.h>

#include <array>
#include <optional>

#include <vgc/core/algorithm.h>
#include <vgc/core/array.h>
#include <vgc/core/colors.h>
#include <vgc/geometry/bezier.h>
#include <vgc/geometry/catmullrom.h>

namespace vgc::geometry {

VGC_DEFINE_ENUM(
    CurveSamplingQuality,
    (Disabled, "Disabled"),
    (UniformVeryLow, "Uniform Very Low"),
    (AdaptiveLow, "Adaptive Low"),
    (UniformHigh, "Uniform High"),
    (AdaptiveHigh, "Adaptive High"),
    (UniformVeryHigh, "Uniform Very High"))

namespace {

using core::DoubleArray;

template<typename ContainerType>
void removeAllExceptLastElement(ContainerType& v) {
    v.first() = v.last();
    v.resize(1);
}

template<typename ContainerType>
void appendUninitializedElement(ContainerType& v) {
    v.append(typename ContainerType::value_type());
}

void computeSample(
    const Vec2d& q0,
    const Vec2d& q1,
    const Vec2d& q2,
    const Vec2d& q3,
    double w0,
    double w1,
    double w2,
    double w3,
    double u,
    Vec2d& leftPosition,
    Vec2d& rightPosition,
    Vec2d& normal) {

    // Compute position and normal
    Vec2d position = cubicBezier(q0, q1, q2, q3, u);
    Vec2d tangent = cubicBezierDer(q0, q1, q2, q3, u);
    normal = tangent.normalized().orthogonalized();

    // Compute half-width
    double halfwidth = 0.5 * cubicBezier(w0, w1, w2, w3, u);

    // Compute left and right positions
    leftPosition = position + halfwidth * normal;
    rightPosition = position - halfwidth * normal;
}

} // namespace

namespace detail {

CubicBezierStroke CubicBezierStroke::fromCurve(const Curve* curve, Int i) {

    core::ConstSpan<Vec2d> positions = curve->positions();
    core::ConstSpan<double> widths = curve->widths();

    CatmullRomSplineParameterization parameterization = {};
    switch (curve->type()) {
    case Curve::Type::OpenUniformCatmullRom:
    case Curve::Type::ClosedUniformCatmullRom: {
        parameterization = CatmullRomSplineParameterization::Uniform;
        break;
    }
    case Curve::Type::OpenCentripetalCatmullRom:
    case Curve::Type::ClosedCentripetalCatmullRom: {
        parameterization = CatmullRomSplineParameterization::Centripetal;
        break;
    }
    }

    bool isWidthUniform =
        (curve->widthVariability() == Curve::AttributeVariability::Constant
         || widths.length() != positions.length());

    if (isWidthUniform) {
        double width = curve->width();
        if (!widths.isEmpty()) {
            // curve->width() is broken at the moment.
            width = widths[0];
        }
        return fromCatmullRomSpline(
            parameterization, positions, width, curve->isClosed(), i);
    }
    else {
        return fromCatmullRomSpline(
            parameterization, positions, widths, curve->isClosed(), i);
    }
}

CubicBezierStroke CubicBezierStroke::fromCatmullRomSpline(
    CatmullRomSplineParameterization parameterization,
    core::ConstSpan<Vec2d> knotPositions,
    core::ConstSpan<double> knotWidths,
    bool isClosed,
    Int i) {

    CubicBezierStroke result;
    result.isWidthUniform_ = false;

    std::array<Int, 4> knotIndices =
        computeKnotIndices_(isClosed, knotPositions.length(), i);
    std::array<Vec2d, 3> knotSegments;
    std::array<double, 3> knotSegmentLengths;

    CornerCase cornerCase = result.initPositions_(
        parameterization, knotPositions, knotIndices, knotSegments, knotSegmentLengths);
    result.initHalfwidths_(knotWidths, knotIndices, knotSegmentLengths, cornerCase);

    return result;
}

CubicBezierStroke CubicBezierStroke::fromCatmullRomSpline(
    CatmullRomSplineParameterization parameterization,
    core::ConstSpan<Vec2d> knotPositions,
    double width,
    bool isClosed,
    Int i) {

    CubicBezierStroke result;
    result.isWidthUniform_ = true;

    std::array<Int, 4> knotIndices =
        computeKnotIndices_(isClosed, knotPositions.length(), i);
    std::array<Vec2d, 3> knotSegments;
    std::array<double, 3> knotSegmentLengths;

    result.initPositions_(
        parameterization, knotPositions, knotIndices, knotSegments, knotSegmentLengths);
    result.halfwidths_[0] = width;
    result.halfwidths_[1] = width;
    result.halfwidths_[2] = width;
    result.halfwidths_[3] = width;

    return result;
}

Vec2d CubicBezierStroke::evalPosition(double u) const {
    return cubicBezierPosCasteljau<Vec2d>(positions_, u);
}

void CubicBezierStroke::evalPositionAndDerivative(
    double u,
    Vec2d& position,
    Vec2d& derivative) const {

    cubicBezierPosAndDerCasteljau<Vec2d>(positions_, u, position, derivative);
}

double CubicBezierStroke::evalHalfwidths(double u) const {
    return cubicBezierPosCasteljau<double>(halfwidths_, u);
}

void CubicBezierStroke::evalHalfwidthsAndDerivative(
    double u,
    double& halfwidth,
    double& derivative) const {

    cubicBezierPosAndDerCasteljau<double>(halfwidths_, u, halfwidth, derivative);
}

CubicBezierStroke::CornerCase CubicBezierStroke::initPositions_(
    CatmullRomSplineParameterization parameterization,
    core::ConstSpan<Vec2d> splineKnots,
    const std::array<Int, 4>& knotIndices,
    std::array<Vec2d, 3>& knotSegments,
    std::array<double, 3>& knotSegmentLengths) {

    std::array<Vec2d, 4> knots = {
        splineKnots[knotIndices[0]],
        splineKnots[knotIndices[1]],
        splineKnots[knotIndices[2]],
        splineKnots[knotIndices[3]]};

    knotSegments = {knots[1] - knots[0], knots[2] - knots[1], knots[3] - knots[2]};
    knotSegmentLengths = {
        knotSegments[0].length(), knotSegments[1].length(), knotSegments[2].length()};

    // Aliases
    const Vec2d p0p1 = knotSegments[0];
    const Vec2d p1p2 = knotSegments[1];
    const Vec2d p2p3 = knotSegments[2];
    const double d01 = knotSegmentLengths[0];
    const double d12 = knotSegmentLengths[1];
    const double d23 = knotSegmentLengths[2];

    // Handle "corner knots", defined as:
    // 1. Two consecutive equal points, or
    // 2. The first/last knot of an open curve
    //
    CornerCase result = CornerCase::None;
    bool isAfterCorner = (d01 == 0);
    bool isCorner = (d12 == 0);
    bool isBeforeCorner = (d23 == 0);
    if (isCorner) {
        positions_[0] = knots[1];
        positions_[1] = knots[1];
        positions_[2] = knots[2];
        positions_[3] = knots[2];
        return CornerCase::Corner;
    }
    else if (isAfterCorner) {
        if (isBeforeCorner) { // (d01 == 0) && (d12 > 0) && (d23 == 0)
            // Linear parametrization
            double u = 1.0 / 3;
            double v = (1 - u);
            positions_[0] = knots[1];
            positions_[1] = v * knots[1] + u * knots[2];
            positions_[2] = u * knots[1] + v * knots[2];
            positions_[3] = knots[2];
            return CornerCase::BetweenCorners;
        }
        else { // (d01 == 0) && (d12 > 0) && (d23 > 0)
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
            positions_[0] = knots[1] + d12 * q;
            positions_[1] = knots[1];
            positions_[2] = knots[2];
            positions_[3] = knots[3];
            knotSegmentLengths[0] = d12;
            result = CornerCase::AfterCorner;
        }
    }
    else if (isBeforeCorner) { // (d01 > 0) && (d12 > 0) && (d23 == 0)
        // Similar as AfterCorner case above.
        Vec2d d = -p0p1 / d01;
        Vec2d n = (p1p2 / d12).orthogonalized();
        Vec2d q = 2 * d.dot(n) * n - d;
        positions_[0] = knots[0];
        positions_[1] = knots[1];
        positions_[2] = knots[2];
        positions_[3] = knots[2] + d12 * q;
        knotSegmentLengths[2] = d12;
        result = CornerCase::BeforeCorner;
    }
    else {
        positions_ = knots;
    }

    switch (parameterization) {
    case CatmullRomSplineParameterization::Uniform: {
        positions_ = uniformCatmullRomToBezier<Vec2d>(positions_);
        break;
    }
    case CatmullRomSplineParameterization::Centripetal: {
        positions_ = centripetalCatmullRomToBezier<Vec2d>(positions_, knotSegmentLengths);
        break;
    }
    }

    return result;
}

void CubicBezierStroke::initHalfwidths_(
    core::ConstSpan<double> splineKnots,
    const std::array<Int, 4>& knotIndices,
    const std::array<double, 3>& knotSegmentLengths,
    CubicBezierStroke::CornerCase cornerCase) {

    std::array<double, 4> knots = {
        splineKnots[knotIndices[0]] * 0.5,
        splineKnots[knotIndices[1]] * 0.5,
        splineKnots[knotIndices[2]] * 0.5,
        splineKnots[knotIndices[3]] * 0.5};

    // Aliases
    const double d01 = knotSegmentLengths[0];
    const double d12 = knotSegmentLengths[1];
    const double d23 = knotSegmentLengths[2];

    // Handle "corner knots", defined as:
    // 1. Two consecutive equal points, or
    // 2. The first/last knot of an open curve
    //
    switch (cornerCase) {
    case CornerCase::None: {
        halfwidths_ = knots;
        break;
    }
    case CornerCase::BetweenCorners:
    case CornerCase::Corner: {
        double u = 1.0 / 3;
        double v = (1 - u);
        halfwidths_[0] = knots[1];
        halfwidths_[1] = v * knots[1] + u * knots[2];
        halfwidths_[2] = u * knots[1] + v * knots[2];
        halfwidths_[3] = knots[2];
        // Fast return.
        return;
    }
    case CornerCase::AfterCorner: {
        // Imaginary control point, see `initPositions_()`.
        halfwidths_[0] = 2 * knots[1] - knots[2];
        halfwidths_[1] = knots[1];
        halfwidths_[2] = knots[2];
        halfwidths_[3] = knots[3];
        break;
    }
    case CornerCase::BeforeCorner: {
        // Imaginary control point, see `initPositions_()`.
        halfwidths_[0] = knots[0];
        halfwidths_[1] = knots[1];
        halfwidths_[2] = knots[2];
        halfwidths_[3] = 2 * knots[2] - knots[1];
        break;
    }
    }

    // Compute Bézier control points for widths such that on both sides of
    // each knot we have the same desired dw/ds.
    //
    double d02 = d01 + d12;
    double d13 = d12 + d23;
    // desired dw/ds at start/end
    double dw_ds_1 = (halfwidths_[2] - halfwidths_[0]) / d02;
    double dw_ds_2 = (halfwidths_[3] - halfwidths_[1]) / d13;
    // 1/3 of ds/du at start/end
    double ds_du_1 = (positions_[1] - positions_[0]).length();
    double ds_du_2 = (positions_[3] - positions_[2]).length();
    // w1 - w0 = 1/3 of dw/du at start; w3 - w2 = 1/3 of dw/du at end
    double w1 = halfwidths_[1] + dw_ds_1 * ds_du_1;
    double w2 = halfwidths_[2] - dw_ds_2 * ds_du_2;
    halfwidths_[0] = halfwidths_[1];
    halfwidths_[3] = halfwidths_[2];
    halfwidths_[1] = w1;
    halfwidths_[2] = w2;
}

std::array<Int, 4>
CubicBezierStroke::computeKnotIndices_(bool isClosed, Int numKnots, Int i) {
    // Ensure we have a valid segment between two control points
    const Int numSegments = isClosed ? numKnots : (numKnots ? numKnots - 1 : 0);
    VGC_ASSERT(i >= 0);
    VGC_ASSERT(i < numSegments);

    // Get indices of points used by the Catmull-Rom interpolation, handle
    // wrapping for closed curves and boundary for open curves.
    std::array<Int, 4> indices = {i - 1, i, i + 1, i + 2};
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

} // namespace detail

DistanceToCurve distanceToCurve(const CurveSampleArray& samples, const Vec2d& position) {

    DistanceToCurve result(core::DoubleInfinity, 0, 0, 0);
    constexpr double hpi = core::pi / 2;

    if (samples.isEmpty()) {
        return result;
    }

    auto it2 = samples.begin();
    Int i = 0;
    for (auto it1 = it2++; it2 != samples.end(); it1 = it2++, ++i) {
        const Vec2d p1 = it1->position();
        const Vec2d p2 = it2->position();
        const Vec2d p1p = position - p1;
        double d = p1p.length();
        if (d > 0) {
            Vec2d p1p2 = p2 - p1;
            double l = p1p2.length();
            if (l > 0) {
                Vec2d p1p2Dir = p1p2 / l;
                double tx = p1p2Dir.dot(p1p);
                if (tx >= 0 && tx <= l) { // does p project in segment?
                    double ty = p1p2Dir.det(p1p);
                    d = std::abs(ty);
                    if (d < result.distance()) {
                        if (d > 0) {
                            double angleFromTangent = (ty < 0) ? -hpi : hpi;
                            result = DistanceToCurve(d, angleFromTangent, i, tx / l);
                        }
                        else {
                            // (p on segment) => no better result can be found.
                            // The angle is ambiguous, we arbitrarily set to hpi.
                            return DistanceToCurve(0, hpi, i, tx / l);
                        }
                    }
                }
                else if (d < result.distance() && tx < 0) {
                    double angleFromTangent = 0;
                    if (i != 0) {
                        angleFromTangent = (it1->normal().dot(p1p) < 0) ? -hpi : hpi;
                    }
                    else {
                        angleFromTangent = it1->tangent().angle(p1p);
                    }
                    result = DistanceToCurve(d, angleFromTangent, i, 0);
                }
            }
        }
        else {
            // (p == sample) => no better result can be found.
            // The angle is ambiguous, we arbitrarily set to hpi.
            return DistanceToCurve(0, hpi, i, 0);
        }
    }

    // test last sample as point
    const CurveSample& sample = samples.last();
    Vec2d q = sample.position();
    Vec2d qp = position - q;
    double d = qp.length();
    if (d < result.distance()) {
        if (d > 0) {
            double angleFromTangent = sample.tangent().angle(qp);
            result = DistanceToCurve(d, angleFromTangent, i, 0);
        }
        else {
            // (p == sample) => no better result can be found.
            // The angle is ambiguous, we arbitrarily set to hpi.
            return DistanceToCurve(0, hpi, i, 0);
        }
    }

    return result;
}

CurveSamplingParameters::CurveSamplingParameters(CurveSamplingQuality quality)
    : maxAngle_(1)
    , minIntraSegmentSamples_(1)
    , maxIntraSegmentSamples_(1) {

    switch (quality) {
    case CurveSamplingQuality::Disabled:
        maxAngle_ = 100;
        minIntraSegmentSamples_ = 0;
        maxIntraSegmentSamples_ = 0;
        break;
    case CurveSamplingQuality::UniformVeryLow:
        maxAngle_ = 100;
        minIntraSegmentSamples_ = 3;
        maxIntraSegmentSamples_ = 3;
        break;
    case CurveSamplingQuality::AdaptiveLow:
        maxAngle_ = 0.075;
        minIntraSegmentSamples_ = 0;
        maxIntraSegmentSamples_ = 511;
        break;
    case CurveSamplingQuality::UniformHigh:
        maxAngle_ = 100;
        minIntraSegmentSamples_ = 31;
        maxIntraSegmentSamples_ = 31;
        break;
    case CurveSamplingQuality::AdaptiveHigh:
        maxAngle_ = 0.025;
        minIntraSegmentSamples_ = 0;
        maxIntraSegmentSamples_ = 1023;
        break;
    case CurveSamplingQuality::UniformVeryHigh:
        maxAngle_ = 100;
        minIntraSegmentSamples_ = 63;
        maxIntraSegmentSamples_ = 63;
        break;
    }
}

namespace {

constexpr bool isClosedType(Curve::Type type) {
    using Type = Curve::Type;
    switch (type) {
    case Type::OpenUniformCatmullRom:
    case Type::OpenCentripetalCatmullRom:
        return false;
    case Type::ClosedUniformCatmullRom:
    case Type::ClosedCentripetalCatmullRom:
        return true;
    }
    return false;
}

} // namespace

Curve::Curve(Type type)
    : type_(type)
    , isClosed_(isClosedType(type))
    , widthVariability_(AttributeVariability::PerControlPoint)
    , color_(core::colors::black) {
}

Curve::Curve(double constantWidth, Type type)
    : type_(type)
    , isClosed_(isClosedType(type))
    , widthVariability_(AttributeVariability::Constant)
    , widthConstant_(constantWidth)
    , color_(core::colors::black) {
}

Int Curve::numSegments() const {
    Int numPositions = positions_.length();
    switch (type_) {
    case Type::OpenUniformCatmullRom:
    case Type::OpenCentripetalCatmullRom:
        return std::max<Int>(0, numPositions - 1);
    case Type::ClosedUniformCatmullRom:
    case Type::ClosedCentripetalCatmullRom:
        return numPositions;
    }
    return 0;
}

double Curve::width() const {
    return averageWidth_;
}

Vec2dArray Curve::triangulate(double maxAngle, Int minQuads, Int maxQuads) const {

    if (positions_.isEmpty()) {
        return {};
    }

    // Result of this computation.
    // Final size = 2 * nSamples
    //   where nSamples = nQuads + 1
    //
    Vec2dArray res;

    // For adaptive sampling, we need to remember a few things about all the
    // samples in the currently processed segment ("segment" means "part of the
    // curve between two control points").
    //
    // These vectors could be declared in an inner loop but we declare them
    // here for performance (reuse vector capacity). All these vectors have
    // the same size.
    //
    Vec2dArray leftPositions;
    Vec2dArray rightPositions;
    Vec2dArray normals;
    core::DoubleArray uParams;

    // Remember which quads do not pass the angle test. The index is relative
    // to the vectors above (e.g., leftPositions).
    //
    core::IntArray failedQuads;

    // Factor out computation of cos(maxAngle)
    double cosMaxAngle = std::cos(maxAngle);

    // Early return if not enough segments
    Int numKnots = this->numKnots();
    Int numSegments = this->numSegments();
    if (numSegments < 1) {
        return res;
    }

    const Vec2d* positions = positions_.data();

    bool varyingWidth = widthVariability() == AttributeVariability::PerControlPoint;
    if (varyingWidth && widths_.length() < numKnots) {
        return {};
    }

    // Iterate over all segments
    for (Int idx = 0; idx < numSegments; ++idx) {
        // Get indices of Catmull-Rom control points for current segment
        Int zero = 0;
        Int i0 = core::clamp(idx - 1, zero, numSegments - 1);
        Int i1 = core::clamp(idx + 0, zero, numSegments - 1);
        Int i2 = core::clamp(idx + 1, zero, numSegments - 1);
        Int i3 = core::clamp(idx + 2, zero, numSegments - 1);

        // Get positions of Catmull-Rom control points
        std::array<Vec2d, 4> points = {
            positions[i0], positions[i1], positions[i2], positions[i3]};

        // Convert positions from Catmull-Rom to Bézier
        std::array<Vec2d, 4> catmullPoints;
        uniformCatmullRomToBezierCapped(points.data(), catmullPoints.data());
        const Vec2d& q0 = catmullPoints[0];
        const Vec2d& q1 = catmullPoints[1];
        const Vec2d& q2 = catmullPoints[2];
        const Vec2d& q3 = catmullPoints[3];

        // Convert widths from Constant or Catmull-Rom to Bézier. Note: we
        // could handle the 'Constant' case more efficiently, but we chose code
        // simplicity over performance here, over the assumption that it's unlikely
        // that width computation is a performance bottleneck.
        double w0, w1, w2, w3;
        if (varyingWidth) {
            const double* widths = widths_.data();
            double v0 = widths[i0];
            double v1 = widths[i1];
            double v2 = widths[i2];
            double v3 = widths[i3];
            uniformCatmullRomToBezier(v0, v1, v2, v3, w0, w1, w2, w3);
        }
        else // if (widthVariability() == AttributeVariability::Constant)
        {
            w0 = widthConstant_;
            w1 = w0;
            w2 = w0;
            w3 = w0;
        }

        // Compute first sample of segment
        if (idx == 0) {
            // Compute first sample of first segment
            double u = 0;
            appendUninitializedElement(leftPositions);
            appendUninitializedElement(rightPositions);
            appendUninitializedElement(normals);
            // clang-format off
            computeSample(
                q0, q1, q2, q3,
                w0, w1, w2, w3,
                u,
                leftPositions.last(),
                rightPositions.last(),
                normals.last());
            // clang-format on

            // Add this sample to res right now. For all the other samples, we
            // need to wait until adaptive sampling is complete.
            res.append(leftPositions.last());
            res.append(rightPositions.last());
        }
        else {
            // re-use last sample of previous segment
            removeAllExceptLastElement(leftPositions);
            removeAllExceptLastElement(rightPositions);
            removeAllExceptLastElement(normals);
        }
        uParams.clear();
        uParams.append(0);

        // Compute uniform samples for this segment
        Int numQuads = 0;
        double du = 1.0 / static_cast<double>(minQuads);
        for (Int j = 1; j <= minQuads; ++j) {
            double u = j * du;
            appendUninitializedElement(leftPositions);
            appendUninitializedElement(rightPositions);
            appendUninitializedElement(normals);
            // clang-format off
            computeSample(
                q0, q1, q2, q3,
                w0, w1, w2, w3,
                u,
                leftPositions.last(),
                rightPositions.last(),
                normals.last());
            // clang-format on

            uParams.append(u);
            ++numQuads;
        }

        // Compute adaptive samples for this segment
        while (numQuads < maxQuads) {
            // Find quads that don't pass the angle test.
            //
            // Quads are indexed from 0 to numQuads-1. A quad of index i is
            // defined by leftPositions[i], rightPositions[i],
            // leftPositions[i+1], and rightPositions[i+1].
            //
            failedQuads.clear();
            for (Int j = 0; j < numQuads; ++j) {
                if (normals[j].dot(normals[j + 1]) < cosMaxAngle) {
                    failedQuads.append(j);
                }
            }

            // All angles are < maxAngle => adaptive sampling is complete :)
            if (failedQuads.empty()) {
                break;
            }

            // We reached max number of quads :(
            numQuads += failedQuads.length();
            if (numQuads > maxQuads) {
                break;
            }

            // For each failed quad, we will recompute a sample at the
            // mid-u-parameter. We do this in-place in decreasing index
            // order so that we never overwrite samples.
            //
            // It's easier to understand the code by unrolling the loops
            // manually with the following example:
            //
            // uParams before = [ 0.0   0.2   0.4   0.6   0.8   1.0 ]
            // failedQuads    = [           1           3           ]
            // uParams after  = [ 0.0   0.2  *0.3*  0.4   0.6  *0.7*  0.8   1.0 ]
            //
            // The asterisks emphasize the two new samples.
            //
            Int numSamplesBefore = uParams.length();                       // 6
            Int numSamplesAfter = uParams.length() + failedQuads.length(); // 8
            leftPositions.resize(numSamplesAfter);
            rightPositions.resize(numSamplesAfter);
            normals.resize(numSamplesAfter);
            uParams.resize(numSamplesAfter);
            Int i = numSamplesBefore - 1;                         // 5
            for (Int j = failedQuads.length() - 1; j >= 0; --j) { // j = 1, then j = 0
                Int k = failedQuads[j];                           // k = 3, then k = 1

                // First, offset index of all samples after the failed quad
                Int offset = j + 1; // offset = 2, then offset = 1
                while (i > k) {     // i = [5, 4], then i = [3, 2]
                    leftPositions[i + offset] = leftPositions[i];
                    rightPositions[i + offset] = rightPositions[i];
                    normals[i + offset] = normals[i];
                    uParams[i + offset] = uParams[i]; // u[7] = 1.0, u[6] = 0.8, then
                                                      // u[4] = 0.6, u[3] = 0.4
                    --i;
                }

                // Then, for i == k, we compute the new sample.
                //
                // Note to maintainer: if you change this code, be very careful
                // to ensure that new values are always computed from old
                // values, not from already overwritten new values.
                //
                double u = 0.5 * (uParams[i] + uParams[i + 1]); // u = 0.7, then u = 0.3
                // clang-format off
                computeSample(
                    q0, q1, q2, q3,
                    w0, w1, w2, w3,
                    u,
                    leftPositions[i + offset],
                    rightPositions[i + offset],
                    normals[i + offset]);
                // clang-format on
                uParams[i + offset] = u;
            }
        }
        // Here are the different states of uParams for the given example:
        //
        // before:         [ 0.0   0.2   0.4   0.6   0.8   1.0 ]
        // resize:         [ 0.0   0.2   0.4   0.6   0.8   1.0   0.0   0.0 ]
        // offset j=1 i=5: [ 0.0   0.2   0.4   0.6   0.8   1.0   0.0   1.0 ]
        // offset j=1 i=4: [ 0.0   0.2   0.4   0.6   0.8   1.0   0.8   1.0 ]
        // new    j=1 i=3: [ 0.0   0.2   0.4   0.6   0.8   0.7   0.8   1.0 ]
        // offset j=0 i=3: [ 0.0   0.2   0.4   0.6   0.6   0.7   0.8   1.0 ]
        // offset j=0 i=2: [ 0.0   0.2   0.4   0.4   0.6   0.7   0.8   1.0 ]
        // new    j=0 i=1: [ 0.0   0.2   0.3   0.4   0.6   0.7   0.8   1.0 ]

        // Transfer local leftPositions and rightPositions into res
        Int numSamples = leftPositions.length();
        for (Int i = 1; i < numSamples; ++i) {
            res.append(leftPositions[i]);
            res.append(rightPositions[i]);
        }
    }

    return res;
}

namespace {

using detail::CubicBezierStroke;

// Currently assumes first derivative at endpoint is non null.
// TODO: Support null derivatives (using limit analysis).
// TODO: Support different halfwidth on both sides.
std::array<Vec2d, 2>
computeOffsetLineTangentsAtEndPoint(const CubicBezierStroke& data, Int endpoint) {

    Vec2d p = {};
    Vec2d dp = {};
    Vec2d ddp = {};
    double w = {};
    double dw = {};

    const std::array<Vec2d, 4>& positions = data.positions();
    const std::array<double, 4>& halfwidths = data.halfwidths();

    if (endpoint) {
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

    Vec2d a = dn * w + n * dw;
    return {(dp + a).normalized(), (dp - a).normalized()};
}

// ---------------------------------------------------------------------------------------
// Simple adaptive sampling in model space. Adapts to the curve widths in the same pass.
// To be deprecated in favor of the multi-view sampling further below.

struct IterativeSamplingSample {
    VGC_WARNING_PUSH
    VGC_WARNING_MSVC_DISABLE(26495) // member variable uninitialized
    IterativeSamplingSample() noexcept
        : pos(core::noInit)
        , normal(core::noInit)
        , tangent(core::noInit)
        , rightPoint(core::noInit)
        , leftPoint(core::noInit) {
    }
    VGC_WARNING_POP

    Vec2d pos;
    Vec2d normal;
    Vec2d tangent;
    Vec2d rightPoint;
    Vec2d leftPoint;
    double radius;
    double radiusDer;
    double u;
    Int subdivLevel = 0;

    // TODO: move these methods into CubicBezierStroke

    void computeFrom(const CubicBezierStroke& stroke, double u_) {
        u = u_;
        stroke.evalPositionAndDerivative(u, pos, tangent);
        if (!stroke.isWidthUniform()) {
            stroke.evalHalfwidthsAndDerivative(u, radius, radiusDer);
        }
        else {
            radius = stroke.halfwidths()[0];
            radiusDer = 0;
        }
        normal = tangent.normalized().orthogonalized();
        computeExtra_();
    }

    void computeFrom(
        const Vec2d& pos_,
        const Vec2d& tangent_,
        double radius_,
        double radiusDer_,
        double u_) {

        u = u_;
        pos = pos_;
        tangent = tangent_;
        radius = radius_;
        radiusDer = radiusDer_;
        normal = tangent.normalized().orthogonalized();
        computeExtra_();
    }

private:
    void computeExtra_() {
        Vec2d orthoRadius = radius * normal;
        rightPoint = pos + orthoRadius;
        leftPoint = pos - orthoRadius;
    }
};

struct IterativeSamplingSampleNode {
    IterativeSamplingSample sample;
    IterativeSamplingSampleNode* previous = nullptr;
    IterativeSamplingSampleNode* next = nullptr;
};

class IterativeSamplingCache {
public:
    std::optional<IterativeSamplingSample> previousSampleN;
    Int segmentIndex = 0;
    double cosMaxAngle;
    // specific to sampleIterLegacy_
    core::Array<IterativeSamplingSample> sampleStack;
    // specific to sampleIter_
    core::Span<IterativeSamplingSampleNode> sampleTree;

    void resetSampleTree(Int newStorageLength) {
        if (newStorageLength > sampleTree.length()) {
            sampleTreeStorage_ =
                std::make_unique<IterativeSamplingSampleNode[]>(newStorageLength);
            sampleTree = core::Span<IterativeSamplingSampleNode>(
                sampleTreeStorage_.get(), newStorageLength);
        }
    }

private:
    std::unique_ptr<IterativeSamplingSampleNode[]> sampleTreeStorage_;
};

bool isCenterLineSegmentUnderTolerance_(
    const IterativeSamplingSample& s0,
    const IterativeSamplingSample& s1,
    double cosMaxAngle) {

    // Test angle between curve normals and center segment normal.
    Vec2d l = s1.pos - s0.pos;
    Vec2d n = l.orthogonalized();
    double nl = n.length();
    double maxDot = cosMaxAngle * nl;
    if (n.dot(s0.normal) < maxDot) {
        return false;
    }
    if (n.dot(s1.normal) < maxDot) {
        return false;
    }
    return true;
}

bool areOffsetLinesAnglesUnderTolerance_(
    const IterativeSamplingSample& s0,
    const IterativeSamplingSample& s1,
    const IterativeSamplingSample& s2,
    double cosMaxAngle) {

    // Test angle between offset line segments of s0s1 and s1s2 on both sides.
    Vec2d l01 = s1.leftPoint - s0.leftPoint;
    Vec2d l12 = s2.leftPoint - s1.leftPoint;
    double ll = l01.length() * l12.length();
    if (l01.dot(l12) < cosMaxAngle * ll) {
        return false;
    }
    Vec2d r01 = s1.rightPoint - s0.rightPoint;
    Vec2d r12 = s2.rightPoint - s1.rightPoint;
    double rl = r01.length() * r12.length();
    if (r01.dot(r12) < cosMaxAngle * rl) {
        return false;
    }
    return true;
}

bool shouldKeepNewSample_(
    const IterativeSamplingSample& previousSample,
    const IterativeSamplingSample& sample,
    const IterativeSamplingSample& nextSample,
    double cosMaxAngle) {

    return !isCenterLineSegmentUnderTolerance_(previousSample, nextSample, cosMaxAngle)
           || !areOffsetLinesAnglesUnderTolerance_(
               previousSample, sample, nextSample, cosMaxAngle);
}

// Samples the segment [data.segmentIndex, data.segmentIndex + 1], and append the
// result to outAppend.
//
// The first sample of the segment is appended only if the cache `data` is new.
// The last sample is always appended.
//
[[maybe_unused]] bool sampleIterLegacy_(
    const Curve* curve,
    const CurveSamplingParameters& params,
    IterativeSamplingCache& data,
    core::Array<CurveSample>& outAppend) {

    auto bezierData = CubicBezierStroke::fromCurve(curve, data.segmentIndex);

    IterativeSamplingSample s0 = {};
    IterativeSamplingSample sN = {};

    // Compute first sample of segment.
    if (!data.previousSampleN.has_value()) {
        s0.computeFrom(bezierData, 0);
        outAppend.emplaceLast(s0.pos, s0.normal, s0.radius);
    }
    else {
        // Re-use last sample of previous segment.
        s0 = *data.previousSampleN;
        s0.u = 0;
    }

    // Compute last sample of segment.
    { sN.computeFrom(bezierData, 1); }

    const double cosMaxAngle = data.cosMaxAngle;
    const Int minISS = params.minIntraSegmentSamples();
    const Int maxISS = params.maxIntraSegmentSamples();
    const Int minSamples = (std::max)(Int(0), minISS) + 2;
    const Int maxSamples = (std::max)(minISS, maxISS) + 2;
    const Int extraSamples = maxSamples - minSamples;
    const Int level0Lines = minSamples - 1;

    const Int extraSamplesPerLevel0Line =
        static_cast<Int>(std::floor(static_cast<float>(extraSamples) / level0Lines));
    const Int maxSubdivLevels =
        static_cast<Int>(std::floor(std::log2(extraSamplesPerLevel0Line + 1)));

    core::Array<IterativeSamplingSample>& sampleStack = data.sampleStack;
    sampleStack.resize(0);
    sampleStack.reserve(extraSamplesPerLevel0Line + 1);

    double duLevel0 = 1.0 / static_cast<double>(minSamples - 1);
    for (Int i = 1; i < minSamples; ++i) {
        // Uniform sample
        IterativeSamplingSample* s = &sampleStack.emplaceLast();
        if (i == minSamples - 1) {
            *s = sN;
        }
        else {
            double u = i * duLevel0;
            s->computeFrom(bezierData, u);
        }
        while (s != nullptr) {
            // Adaptive sampling
            Int subdivLevel = (std::max)(s0.subdivLevel, s->subdivLevel);
            bool subdivided = false;
            if (subdivLevel < maxSubdivLevels) {
                double u = (s0.u + s->u) * 0.5;
                IterativeSamplingSample* s1 = &sampleStack.emplaceLast();
                s1->computeFrom(bezierData, u);
                if (shouldKeepNewSample_(s0, *s1, *s, cosMaxAngle)) {
                    subdivided = true;
                    s1->subdivLevel = subdivLevel + 1;
                    s = s1;
                }
                else {
                    sampleStack.pop();
                }
            }
            if (!subdivided) {
                s0 = *s;
                outAppend.emplaceLast(s0.pos, s0.normal, s0.radius);
                sampleStack.pop();
                s = sampleStack.isEmpty() ? nullptr : &sampleStack.last();
            }
        }
    }

    data.segmentIndex += 1;
    data.previousSampleN = sN;
    return true;
}

// Samples the segment [data.segmentIndex, data.segmentIndex + 1], and append the
// result to outAppend.
//
// The first sample of the segment is appended only if the cache `data` is new.
// The last sample is always appended.
//
bool sampleIter_(
    const Curve* curve,
    const CurveSamplingParameters& params,
    IterativeSamplingCache& data,
    core::Array<CurveSample>& outAppend) {

    const double cosMaxAngle = data.cosMaxAngle;
    const Int minISS = params.minIntraSegmentSamples(); // 0 -> 2 samples minimum
    const Int maxISS = params.maxIntraSegmentSamples(); // 1 -> 3 samples maximum
    const Int minSamples = std::max<Int>(0, minISS) + 2;
    const Int maxSamples = std::max<Int>(minSamples, maxISS + 2);

    auto bezierData = CubicBezierStroke::fromCurve(curve, data.segmentIndex);

    data.resetSampleTree(maxSamples);

    // Setup first and last sample nodes of segment.
    IterativeSamplingSampleNode* s0 = &data.sampleTree[0];
    if (data.previousSampleN.has_value()) {
        // Re-use last sample of previous segment.
        s0->sample = *data.previousSampleN;
        s0->sample.u = 0;
    }
    else {
        s0->sample.computeFrom(bezierData, 0);
        const IterativeSamplingSample& ss = s0->sample;
        outAppend.emplaceLast(ss.pos, ss.normal, ss.radius);
    }
    IterativeSamplingSampleNode* sN = &data.sampleTree[1];
    sN->sample.computeFrom(bezierData, 1);
    s0->previous = nullptr;
    s0->next = sN;
    sN->previous = s0;
    sN->next = nullptr;
    data.segmentIndex += 1;
    data.previousSampleN = sN->sample;

    auto linkNode = [](IterativeSamplingSampleNode* node,
                       IterativeSamplingSampleNode* previous) {
        IterativeSamplingSampleNode* next = previous->next;
        next->previous = node;
        previous->next = node;
        node->previous = previous;
        node->next = next;
    };
    Int nextNodeIndex = 2;

    // Compute `minIntraSegmentSamples` uniform samples.
    IterativeSamplingSampleNode* previousNode = s0;
    for (Int i = 1; i < minISS; ++i) {
        IterativeSamplingSampleNode* node = &data.sampleTree[nextNodeIndex++];
        double u = static_cast<double>(i) / minISS;
        node->sample.computeFrom(bezierData, u);
        linkNode(node, previousNode);
        previousNode = node;
    }

    const Int sampleTreeLength = data.sampleTree.length();
    Int previousLevelStartIndex = 2;
    Int previousLevelEndIndex = nextNodeIndex;

    // Fallback to using the last sample as previous level sample
    // when we added no uniform samples.
    if (previousLevelStartIndex == previousLevelEndIndex) {
        previousLevelStartIndex = 1;
    }

    auto trySubdivide = [&](IterativeSamplingSampleNode* n0,
                            IterativeSamplingSampleNode* n1) -> bool {
        IterativeSamplingSampleNode* node = &data.sampleTree[nextNodeIndex];
        node->sample.computeFrom(bezierData, 0.5 * (n0->sample.u + n1->sample.u));
        if (shouldKeepNewSample_(n0->sample, node->sample, n1->sample, cosMaxAngle)) {
            nextNodeIndex++;
            linkNode(node, n0);
            return true;
        }
        return false;
    };

    while (nextNodeIndex < sampleTreeLength) {
        // Since we create a candidate on the left and right of each previous level node,
        // each pass can add as much as twice the amount of nodes of the previous level.
        for (Int i = previousLevelStartIndex; i < previousLevelEndIndex; ++i) {
            IterativeSamplingSampleNode* previousLevelNode = &data.sampleTree[i];
            // Try subdivide left.
            bool found = trySubdivide(previousLevelNode->previous, previousLevelNode);
            if (found && nextNodeIndex == sampleTreeLength) {
                break;
            }
            // We subdivide right only if it is not the last point.
            if (!previousLevelNode->next) {
                continue;
            }
            // Try subdivide right.
            found = trySubdivide(previousLevelNode, previousLevelNode->next);
            if (found && nextNodeIndex == sampleTreeLength) {
                break;
            }
        }
        if (nextNodeIndex == previousLevelEndIndex) {
            // No new candidate, let's stop here.
            break;
        }
        previousLevelStartIndex = previousLevelEndIndex;
        previousLevelEndIndex = nextNodeIndex;
    }

    IterativeSamplingSampleNode* node = data.sampleTree[0].next;
    while (node) {
        const IterativeSamplingSample& ss = node->sample;
        outAppend.emplaceLast(ss.pos, ss.normal, ss.radius);
        node = node->next;
    }

    return true;
}

} // namespace

void Curve::sampleRange(
    core::Array<CurveSample>& outAppend,
    const CurveSamplingParameters& parameters,
    Int startKnot,
    Int numSegments,
    bool computeArclength) const {

    Int numKnots = this->numKnots();
    Int n = this->numSegments();

    // Verify we have at least one knot, since a post-condition of this
    // function is to return at least one sample.
    if (numKnots == 0) {
        throw vgc::core::IndexError("Cannot sample a curve with no knots.");
    }

    // Verify and wrap startKnot
    if (startKnot < -numKnots || startKnot > numKnots - 1) {
        throw vgc::core::IndexError(vgc::core::format(
            "Parameter startKnot ({}) out of valid knot index range [{}, {}].",
            startKnot,
            -numKnots,
            numKnots - 1));
    }
    if (startKnot < 0) {
        startKnot += numKnots; // -1 becomes numKnots - 1 (=> last knot)
    }

    // Verify and wrap numSegments
    if (numSegments < -n - 1 || numSegments > n) {
        throw vgc::core::IndexError(vgc::core::format(
            "Parameter numSegments ({}) out of valid number of segments range [{}, {}].",
            numSegments,
            -n - 1,
            n));
    }
    if (numSegments < 0) {
        numSegments += n + 1; // -1 becomes n (=> all segments)
    }
    if (!isClosed() && numSegments > n - startKnot) {
        throw vgc::core::IndexError(core::format(
            "Parameter numSegments ({} after negative-wrap) exceeds remaining number of "
            "segments when starting at the given startKnot ({} after negative-wrap): "
            "valid range is [0, {}] since the curve is open and has {} knots.",
            numSegments,
            startKnot,
            n - startKnot,
            numKnots));
    }

    // Remember old length of outAppend
    const Int oldLength = outAppend.length();

    if (numKnots == 1) {
        // Handle case where there are no segments at all the curve.
        //
        // Note that this is different from `start == end` with `n > 1`, in
        // which case we need to actually evaluate a Bezier curve to get the
        // normal.
        //
        const bool isWidthUniform =
            (widthVariability() == Curve::AttributeVariability::Constant);
        Vec2d position = positions()[0];
        Vec2d normal(0, 0);
        double halfwidth = 0.5 * (isWidthUniform ? width() : widths_[0]);
        outAppend.emplaceLast(position, normal, halfwidth);
    }
    else {
        if (numSegments == 0) {
            // Add a point manually if it is a single point segment.
            IterativeSamplingSample lastSample;
            CubicBezierStroke bezierData;
            double u;
            if (startKnot < n) {
                bezierData = CubicBezierStroke::fromCurve(this, startKnot);
                u = 0;
            }
            else { // start == n
                bezierData = CubicBezierStroke::fromCurve(this, n - 1);
                u = 1;
            }
            lastSample.computeFrom(bezierData, u);
            outAppend.emplaceLast(lastSample.pos, lastSample.normal, lastSample.radius);
        }
        else {
            // Reserve memory space
            const Int minSegmentSamples =
                std::max<Int>(0, parameters.minIntraSegmentSamples()) + 1;
            outAppend.reserve(outAppend.length() + 1 + numSegments * minSegmentSamples);

            // Iterate over all segments
            IterativeSamplingCache data = {};
            data.cosMaxAngle = std::cos(parameters.maxAngle());
            data.segmentIndex = startKnot;
            for (Int i = 0; i < numSegments; ++i) {
                sampleIter_(this, parameters, data, outAppend);
                data.segmentIndex = data.segmentIndex % n;
            }
        }
    }

    // Compute arclength.
    //
    if (computeArclength) {

        // Compute arclength of first new sample
        double s = 0;
        auto it = outAppend.begin() + oldLength;
        if (oldLength > 0) {
            CurveSample& firstNewSample = *it;
            CurveSample& lastOldSample = *(it - 1);
            s = lastOldSample.s()
                + (firstNewSample.position() - lastOldSample.position()).length();
        }
        it->setS(s);
        Vec2d lastPoint = it->position();

        // Compute arclength of all subsequent samples
        for (++it; it != outAppend.end(); ++it) {
            Vec2d point = it->position();
            s += (point - lastPoint).length();
            it->setS(s);
            lastPoint = point;
        }
    }
}

std::array<Vec2d, 2>
Curve::getOffsetLineTangentsAtSegmentEndpoint(Int segmentIndex, Int endpointIndex) const {

    if (segmentIndex < 0 || segmentIndex >= numSegments()) {
        throw vgc::core::IndexError(core::format(
            "The given `segmentIndex` ({}) is out of range "
            " `[0, numSegments() - 1]` ([0, {}])",
            segmentIndex,
            numSegments() - 1));
    }

    if (endpointIndex < 0 || endpointIndex > 1) {
        throw vgc::core::IndexError(core::format(
            "The given `endpointIndex` ({}) must be `0` or `1`", endpointIndex));
    }

    auto bezierData = CubicBezierStroke::fromCurve(this, segmentIndex);
    return computeOffsetLineTangentsAtEndPoint(bezierData, endpointIndex);
}

void Curve::onWidthsChanged_() {
    // todo, compute max and average
}

} // namespace vgc::geometry

/*
############################# Implementation notes #############################

[1]

In the future, we may want to extend the Curve class with:
    - more curve type (e.g., bezier, bspline, nurbs, ellipticalarc. etc.)
    - variable color
    - variable custom attributes (e.g., that can be passed to shaders)
    - dimension other than 2? Probably not: That may be a separate type of
      curve

Supporting other types of curves in the future is why we use a
std::vector<double> of size 2*n instead of a std::vector<Vec2d> of size n.
Indeed, other types of curve may need additional data, such as knot values,
homogeneous coordinates, etc.

A "cleaner" approach with more type-safety would be to have different
classes for different types of curves. Unfortunately, this has other
drawbacks, in particular, switching from one curve type to the other
dynamically would be harder. Also, it is quite useful to have a continuous
array of doubles that can directly be passed to C-style functions, such as
OpenGL, etc.

[2] Should the "Curve" class be called Curve2d?

*/
