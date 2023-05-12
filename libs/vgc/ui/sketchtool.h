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

#ifndef VGC_UI_SKETCHTOOL_H
#define VGC_UI_SKETCHTOOL_H

#include <vgc/core/array.h>
#include <vgc/core/color.h>
#include <vgc/core/history.h>
#include <vgc/dom/element.h>
#include <vgc/geometry/vec2d.h>
#include <vgc/ui/api.h>
#include <vgc/ui/canvastool.h>
#include <vgc/ui/cursor.h>
#include <vgc/workspace/workspace.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(SketchTool);

/// \class vgc::ui::SketchTool
/// \brief A CanvasTool that implements sketching strokes.
///
class VGC_UI_API SketchTool : public CanvasTool {
private:
    VGC_OBJECT(SketchTool, CanvasTool)

protected:
    /// This is an implementation details.
    /// Please use `SketchTool::create()` instead.
    ///
    SketchTool();

public:
    /// Creates a `SketchTool`.
    ///
    static SketchToolPtr create();

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
    bool onKeyPress(KeyEvent* event) override;
    bool onMouseMove(MouseEvent* event) override;
    bool onMousePress(MouseEvent* event) override;
    bool onMouseRelease(MouseEvent* event) override;
    bool onMouseEnter() override;
    bool onMouseLeave() override;
    void onResize() override;
    void onPaintCreate(graphics::Engine* engine) override;
    void onPaintDraw(graphics::Engine* engine, PaintOptions options) override;
    void onPaintDestroy(graphics::Engine* engine) override;

protected:
    // Stroke style
    core::Color penColor_ = core::Color(0, 0, 0, 1);

    // Flags
    bool reload_ = true;

    // Cursor
    CursorChanger cursorChanger_;

    // Curve draw
    bool isSketching_ = false;
    bool isCurveStarted_ = false;
    bool hasPressure_ = false;
    core::UndoGroup* drawCurveUndoGroup_ = nullptr;
    core::ConnectionHandle drawCurveUndoGroupConnectionHandle_ = {};
    dom::Element* endVertex_ = nullptr;
    dom::Element* edge_ = nullptr;

    // Raw input in widget space (pixels).
    //
    // Invariant: all arrays have the same length.
    //
    // Notes:
    // - for now, we do not smooth widths.
    // - we assume the view matrix does not change during the operation.
    //
    geometry::Vec2fArray inputPoints_;
    core::DoubleArray inputWidths_;
    core::DoubleArray inputTimestamps_;
    geometry::Mat4d canvasToWorkspaceMatrix_;
    bool isInputPointsQuantized_ = false;
    bool isInputWidthsQuantized_ = false;
    bool isInputTimestampsQuantized_ = false;

    // Dequantization.
    //
    // This step removes the "staircase effect" that happens when input points
    // come from a source only providing integer coordinates (e.g. a mouse).
    //
    // The general idea is to discard and/or slighly adjust the position of the
    // samples to reconstruct what the mouse positions actually were if there
    // had been no rounding to integer (= "quantization") in the first place.
    //
    // This step is fundamently different from smoothing, because the staircase
    // patterns caused by quantization are not equivalent to random noise:
    // during quantization, the perturbations applied to consecutive samples
    // are correlated, while with random noise they are not.
    //
    // Therefore, while in smoothing we generally want to mimize some
    // least-square distance to samples, in dequantization we want instead to
    // minimize some other objective (e.g., curve length, curvature, or change
    // of curvature) subject to the non-linear constraint "pass through each
    // input pixel". As long as the dequantized curve passes through the input
    // pixel, we do not want to value more "being closer to the center of the
    // pixel".
    //
    // Invariant: all arrays have the same length.
    //
    geometry::Vec2fArray dequantizerBuffer_;
    Int dequantizerBufferStartIndex = 0;
    geometry::Vec2fArray unquantizedPoints_;
    core::DoubleArray unquantizedWidths_;
    core::DoubleArray unquantizedTimestamps_;
    void updateUnquantizedData_(bool isFinalPass);

    // Pre-transform processing.
    //
    // In this step, we apply any processing that we'd like to do before the
    // transform step, that is, all processing that relies on positions in
    // canvas coordinates space rather than positions in workspace coordinates.
    //
    // Invariant: all arrays have the same length.
    //
    geometry::Vec2fArray preTransformedPoints_;
    core::DoubleArray preTransformedWidths_;
    core::DoubleArray preTransformedTimestamps_;
    void updatePreTransformedData_(bool isFinalPass);

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
    geometry::Vec2dArray transformedPoints_;
    core::DoubleArray transformedWidths_;
    core::DoubleArray transformedTimestamps_;
    void updateTransformedData_(bool isFinalPass);

    // Smoothing.
    //
    // Invariant: all arrays have the same length.
    //
    geometry::Vec2dArray smoothedPoints_;
    core::DoubleArray smoothedWidths_;
    void updateSmoothedData_(bool isFinalPass);

    // Snapping
    //
    // Invariant: all arrays have the same length.
    //
    // Note: keep in mind that isSnappingEnabled() may change between
    // startCurve() and finishCurve().
    //
    bool hasStartSnap_ = false;
    geometry::Vec2d startSnapPosition_;
    geometry::Vec2dArray snappedPoints_;
    core::DoubleArray snappedWidths_;
    void updateSnappedData_(bool isFinalPass);
    void updateEndSnappedData_(const geometry::Vec2d& endSnapPosition);

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
    void startCurve_(MouseEvent* event);
    void continueCurve_(MouseEvent* event);
    void finishCurve_(MouseEvent* event);
    void resetData_();

    // The length of curve that snapping is allowed to deform
    double snapFalloff() const;

    workspace::Element*
    computeSnapVertex_(const geometry::Vec2d& position, dom::Element* excludedElement_);
};

} // namespace vgc::ui

#endif // VGC_UI_SKETCHTOOL_H
