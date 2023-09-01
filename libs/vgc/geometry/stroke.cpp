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

#include <vgc/geometry/stroke.h>

#include <array>
#include <optional>

#include <vgc/core/algorithm.h>
#include <vgc/core/array.h>
#include <vgc/core/colors.h>
#include <vgc/geometry/bezier.h>
#include <vgc/geometry/catmullrom.h>
#include <vgc/geometry/curve.h>

namespace vgc::geometry {

DistanceToCurve
distanceToCurve(const StrokeSample2dArray& samples, const Vec2d& position) {
    return detail::distanceToCurve<StrokeSample2d>(samples, position);
}

DistanceToCurve
distanceToCurve(const StrokeSampleEx2dArray& samples, const Vec2d& position) {
    return detail::distanceToCurve<StrokeSampleEx2d>(samples, position);
}

namespace detail {

bool isCenterlineSegmentUnderTolerance(
    const StrokeSampleEx2d& s0,
    const StrokeSampleEx2d& s1,
    double cosMaxAngle) {

    // Test angle between curve normals and center segment normal.
    Vec2d t = s1.position() - s0.position();
    double tl = t.length();
    double maxDot = cosMaxAngle * tl;
    if (t.dot(s0.tangent()) < maxDot) {
        return false;
    }
    if (t.dot(s1.tangent()) < maxDot) {
        return false;
    }
    return true;
}

bool areOffsetLinesAnglesUnderTolerance(
    const StrokeSampleEx2d& s0,
    const StrokeSampleEx2d& s1,
    const StrokeSampleEx2d& s2,
    double cosMaxAngle) {

    // Test angle between offset line segments of s0s1 and s1s2 on both sides.
    Vec2d l01 = s1.offsetPoint(1) - s0.offsetPoint(1);
    Vec2d l12 = s2.offsetPoint(1) - s1.offsetPoint(1);
    double ll = l01.length() * l12.length();
    if (l01.dot(l12) < cosMaxAngle * ll) {
        return false;
    }
    Vec2d r01 = s1.offsetPoint(0) - s0.offsetPoint(0);
    Vec2d r12 = s2.offsetPoint(0) - s1.offsetPoint(0);
    double rl = r01.length() * r12.length();
    if (r01.dot(r12) < cosMaxAngle * rl) {
        return false;
    }
    return true;
}

bool shouldKeepNewSample(
    const StrokeSampleEx2d& previousSample,
    const StrokeSampleEx2d& sample,
    const StrokeSampleEx2d& nextSample,
    const CurveSamplingParameters& params) {

    return !isCenterlineSegmentUnderTolerance(
               previousSample, nextSample, params.cosMaxAngle())
           || !areOffsetLinesAnglesUnderTolerance(
               previousSample, sample, nextSample, params.cosMaxAngle());
}

} // namespace detail

namespace {

using OptionalInt = std::optional<Int>;

// Returns the index of the segment just before the given `knotIndex`, if any.
//
OptionalInt indexOfSegmentBeforeKnot_(const AbstractStroke2d* stroke, Int knotIndex) {
    if (stroke->isClosed()) {
        Int segmentIndex = knotIndex - 1;
        if (segmentIndex < 0) {
            segmentIndex += stroke->numSegments();
        }
        return segmentIndex;
    }
    else {
        if (knotIndex > 0) {
            return knotIndex - 1;
        }
        else {
            return std::nullopt;
        }
    }
}

OptionalInt indexOfFirstNonZeroLengthSegmentBeforeKnot_(
    const AbstractStroke2d* stroke,
    Int knotIndex) {

    if (stroke->isClosed()) {
        Int numSegments = stroke->numSegments();
        Int start = knotIndex - 1 + numSegments; // Ensures `start - i >= 0`
        for (Int i = 0; i < numSegments; ++i) {
            Int segmentIndex = (start - i) % numSegments;
            if (!stroke->isZeroLengthSegment(segmentIndex)) {
                return segmentIndex;
            }
        }
        return std::nullopt;
    }
    else {
        Int segmentIndex = knotIndex - 1;
        while (segmentIndex >= 0) {
            if (!stroke->isZeroLengthSegment(segmentIndex)) {
                return segmentIndex;
            }
            else {
                --segmentIndex;
            }
        }
        return std::nullopt;
    }
}

OptionalInt indexOfFirstNonZeroLengthSegmentAfterKnot_(
    const AbstractStroke2d* stroke,
    Int knotIndex) {

    if (stroke->isClosed()) {
        Int numSegments = stroke->numSegments();
        for (Int i = 0; i < numSegments; ++i) {
            Int segmentIndex = (knotIndex + i) % numSegments;
            if (!stroke->isZeroLengthSegment(segmentIndex)) {
                return segmentIndex;
            }
        }
        return std::nullopt;
    }
    else {
        Int segmentIndex = knotIndex;
        Int numSegments = stroke->numSegments();
        while (segmentIndex < numSegments) {
            if (!stroke->isZeroLengthSegment(segmentIndex)) {
                return segmentIndex;
            }
            else {
                ++segmentIndex;
            }
        }
        return std::nullopt;
    }
}

//StrokeSample2d
//computeSampleFromBezier_(const StrokeView2d* stroke, Int segmentIndex, double u) {
//    // VIRTUAL
//    auto bezier = CubicBezierStroke::fromStroke(stroke, segmentIndex);
//    IterativeSamplingSample s;
//    s.computeFrom(bezier, u);
//    return StrokeSample2d(s.pos, s.normal, s.radius);
//}

} // namespace

Vec2d AbstractStroke2d::evalCenterline(Int segmentIndex, double u) const {
    if (fixEvalLocation_(segmentIndex, u)) {
        return evalNonZeroCenterline(segmentIndex, u);
    }
    else {
        StrokeSample2d sample = zeroLengthStrokeSample();
        return sample.position();
    }
}

Vec2d AbstractStroke2d::evalCenterline(Int segmentIndex, double u, Vec2d& derivative)
    const {

    if (fixEvalLocation_(segmentIndex, u)) {
        return evalNonZeroCenterline(segmentIndex, u, derivative);
    }
    else {
        StrokeSampleEx2d sample = zeroLengthStrokeSample();
        derivative = sample.velocity();
        return sample.position();
    }
}

StrokeSampleEx2d AbstractStroke2d::eval(Int segmentIndex, double u) const {
    if (fixEvalLocation_(segmentIndex, u)) {
        return evalNonZero(segmentIndex, u);
    }
    else {
        return zeroLengthStrokeSample();
    }
}

void AbstractStroke2d::sampleSegment(
    StrokeSampleEx2dArray& out,
    Int segmentIndex,
    const CurveSamplingParameters& params) const {

    detail::AdaptiveStrokeSampler sampler = {};
    sampleSegment(out, segmentIndex, params, sampler);
}

void AbstractStroke2d::sampleSegment(
    StrokeSampleEx2dArray& out,
    Int segmentIndex,
    const CurveSamplingParameters& params,
    detail::AdaptiveStrokeSampler& sampler) const {

    if (isZeroLengthSegment(segmentIndex)) {
        Int numSegments = this->numSegments();
        Int startKnot = segmentIndex;
        Int endKnot = startKnot + 1;
        if (isClosed() && endKnot > numSegments) {
            endKnot -= numSegments;
        }

        // Determine whether the segment just before this segment
        // exists and is non-zero-length.
        OptionalInt nonZeroJustBefore = indexOfSegmentBeforeKnot_(this, startKnot);
        if (nonZeroJustBefore && isZeroLengthSegment(*nonZeroJustBefore)) {
            nonZeroJustBefore = std::nullopt;
        }

        // Determine whether a non-zero-length segment exists after this segment
        OptionalInt nonZeroAfter =
            indexOfFirstNonZeroLengthSegmentAfterKnot_(this, endKnot);

        if (nonZeroJustBefore) {
            if (nonZeroAfter) {
                // If the previous segment is non-zero-length, and there exists
                // a non-zero-length segment after this segment, then this
                // zero-length segment is responsible for the join. For now, we
                // do a bevel from the last sample of the previous segment to the
                // first sample of the first non-zero-length segment after this
                // segment.
                //
                // In the future, we may want to support round join/miter joins,
                // although this is complicated in case of varying width, and for
                // now the design is to only have such complicated joins we only
                // implement this complexity at vertices, not at handle this
                // complexity at vertices.
                //
                out.append(evalNonZero(*nonZeroJustBefore, 1));
                out.last().setCornerStart(true);
                out.append(evalNonZero(*nonZeroAfter, 0));
            }
            else {
                // This is the end of an open curve: no join to compute, just use
                // the last sample of the previous segment.
                //
                out.append(evalNonZero(*nonZeroJustBefore, 1));
            }
        }
        else if (nonZeroAfter) {
            // Only add the first sample of the first non-zero-length segment after
            // this segment. Any potential join is already handled by a zero-length
            // segment before this one.
            //
            out.append(evalNonZero(*nonZeroAfter, 0));
        }
        else {
            // This is the end of an open curve: no join to compute, just use
            // the last sample of the first non-zero-length segment before this
            // segment.
            //
            OptionalInt nonZeroBefore =
                indexOfFirstNonZeroLengthSegmentBeforeKnot_(this, startKnot);

            if (nonZeroBefore) {
                out.append(evalNonZero(*nonZeroBefore, 1));
            }
            else {
                // Segment at segmentIndex is zero-length, and there is no non-zero-length
                // segment before or after it, so this means all segments are zero-length.
                //
                out.append(zeroLengthStrokeSample());
            }
        }
    }
    else {
        return sampleNonZeroSegment(out, segmentIndex, params, sampler);
    }
}

void AbstractStroke2d::sampleRange(
    StrokeSampleEx2dArray& out,
    const CurveSamplingParameters& params,
    Int startKnotIndex,
    Int numSegments,
    bool computeArcLengths) const {

    Int numKnots = this->numKnots();
    Int numSegmentsInStroke = this->numSegments();

    // Verify we have at least one knot, since a post-condition of this
    // function is to return at least one sample.
    if (numKnots == 0) {
        throw vgc::core::IndexError("Cannot sample a stroke with 0 knots.");
    }

    // Verify and wrap startKnotIndex
    if (startKnotIndex < -numKnots || startKnotIndex > numKnots - 1) {
        throw vgc::core::IndexError(vgc::core::format(
            "Parameter startKnot ({}) out of valid knot index range [{}, {}].",
            startKnotIndex,
            -numKnots,
            numKnots - 1));
    }
    if (startKnotIndex < 0) {
        startKnotIndex += numKnots; // -1 becomes numKnots - 1 (=> last knot)
    }

    // Verify and wrap numSegments
    if (numSegments < -numSegmentsInStroke - 1 || numSegments > numSegmentsInStroke) {
        throw vgc::core::IndexError(vgc::core::format(
            "Parameter numSegments ({}) out of valid number of segments range "
            "[{}, {}].",
            numSegments,
            -numSegmentsInStroke - 1,
            numSegmentsInStroke));
    }
    if (numSegments < 0) {
        numSegments += numSegmentsInStroke + 1; // -1 becomes n (=> all segments)
    }
    if (!isClosed() && numSegments > numSegmentsInStroke - startKnotIndex) {
        throw vgc::core::IndexError(core::format(
            "Parameter numSegments ({} after negative-wrap) exceeds remaining "
            "number of segments when starting at the given startKnotIndex ({} after "
            "negative-wrap): valid range is [0, {}] since the curve is open and has {} "
            "knots.",
            numSegments,
            startKnotIndex,
            numSegmentsInStroke - startKnotIndex,
            numKnots));
    }

    // Remember old length of `out`
    const Int oldLength = out.length();

    if (numSegments == 0) {
        out.append(sampleKnot_(startKnotIndex));
    }
    else {
        // Reserve memory space
        if (out.isEmpty()) {
            Int minSegmentSamples = params.minIntraSegmentSamples() + 1;
            out.reserve(1 + numSegments * minSegmentSamples);
        }

        // Iterate over all segments
        for (Int i = 0; i < numSegments; ++i) {
            Int segmentIndex = (startKnotIndex + i) % numSegmentsInStroke;
            if (i != 0) {
                // Remove last sample of previous segment (recomputed below)
                out.pop();
            }
            sampleSegment(out, segmentIndex, params);
        }
    }

    // Compute arc lengths.
    //
    if (computeArcLengths) {
        // Compute arc length of first new sample.
        double s = 0;
        auto it = out.begin() + oldLength;
        if (oldLength > 0) {
            StrokeSampleEx2d& firstNewSample = *it;
            StrokeSampleEx2d& lastOldSample = *(it - 1);
            s = lastOldSample.s()
                + (firstNewSample.position() - lastOldSample.position()).length();
        }
        it->setS(s);
        Vec2d lastPoint = it->position();
        // Compute arclength of all subsequent samples.
        for (++it; it != out.end(); ++it) {
            Vec2d point = it->position();
            s += (point - lastPoint).length();
            it->setS(s);
            lastPoint = point;
        }
    }
}

/// Expects positions in object space.
///
StrokeSampling2d
AbstractStroke2d::computeSampling(const geometry::CurveSamplingParameters& params) const {

    geometry::StrokeSampleEx2dArray samplesEx;
    StrokeBoundaryInfo boundaryInfo;

    Int n = numKnots();
    if (n == 0) {
        // fallback to segment
        Vec2d tangent(0, 1);
        Vec2d normal = tangent.orthogonalized();
        Vec2d halfwidths(1.0, 1.0);
        samplesEx.emplaceLast(Vec2d(), tangent, normal, halfwidths, 0.0);
        boundaryInfo[0] = StrokeEndInfo(Vec2d(), tangent, halfwidths);
        boundaryInfo[0].setOffsetLineTangents({tangent, tangent});
        boundaryInfo[1] = boundaryInfo[0];
    }
    else {
        sampleRange(samplesEx, params);
        boundaryInfo = computeBoundaryInfo();
    }

    VGC_ASSERT(samplesEx.length() > 0);
    StrokeSampling2d result{geometry::StrokeSample2dArray(samplesEx)};
    result.setBoundaryInfo(boundaryInfo);

    return result;
}

std::unique_ptr<AbstractStroke2d> AbstractStroke2d::convert_(const AbstractStroke2d* source) const {
    std::unique_ptr<AbstractStroke2d> result = clone_();
    if (result->convertAssign_(source)) {
        return result;
    }
    return nullptr;
}

StrokeSampleEx2d AbstractStroke2d::sampleKnot_(Int index) const {

    // Use the first non-zero-length segment after the knot if it exists.
    if (OptionalInt i = indexOfFirstNonZeroLengthSegmentAfterKnot_(this, index)) {
        return evalNonZero(*i, 0);
    }

    // Otherwise, use the first non-zero-length segment before the knot if it exists.
    if (OptionalInt i = indexOfFirstNonZeroLengthSegmentBeforeKnot_(this, index)) {
        return evalNonZero(*i, 1);
    }

    // Otherwise, this means that all segments are zero-length segments.
    return zeroLengthStrokeSample();
}

bool AbstractStroke2d::fixEvalLocation_(Int& segmentIndex, double& u) const {

    if (isZeroLengthSegment(segmentIndex)) {

        Int numSegments = this->numSegments();
        Int isClosed = this->isClosed();
        Int startKnot = segmentIndex;
        Int endKnot = startKnot + 1;
        if (isClosed && endKnot > numSegments) {
            endKnot -= numSegments;
        }

        // Determine whether a non-zero-length segment exists after this segment.
        OptionalInt nonZeroAfter =
            indexOfFirstNonZeroLengthSegmentAfterKnot_(this, endKnot);
        if (nonZeroAfter) {
            segmentIndex = *nonZeroAfter;
            u = 0;
            return true;
        }

        // Determine whether a non-zero-length segment exists before this segment.
        OptionalInt nonZeroBefore =
            indexOfFirstNonZeroLengthSegmentBeforeKnot_(this, startKnot);
        if (nonZeroBefore) {
            segmentIndex = *nonZeroBefore;
            u = 1;
            return true;
        }

        // Otherwise, it's a zero-length stroke.
        return false;
    }
    else {
        // It is a non-zero-length segment.
        return true;
    }
}

} // namespace vgc::geometry

/*
############################# Implementation notes #############################

[1]

In the future, we may want to extend the StrokeView2d class with:
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

*/
