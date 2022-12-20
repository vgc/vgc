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

Curve::Curve(Type type)
    : type_(type)
    , positionData_()
    , widthVariability_(AttributeVariability::PerControlPoint)
    , widthData_()
    , color_(core::colors::black) {
}

Curve::Curve(double constantWidth, Type type)
    : type_(type)
    , positionData_()
    , widthVariability_(AttributeVariability::Constant)
    , widthData_(1, constantWidth) // = vector containing a single element
    , color_(core::colors::black) {
}

double Curve::width() const {
    return averageWidth_;
}

//void Curve::addControlPoint(double x, double y) {
//
//    // Set position
//    positionData_.emplaceLast(x, y);
//
//    // Set width
//    if (widthVariability() == AttributeVariability::PerControlPoint) {
//        double width = 1.0;
//        if (widthData_.size() > 0) {
//            width = widthData_.last();
//        }
//        widthData_.append(width);
//    }
//}
//
//void Curve::addControlPoint(const Vec2d& position) {
//    addControlPoint(position.x(), position.y());
//}
//
//void Curve::addControlPoint(double x, double y, double width) {
//
//    // Set position
//    positionData_.emplaceLast(x, y);
//
//    // Set width
//    if (widthVariability() == AttributeVariability::PerControlPoint) {
//        widthData_.append(width);
//    }
//}
//
//void Curve::addControlPoint(const Vec2d& position, double width) {
//    addControlPoint(position.x(), position.y(), width);
//}

Vec2dArray Curve::triangulate(double maxAngle, Int minQuads, Int maxQuads) const {

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

    const Vec2dArray& positionData = positionData_.get();

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
            positionData[i0], positionData[i1], positionData[i2], positionData[i3]};

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
        if (widthVariability() == AttributeVariability::PerControlPoint) {
            double v0 = widthData()[i0];
            double v1 = widthData()[i1];
            double v2 = widthData()[i2];
            double v3 = widthData()[i3];
            uniformCatmullRomToBezier(v0, v1, v2, v3, w0, w1, w2, w3);
        }
        else // if (widthVariability() == AttributeVariability::Constant)
        {
            w0 = widthData()[0];
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

    void computeFrom(
        const std::array<Vec2d, 4>& controlPoints,
        const std::array<double, 4>& radii,
        double u_,
        bool isWidthUniform) {

        u = u_;
        cubicBezierPosAndDerCasteljau<Vec2d>(controlPoints, u_, pos, tangent);
        if (!isWidthUniform) {
            cubicBezierPosAndDerCasteljau<double>(radii, u, radius, radiusDer);
        }
        else {
            radius = radii[0];
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

bool sampleIter_(
    const Curve* curve,
    const CurveSamplingParameters& params,
    IterativeSamplingCache& data,
    core::Array<CurveSample>& outAppend) {

    const Int idx = data.segmentIndex;

    // Get indices of interpolated points for current segment.
    const Int numPts = curve->numPoints();
    Int i0 = (std::max)(idx - 1, Int(0));
    Int i1 = idx;
    Int i2 = idx + 1;
    Int i3 = (std::min)(idx + 2, numPts - 1);

    // Return now if idx is not a valid segment index.
    if (i1 < 0 || i2 >= numPts) {
        return false;
    }

    // Get positions of interpolated points.
    const Vec2dArray& positionData = curve->positionData();
    std::array<Vec2d, 4> cps = {
        positionData[i0], positionData[i1], positionData[i2], positionData[i3]};

    // Convert points to Bézier.
    uniformCatmullRomToBezierCappedInPlace(cps.data());

    // Convert widths from Catmull-Rom to Bézier if not uniform.
    const core::DoubleArray& widthData = curve->widthData();
    std::array<double, 4> radii = {
        widthData[i0] * 0.5,
        widthData[i1] * 0.5,
        widthData[i2] * 0.5,
        widthData[i3] * 0.5};
    const bool isWidthUniform =
        (curve->widthVariability() == Curve::AttributeVariability::Constant);
    if (!isWidthUniform) {
        // XXX use something that does not overshoot
        uniformCatmullRomToBezierInPlace(radii.data());
    }

    IterativeSamplingSample s0 = {};
    IterativeSamplingSample sN = {};

    // Compute first sample of segment.
    if (!data.previousSampleN.has_value()) {
        double radiusDer = isWidthUniform ? 0 : radii[1] - radii[0];
        s0.computeFrom(cps[0], 3 * (cps[1] - cps[0]), radii[0], radiusDer, 0);
        outAppend.emplaceLast(s0.pos, s0.normal, s0.radius);
    }
    else {
        // Re-use last sample of previous segment.
        s0 = *data.previousSampleN;
        s0.u = 0;
    }

    // Compute last sample of segment.
    {
        double radiusDer = isWidthUniform ? 0 : radii[3] - radii[2];
        sN.computeFrom(cps[3], 3 * (cps[3] - cps[2]), radii[3], radiusDer, 1);
    }

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
        1 + static_cast<Int>(std::floor(std::log2(extraSamplesPerLevel0Line + 1)));

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
            s->computeFrom(cps, radii, u, isWidthUniform);
        }
        while (s != nullptr) {
            // Adaptive sampling
            Int subdivLevel = (std::max)(s0.subdivLevel, s->subdivLevel);
            if (subdivLevel < maxSubdivLevels
                && !testLine_(s0, *s, cosMaxAngle, isWidthUniform)) {

                double u = (s0.u + s->u) * 0.5;
                s = &sampleStack.emplaceLast();
                s->computeFrom(cps, radii, u, isWidthUniform);
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

void Curve::sampleRange(
    const CurveSamplingParameters& parameters,
    core::Array<CurveSample>& outAppend,
    Int start,
    Int end) const {

    Int numSegs = (std::max)(Int(0), numPoints() - 1);
    if (start < 0) {
        start = (std::max)(Int(0), numSegs - start);
    }
    else if (start > numSegs - 1) {
        return;
    }
    if (end < 0 || end > numSegs) {
        end = numSegs;
    }

    const Int minSegmentSamples =
        (std::max)(Int(0), parameters.minIntraSegmentSamples()) + 1;
    outAppend.reserve(outAppend.length() + 1 + (end - start) * minSegmentSamples);

    IterativeSamplingCache data = {};
    data.cosMaxAngle = std::cos(parameters.maxAngle());
    data.segmentIndex = start;
    for (Int i = start; i < end; ++i) {
        sampleIter_(this, parameters, data, outAppend);
    }
}

void Curve::onWidthDataChanged_() {
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
