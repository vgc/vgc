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

#include <variant>

#include <vgc/core/array.h>
#include <vgc/core/color.h>
#include <vgc/core/object.h>
#include <vgc/core/performancelog.h>
#include <vgc/geometry/camera2d.h>
#include <vgc/geometry/vec2d.h>
#include <vgc/ui/api.h>
#include <vgc/ui/canvastool.h>
#include <vgc/ui/cursor.h>
#include <vgc/workspace/workspace.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(SketchTool);

/// \class vgc::ui::SketchTool
/// \brief A canvas sketch tool widget.
///
class VGC_UI_API SketchTool : public CanvasTool {
private:
    VGC_OBJECT(SketchTool, CanvasTool)

protected:
    /// This is an implementation details.
    /// Please use Canvas::create() instead.
    ///
    SketchTool();

public:
    /// Creates a Canvas.
    ///
    static SketchToolPtr create();

    void setPenColor(const core::Color& color) {
        penColor_ = color;
    }

    void setPenWidth(double width) {
        penWidth_ = width;
    }

protected:
    // Reimplementation of Widget virtual methods
    bool onKeyPress(KeyEvent* event) override;
    bool onMouseMove(MouseEvent* event) override;
    bool onMousePress(MouseEvent* event) override;
    bool onMouseRelease(MouseEvent* event) override;
    bool onMouseEnter() override;
    bool onMouseLeave() override;
    void onVisible() override;
    void onHidden() override;
    void onResize() override;
    void onPaintCreate(graphics::Engine* engine) override;
    void onPaintDraw(graphics::Engine* engine, PaintOptions options) override;
    void onPaintDestroy(graphics::Engine* engine) override;
    //

protected:
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
    geometry::Vec2dArray points_;
    core::DoubleArray widths_;
    // for now we just get cursor pos at the end of the paint, there are still widgets
    // to draw after that but our current architecture doesn't let us have deferred
    // widget draws.. widget does not even know it's window.
    std::array<geometry::Vec2d, 3> minimalLatencyStrokePoints_;
    std::array<double, 3> minimalLatencyStrokeWidths_;
    graphics::GeometryViewPtr minimalLatencyStrokeGeometry_;
    bool minimalLatencyStrokeReload_ = false;
    geometry::Vec2f lastImmediateCursorPos_ = {};

    void startCurve_(const geometry::Vec2d& p, double width = 1.0);
    void continueCurve_(const geometry::Vec2d& p, double width = 1.0);

    core::Color penColor_ = core::Color(0, 0, 0, 1);
    double penWidth_ = 5.0;

    double pressurePenWidth_(const MouseEvent* event) const;
};

} // namespace vgc::ui

#endif // VGC_UI_CANVAS_H
