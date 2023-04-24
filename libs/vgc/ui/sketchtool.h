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
    core::UndoGroup* drawCurveUndoGroup_ = nullptr;
    core::ConnectionHandle drawCurveUndoGroupConnectionHandle_ = {};
    dom::Element* endVertex_ = nullptr;
    dom::Element* edge_ = nullptr;

    // Raw input.
    //
    // Notes:
    // - for now, we do not smooth widths
    //
    geometry::Vec2dArray inputPoints_;
    core::DoubleArray inputWidths_;

    // Smoothing. Invariant: both arrays have the same length.
    geometry::Vec2dArray smoothedInputPoints_;
    core::DoubleArray smoothedInputArclengths_;
    void updateSmoothedData_();

    // Snapping
    //
    // Note: keep in mind that isSnappingEnabled() may change between
    // startCurve() and finishCurve().
    //
    bool hasStartSnap_ = false;
    geometry::Vec2d startSnapPosition_;

    // Final points
    geometry::Vec2dArray points_;
    core::DoubleArray widths_;

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

    void startCurve_(const geometry::Vec2d& p, double width);
    void continueCurve_(const geometry::Vec2d& p, double width);
    void finishCurve_();
    bool resetData_();

    // The length of curve that snapping is allowed to deform
    double snapFalloff() const;

    workspace::Element*
    computeSnapVertex_(const geometry::Vec2d& position, dom::Element* excludedElement_);
};

} // namespace vgc::ui

#endif // VGC_UI_SKETCHTOOL_H
