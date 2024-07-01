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
#include <vgc/tools/sketchpasses.h>
#include <vgc/ui/command.h>
#include <vgc/ui/cursor.h>
#include <vgc/ui/module.h>
#include <vgc/workspace/workspace.h>

namespace vgc::tools {

enum class SketchPreprocessing : Int8 {
    // Use the default sketch preprocessing method
    Default,

    // The input points are used as is as control points.
    NoPreprocessing,

    // A gaussian smoothing is applied to the input points.
    IndexGaussianSmoothing,

    // The Douglas-Peucker algorithm is used to discard some of the input
    // points.
    DouglasPeucker,

    // Outputs a single line segment from the first to the last input point.
    SingleLineSegmentWithFixedEndpoints,

    // Fits a single line segment through the input points.
    SingleLineSegmentWithFreeEndpoints,

    // Fits a single quadratic segment through the input points,
    // starting and ending exactly at the first and last input points.
    SingleQuadraticSegmentWithFixedEndpoints,

    // Fits a sequence of quadratic segments through the input points.
    QuadraticSpline,

    // Blends overlapping local quadratic fits together.
    QuadraticBlend,
};

VGC_TOOLS_API
VGC_DECLARE_ENUM(SketchPreprocessing)

VGC_DECLARE_OBJECT(SketchModule);

/// \class vgc::tools::SketchModule
/// \brief A module with sketch-related commands and actions.
///
class VGC_TOOLS_API SketchModule : public ui::Module {
private:
    VGC_OBJECT(SketchModule, ui::Module)

protected:
    SketchModule(CreateKey, const ui::ModuleContext& context);

public:
    static SketchModulePtr create(const ui::ModuleContext& context);

    SketchPreprocessing preprocessing() const;

private:
    void onPreprocessingChanged_();
    VGC_SLOT(onPreprocessingChanged_)

    void reProcessExistingEdges_();
};

VGC_DECLARE_OBJECT(Sketch);

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

    /// Make the SketchTool aware of the SketchModule.
    ///
    // TODO: Make it possible to do this in the constructor of the tool, e.g.,
    // via a ToolContext that allows for context.importModule<SketchModule>();
    //
    void setSketchModule(SketchModuleWeakPtr sketchModule) {
        sketchModule_ = sketchModule;
    }

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
    ui::WidgetPtr doCreateOptionsWidget() const override;

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
    SketchModuleWeakPtr sketchModule_;

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
    std::optional<SketchPreprocessing> lastPreprocessing_;

    // Raw input in widget space (pixels)
    SketchPointBuffer inputPoints_;

    // Sequence of sketch passes to apply to the input
    SketchPipeline pipeline_;

    // Pending Clean Input
    //
    // TODO:
    // - Update terminology.
    // - "Clean" should probably be renamed "PreSnapped".
    // - Make snapping a SketchPass?
    // - Is cleanInputStartPointOverride_ still necessary?
    //  (it was implemented before SketchPipeline)
    //
    Int cleanInputStartIndex_ = 0;
    std::optional<SketchPoint> cleanInputStartPointOverride_;

    core::ConstSpan<SketchPoint> cleanInputPoints_() const;
    Int numStableCleanInputPoints_() const;

    // Snapping
    //
    // Note: keep in mind that isSnappingEnabled() may change between
    // startCurve() and finishCurve().
    //
    std::optional<geometry::Vec2d> snapStartPosition_;
    geometry::Vec2dArray startSnappedCleanInputPositions_;
    Int numStableStartSnappedCleanInputPositions_ = 0;
    core::Id snapEndVertexItemId_ = 0;

    void updateStartSnappedCleanInputPositions_();
    void endSnapStartSnappedCleanInputPositions_(geometry::Vec2dArray& result);

    workspace::Element*
    computeSnapVertex_(const geometry::Vec2d& position, core::Id tmpVertexItemId);

    // The length of curve that snapping is allowed to deform
    double snapFalloff_() const;

    // Pending Edge
    //core::Id firstStartVertexItemId_ = 0;
    core::Id startVertexItemId_ = 0;
    core::Id endVertexItemId_ = 0;
    core::Id edgeItemId_ = 0;
    geometry::Vec2dArray pendingPositions_;
    core::DoubleArray pendingWidths_;
    Int numStablePendingWidths_ = 0;

    void updatePendingPositions_();
    void updatePendingWidths_();

    // Snapping/Cutting Cache

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

    VertexInfo* searchVertexInfo_(core::Id itemId);
    void appendVertexInfo_(const geometry::Vec2d& position, core::Id itemId);

    EdgeInfo* searchEdgeInfo_(core::Id itemId);
    // void appendEdgeInfo_(); when adding a vertex on cut.
    // void removeEdgeInfo_(); when adding a vertex on cut.
    // void invalidateVertexSelectability_(); // after face cut winding can change.

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

    // Assumes canvas() is non-null.
    void startCurve_(ui::MouseEvent* event);
    void continueCurve_(ui::MouseEvent* event);
    void finishCurve_(ui::MouseEvent* event);
    void resetData_();
};

} // namespace vgc::tools

#endif // VGC_TOOLS_SKETCH_H
