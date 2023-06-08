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

    geometry::Curve curve;
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
    return res;
}

vacomplex::EdgeSampling FreehandEdgeGeometry::computeSampling(
    geometry::CurveSamplingQuality quality,
    bool isClosed,
    vacomplex::EdgeSnapTransformationMode /*mode*/) const {

    geometry::Curve curve(
        isClosed ? geometry::Curve::Type::ClosedUniformCatmullRom
                 : geometry::Curve::Type::OpenUniformCatmullRom);
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
    SculptPoint(const geometry::Vec2d& pos, double width, double u)
        : pos(pos)
        , width(width)
        , u(u)
        , wMin(0) {
    }

    geometry::Vec2d pos;
    double width = 0;
    double u = 0;
    double wMin = 0;
    double cumulativeS = 0;
};

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
        double ds = b.cumulativeS - a.cumulativeS;

        // Compute which sample between A and B has an offset point
        // furthest from the offset line AB.
        double maxOffsetDiff = tolerance;
        Int maxOffsetDiffPointIndex = -1;
        for (Int j = iA + 1; j < iB; ++j) {
            const SculptPoint& p = points[j];
            double t = (p.cumulativeS - a.cumulativeS) / ds;
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

Int filterSculptPointsStep(
    core::Array<SculptPoint>& points,
    core::IntArray& indices,
    Int intervalStart,
    double tolerance) {

    Int i = intervalStart;
    Int endIndex = indices[i + 1];
    while (indices[i] != endIndex) {

        // Get line AB. Fast discard if AB too small.
        Int iA = indices[i];
        Int iB = indices[i + 1];
        geometry::Vec2d a = points[iA].pos;
        geometry::Vec2d b = points[iB].pos;
        geometry::Vec2d ab = b - a;
        double abLen = ab.length();
        if (abLen < core::epsilon) {
            ++i;
            continue;
        }

        // Compute which sample between A and B has a position
        // furthest from the line AB.
        double maxDist = tolerance;
        Int maxDistPointIndex = -1;
        for (Int j = iA + 1; j < iB; ++j) {
            geometry::Vec2d p = points[j].pos;
            geometry::Vec2d ap = p - a;
            double dist = (std::abs)(ab.det(ap) / abLen);
            if (dist > maxDist) {
                maxDist = dist;
                maxDistPointIndex = j;
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
    bool /*isClosed*/) {

    // Let's consider tolerance will be ~= pixelSize for now.
    //
    // sampleStep is screen-space-dependent.
    //   -> doesn't look like a good parameter..

    VGC_ASSERT(isBeingEdited_);
    if (points_.isEmpty()) {
        return endPosition;
    }

    geometry::Vec2d delta = endPosition - startPosition;

    geometry::Curve curve;
    geometry::CurveSampleArray samples;
    core::Array<Int> pointSampleIndices;

    curve.setPositions(points_);
    curve.setWidths(widths_);
    Int numPoints = points_.length();

    // Ideally we would like a version of Curve::sampleRange that only samples the centerline.
    // and a geometry::distanceToCurve() that accepts the results as argument.

    pointSampleIndices.resize(numPoints);
    curve.sampleRange(samples, geometry::CurveSamplingQuality::AdaptiveLow, 0, 1, false);
    for (Int i = 1; i < numPoints - 1; ++i) {
        samples.pop();
        pointSampleIndices[i] = samples.length();
        curve.sampleRange(
            samples, geometry::CurveSamplingQuality::AdaptiveLow, i, 1, false);
    }
    pointSampleIndices.last() = samples.length() - 1;
    Int numSamples = samples.length();
    VGC_ASSERT(numSamples > 0);

    geometry::DistanceToCurve d = geometry::distanceToCurve(samples, startPosition);
    if (d.distance() > radius) {
        return endPosition;
    }

    Int segmentIndex = d.segmentIndex();
    double segmentParameter = d.segmentParameter();

    core::Array<SculptPoint> sculptPoints;

    // Add closest point with weight 1.
    geometry::CurveSample middleSample = samples[segmentIndex];
    Int numSegments = numSamples - 1;
    Int beforeMiddleSampleIndex = segmentIndex > 0 ? segmentIndex - 1 : 0;
    Int afterMiddleSampleIndex =
        segmentIndex < numSegments ? segmentIndex + 1 : numSegments;

    if (segmentParameter > 0 && segmentIndex < numSegments) {
        beforeMiddleSampleIndex = segmentIndex;
        middleSample =
            geometry::lerp(middleSample, samples[segmentIndex + 1], segmentParameter);
    }
    sculptPoints.emplaceLast(middleSample.position(), middleSample.halfwidth(0) * 2, 1);

    // Here we want to uniformly sample the curve over an arclength of
    // `radius` on both sides of the closest point.
    const Int numSculptSamplesPerSide = static_cast<Int>(radius / (tolerance * 2.0)) + 1;
    const double ds = radius / numSculptSamplesPerSide;
    Int j = 0;
    double acc = 0;
    double uMin = 0;
    geometry::CurveSample s1 = middleSample;

    auto sampleUniformStep = [&](Int i) -> bool {
        geometry::CurveSample s2 = samples[i];
        geometry::Vec2d p1p2 = s2.position() - s1.position();
        double d = p1p2.length();
        if (d <= core::epsilon) {
            return false;
        }
        double x = ds - acc;
        double a = 0;
        while (x < d) {
            a = x;
            acc = 0;
            double t = x / d;
            double oneMinusT = 1.0 - t;
            ++j;
            geometry::Vec2d p = s1.position() * oneMinusT + s2.position() * t;
            double w = (s1.halfwidth(0) * oneMinusT + s2.halfwidth(0) * t) * 2;
            double u = 1.0 - static_cast<double>(j) / numSculptSamplesPerSide;
            sculptPoints.emplaceLast(p, w, u);
            uMin = u;
            x += ds;
            if (j == numSculptSamplesPerSide) {
                return true;
            }
        }
        acc += d - a;
        if (i == 0 || i == numSamples - 1) {
            uMin = 1.0 - (static_cast<double>(j) + (acc / ds)) / numSculptSamplesPerSide;
        }
        s1 = s2;
        return false;
    };

    Int startSampleIndex = -1;
    for (Int i = beforeMiddleSampleIndex; i >= 0; --i) {
        if (sampleUniformStep(i)) {
            startSampleIndex = i;
            break;
        }
    }
    if (uMin != 0) {
        double wMin = cubicEaseInOut(uMin);
        for (SculptPoint& sp : sculptPoints) {
            sp.wMin = wMin;
        }
    }
    std::reverse(sculptPoints.begin(), sculptPoints.end());
    Int wMinFillIndex = sculptPoints.length();

    j = 0;
    acc = 0;
    uMin = 0;
    s1 = middleSample;
    Int endSampleIndex = numSamples - 1;
    for (Int i = afterMiddleSampleIndex; i < numSamples; ++i) {
        if (sampleUniformStep(i)) {
            endSampleIndex = i;
            break;
        }
    }
    if (uMin != 0) {
        double wMin = cubicEaseInOut(uMin);
        for (Int i = wMinFillIndex; i < sculptPoints.length(); ++i) {
            sculptPoints[i].wMin = wMin;
        }
    }

    bool hasWidths = !widths_.isEmpty();

    Int startPointIndex = 0;
    for (Int i = 0; i < numPoints; ++i) {
        Int pointSampleIndex = pointSampleIndices[i];
        if (pointSampleIndex > startSampleIndex) {
            break;
        }
        startPointIndex = i;
    }

    Int endPointIndex = numPoints - 1;
    for (Int i = 0; i < numPoints; ++i) {
        Int pointSampleIndex = pointSampleIndices[i];
        if (pointSampleIndex >= endSampleIndex) {
            endPointIndex = i;
            break;
        }
    }

    for (Int i = 0; i < sculptPoints.length(); ++i) {
        SculptPoint& sp = sculptPoints[i];
        if (sp.wMin < 1) {
            double t = cubicEaseInOut(sp.u);
            double a = sp.wMin;
            t = (t - a) / (1 - a);
            sp.pos += delta * t;
        }
    }

    core::IntArray indices({0, sculptPoints.length() - 1});
    filterSculptPointsStep(sculptPoints, indices, 0, tolerance * 0.5);
    Int numPatchPoints = indices.length();

    //indices.clear();
    //for (Int i = 0; i < sculptPoints.length(); ++i) {
    //    indices.append(i);
    //}
    //numPatchPoints = sculptPoints.length();

    Int sculptPointsStartIndex = startPointIndex + 1;
    points_.erase(
        points_.begin() + sculptPointsStartIndex, points_.begin() + endPointIndex);
    if (hasWidths) {
        widths_.erase(
            widths_.begin() + sculptPointsStartIndex, widths_.begin() + endPointIndex);
    }
    points_.insert(sculptPointsStartIndex, numPatchPoints, {});
    if (hasWidths) {
        widths_.insert(sculptPointsStartIndex, numPatchPoints, {});
    }

    for (Int i = 0; i < numPatchPoints; ++i) {
        const SculptPoint& sp = sculptPoints[indices[i]];
        points_[sculptPointsStartIndex + i] = sp.pos;
        if (hasWidths) {
            widths_[sculptPointsStartIndex + i] = sp.width;
        }
    }

    dirtyEdgeSampling();

    return middleSample.position() + delta;

    // Depending on the sculpt kernel we may have to duplicate the points
    // at the sculpt boundary to "extrude" properly.

    // problem: cannot reuse distanceToCurve.. samples don't have their segment index :(

    // In arclength mode, step is not supported so we have to do this only once.
    // In spatial mode, step is supported and we may have to do this at every step.
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
