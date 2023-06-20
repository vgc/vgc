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

#ifndef VGC_WORKSPACE_FREEHANDEDGEGEOMETRY_H
#define VGC_WORKSPACE_FREEHANDEDGEGEOMETRY_H

#include <vgc/vacomplex/keyedge.h>
#include <vgc/workspace/api.h>
#include <vgc/workspace/edgegeometry.h>

namespace vgc::workspace {

class VGC_WORKSPACE_API FreehandEdgeGeometry : public EdgeGeometry {
public:
    using SharedConstPoints = geometry::SharedConstVec2dArray;
    using SharedConstWidths = core::SharedConstDoubleArray;

    FreehandEdgeGeometry() = default;

    FreehandEdgeGeometry(const SharedConstPoints& points, const SharedConstWidths& widths)
        : points_(points)
        , widths_(widths) {
    }

    const geometry::Vec2dArray& points() const {
        return isBeingEdited_ ? points_ : sharedConstPoints_;
    }

    const core::DoubleArray& widths() const {
        return isBeingEdited_ ? widths_ : sharedConstWidths_;
    }

    void setPoints(const SharedConstPoints& points);

    void setPoints(geometry::Vec2dArray points);

    void setWidths(const SharedConstWidths& widths);

    void setWidths(core::DoubleArray widths);

    std::shared_ptr<KeyEdgeGeometry> clone() const override;

    /// Expects positions in object space.
    ///
    vacomplex::EdgeSampling computeSampling(
        geometry::CurveSamplingQuality quality,
        const geometry::Vec2d& snapStartPosition,
        const geometry::Vec2d& snapEndPosition,
        vacomplex::EdgeSnapTransformationMode mode) const override;

    vacomplex::EdgeSampling computeSampling(
        geometry::CurveSamplingQuality quality,
        bool isClosed,
        vacomplex::EdgeSnapTransformationMode mode) const override;

    void startEdit() override;
    void resetEdit() override;
    void finishEdit() override;
    void abortEdit() override;

    /// Expects delta in object space.
    ///
    void translate(const geometry::Vec2d& delta) override;

    /// Expects positions in object space.
    ///
    void snap(
        const geometry::Vec2d& snapStartPosition,
        const geometry::Vec2d& snapEndPosition,
        vacomplex::EdgeSnapTransformationMode mode) override;

    geometry::Vec2d sculptGrab(
        const geometry::Vec2d& startPosition,
        const geometry::Vec2d& endPosition,
        double radius,
        double strength,
        double tolerance,
        bool isClosed) override;

    geometry::Vec2d sculptSmooth(
        const geometry::Vec2d& position,
        double radius,
        double strength,
        double tolerance,
        bool isClosed) override;

    bool updateFromDomEdge_(dom::Element* element) override;
    void writeToDomEdge_(dom::Element* element) const override;
    void removeFromDomEdge_(dom::Element* element) const override;

private:
    SharedConstPoints sharedConstPoints_;
    SharedConstWidths sharedConstWidths_;
    core::DoubleArray originalArclengths_;
    geometry::Vec2dArray points_;
    core::DoubleArray widths_;
    geometry::Vec2d editStartPosition_ = {};
    bool isBeingEdited_ = false;

    static void computeSnappedLinearS_(
        geometry::Vec2dArray& outPoints,
        const geometry::Vec2dArray& srcPoints,
        core::DoubleArray& srcArclengths,
        const geometry::Vec2d& snapStartPosition,
        const geometry::Vec2d& snapEndPosition);

    static void computeArclengths_(
        core::DoubleArray& outArclengths,
        const geometry::Vec2dArray& srcPoints);
};

} // namespace vgc::workspace

#endif // VGC_WORKSPACE_FREEHANDEDGEGEOMETRY_H
