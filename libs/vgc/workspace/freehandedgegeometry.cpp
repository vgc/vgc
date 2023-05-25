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
    bool isBeingEdited = isBeingEdited_;
    if (isBeingEdited) {
        abortEdit();
    }
    domPoints_ = points;
    originalArclengths_.clear();
    dirtyEdgeSampling();
    if (isBeingEdited) {
        startEdit();
    }
}

void FreehandEdgeGeometry::setPoints(geometry::Vec2dArray points) {
    setPoints(SharedConstPoints(std::move(points)));
}

void FreehandEdgeGeometry::setWidths(const SharedConstWidths& widths) {
    bool isBeingEdited = isBeingEdited_;
    if (isBeingEdited) {
        abortEdit();
    }
    widths_ = widths;
    originalArclengths_.clear();
    dirtyEdgeSampling();
    if (isBeingEdited) {
        startEdit();
    }
}

void FreehandEdgeGeometry::setWidths(core::DoubleArray widths) {
    setWidths(SharedConstWidths(std::move(widths)));
}

std::shared_ptr<vacomplex::KeyEdgeGeometry> FreehandEdgeGeometry::clone() const {
    auto ret = std::make_shared<FreehandEdgeGeometry>();
    ret->domPoints_ = domPoints_;
    ret->domWidths_ = domWidths_;
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

    const geometry::Vec2dArray& points = isBeingEdited_ ? points_ : domPoints_;
    const core::DoubleArray& widths = isBeingEdited_ ? widths_ : domWidths_;

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

    return vacomplex::EdgeSampling(std::move(samples));
}

void FreehandEdgeGeometry::startEdit() {
    VGC_ASSERT(!isBeingEdited_);
    points_ = domPoints_.get();
    widths_ = domWidths_.get();
    isBeingEdited_ = true;
}

void FreehandEdgeGeometry::resetEdit() {
    if (isBeingEdited_) {
        points_ = domPoints_.get();
        widths_ = domWidths_.get();
        dirtyEdgeSampling();
    }
}

void FreehandEdgeGeometry::finishEdit() {
    if (isBeingEdited_) {
        domPoints_ = SharedConstPoints(points_);
        domWidths_ = SharedConstWidths(widths_);
        geometry::Vec2dArray().swap(points_);
        core::DoubleArray().swap(widths_);
        core::DoubleArray().swap(originalArclengths_);
        dirtyEdgeSampling();
        isBeingEdited_ = false;
    }
}

void FreehandEdgeGeometry::abortEdit() {
    if (isBeingEdited_) {
        geometry::Vec2dArray().swap(points_);
        core::DoubleArray().swap(widths_);
        core::DoubleArray().swap(originalArclengths_);
        dirtyEdgeSampling();
        isBeingEdited_ = false;
    }
}

void FreehandEdgeGeometry::snap(
    const geometry::Vec2d& snapStartPosition,
    const geometry::Vec2d& snapEndPosition,
    vacomplex::EdgeSnapTransformationMode /*mode*/) {

    const geometry::Vec2dArray& points = domPoints_;
    if (!points.isEmpty() && points.first() == snapStartPosition
        && points.last() == snapEndPosition) {
        // already snapped
        return;
    }

    geometry::Vec2dArray snappedPoints;
    computeSnappedLinearS_(
        snappedPoints, points, originalArclengths_, snapStartPosition, snapEndPosition);

    domPoints_ = SharedConstPoints(snappedPoints);

    originalArclengths_.clear();
    dirtyEdgeSampling();
}

bool FreehandEdgeGeometry::updateFromDomEdge_(dom::Element* element) {
    namespace ds = dom::strings;

    bool changed = false;

    const auto& domPoints = element->getAttribute(ds::positions).getVec2dArray();
    if (domPoints_ != domPoints) {
        domPoints_ = domPoints;
        originalArclengths_.clear();
        dirtyEdgeSampling();
        changed = true;
    }

    const auto& domWidths = element->getAttribute(ds::widths).getDoubleArray();
    if (domWidths_ != domWidths) {
        domWidths_ = domWidths;
        dirtyEdgeSampling();
        changed = true;
    }

    return changed;
}

void FreehandEdgeGeometry::writeToDomEdge_(dom::Element* element) const {
    namespace ds = dom::strings;

    const auto& domPoints = element->getAttribute(ds::positions).getVec2dArray();
    if (domPoints_ != domPoints) {
        element->setAttribute(ds::positions, domPoints_);
    }

    const auto& domWidths = element->getAttribute(ds::widths).getDoubleArray();
    if (domWidths_ != domWidths) {
        element->setAttribute(ds::widths, domWidths_);
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
