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

#ifndef VGC_TOOLS_SKETCH_H
#define VGC_TOOLS_SKETCH_H

#include <vgc/canvas/canvastool.h>
#include <vgc/core/array.h>
#include <vgc/core/color.h>
#include <vgc/core/history.h>
#include <vgc/dom/element.h>
#include <vgc/geometry/vec2d.h>
#include <vgc/tools/api.h>
#include <vgc/ui/cursor.h>
#include <vgc/workspace/workspace.h>

namespace vgc::tools {

VGC_DECLARE_OBJECT(Sketch);

class VGC_TOOLS_API SketchPoint {
public:
    constexpr SketchPoint() noexcept
        : position_()
        , pressure_(0)
        , timestamp_(0)
        , width_(0)
        , s_(0) {
    }

    VGC_WARNING_PUSH
    VGC_WARNING_MSVC_DISABLE(26495) // member variable uninitialized
    SketchPoint(core::NoInit) noexcept
        : position_(core::noInit) {
    }
    VGC_WARNING_POP

    SketchPoint(
        const geometry::Vec2d& position,
        double pressure,
        double timestamp,
        double width,
        double s = 0) noexcept

        : position_(position)
        , pressure_(pressure)
        , timestamp_(timestamp)
        , width_(width)
        , s_(s) {
    }

    const geometry::Vec2d& position() const {
        return position_;
    }

    void setPosition(const geometry::Vec2d& position) {
        position_ = position;
    }

    double pressure() const {
        return pressure_;
    }

    void setPressure(double pressure) {
        pressure_ = pressure;
    }

    double timestamp() const {
        return timestamp_;
    }

    void setTimestamp(double timestamp) {
        timestamp_ = timestamp;
    }

    bool hasTimestamp() const {
        return timestamp_ != 0;
    }

    double width() const {
        return width_;
    }

    void setWidth(double width) {
        width_ = width;
    }

    /// Returns the cumulative chordal distance from the first point to this point.
    double s() const {
        return s_;
    }

    void setS(double s) {
        s_ = s;
    }

    void offsetS(double offset) {
        s_ += offset;
    }

private:
    geometry::Vec2d position_;
    double pressure_;
    double timestamp_;
    double width_;
    double s_;
};

using SketchPointArray = core::Array<SketchPoint>;

class VGC_TOOLS_API SketchPointBuffer {
public:
    SketchPointBuffer() noexcept = default;

    const SketchPoint& operator[](Int i) const {
        return points_[i];
    }

    SketchPoint& getRef(Int i) {
        if (i < numStablePoints_) {
            throw core::LogicError(
                "getRef(): cannot get a non-const reference to a stable point.");
        }
        return points_[i];
    }

    core::Span<SketchPoint> unstableSpan() {
        return core::Span<SketchPoint>(points_.begin() + numStablePoints_, points_.end());
    }

    const SketchPointArray& data() const {
        return points_;
    }

    Int length() const {
        return points_.length();
    }

    void reserve(Int count) {
        return points_.reserve(count);
    }

    void resize(Int count) {
        if (count < numStablePoints_) {
            throw core::LogicError("resize(): cannot decrease number of stable points.");
        }
        return points_.resize(count);
    }

    void clear() {
        points_.clear();
        numStablePoints_ = 0;
    }

    Int numStablePoints() const {
        return numStablePoints_;
    }

    void setNumStablePoints(Int numStablePoints) {
        if (numStablePoints < numStablePoints_) {
            throw core::LogicError(
                "setNumStablePoints(): cannot decrease number of stable points.");
        }
        else if (numStablePoints > points_.length()) {
            throw core::LogicError("setNumStablePoints(): number of stable points cannot "
                                   "be greater than number of points.");
        }
        numStablePoints_ = numStablePoints;
    }

    SketchPoint& append(const SketchPoint& point) {
        points_.append(point);
        return points_.last();
    }

    SketchPoint& emplaceLast(
        const geometry::Vec2d& position,
        double pressure,
        double timestamp,
        double width,
        double s = 0) {

        return points_.emplaceLast(position, pressure, timestamp, width, s);
    }

    template<typename InputIt, VGC_REQUIRES(core::isInputIterator<InputIt>)>
    void extend(InputIt first, InputIt last) {
        points_.extend(first, last);
    }

private:
    SketchPointArray points_;
    Int numStablePoints_ = 0;
};

class VGC_TOOLS_API SketchPointsProcessingPass {
protected:
    SketchPointsProcessingPass() noexcept = default;
    virtual ~SketchPointsProcessingPass() = default;

public:
    void updateResultFrom(const SketchPointBuffer& input) {
        Int numStablePoints = update_(input, lastNumStableInputPoints_);
        buffer_.setNumStablePoints(numStablePoints);
        lastNumStableInputPoints_ = input.numStablePoints();
    }

    void reset() {
        buffer_.clear();
        lastNumStableInputPoints_ = 0;
        reset_();
    }

    const SketchPointBuffer& buffer() const {
        return buffer_;
    }

protected:
    const SketchPoint& getPoint(Int i) {
        return buffer_[i];
    }

    SketchPoint& getPointRef(Int i) {
        return buffer_.getRef(i);
    }

    core::Span<SketchPoint> unstablePointSpan() {
        return buffer_.unstableSpan();
    }

    const SketchPointArray& points() const {
        return buffer_.data();
    }

    Int numPoints() const {
        return buffer_.length();
    }

    void reservePoints(Int count) {
        return buffer_.reserve(count);
    }

    void resizePoints(Int count) {
        return buffer_.resize(count);
    }

    Int numStablePoints() const {
        return buffer_.numStablePoints();
    }

    void appendPoint(const SketchPoint& point) {
        buffer_.append(point);
    }

    void emplacePointLast(
        const geometry::Vec2d& position,
        double pressure,
        double timestamp,
        double width,
        double s = 0) {

        buffer_.emplaceLast(position, pressure, timestamp, width, s);
    }

    template<typename InputIt, VGC_REQUIRES(core::isInputIterator<InputIt>)>
    void extendPoints(InputIt first, InputIt last) {
        buffer_.extend(first, last);
    }

    void updateCumulativeChordalDistances();

protected:
    // Must return the new number of stable result points.
    // This number must not be less than its previous value.
    virtual Int update_(const SketchPointBuffer& input, Int lastNumStableInputPoints) = 0;

    virtual void reset_() = 0;

private:
    SketchPointBuffer buffer_;
    Int lastNumStableInputPoints_ = 0;
};

namespace detail {

class VGC_TOOLS_API EmptyPass : public SketchPointsProcessingPass {
public:
    EmptyPass() noexcept = default;

protected:
    Int update_(const SketchPointBuffer& input, Int lastNumStableInputPoints) override;

    void reset_() override;
};

class VGC_TOOLS_API SmoothingPass : public SketchPointsProcessingPass {
public:
    SmoothingPass() noexcept = default;

protected:
    Int update_(const SketchPointBuffer& input, Int lastNumStableInputPoints) override;

    void reset_() override;
};

} // namespace detail

/// \class vgc::tools::SketchTool
/// \brief A CanvasTool that implements sketching strokes.
///
class VGC_TOOLS_API Sketch : public canvas::CanvasTool {
private:
    VGC_OBJECT(Sketch, canvas::CanvasTool)

protected:
    /// This is an implementation details.
    /// Please use `SketchTool::create()` instead.
    ///
    Sketch(CreateKey);

public:
    /// Creates a `SketchTool`.
    ///
    static SketchPtr create();

    /// Returns the pen color of the tool.
    ///
    core::Color penColor() const {
        return penColor_;
    }

    /// Sets the pen color of the tool.
    ///
    void setPenColor(const core::Color& color) {
        penColor_ = color;
    }

    /// Returns the width of the tool.
    ///
    double penWidth() const;

    /// Sets the pen width of the tool.
    ///
    void setPenWidth(double width);

    /// Returns whether sketched strokes are automatically snapped to end
    /// points of existing strokes.
    ///
    bool isSnappingEnabled() const;

    /// Sets whether sketched strokes are automatically snapped
    /// to end points of existing strokes.
    ///
    void setSnappingEnabled(bool enabled);

protected:
    // Reimplementation of CanvasTool virtual methods
    ui::WidgetPtr createOptionsWidget() const override;

    // Reimplementation of Widget virtual methods
    bool onKeyPress(ui::KeyPressEvent* event) override;
    bool onMouseMove(ui::MouseMoveEvent* event) override;
    bool onMousePress(ui::MousePressEvent* event) override;
    bool onMouseRelease(ui::MouseReleaseEvent* event) override;
    void onMouseEnter() override;
    void onMouseLeave() override;
    void onResize() override;
    void onPaintCreate(graphics::Engine* engine) override;
    void onPaintDraw(graphics::Engine* engine, ui::PaintOptions options) override;
    void onPaintDestroy(graphics::Engine* engine) override;

protected:
    // Stroke style
    core::Color penColor_ = core::Color(0, 0, 0, 1);

    // Flags
    bool reload_ = true;

    // Cursor
    ui::CursorChanger cursorChanger_;

    // Curve draw
    bool isSketching_ = false;
    bool isCurveStarted_ = false;
    bool hasPressure_ = false;
    core::UndoGroup* drawCurveUndoGroup_ = nullptr;
    core::ConnectionHandle drawCurveUndoGroupConnectionHandle_ = {};

    double startTime_ = 0;
    core::Id tmpEndVertexItemId_ = 0;
    core::Id edgeItemId_ = 0;

    // Raw input in widget space (pixels).
    //
    // Invariant: all arrays have the same length.
    //
    // Notes:
    // - for now, we do not smooth widths.
    // - we assume the view matrix does not change during the operation.
    //
    SketchPointBuffer inputPoints_;

    // Pre-transform processing.
    //
    // In this step, we apply any processing that we'd like to do before the
    // transform step, that is, all processing that relies on positions in
    // canvas coordinates space rather than positions in workspace coordinates.
    //
    // Invariant: all arrays have the same length.
    //
    detail::EmptyPass dummyPreTransformPass_;

    void updatePreTransformPassesResult_();
    void clearPreTransformPasses_();
    const SketchPointBuffer& preTransformPassesResult_();

    // Transformation.
    //
    // This step applies the transformation from canvas coordinates to
    // workspace/group coordinates.
    //
    // It also translate the timestamps such that the timestamp of the
    // first sample is 0.
    //
    // Invariant: all arrays have the same length.
    //
    geometry::Mat4d canvasToWorkspaceMatrix_;
    SketchPointBuffer transformedPoints_;
    void updateTransformedPoints_();

    // Smoothing.
    //
    // Invariant: all arrays have the same length.
    //
    detail::SmoothingPass smoothingPass_;

    void updatePostTransformPassesResult_();
    void clearPostTransformPasses_();
    const SketchPointBuffer& postTransformPassesResult_();

    // Snapping
    //
    // Invariant: all arrays have the same length.
    //
    // Note: keep in mind that isSnappingEnabled() may change between
    // startCurve() and finishCurve().
    //
    Int pendingEdgeStartPointIndex_ = 0;
    std::optional<geometry::Vec2d> startSnapPosition_;
    geometry::Vec2dArray startSnappedPositions_;
    Int numStartSnappedStablePoints_ = 0;
    core::Id endSnapVertexItemId_ = 0;
    geometry::Vec2d endSnapPosition_;
    core::DoubleArray pendingEdgeWidths_;
    geometry::Vec2dArray pendingEdgePositions_;
    void updateStartSnappedPoints_();
    void updateEndSnappedPositions_();
    void clearSnappingData_();

    // The length of curve that snapping is allowed to deform
    double snapFalloff_() const;

    struct VertexInfo {
        // fast access to position to do snap tests
        geometry::Vec2d position;
        core::Id itemId;
        std::optional<bool> isSelectable;
    };
    core::Array<VertexInfo> vertexInfos_;

    struct EdgeInfo {
        // fast access to geometry to do cut tests
        std::shared_ptr<const geometry::StrokeSampling2d> sampling;
        core::Id itemId;
    };
    core::Array<EdgeInfo> edgeInfos_;

    void initCellInfoArrays_();

    void appendVertexInfo_(const geometry::Vec2d& position, core::Id itemId);

    workspace::Element*
    computeSnapVertex_(const geometry::Vec2d& position, core::Id tmpVertexItemId);

    // Draw additional points at the stroke tip, based on global cursor
    // position, to reduce perceived input lag.
    //
    // Note: for now, we get the global cursor position at the end of the
    // paint, which is not perfect since there may still be widgets to be
    // drawn. Unfortunately, our current architecture doesn't allow us to do
    // better, for example by having deferred widget draws which we would
    // enable for the Canvas.
    //
    graphics::GeometryViewPtr minimalLatencyStrokeGeometry_;
    bool minimalLatencyStrokeReload_ = false;
    geometry::Vec2f lastImmediateCursorPos_ = {};
    geometry::Vec2d minimalLatencySnappedCursor_ = {};

    graphics::GeometryViewPtr mouseInputGeometry_;

    // Assumes canvas() is non-null.
    void startCurve_(ui::MouseEvent* event);
    void continueCurve_(ui::MouseEvent* event);
    void finishCurve_(ui::MouseEvent* event);
    void resetData_();
};

} // namespace vgc::tools

#endif // VGC_TOOLS_SKETCH_H
