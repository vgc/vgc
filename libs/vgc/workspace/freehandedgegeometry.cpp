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

#include <vgc/dom/strings.h>
#include <vgc/geometry/bezier.h>
#include <vgc/geometry/vec4d.h>
#include <vgc/workspace/freehandedgegeometry.h>
#include <vgc/workspace/logcategories.h>

namespace vgc::workspace {

namespace {

double cubicEaseInOut(double t) {
    double t2 = t * t;
    return -2 * t * t2 + 3 * t2;
}

using CurveType = geometry::Curve::Type;

constexpr CurveType openCurveType = CurveType::OpenCentripetalCatmullRom;
constexpr CurveType closedCurveType = CurveType::ClosedCentripetalCatmullRom;

} // namespace

void FreehandEdgeGeometry::setPoints(const SharedConstPoints& points) {
    if (isBeingEdited_) {
        points_ = points;
    }
    else {
        sharedConstPoints_ = points;
        originalArclengths_.clear();
    }
    dirtyEdgeSampling();
}

void FreehandEdgeGeometry::setPoints(geometry::Vec2dArray points) {
    if (isBeingEdited_) {
        points_ = std::move(points);
    }
    else {
        sharedConstPoints_ = SharedConstPoints(std::move(points));
        originalArclengths_.clear();
    }
    dirtyEdgeSampling();
}

void FreehandEdgeGeometry::setWidths(const SharedConstWidths& widths) {
    if (isBeingEdited_) {
        widths_ = widths;
    }
    else {
        sharedConstWidths_ = widths;
    }
    dirtyEdgeSampling();
}

void FreehandEdgeGeometry::setWidths(core::DoubleArray widths) {
    if (isBeingEdited_) {
        widths_ = std::move(widths);
        dirtyEdgeSampling();
    }
    else {
        sharedConstWidths_ = SharedConstWidths(std::move(widths));
    }
}

std::shared_ptr<vacomplex::KeyEdgeGeometry> FreehandEdgeGeometry::clone() const {
    auto ret = std::make_shared<FreehandEdgeGeometry>();
    ret->sharedConstPoints_ = sharedConstPoints_;
    ret->sharedConstWidths_ = sharedConstWidths_;
    return ret;
}

vacomplex::EdgeSampling FreehandEdgeGeometry::computeSampling(
    geometry::CurveSamplingQuality quality,
    const geometry::Vec2d& snapStartPosition,
    const geometry::Vec2d& snapEndPosition,
    vacomplex::EdgeSnapTransformationMode /*mode*/) const {

    geometry::Curve curve(openCurveType);
    geometry::CurveSampleArray samples;
    geometry::Vec2dArray tmpPoints;
    core::DoubleArray tmpWidths;

    const geometry::Vec2dArray& points = this->points();
    const core::DoubleArray& widths = this->widths();

    if (points.isEmpty()) {
        // fallback to segment
        tmpPoints = {snapStartPosition, snapEndPosition};
        tmpWidths = {1.0, 1.0};
        curve.setPositions(tmpPoints);
        curve.setWidths(tmpWidths);
    }
    else if (points.first() == snapStartPosition && points.last() == snapEndPosition) {
        curve.setPositions(points);
        curve.setWidths(widths);
    }
    else {
        core::DoubleArray tmpArclengths;
        computeSnappedLinearS_(
            tmpPoints, points, tmpArclengths, snapStartPosition, snapEndPosition);
        curve.setPositions(tmpPoints);
        curve.setWidths(widths);
    }

    if (isBeingEdited_) {
        quality = geometry::CurveSamplingQuality::AdaptiveLow;
    }

    curve.sampleRange(samples, geometry::CurveSamplingParameters(quality));
    VGC_ASSERT(samples.length() > 0);

    vacomplex::EdgeSampling res(std::move(samples));
    if (curve.numSegments() >= 1) {
        std::array<geometry::Vec2d, 2> tangents =
            curve.getOffsetLineTangentsAtSegmentEndpoint(0, 0);
        res.setOffsetLineTangentsAtEndpoint(0, tangents);
        tangents =
            curve.getOffsetLineTangentsAtSegmentEndpoint(curve.numSegments() - 1, 1);
        res.setOffsetLineTangentsAtEndpoint(1, tangents);
    }
    return res;
}

vacomplex::EdgeSampling FreehandEdgeGeometry::computeSampling(
    geometry::CurveSamplingQuality quality,
    bool isClosed,
    vacomplex::EdgeSnapTransformationMode /*mode*/) const {

    geometry::Curve curve(isClosed ? closedCurveType : openCurveType);

    geometry::CurveSampleArray samples;
    geometry::Vec2dArray tmpPoints;
    core::DoubleArray tmpWidths;

    const geometry::Vec2dArray& points = this->points();
    const core::DoubleArray& widths = this->widths();

    if (points.isEmpty()) {
        // fallback to segment
        tmpPoints = {geometry::Vec2d(), geometry::Vec2d()};
        tmpWidths = {1.0, 1.0};
        curve.setPositions(tmpPoints);
        curve.setWidths(tmpWidths);
    }
    else {
        curve.setPositions(points);
        curve.setWidths(widths);
    }

    if (isBeingEdited_) {
        quality = geometry::CurveSamplingQuality::AdaptiveLow;
    }

    curve.sampleRange(samples, geometry::CurveSamplingParameters(quality));
    VGC_ASSERT(samples.length() > 0);

    vacomplex::EdgeSampling res(std::move(samples));
    if (curve.numSegments() >= 1) {
        std::array<geometry::Vec2d, 2> tangents =
            curve.getOffsetLineTangentsAtSegmentEndpoint(0, 0);
        res.setOffsetLineTangentsAtEndpoint(0, tangents);
        tangents =
            curve.getOffsetLineTangentsAtSegmentEndpoint(curve.numSegments() - 1, 1);
        res.setOffsetLineTangentsAtEndpoint(1, tangents);
    }
    return res;
}

void FreehandEdgeGeometry::startEdit() {
    if (!isBeingEdited_) {
        points_ = sharedConstPoints_.get();
        widths_ = sharedConstWidths_.get();
        isBeingEdited_ = true;
    }
}

void FreehandEdgeGeometry::resetEdit() {
    if (isBeingEdited_) {
        points_ = sharedConstPoints_.get();
        widths_ = sharedConstWidths_.get();
        dirtyEdgeSampling();
    }
}

void FreehandEdgeGeometry::finishEdit() {
    if (isBeingEdited_) {
        sharedConstPoints_ = SharedConstPoints(std::move(points_));
        sharedConstWidths_ = SharedConstWidths(std::move(widths_));
        points_ = geometry::Vec2dArray();
        widths_ = core::DoubleArray();
        originalArclengths_.clear();
        originalArclengths_.shrinkToFit();
        dirtyEdgeSampling();
        isBeingEdited_ = false;
    }
}

void FreehandEdgeGeometry::abortEdit() {
    if (isBeingEdited_) {
        points_ = geometry::Vec2dArray();
        widths_ = core::DoubleArray();
        originalArclengths_.clear();
        originalArclengths_.shrinkToFit();
        dirtyEdgeSampling();
        isBeingEdited_ = false;
    }
}

void FreehandEdgeGeometry::translate(const geometry::Vec2d& delta) {
    const geometry::Vec2dArray& points = this->points();
    points_ = points;
    for (geometry::Vec2d& p : points_) {
        p += delta;
    }
    if (!isBeingEdited_) {
        sharedConstPoints_ = SharedConstPoints(std::move(points_));
        points_ = geometry::Vec2dArray();
    }
    originalArclengths_.clear();
    dirtyEdgeSampling();
}

void FreehandEdgeGeometry::snap(
    const geometry::Vec2d& snapStartPosition,
    const geometry::Vec2d& snapEndPosition,
    vacomplex::EdgeSnapTransformationMode /*mode*/) {

    const geometry::Vec2dArray& points = this->points();
    if (!points.isEmpty() && points.first() == snapStartPosition
        && points.last() == snapEndPosition) {
        // already snapped
        return;
    }

    if (isBeingEdited_) {
        computeSnappedLinearS_(
            points_, points, originalArclengths_, snapStartPosition, snapEndPosition);
    }
    else {
        computeSnappedLinearS_(
            points_, points, originalArclengths_, snapStartPosition, snapEndPosition);
        sharedConstPoints_ = SharedConstPoints(std::move(points_));
        points_ = geometry::Vec2dArray();
    }

    originalArclengths_.clear();
    dirtyEdgeSampling();
}

namespace {

struct SculptPoint {
    SculptPoint() noexcept = default;

    SculptPoint(const geometry::Vec2d& pos, double width, double d, double s) noexcept
        : pos(pos)
        , width(width)
        , d(d)
        , s(s) {
    }

    geometry::Vec2d pos = {};
    // halfwidths are not supported yet.
    double width = 0;
    // signed distance in arclength from central sculpt point.
    double d = 0;
    // position in arclength on the related edge.
    double s = 0;
};

struct SculptSampling {
    core::Array<SculptPoint> sculptPoints;
    // sampling boundaries in arclength from central sculpt point.
    geometry::Vec2d cappedRadii = {};
    double ds = 0;
    double radius = 0;
    // is sculpt interval closed ?
    bool isClosed = false;
    // is sculpt interval across the join ?
    bool isRadiusOverlappingStart = false;
    Int middleSculptPointIndex = -1;
};

/// Converts between arithmetic types that may cause narrowing.
///
/// This is the same as `static_cast` but clarifies intent and makes it easy to
/// search in the codebase.
///
// XXX Improve (SFINAE-restrict to arithmetic types?) and move to core/arithmetic.h
//
template<typename T, typename U>
T narrow_cast(U x) {
    return static_cast<T>(std::floor(x));
}

/// Rounds the given floating point `x` towards -infinity and narrow_cast it to
/// an integer.
///
// XXX Add tests, SFINAE checks, and move to core/arithmetic.h
//
template<typename T, typename U>
T floor_narrow_cast(U x) {
    return static_cast<T>(std::floor(x));
}

/*
/// Rounds the given floating point `x` towards infinity and narrow_cast it to
/// an integer.
///
// XXX Add tests, SFINAE checks, and move to core/arithmetic.h
//
template<typename T, typename U>
T ceil_narrow_cast(U x) {
    return static_cast<T>(std::ceil(x));
}
*/

// Computes a uniform sampling of the subset of the curve centered around the
// middle sculpt point `sMsp` and extending on both sides by `radius` in
// arclength (if possible, otherwise capped at the endpoints).
//
// Assumes:
// - radius > 0
// - sMsp is in [samples.first.s(), samples.last.s()].
//
void computeSculptSampling(
    SculptSampling& outSampling,
    geometry::CurveSampleArray& samples,
    double sMsp,
    double radius,
    double maxDs,
    bool isClosed) {

    core::Array<SculptPoint>& sculptPoints = outSampling.sculptPoints;

    Int numSamples = samples.length();
    VGC_ASSERT(numSamples > 0);

    // First, we determine how many sculpt points we want (and the
    // corresponding ds), based on the curve length, the location of the middle
    // sculpt point in the curve, the sculpt radius, and maxDS.

    Int numSculptPointsBeforeMsp = 0;
    Int numSculptPointsAfterMsp = 0;
    geometry::Vec2d cappedRadii = {};
    double ds = 0;
    double curveLength = samples.last().s();
    if (!isClosed) {
        // Compute ds such that it is no larger than maxDs, and such that radius is
        // a multiple of ds (if "uncapped", that is, if the radius doesn't
        // extend further than one of the endpoints of the curve).
        //
        double n = std::ceil(radius / maxDs);
        ds = radius / n;
        double sBeforeMsp = sMsp;
        if (radius <= sBeforeMsp) {
            // uncapped before
            numSculptPointsBeforeMsp = narrow_cast<Int>(n);
            cappedRadii[0] = radius;
        }
        else {
            // capped before
            numSculptPointsBeforeMsp = floor_narrow_cast<Int>(sBeforeMsp / ds);
            cappedRadii[0] = sBeforeMsp;
        }
        double sAfterMsp = curveLength - sMsp;
        if (radius <= sAfterMsp) {
            // uncapped after
            numSculptPointsAfterMsp = narrow_cast<Int>(n);
            cappedRadii[1] = radius;
        }
        else {
            // capped after
            numSculptPointsAfterMsp = floor_narrow_cast<Int>(sAfterMsp / ds);
            cappedRadii[1] = sAfterMsp;
        }
    }
    else { // isClosed
        double curveHalfLength = curveLength * 0.5;
        if (curveHalfLength <= radius) {
            // If the sculpt interval encompasses the full curve and the curve
            // is closed then we want to produce a closed sculpt sampling.
            //
            // In order to have the sculpt points all exactly spaced by `ds`
            // and looping around, we have to adjust ds and
            // numSculptSamplesPerSide such that curveLength is a multiple of
            // ds.
            //
            double n = std::ceil(curveHalfLength / maxDs);
            numSculptPointsBeforeMsp = narrow_cast<Int>(n);
            numSculptPointsAfterMsp = std::max<Int>(numSculptPointsBeforeMsp - 1, 0);
            ds = curveHalfLength / n;
            outSampling.isClosed = true;
            cappedRadii[0] = curveHalfLength;
            cappedRadii[1] = curveHalfLength;
        }
        else {
            // If the curve is closed then we do not cap the radii to the input interval.
            //
            double n = std::ceil(radius / maxDs);
            numSculptPointsBeforeMsp = narrow_cast<Int>(n);
            numSculptPointsAfterMsp = narrow_cast<Int>(n);
            ds = radius / n;
            cappedRadii[0] = radius;
            cappedRadii[1] = radius;
            // Find out if interval overlaps the start point.
            if (sMsp - radius < 0 || sMsp + radius > curveLength) {
                outSampling.isRadiusOverlappingStart = true;
            }
        }
    }
    Int targetNumSculptPoints = numSculptPointsBeforeMsp + numSculptPointsAfterMsp + 1;

    // Once we know ds and how many sculpt points we want, let's generate them
    // by resampling the samples linearly.

    if (curveLength == 0) {
        sculptPoints.emplaceLast(
            samples[0].position(), 2.0 * samples[0].halfwidth(0), 0, 0);
    }
    else {
        bool isDone = false;
        Int spEndIndex = numSculptPointsAfterMsp + 1;
        Int spIndex = -numSculptPointsBeforeMsp;

        double sculptPointSOffset = 0;
        if (isClosed && sMsp + spIndex * ds < 0) {
            sculptPointSOffset = curveLength;
        }
        double nextSculptPointS = sculptPointSOffset + sMsp + spIndex * ds;
        if (nextSculptPointS < 0) { // s of first sample is 0.
            // Fix potential floating point error that made it overshoot the start of the curve.
            nextSculptPointS = 0;
        }

        const Int maxIter = isClosed ? 2 : 1; // If the curve is closed we allow 2 passes.
        for (Int iter = 0; iter < maxIter; ++iter) {
            // Iterate over sample segments
            // Loop invariant: `nextSculptPointS >= sa1->s()` (as long as `sa2->s() >= sa1->s()`)
            //
            const geometry::CurveSample* sa1 = &samples[0];
            for (Int iSample2 = 1; iSample2 < numSamples && !isDone; ++iSample2) {
                const geometry::CurveSample* sa2 = &samples[iSample2];
                const double d = sa2->s() - sa1->s();
                // Skip the segment if it is degenerate.
                if (d > 0) {
                    double invD = 1.0 / d;
                    while (nextSculptPointS <= sa2->s()) {
                        // Sample a sculpt point at t in segment [sa1:0, sa2:1].
                        double t = (nextSculptPointS - sa1->s()) * invD;
                        double u = 1.0 - t;
                        geometry::Vec2d p = u * sa1->position() + t * sa2->position();
                        double w = (u * sa1->halfwidth(0) + t * sa2->halfwidth(0)) * 2.0;
                        sculptPoints.emplaceLast(p, w, spIndex * ds, nextSculptPointS);
                        ++spIndex;
                        nextSculptPointS = sculptPointSOffset + sMsp + spIndex * ds;
                        if (spIndex >= spEndIndex - 1) {
                            if (spIndex == spEndIndex) {
                                // All sculpt points have been sampled.
                                isDone = true;
                                break;
                            }
                            else { // spIndex == spEndIndex - 1
                                if (!isClosed || iter == 1) {
                                    if (nextSculptPointS > samples.last().s()) {
                                        // Fix potential floating point error that made it
                                        // overshoot the end of the curve.
                                        nextSculptPointS = samples.last().s();
                                    }
                                }
                            }
                        }
                    }
                }
                sa1 = sa2;
            }
            if (!isDone) {
                if (isClosed && iter == 0) {
                    // We loop only if the curve is closed
                    sculptPointSOffset -= curveLength;
                    nextSculptPointS -= curveLength;
                }
            }
            else {
                break;
            }
        }
    }

    VGC_ASSERT(targetNumSculptPoints > 0);
    if (sculptPoints.length() != targetNumSculptPoints) {

        // This may indicate either a bug in this function, or dubious
        // parameters passed to this function (e.g., sMsp not in
        // [samples.first.s(), samples.last.s()], or incorrect samples[i].s())
        //
        VGC_WARNING(
            LogVgcWorkspace,
            "Fewer sculpt points generated ({}) than targeted ({}).",
            sculptPoints.length(),
            targetNumSculptPoints);

        // We really want at least one sculpt point, so we add one if there is
        // none. However, it's not a critical issue not to have exactly
        // targetNumSculptPoints, so we don't try to recover from this.
        //
        if (sculptPoints.isEmpty()) {
            sculptPoints.emplaceLast(
                samples[0].position(), 2.0 * samples[0].halfwidth(0), 0, 0);
        }
    }

    outSampling.middleSculptPointIndex = numSculptPointsBeforeMsp;
    outSampling.cappedRadii = cappedRadii;
    outSampling.ds = ds;
    outSampling.radius = radius;
}

[[maybe_unused]] Int filterSculptPointsWidthStep(
    core::Array<SculptPoint>& points,
    core::IntArray& indices,
    Int intervalStart,
    double tolerance) {

    Int i = intervalStart;
    Int endIndex = indices[i + 1];
    while (indices[i] != endIndex) {

        Int iA = indices[i];
        Int iB = indices[i + 1];
        const SculptPoint& a = points[iA];
        const SculptPoint& b = points[iB];
        double ds = b.s - a.s;

        // Compute which sample between A and B has an offset point
        // furthest from the offset line AB.
        double maxOffsetDiff = tolerance;
        Int maxOffsetDiffPointIndex = -1;
        for (Int j = iA + 1; j < iB; ++j) {
            const SculptPoint& p = points[j];
            double t = (p.s - a.s) / ds;
            double w = (1 - t) * a.width + t * b.width;
            double dist = (std::abs)(w - p.width);
            if (dist > maxOffsetDiff) {
                maxOffsetDiff = dist;
                maxOffsetDiffPointIndex = j;
            }
        }

        // If the distance exceeds the tolerance, then recurse.
        // Otherwise, stop the recursion and move one to the next segment.
        if (maxOffsetDiffPointIndex != -1) {
            // Add sample to the list of selected samples
            indices.insert(i + 1, maxOffsetDiffPointIndex);
        }
        else {
            ++i;
        }
    }
    return i;
}

template<typename TPoint, typename PositionGetter, typename WidthGetter>
Int filterPointsStep(
    core::Array<TPoint>& points,
    core::IntArray& indices,
    Int intervalStart,
    double tolerance,
    PositionGetter positionGetter,
    WidthGetter /*widthGetter*/) {

    Int i = intervalStart;
    Int endIndex = indices[i + 1];
    while (indices[i] != endIndex) {

        // Get line AB.
        Int iA = indices[i];
        Int iB = indices[i + 1];
        geometry::Vec2d a = positionGetter(points[iA]);
        geometry::Vec2d b = positionGetter(points[iB]);
        geometry::Vec2d ab = b - a;
        double abLen = ab.length();

        // Compute which sample between A and B has a position
        // furthest from the line AB.
        double maxDist = tolerance;
        Int maxDistPointIndex = -1;
        if (abLen > 0) {
            for (Int j = iA + 1; j < iB; ++j) {
                geometry::Vec2d p = positionGetter(points[j]);
                geometry::Vec2d ap = p - a;
                double dist = (std::abs)(ab.det(ap) / abLen);
                if (dist > maxDist) {
                    maxDist = dist;
                    maxDistPointIndex = j;
                }
            }
        }
        else {
            for (Int j = iA + 1; j < iB; ++j) {
                geometry::Vec2d p = positionGetter(points[j]);
                geometry::Vec2d ap = p - a;
                double dist = ap.length();
                if (dist > maxDist) {
                    maxDist = dist;
                    maxDistPointIndex = j;
                }
            }
        }

        // If the furthest point is too far from AB, then recurse.
        // Otherwise, stop the recursion and move one to the next segment.
        if (maxDistPointIndex != -1) {
            // Add sample to the list of selected samples
            indices.insert(i + 1, maxDistPointIndex);
        }
        else {
            //
            // Note: The width filtering step is disabled for now since it introduces
            // unwanted noisy widening of the curve.
            // This is probably caused by our use of Catmull-Rom to interpolate
            // widths: they can overshoot, then get sampled as sculpt points and
            // kept as new control points that overshoot further on the next grab.
            //
            /*
            Int i0 = indices[i];
            Int i1 = indices[i + 1];
            geometry::Vec2d previousPos = points[i0].pos;
            double s = points[i0].cumulativeS;
            for (Int j = i0 + 1; j < i1; ++j) {
            SculptPoint& sp = points[j];
            s += (sp.pos - previousPos).length();
            sp.cumulativeS = s;
            previousPos = sp.pos;
            }
            i = filterSculptPointsWidthStep(points, indices, i, tolerance);
            */
            ++i;
        }
    }
    return i;
}

} // namespace

geometry::Vec2d FreehandEdgeGeometry::sculptGrab(
    const geometry::Vec2d& startPosition,
    const geometry::Vec2d& endPosition,
    double radius,
    double /*strength*/,
    double tolerance,
    bool isClosed) {

    // Let's consider tolerance will be ~= pixelSize for now.
    //
    // sampleStep is screen-space-dependent.
    //   -> doesn't look like a good parameter..

    VGC_ASSERT(isBeingEdited_);

    Int numPoints = points_.length();
    if (numPoints == 0) {
        return endPosition;
    }

    // Note: We sample with widths even though we only need widths for samples in radius.
    // We could benefit from a two step sampling (sample centerline points, then sample
    // cross sections on an sub-interval).
    geometry::CurveSampleArray samples;
    geometry::Curve curve(isClosed ? closedCurveType : openCurveType);
    curve.setPositions(points_);
    curve.setWidths(widths_);
    core::Array<double> pointsS(numPoints, core::noInit);
    samples.emplaceLast();
    for (Int i = 0; i < numPoints; ++i) {
        pointsS[i] = samples.last().s();
        samples.pop();
        curve.sampleRange(
            samples,
            geometry::CurveSamplingQuality::AdaptiveLow,
            i,
            Int{(!isClosed && i == numPoints - 1) ? 0 : 1},
            true);
    }

    // Note: we could have a distanceToCurve specialized for our geometry.
    // It could check each control polygon region first to skip sampling the ones
    // that are strictly farther than an other.
    geometry::DistanceToCurve d = geometry::distanceToCurve(samples, startPosition);
    if (d.distance() > radius) {
        return endPosition;
    }

    // Compute middle sculpt point info (closest point).
    Int mspSegmentIndex = d.segmentIndex();
    double mspSegmentParameter = d.segmentParameter();
    geometry::CurveSample mspSample = samples[mspSegmentIndex];
    if (mspSegmentParameter > 0 && mspSegmentIndex + 1 < samples.length()) {
        const geometry::CurveSample& s2 = samples[mspSegmentIndex + 1];
        mspSample = geometry::lerp(mspSample, s2, mspSegmentParameter);
    }
    double sMsp = mspSample.s();

    const double maxDs = (tolerance * 2.0);

    SculptSampling sculptSampling = {};
    computeSculptSampling(sculptSampling, samples, sMsp, radius, maxDs, isClosed);

    core::Array<SculptPoint>& sculptPoints = sculptSampling.sculptPoints;

    geometry::Vec2d delta = endPosition - startPosition;

    if (!isClosed) {
        geometry::Vec2d uMins =
            geometry::Vec2d(1.0, 1.0) - sculptSampling.cappedRadii / radius;
        geometry::Vec2d wMins(cubicEaseInOut(uMins[0]), cubicEaseInOut(uMins[1]));
        for (Int i = 0; i < sculptPoints.length(); ++i) {
            SculptPoint& sp = sculptPoints[i];
            double u = {};
            double wMin = {};
            if (sp.d < 0) {
                u = 1.0 - (-sp.d / radius);
                wMin = wMins[0];
            }
            else if (sp.d > 0) {
                u = 1.0 - (sp.d / radius);
                wMin = wMins[1];
            }
            else {
                // middle sculpt point
                u = 1.0;
                wMin = 0.0;
            }
            double w = cubicEaseInOut(u);
            double t = (w - wMin) / (1 - wMin);
            sp.pos += delta * t;
        }
    }
    else {
        // In this case capped radii are expected to be equal.
        double cappedRadius = sculptSampling.cappedRadii[0];
        double uMin = 1.0 - cappedRadius / radius;
        double wMin = cubicEaseInOut(uMin);
        for (Int i = 0; i < sculptPoints.length(); ++i) {
            SculptPoint& sp = sculptPoints[i];
            double u = {};
            if (sp.d < 0) {
                u = 1.0 - (-sp.d / cappedRadius);
            }
            else if (sp.d > 0) {
                u = 1.0 - (sp.d / cappedRadius);
            }
            else {
                // middle sculpt point
                u = 1.0;
            }
            double w = cubicEaseInOut(u);
            w *= (1.0 - wMin);
            w += wMin;
            sp.pos += delta * w;
        }
    }

    bool hasWidths = !widths_.isEmpty();

    core::IntArray indices = {};

    constexpr bool isFilteringEnabled = true;
    if (isFilteringEnabled) {
        if (!isClosed) {
            // When the sampling is capped at an edge endpoint we want to be able
            // to remove the uniformely sampled sculpt point next to the endpoint
            // since it is closer than ds.
            if (sculptSampling.cappedRadii[0] < radius) {
                double width = hasWidths ? widths_.first() : samples[0].halfwidth(0) * 2;
                sculptPoints.emplaceFirst(
                    points_.first(),
                    width,
                    -sculptSampling.cappedRadii[0],
                    pointsS.first());
            }
            if (sculptSampling.cappedRadii[1] < radius) {
                double width = hasWidths ? widths_.last() : samples[0].halfwidth(0) * 2;
                sculptPoints.emplaceLast(
                    points_.last(), width, sculptSampling.cappedRadii[1], pointsS.last());
            }
        }
        indices.extend({0, sculptPoints.length() - 1});
        filterPointsStep(
            sculptPoints,
            indices,
            0,
            tolerance * 0.5,
            [](const SculptPoint& p) { return p.pos; },
            [](const SculptPoint& p) { return p.width; });
    }
    else {
        indices.reserve(sculptPoints.length());
        for (Int i = 0; i < sculptPoints.length(); ++i) {
            indices.append(i);
        }
    }

    double s0 = sculptSampling.sculptPoints.first().s;
    double sN = sculptSampling.sculptPoints.last().s;
    Int numPatchPoints = indices.length();

    // Insert sculpt points in input points
    if (sculptSampling.isClosed) {
        points_.resize(numPatchPoints);
        for (Int i = 0; i < numPatchPoints; ++i) {
            const SculptPoint& sp = sculptPoints[indices[i]];
            points_[i] = sp.pos;
        }
        if (hasWidths) {
            widths_.resize(numPatchPoints);
            for (Int i = 0; i < numPatchPoints; ++i) {
                const SculptPoint& sp = sculptPoints[indices[i]];
                widths_[i] = sp.width;
            }
        }
    }
    else if (sculptSampling.isRadiusOverlappingStart && sN <= s0) {
        // original points to keep are in the middle of the original array
        //
        //  original points:  x----x--x----x-----x----x
        //  sculpt points:      x x x n)        (0 x x
        //  keepIndex:                     x            (first > sn)
        //  keepCount:                     1            (count until next >= sn)
        //
        Int keepIndex = 0;
        for (; keepIndex < numPoints; ++keepIndex) {
            if (pointsS[keepIndex] > sN) {
                break;
            }
        }
        Int keepEndIndex = keepIndex;
        for (; keepEndIndex < numPoints; ++keepEndIndex) {
            if (pointsS[keepEndIndex] >= s0) {
                break;
            }
        }
        Int keepCount = keepEndIndex - keepIndex;

        points_.erase(points_.begin(), points_.begin() + keepIndex);
        points_.resize(keepCount + numPatchPoints);
        for (Int i = 0; i < numPatchPoints; ++i) {
            const SculptPoint& sp = sculptPoints[indices[i]];
            points_[keepCount + i] = sp.pos;
        }
        if (hasWidths) {
            widths_.erase(widths_.begin(), widths_.begin() + keepIndex);
            widths_.resize(keepCount + numPatchPoints);
            for (Int i = 0; i < numPatchPoints; ++i) {
                const SculptPoint& sp = sculptPoints[indices[i]];
                widths_[keepCount + i] = sp.width;
            }
        }
    }
    else {
        VGC_ASSERT(s0 <= sN);
        // original points to keep are at the beginning and end of the original array
        //
        //  original points:  x----x--x----x-----x----x
        //  sculpt points:        (0 x x x n)
        //  insertIndex:           x                    (first >= sn)
        //  insertEndIndex:                      x      (next > sn)
        //
        Int insertIndex = 0;
        for (; insertIndex < numPoints; ++insertIndex) {
            if (pointsS[insertIndex] >= s0) {
                break;
            }
        }
        Int insertEndIndex = insertIndex;
        for (; insertEndIndex < numPoints; ++insertEndIndex) {
            if (pointsS[insertEndIndex] > sN) {
                break;
            }
        }

        points_.erase(points_.begin() + insertIndex, points_.begin() + insertEndIndex);
        points_.insert(insertIndex, numPatchPoints, {});
        for (Int i = 0; i < numPatchPoints; ++i) {
            const SculptPoint& sp = sculptPoints[indices[i]];
            points_[insertIndex + i] = sp.pos;
        }
        if (hasWidths) {
            widths_.erase(
                widths_.begin() + insertIndex, widths_.begin() + insertEndIndex);
            widths_.insert(insertIndex, numPatchPoints, {});
            for (Int i = 0; i < numPatchPoints; ++i) {
                const SculptPoint& sp = sculptPoints[indices[i]];
                widths_[insertIndex + i] = sp.width;
            }
        }
    }

    dirtyEdgeSampling();

    return sculptPoints[sculptSampling.middleSculptPointIndex].pos;

    // Depending on the sculpt kernel we may have to duplicate the points
    // at the sculpt boundary to "extrude" properly.

    // problem: cannot reuse distanceToCurve.. samples don't have their segment index :(

    // In arclength mode, step is not supported so we have to do this only once.
    // In spatial mode, step is supported and we may have to do this at every step.
}

namespace {

class SculptSmoothAlgorithm {
public:
    bool execute(
        geometry::Vec2dArray& outPoints,
        core::DoubleArray& outWidths,
        geometry::Vec2d& outSculptCursorPosition,
        const geometry::Vec2d& position,
        double strength,
        double radius,
        const geometry::Vec2dArray& points,
        const core::DoubleArray& widths,
        bool isClosed,
        geometry::CurveSamplingQuality samplingQuality,
        double maxDs,
        double simplifyTolerance) {

        points_ = &points;
        widths_ = &widths;
        isClosed_ = isClosed;
        hasWidths_ = widths_->length() == points_->length();

        outSculptCursorPosition = position;

        if (!initCurveSampling_(samplingQuality)) {
            return false;
        }

        if (!initSculptSampling_(position, radius, maxDs)) {
            return false;
        }

        if (totalS_ < maxDs * 0.5) {
            outSculptCursorPosition =
                sculptSampling_.sculptPoints[sculptSampling_.middleSculptPointIndex].pos;
            return false;
        }

        sculptedKnotsInterval_ = computeSculptedKnotsInterval_();
        if (sculptedKnotsInterval_.count == 0) {
            outSculptCursorPosition =
                sculptSampling_.sculptPoints[sculptSampling_.middleSculptPointIndex].pos;
            return false;
        }

        newStartPointIndex_ = 0;
        appendUnmodifiedPoints1_();
        Int simplifyFirstIndex = std::max<Int>(0, newPoints_.length() - 1);
        appendTransformedPoints_(strength);
        Int simplifyLastIndex = newPoints_.length();
        appendUnmodifiedPoints2_();
        simplifyLastIndex = std::min<Int>(simplifyLastIndex, newPoints_.length() - 1);

        if (newStartPointIndex_ == newPoints_.length()) {
            newStartPointIndex_ = 0;
        }

        core::IntArray indices;
        indices.extend({simplifyFirstIndex, simplifyLastIndex});
        filterPointsStep(
            newPoints_,
            indices,
            0,
            simplifyTolerance,
            [](const geometry::Vec2d& p) { return p; },
            [](const geometry::Vec2d& p) { return 1.0; });

        outPoints.clear();
        outWidths.clear();
        Int n = simplifyFirstIndex + indices.length()
                + (newPoints_.length() - (simplifyLastIndex + 1));
        outPoints.reserve(n);
        if (hasWidths_) {
            outWidths.reserve(n);
        }

        if (newStartPointIndex_ > 0) {
            for (Int i : indices) {
                if (i >= newStartPointIndex_) {
                    outPoints.append(newPoints_[i]);
                    if (hasWidths_) {
                        outWidths.append(newWidths_[i]);
                    }
                }
            }
            outPoints.extend(
                newPoints_.begin() + simplifyLastIndex + 1, newPoints_.end());
            if (hasWidths_) {
                outWidths.extend(
                    newWidths_.begin() + simplifyLastIndex + 1, newWidths_.end());
            }
            outPoints.extend(newPoints_.begin(), newPoints_.begin() + simplifyFirstIndex);
            if (hasWidths_) {
                outWidths.extend(
                    newWidths_.begin(), newWidths_.begin() + simplifyFirstIndex);
            }
            for (Int i : indices) {
                if (i < newStartPointIndex_) {
                    outPoints.append(newPoints_[i]);
                    if (hasWidths_) {
                        outWidths.append(newWidths_[i]);
                    }
                }
            }
        }
        else {
            outPoints.extend(newPoints_.begin(), newPoints_.begin() + simplifyFirstIndex);
            if (hasWidths_) {
                outWidths.extend(
                    newWidths_.begin(), newWidths_.begin() + simplifyFirstIndex);
            }
            for (Int i : indices) {
                outPoints.append(newPoints_[i]);
                if (hasWidths_) {
                    outWidths.append(newWidths_[i]);
                }
            }
            outPoints.extend(
                newPoints_.begin() + simplifyLastIndex + 1, newPoints_.end());
            if (hasWidths_) {
                outWidths.extend(
                    newWidths_.begin() + simplifyLastIndex + 1, newWidths_.end());
            }
        }

        outSculptCursorPosition = outSculptCursorPosition_;

        return true;
    }

private:
    bool initCurveSampling_(geometry::CurveSamplingQuality quality) {
        const Int numPoints = points_->length();
        if (numPoints < 2) {
            return false;
        }
        geometry::Curve curve(
            isClosed_ ? geometry::Curve::Type::ClosedUniformCatmullRom
                      : geometry::Curve::Type::OpenUniformCatmullRom);
        curve.setPositions(*points_);
        curve.setWidths(*widths_);
        pointsS_.resizeNoInit(numPoints);
        pointsS_[0] = 0;
        samples_.clear();
        samples_.reserve(numPoints);
        for (Int i = 0; i < numPoints - 1; ++i) {
            curve.sampleRange(samples_, quality, i, 1, true);
            pointsS_[i + 1] = samples_.last().s();
            samples_.pop();
        }
        curve.sampleRange(samples_, quality, numPoints - 1, Int{isClosed_ ? 1 : 0}, true);
        totalS_ = samples_.last().s();
        return true;
    }

    bool
    initSculptSampling_(const geometry::Vec2d& position, double radius, double maxDs) {
        // Note: we could have a distanceToCurve specialized for our geometry.
        // It could check each control polygon region first to skip sampling the ones
        // that are strictly farther than an other.
        geometry::DistanceToCurve d = geometry::distanceToCurve(samples_, position);
        if (d.distance() > radius) {
            return false;
        }

        // Compute middle sculpt point info (closest point).
        Int mspSegmentIndex = d.segmentIndex();
        double mspSegmentParameter = d.segmentParameter();
        mspSample_ = samples_[mspSegmentIndex];
        if (mspSegmentParameter > 0 && mspSegmentIndex + 1 < samples_.length()) {
            const geometry::CurveSample& s2 = samples_[mspSegmentIndex + 1];
            mspSample_ = geometry::lerp(mspSample_, s2, mspSegmentParameter);
        }

        computeSculptSampling(
            sculptSampling_, samples_, mspSample_.s(), radius, maxDs, isClosed_);

        core::Array<SculptPoint>& sculptPoints = sculptSampling_.sculptPoints;
        s0_ = sculptPoints.first().s;
        sN_ = sculptPoints.last().s;

        if (sculptSampling_.isClosed) {
            sculptPoints.emplaceLast(sculptPoints.first());
        }

        return true;
    }

    struct KnotsInterval {
        Int start = 0;
        Int count = 0;
    };

    KnotsInterval computeSculptedKnotsInterval_() {

        // THIS FUNCTION IS BUGGY ATM

        KnotsInterval res = {};
        const Int numPoints = pointsS_.length();

        // Search index of first point at or just after s0.
        Int i0 = 0;
        for (; i0 < numPoints; ++i0) {
            if (s0_ <= pointsS_[i0]) {
                break;
            }
        }
        if (i0 == numPoints) {
            i0 = isClosed_ ? 0 : numPoints - 1;
        }
        res.start = i0;

        // Count knots in interval
        if (sculptSampling_.isClosed) {
            res.count = numPoints;
            VGC_DEBUG_TMP("start:{} count:{} (isClosed)", res.start, res.count);
        }
        else {
            // Search index of first point just after sN.
            Int iN = 0;
            for (; iN < numPoints; ++iN) {
                if (sN_ < pointsS_[iN]) {
                    break;
                }
            }
            if (iN == numPoints) {
                iN = (isClosed_ && sN_ >= totalS_) ? 1 : 0;
            }
            // Build interval.
            if (sculptSampling_.isRadiusOverlappingStart && iN <= i0) {
                res.count = numPoints - i0 + iN;
                if (res.count > numPoints) {
                    // If interval starts and ends exactly on a knot/
                    // Note: this should happen only if interval is closed
                    // but better be safe against floating point errors.
                    res.count = numPoints;
                }
            }
            else {
                if (isClosed_ && iN == 0 && i0 > 0) {
                    res.count = numPoints - i0;
                }
                else {
                    // always `i0 <= iN` ?
                    res.count = iN - i0;
                }
            }
            VGC_DEBUG_TMP(
                "start:{} count:{}/{} iN:{}", res.start, res.count, numPoints, iN);
        }

        return res;
    }

    void appendUnmodifiedPoints1_() {
        const Int numPoints = pointsS_.length();
        const bool isOverlappingStart =
            sculptedKnotsInterval_.start + sculptedKnotsInterval_.count > numPoints;
        if (!isOverlappingStart) {
            // Append first points that will not change.
            Int n = sculptedKnotsInterval_.start;
            if (!isClosed_ && n == 0) {
                n = 1;
            }
            newPoints_.extend(points_->begin(), points_->begin() + n);
            if (hasWidths_) {
                newWidths_.extend(widths_->begin(), widths_->begin() + n);
            }
        }
        else {
            // Append all points that will not change.
            Int start = sculptedKnotsInterval_.start + sculptedKnotsInterval_.count;
            Int n = numPoints - sculptedKnotsInterval_.count;
            for (Int i = 0; i < n; ++i) {
                Int j = start + i;
                j = (numPoints + (j % numPoints)) % numPoints;
                newPoints_.emplaceLast((*points_)[j]);
                if (hasWidths_) {
                    newWidths_.emplaceLast((*widths_)[j]);
                }
            }
        }
    }

    void appendUnmodifiedPoints2_() {
        const Int numPoints = points_->length();
        const bool isOverlappingStart =
            sculptedKnotsInterval_.start + sculptedKnotsInterval_.count > numPoints;
        if (!isOverlappingStart) {
            // Append end points that will not change.
            Int n = sculptedKnotsInterval_.start + sculptedKnotsInterval_.count;
            if (!isClosed_ && n == numPoints) {
                n = numPoints - 1;
            }
            newPoints_.extend(points_->begin() + n, points_->end());
            if (hasWidths_) {
                newWidths_.extend(widths_->begin() + n, widths_->end());
            }
        }
    }

    void appendTransformedPoints_(double strength) {
        if (sculptedKnotsInterval_.count == 0) {
            return;
        }

        core::Array<SculptPoint>& sculptPoints = sculptSampling_.sculptPoints;

        // For each pair of consecutive sculpt points, average the original points in between
        // and move the result based on the sculpt point transformation.

        initWeightedAveragingData_();
        auto computeWeightedAverageSculptPointFn =
            sculptSampling_.isClosed
                ? &SculptSmoothAlgorithm::computeWeightedAverageSculptPoint_<true>
                : &SculptSmoothAlgorithm::computeWeightedAverageSculptPoint_<false>;

        SculptPoint wasp1 = {};
        Int iWasp1 = -1;
        Int pointIndex = sculptedKnotsInterval_.start;
        if (pointIndex == 0) {
            newStartPointIndex_ = newPoints_.length();
        }

        double lastPointS = pointsS_.last();
        for (Int i = 1; i < sculptPoints.length(); ++i) {
            const SculptPoint& sp1 = sculptPoints[i - 1];
            const SculptPoint& sp2 = sculptPoints[i];

            // PB: s1 > pointsS.last().s

            double s1 = sp1.s;
            double s2 = sp2.s;

            bool isWrapping = s2 < s1;

            if (isWrapping) {
                s2 += totalS_;
            }

            double meanS = 0;
            Int n = 0;
            accumulatePointsS_(pointIndex, s1, s2, meanS, n);

            if (isWrapping) {
                pointIndex = 0;
                if (pointIndex == 0) {
                    newStartPointIndex_ = newPoints_.length();
                }
                if (n > 0) {
                    meanS -= n * totalS_;
                }
                s1 -= totalS_;
                s2 = sculptPoints[i].s;
                accumulatePointsS_(pointIndex, s1, s2, meanS, n);
            }

            if (n == 0) {
                continue;
            }

            VGC_DEBUG_TMP("s1:{} s2:{}", s1, s2);

            // finalize mean
            meanS /= static_cast<double>(n);

            // transform
            if (iWasp1 != i - 1) {
                wasp1 = (this->*computeWeightedAverageSculptPointFn)(i - 1);
            }
            SculptPoint wasp2 = (this->*computeWeightedAverageSculptPointFn)(i);

            double t = (meanS - s1) / (s2 - s1);
            double u = 1.0 - t;

            geometry::Vec2d dp = u * (wasp1.pos - sp1.pos) + t * (wasp2.pos - sp2.pos);
            geometry::Vec2d p = u * sp1.pos + t * sp2.pos;
            geometry::Vec2d np = p + strength * dp;
            newPoints_.append(np);
            if (std::isnan(np[0]) || std::isnan(np[1])) {
                VGC_WARNING(LogVgcWorkspace, "generated nan point");
            }
            if (hasWidths_) {
                double dw = u * (wasp1.width - sp1.width) + t * (wasp2.width - sp2.width);
                double w = u * sp1.width + t * sp2.width;
                newWidths_.append(w + strength * dw);
            }
        }

        geometry::Vec2d scp =
            sculptSampling_.sculptPoints[sculptSampling_.middleSculptPointIndex].pos;
        geometry::Vec2d wascp = (this->*computeWeightedAverageSculptPointFn)(
                                    sculptSampling_.middleSculptPointIndex)
                                    .pos;
        outSculptCursorPosition_ = scp + (wascp - scp) * strength;
    }

    void accumulatePointsS_(Int& i, double s1, double s2, double& meanS, Int& n) {
        if (i < points_->length() && pointsS_[i] >= s1) {
            while (pointsS_[i] <= s2) {
                VGC_DEBUG_TMP("acc point {}", i);
                meanS += pointsS_[i];
                ++n;
                ++i;
                if (i >= points_->length()) {
                    break;
                }
            }
        }
    }

    void initWeightedAveragingData_() {
        const core::Array<SculptPoint>& sculptPoints = sculptSampling_.sculptPoints;
        const Int numSculptPoints = sculptPoints.length();
        numInfluencingPointsPerSide_ =
            static_cast<Int>(std::floor(sculptSampling_.radius / sculptSampling_.ds));
        repeatN_ = (numSculptPoints - 1) * 2;
        repeatDelta_ = (sculptPoints.last().pos - sculptPoints.first().pos) * 2;
    }

    template<bool isSculptSamplingClosed>
    SculptPoint computeWeightedAverageSculptPoint_(Int i) {
        const core::Array<SculptPoint>& sculptPoints = sculptSampling_.sculptPoints;
        SculptPoint res = sculptPoints[i];
        double wSum = cubicEaseInOut(1.0);
        res.pos *= wSum;
        res.width *= wSum;
        for (Int j = 1; j < numInfluencingPointsPerSide_; ++j) {
            double u = 1.0 - static_cast<double>(j) / numInfluencingPointsPerSide_;
            double w = cubicEaseInOut(u);
            SculptPoint sp1 = getInfluencePoint_<isSculptSamplingClosed>(i - j);
            SculptPoint sp2 = getInfluencePoint_<isSculptSamplingClosed>(i + j);
            res.pos += w * sp1.pos;
            res.pos += w * sp2.pos;
            res.width += w * sp1.width;
            res.width += w * sp2.width;
            wSum += 2 * w;
        }
        res.pos /= wSum;
        res.width /= wSum;
        return res;
    };

    template<bool isSculptSamplingClosed>
    SculptPoint getInfluencePoint_(Int i) {
        core::Array<SculptPoint>& sculptPoints = sculptSampling_.sculptPoints;
        const Int n = sculptPoints.length();
        // Note: last sculpt point == first sculpt point
        if constexpr (!isSculptSamplingClosed) {
            SculptPoint res = {};
            auto d = std::div(i, repeatN_);
            if (d.rem < 0) {
                d.rem += repeatN_;
                d.quot -= 1;
            }
            geometry::Vec2d p = {};
            double w = 0;
            Int k = d.rem;
            if (d.rem >= n) {
                k = repeatN_ - k;
                const SculptPoint& sp = sculptPoints[k];
                p = repeatDelta_ - sp.pos + 2 * sculptPoints[0].pos;
                w = sp.width;
            }
            else {
                const SculptPoint& sp = sculptPoints[k];
                p = sp.pos;
                w = sp.width;
            }
            p += repeatDelta_ * static_cast<double>(d.quot);
            res.pos = p;
            res.width = w;
            //VGC_DEBUG_TMP("i:{}->k:{} /{}", i, k, n);
            return res;
        }
        else {
            const Int m = n - 1;
            Int k = (m + (i % m)) % m;
            return sculptPoints[k];
        }
    };

    const geometry::Vec2dArray* points_ = nullptr;
    const core::DoubleArray* widths_ = nullptr;
    bool isClosed_ = false;
    bool hasWidths_ = false;
    geometry::CurveSampleArray samples_;
    core::Array<double> pointsS_;
    double totalS_ = 0;
    geometry::CurveSample mspSample_;
    SculptSampling sculptSampling_;
    double s0_ = 0;
    double sN_ = 0;
    KnotsInterval sculptedKnotsInterval_ = {};
    geometry::Vec2dArray newPoints_;
    core::DoubleArray newWidths_;
    geometry::Vec2d outSculptCursorPosition_;
    Int newStartPointIndex_ = 0;
    // weighted averaging
    Int numInfluencingPointsPerSide_ = 0;
    Int repeatN_ = 0;
    geometry::Vec2d repeatDelta_;
};

} // namespace

geometry::Vec2d FreehandEdgeGeometry::sculptSmooth(
    const geometry::Vec2d& position,
    double radius,
    double strength,
    double tolerance,
    bool isClosed) {

    // Let's consider tolerance will be ~= pixelSize for now.

    VGC_ASSERT(isBeingEdited_);

    const double maxDs = (std::max)(radius / 100, tolerance * 2.0);

    geometry::Vec2dArray newPoints;
    core::DoubleArray newWidths;
    geometry::Vec2d sculptCursorPosition = position;

    SculptSmoothAlgorithm alg;
    bool success = alg.execute(
        newPoints,
        newWidths,
        sculptCursorPosition,
        position,
        strength,
        radius,
        points_,
        widths_,
        isClosed,
        geometry::CurveSamplingQuality::AdaptiveLow,
        maxDs,
        tolerance * 0.5);

    if (success) {
        bool hasWidths = widths_.length() == points_.length();
        points_.swap(newPoints);
        if (hasWidths) {
            widths_.swap(newWidths);
        }
        dirtyEdgeSampling();
    }

    return sculptCursorPosition;
}

bool FreehandEdgeGeometry::updateFromDomEdge_(dom::Element* element) {
    namespace ds = dom::strings;

    bool changed = false;

    const auto& domPoints = element->getAttribute(ds::positions).getVec2dArray();
    if (sharedConstPoints_ != domPoints) {
        sharedConstPoints_ = domPoints;
        originalArclengths_.clear();
        dirtyEdgeSampling();
        changed = true;
    }

    const auto& domWidths = element->getAttribute(ds::widths).getDoubleArray();
    if (sharedConstWidths_ != domWidths) {
        sharedConstWidths_ = domWidths;
        dirtyEdgeSampling();
        changed = true;
    }

    return changed;
}

void FreehandEdgeGeometry::writeToDomEdge_(dom::Element* element) const {
    namespace ds = dom::strings;

    const auto& domPoints = element->getAttribute(ds::positions).getVec2dArray();
    if (sharedConstPoints_ != domPoints) {
        element->setAttribute(ds::positions, sharedConstPoints_);
    }

    const auto& domWidths = element->getAttribute(ds::widths).getDoubleArray();
    if (sharedConstWidths_ != domWidths) {
        element->setAttribute(ds::widths, sharedConstWidths_);
    }
}

void FreehandEdgeGeometry::removeFromDomEdge_(dom::Element* element) const {
    namespace ds = dom::strings;
    element->clearAttribute(ds::positions);
    element->clearAttribute(ds::widths);
}

void FreehandEdgeGeometry::computeSnappedLinearS_(
    geometry::Vec2dArray& outPoints,
    const geometry::Vec2dArray& srcPoints,
    core::DoubleArray& srcArclengths,
    const geometry::Vec2d& snapStartPosition,
    const geometry::Vec2d& snapEndPosition) {

    outPoints.resize(srcPoints.length());
    Int numPoints = outPoints.length();

    geometry::Vec2d a = snapStartPosition;
    geometry::Vec2d b = snapEndPosition;

    if (numPoints == 1) {
        // We would have to deal with "widths" if we want
        // to change the number of points.
        outPoints[0] = (a + b) * 0.5;
    }
    else if (numPoints == 2) {
        // We would have to deal with "widths" if we want
        // to change the number of points.
        outPoints[0] = a;
        outPoints[1] = b;
    }
    else {
        geometry::Vec2d d1 = a - srcPoints.first();
        geometry::Vec2d d2 = b - srcPoints.last();

        if (d1 == d2) {
            for (Int i = 0; i < numPoints; ++i) {
                outPoints[i] = srcPoints[i] + d1;
            }
        }
        else {
            if (srcArclengths.isEmpty()) {
                computeArclengths_(srcArclengths, srcPoints);
            }
            double curveLength = srcArclengths.last();
            if (curveLength > 0) {
                // linear deformation in rough "s"
                for (Int i = 0; i < numPoints; ++i) {
                    double t = srcArclengths[i] / curveLength;
                    outPoints[i] = srcPoints[i] + (d1 + t * (d2 - d1));
                }
            }
            else {
                for (Int i = 0; i < numPoints; ++i) {
                    outPoints[i] = srcPoints[i] + d1;
                }
            }
        }
    }
}

void FreehandEdgeGeometry::computeArclengths_(
    core::DoubleArray& outArclengths,
    const geometry::Vec2dArray& srcPoints) {

    Int numPoints = srcPoints.length();
    outArclengths.resize(numPoints);
    if (numPoints == 0) {
        return;
    }

    outArclengths[0] = 0;
    geometry::Curve curve(0.0);
    geometry::CurveSampleArray sampling;
    geometry::CurveSamplingParameters sParams(
        geometry::CurveSamplingQuality::AdaptiveLow);
    curve.setPositions(srcPoints);
    double s = 0;
    for (Int i = 1; i < numPoints; ++i) {
        curve.sampleRange(sampling, sParams, i - 1, 1, true);
        s += sampling.last().s();
        outArclengths[i] = s;
        sampling.clear();
    }
}

} // namespace vgc::workspace
