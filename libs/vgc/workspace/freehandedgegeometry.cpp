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
#include <vgc/workspace/freehandedgegeometry.h>

namespace vgc::workspace {

void FreehandEdgeGeometry::setPoints(const SharedConstPoints& points) {
    points_ = points;
    dirtyArclengths_();
    dirtyEdgeSampling();
}

void FreehandEdgeGeometry::setPoints(geometry::Vec2dArray points) {
    points_ = SharedConstPoints(std::move(points));
    dirtyArclengths_();
    dirtyEdgeSampling();
}

void FreehandEdgeGeometry::setWidths(const SharedConstWidths& widths) {
    widths_ = widths;
    dirtyEdgeSampling();
}

void FreehandEdgeGeometry::setWidths(core::DoubleArray widths) {
    widths_ = SharedConstWidths(std::move(widths));
    dirtyEdgeSampling();
}

std::shared_ptr<vacomplex::KeyEdgeGeometry> FreehandEdgeGeometry::clone() const {
    auto ret = std::make_shared<FreehandEdgeGeometry>();
    ret->points_ = points_;
    ret->widths_ = widths_;
    ret->arclengths_ = arclengths_;
    return ret;
}

void FreehandEdgeGeometry::snap(
    const geometry::Vec2d& snapStartPosition,
    const geometry::Vec2d& snapEndPosition,
    vacomplex::EdgeSnapTransformationMode /*mode*/) {

    if (!snappedPoints_.isEmpty() && snappedPoints_.first() == snapStartPosition
        && snappedPoints_.last() == snapEndPosition) {

        points_ = SharedConstPoints(snappedPoints_);
        dirtyArclengths_();
        dirtyEdgeSampling();
    }
    else {
        geometry::Vec2dArray newPoints = points_.get();
        computeSnappedLinearS_(
            newPoints, newPoints, arclengths_, snapStartPosition, snapEndPosition);
        points_ = SharedConstPoints(newPoints);
        dirtyArclengths_();
        dirtyEdgeSampling();
    }

    snappedPoints_.clear();
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

    const geometry::Vec2dArray& points = points_.get();

    if (!snappedPoints_.isEmpty() && snappedPoints_.first() == snapStartPosition
        && snappedPoints_.last() == snapEndPosition) {

        curve.setPositions(snappedPoints_);
        curve.setWidths(widths_.get());
    }
    else if (points.isEmpty()) {
        // fallback to segment
        tmpPoints = {snapStartPosition, snapEndPosition};
        tmpWidths = {1.0, 1.0};
        curve.setPositions(tmpPoints);
        curve.setWidths(tmpWidths);
    }
    else if (points.first() == snapStartPosition && points.last() == snapEndPosition) {
        // no snapping necessary
        curve.setPositions(points);
        curve.setWidths(widths_.get());
    }
    else {
        computeSnappedLinearS_(
            snappedPoints_, points, arclengths_, snapStartPosition, snapEndPosition);
        curve.setPositions(snappedPoints_);
        curve.setWidths(widths_.get());
    }

    curve.sampleRange(samples, geometry::CurveSamplingParameters(quality));
    VGC_ASSERT(samples.length() > 0);

    return vacomplex::EdgeSampling(std::move(samples));
}

bool FreehandEdgeGeometry::updateFromDomEdge_(dom::Element* element) {
    namespace ds = dom::strings;

    bool changed = false;

    const auto& points = element->getAttribute(ds::positions).getVec2dArray();
    if (points_ != points) {
        points_ = points;
        dirtyEdgeSampling();
        changed = true;
    }

    const auto& widths = element->getAttribute(ds::widths).getDoubleArray();
    if (widths_ != widths) {
        widths_ = widths;
        dirtyEdgeSampling();
        changed = true;
    }

    return changed;
}

void FreehandEdgeGeometry::writeToDomEdge_(dom::Element* element) const {
    namespace ds = dom::strings;

    const auto& points = element->getAttribute(ds::positions).getVec2dArray();
    if (points_ != points) {
        element->setAttribute(ds::positions, points_);
    }

    const auto& widths = element->getAttribute(ds::widths).getDoubleArray();
    if (widths_ != widths) {
        element->setAttribute(ds::widths, widths_);
    }
}

void FreehandEdgeGeometry::removeFromDomEdge_(dom::Element* element) const {
    namespace ds = dom::strings;
    element->clearAttribute(ds::positions);
    element->clearAttribute(ds::widths);
}

void FreehandEdgeGeometry::updateArclengths_() const {
    if (arclengths_.isEmpty()) {
        computeArclengths_(arclengths_, points_.get());
    }
}

void FreehandEdgeGeometry::dirtyArclengths_() {
    arclengths_.clear();
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
            double totalS = srcArclengths.last();
            if (totalS > 0) {
                // linear deformation in rough "s"
                for (Int i = 0; i < numPoints; ++i) {
                    double t = srcArclengths[i] / totalS;
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
        curve.sampleRange(sampling, sParams, i - 1, i, true);
        s += sampling.last().s();
        outArclengths[i] = s;
        sampling.clear();
    }
}

} // namespace vgc::workspace
