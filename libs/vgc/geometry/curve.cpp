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
    (UniformLow, "Uniform Low"),
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
    case CurveSamplingQuality::UniformLow:
        maxAngle_ = 100;
        minIntraSegmentSamples_ = 3;
        maxIntraSegmentSamples_ = 3;
        break;
    case CurveSamplingQuality::AdaptiveLow:
        maxAngle_ = 0.05;
        minIntraSegmentSamples_ = 0;
        maxIntraSegmentSamples_ = 7;
        break;
    case CurveSamplingQuality::UniformHigh:
        maxAngle_ = 100;
        minIntraSegmentSamples_ = 15;
        maxIntraSegmentSamples_ = 15;
        break;
    case CurveSamplingQuality::AdaptiveHigh:
        maxAngle_ = 0.025;
        minIntraSegmentSamples_ = 0;
        maxIntraSegmentSamples_ = 31;
        break;
    case CurveSamplingQuality::UniformVeryHigh:
        maxAngle_ = 100;
        minIntraSegmentSamples_ = 63;
        maxIntraSegmentSamples_ = 63;
        break;
    }
}

Curve::Curve(Type type)
    : type_(type)
    , widthVariability_(AttributeVariability::PerControlPoint)
    , color_(core::colors::black) {
}

Curve::Curve(double constantWidth, Type type)
    : type_(type)
    , widthVariability_(AttributeVariability::Constant)
    , widthConstant_(constantWidth)
    , color_(core::colors::black) {
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
    Int numCPs = numPoints();
    Int numSegments = numCPs - 1;
    if (numSegments < 1) {
        return res;
    }

    const Vec2d* positions = positions_.data();

    bool varyingWidth = widthVariability() == AttributeVariability::PerControlPoint;
    if (varyingWidth && widths_.length() < numCPs) {
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
        Vec2d q0, q1, q2, q3;
        uniformCatmullRomToBezierCappedInPlace(points.data());
        q0 = points[0];
        q1 = points[1];
        q2 = points[2];
        q3 = points[3];

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

struct CubicBezierData {
    std::array<Vec2d, 4> positions;
    std::array<double, 4> halfwidths;
    bool isWidthUniform = false;

    // Uninitialized
    //
    CubicBezierData() {
    }

    // Returns the CubicBezierData corresponding to the segment at index [i,
    // i+1] in the given Curve.
    //
    CubicBezierData(const Curve* curve, Int i) {

        // Ensure we have a valid segment between two control points
        const Int numPts = curve->numPoints();
        VGC_ASSERT(i >= 0);
        VGC_ASSERT(i <= numPts - 2);

        // Get indices of points used by the Catmull-Rom interpolation
        Int i0 = (std::max)(i - 1, Int(0));
        Int i1 = i;
        Int i2 = i + 1;
        Int i3 = (std::min)(i + 2, numPts - 1);

        // Get positions
        const Vec2d* p = curve->positions().data();
        positions = {p[i0], p[i1], p[i2], p[i3]};

        // Get widths
        isWidthUniform =
            (curve->widthVariability() == Curve::AttributeVariability::Constant);
        if (isWidthUniform) {
            double hw = curve->width() * 0.5;
            halfwidths = {hw, hw, hw, hw};
        }
        else {
            const double* w = curve->widths().data();
            halfwidths = {w[i0] * 0.5, w[i1] * 0.5, w[i2] * 0.5, w[i3] * 0.5};
        }

        // Convert from Catmull-Rom to Bézier.
        uniformCatmullRomToBezierCappedInPlace(positions.data());
        if (!isWidthUniform) {
            uniformCatmullRomToBezierInPlace(halfwidths.data());
        }

        // Set mirror tangents at endpoints.
        bool isEndSegment = (i + 1) == (numPts - 1);
        if (i == 0) {
            if (!isEndSegment) {
                Vec2d n = (positions[3] - positions[0]).orthogonalized().normalized();
                Vec2d d = positions[2] - positions[3];
                d = (2.0 * (n.dot(d)) * n) - d;
                positions[1] = positions[0] + d;
            }
        }
        else if (isEndSegment) {
            Vec2d n = (positions[3] - positions[0]).orthogonalized().normalized();
            Vec2d d = positions[1] - positions[0];
            d = 2 * (n.dot(d)) * n - d;
            positions[2] = positions[3] + d;
        }
    }
};

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
        , leftPoint(core::noInit)
        , rightPointNormal(core::noInit)
        , leftPointNormal(core::noInit) {
    }
    VGC_WARNING_POP

    Vec2d pos;
    Vec2d normal;
    Vec2d tangent;
    Vec2d rightPoint;
    Vec2d leftPoint;
    Vec2d rightPointNormal;
    Vec2d leftPointNormal;
    double radius;
    double radiusDer;
    double u;
    Int subdivLevel = 0;

    void computeFrom(const CubicBezierData& data, double u_) {
        u = u_;
        cubicBezierPosAndDerCasteljau<Vec2d>(data.positions, u, pos, tangent);
        if (!data.isWidthUniform) {
            cubicBezierPosAndDerCasteljau<double>(data.halfwidths, u, radius, radiusDer);
        }
        else {
            radius = data.halfwidths[0];
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
        if (radiusDer != 0) {
            Vec2d dr = radiusDer * normal;
            rightPointNormal = (tangent + dr).normalized().orthogonalized();
            leftPointNormal = -(tangent - dr).normalized().orthogonalized();
        }
        else {
            rightPointNormal = normal;
            leftPointNormal = -normal;
        }
        Vec2d orthoRadius = radius * normal;
        rightPoint = pos + orthoRadius;
        leftPoint = pos - orthoRadius;
    }
};

struct IterativeSamplingCache {
    std::optional<IterativeSamplingSample> previousSampleN;
    Int segmentIndex = 0;
    double cosMaxAngle;
    core::Array<IterativeSamplingSample> sampleStack;
};

bool testLine_(
    const IterativeSamplingSample& s0,
    const IterativeSamplingSample& s1,
    double cosMaxAngle,
    bool isWidthUniform) {

    // Test angle between curve normals and center segment normal.
    geometry::Vec2d l = s1.pos - s0.pos;
    geometry::Vec2d n = l.normalized().orthogonalized();
    if (n.dot(s0.normal) < cosMaxAngle) {
        return false;
    }
    if (n.dot(s1.normal) < cosMaxAngle) {
        return false;
    }
    if (isWidthUniform) {
        return true;
    }

    // Test angle between curve normals and outline segments normal.
    geometry::Vec2d ll = s1.leftPoint - s0.leftPoint;
    geometry::Vec2d lln = -ll.normalized().orthogonalized();
    if (lln.dot(s0.leftPointNormal) < cosMaxAngle) {
        return false;
    }
    if (lln.dot(s1.leftPointNormal) < cosMaxAngle) {
        return false;
    }
    geometry::Vec2d rl = s1.rightPoint - s0.rightPoint;
    geometry::Vec2d rln = rl.normalized().orthogonalized();
    if (rln.dot(s0.rightPointNormal) < cosMaxAngle) {
        return false;
    }
    if (rln.dot(s1.rightPointNormal) < cosMaxAngle) {
        return false;
    }
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

    CubicBezierData bezierData(curve, data.segmentIndex);

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
            if (subdivLevel < maxSubdivLevels
                && !testLine_(s0, *s, cosMaxAngle, bezierData.isWidthUniform)) {

                double u = (s0.u + s->u) * 0.5;
                s = &sampleStack.emplaceLast();
                s->computeFrom(bezierData, u);
                s->subdivLevel = subdivLevel + 1;
            }
            else {
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

// Python-like index wrapping
Int wrapSampleIndex(Int i, Int n) {
    if (n == 0) {
        throw vgc::core::IndexError("cannot sample a curve with no points.");
    }
    else if (i < -n || i > n - 1) {
        throw vgc::core::IndexError(vgc::core::format(
            "index {} out of range [{}, {}] (num points is {})", i, -n, n - 1, n));
    }
    else {
        if (i < 0) {
            i += n;
        }
        return i;
    }
}

} // namespace

void Curve::sampleRange(
    core::Array<CurveSample>& outAppend,
    const CurveSamplingParameters& parameters,
    Int start,
    Int end,
    bool computeArclength) const {

    // Cleanup start and end indices
    Int n = numPoints();
    start = wrapSampleIndex(start, n);
    end = wrapSampleIndex(end, n);
    if (start > end) {
        throw vgc::core::IndexError(
            vgc::core::format("start index ({}) > end index ({})", start, end));
    }

    // Remember old length of outAppend
    const Int oldLength = outAppend.length();

    if (n == 1) {
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
        // Reserve memory space
        const Int minSegmentSamples =
            (std::max)(Int(0), parameters.minIntraSegmentSamples()) + 1;
        outAppend.reserve(outAppend.length() + 1 + (end - start) * minSegmentSamples);

        if (start == end) {
            // Add a point manually if it is a single point segment.
            IterativeSamplingSample lastSample;
            CubicBezierData bezierData;
            double u;
            if (start < n - 1) {
                bezierData = CubicBezierData(this, start);
                u = 0;
            }
            else { // start == n - 1
                bezierData = CubicBezierData(this, n - 2);
                u = 1;
            }
            lastSample.computeFrom(bezierData, u);
            outAppend.emplaceLast(lastSample.pos, lastSample.normal, lastSample.radius);
        }
        else {
            // Iterate over all segments
            IterativeSamplingCache data = {};
            data.cosMaxAngle = std::cos(parameters.maxAngle());
            data.segmentIndex = start;
            for (Int i = start; i < end; ++i) {
                sampleIter_(this, parameters, data, outAppend);
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
        geometry::Vec2d lastPoint = it->position();

        // Compute arclength of all subsequent samples
        for (++it; it != outAppend.end(); ++it) {
            geometry::Vec2d point = it->position();
            s += (point - lastPoint).length();
            it->setS(s);
            lastPoint = point;
        }
    }
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
