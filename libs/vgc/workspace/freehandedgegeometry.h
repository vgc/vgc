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

#include <vgc/geometry/catmullrom.h>
#include <vgc/geometry/yuksel.h>
#include <vgc/vacomplex/keyedge.h>
#include <vgc/workspace/api.h>
#include <vgc/workspace/edgegeometry.h>

namespace vgc::workspace {

class VGC_WORKSPACE_API FreehandEdgeGeometry : public EdgeGeometry {
public:
    using SharedConstPositions = geometry::SharedConstVec2dArray;
    using SharedConstWidths = core::SharedConstDoubleArray;

    using StrokeType = geometry::CatmullRomSplineStroke2d;
    //using StrokeType = geometry::YukselSplineStroke2d;

    FreehandEdgeGeometry(bool isClosed)
        : EdgeGeometry(isClosed) {

        stroke_ = createStroke_();
    }

    FreehandEdgeGeometry(bool isClosed, double constantWidth)
        : EdgeGeometry(isClosed) {

        stroke_ = createStroke_();
        stroke_->setConstantWidth(constantWidth);
    }

    FreehandEdgeGeometry(
        const SharedConstPositions& positions,
        const SharedConstWidths& widths,
        bool isClosed,
        bool isWidthConstant)

        : EdgeGeometry(isClosed) {

        stroke_ = createStroke_();
        if (isWidthConstant) {
            stroke_->setConstantWidth(widths.get()[0]);
        }
        else {
            stroke_->setWidths(widths);
        }
        stroke_->setPositions(positions);
    }

    const geometry::Vec2dArray& positions() const {
        return isBeingEdited_ ? editPositions_ : stroke_->positions();
    }

    const core::DoubleArray& widths() const {
        return isBeingEdited_ ? editWidths_ : stroke_->widths();
    }

    void setPositions(const SharedConstPositions& positions);

    void setPositions(geometry::Vec2dArray positions);

    void setWidths(const SharedConstWidths& widths);

    void setWidths(core::DoubleArray widths);

    std::shared_ptr<KeyEdgeGeometry> clone() const override;

    /// Expects positions in object space.
    ///
    vacomplex::EdgeSampling computeSampling(
        const geometry::CurveSamplingParameters& params,
        const geometry::Vec2d& snapStartPosition,
        const geometry::Vec2d& snapEndPosition,
        vacomplex::EdgeSnapTransformationMode mode) const override;

    vacomplex::EdgeSampling
    computeSampling(const geometry::CurveSamplingParameters& params) const override;

    void startEdit() override;
    void resetEdit() override;
    void finishEdit() override;
    void abortEdit() override;

    /// Expects delta in object space.
    ///
    void translate(const geometry::Vec2d& delta) override;

    /// Expects transformation in object space.
    ///
    void transform(const geometry::Mat3d& transformation) override;

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

    geometry::Vec2d sculptRadius(
        const geometry::Vec2d& position,
        double delta,
        double radius,
        double tolerance,
        bool isClosed = false) override;

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
    SharedConstPositions sharedConstPositions_;
    SharedConstWidths sharedConstWidths_;
    std::unique_ptr<StrokeType> stroke_;
    geometry::Vec2dArray editPositions_;
    core::DoubleArray editWidths_;
    core::DoubleArray originalKnotArclengths_;
    geometry::Vec2d editStartPosition_ = {};
    bool isBeingEdited_ = false;

    static void computeSnappedLinearS_(
        geometry::Vec2dArray& outPoints,
        StrokeType* srcStroke,
        core::DoubleArray& srcArclengths,
        const geometry::Vec2d& snapStartPosition,
        const geometry::Vec2d& snapEndPosition);

    static void
    computeKnotArclengths_(core::DoubleArray& outArclengths, StrokeType* srcStroke);

    std::unique_ptr<StrokeType> createStroke_() const;
};

} // namespace vgc::workspace

#endif // VGC_WORKSPACE_FREEHANDEDGEGEOMETRY_H
