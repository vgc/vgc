// Copyright 2022 The VGC Developers
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

#ifndef VGC_CANVAS_CANVAS_H
#define VGC_CANVAS_CANVAS_H

#include <variant>

#include <vgc/canvas/api.h>
#include <vgc/core/array.h>
#include <vgc/core/color.h>
#include <vgc/core/object.h>
#include <vgc/core/performancelog.h>
#include <vgc/geometry/camera2d.h>
#include <vgc/geometry/vec2d.h>
#include <vgc/ui/cursor.h>
#include <vgc/ui/widget.h>
#include <vgc/workspace/edge.h>
#include <vgc/workspace/workspace.h>

namespace vgc::canvas {

/// \enum vgc::canvas::DisplayMode
/// \brief Specifies display mode.
///
enum class DisplayMode {
    Normal,
    Outline,
    OutlineOverlay,
    // Draft, Preview, Pixel, Print..
};

VGC_DECLARE_OBJECT(SelectionListHistory);
VGC_DECLARE_OBJECT(Canvas);

using SelectionList = core::Array<core::Id>;

class VGC_CANVAS_API SelectionListHistory : public core::Object {
private:
    VGC_OBJECT(SelectionListHistory, core::Object)

    struct PrivateKey {};
    SelectionListHistory(CreateKey, PrivateKey);

public:
    SelectionListHistoryPtr create();

    const SelectionList& currentSelection() const {
        return lists_.last();
    }

    void pushSelection(const SelectionList& list);
    void pushSelection(SelectionList&& list);

    VGC_SIGNAL(selectionChanged)

private:
    core::Array<SelectionList> lists_;
};

/// \class vgc::canvas::SelectionCandidate
/// \brief A workspace item candidate for selection.
///
class VGC_CANVAS_API SelectionCandidate {
public:
    SelectionCandidate(core::Id id, double distance, Int priority = 1)
        : id_(id)
        , distance_(distance)
        , priority_(priority) {
    }

    core::Id id() const {
        return id_;
    }

    double distance() const {
        return distance_;
    }

    Int priority() const {
        return priority_;
    }

private:
    core::Id id_;
    double distance_;
    Int priority_;
};

/// \class vgc::canvas::Canvas
/// \brief A document canvas widget.
///
class VGC_CANVAS_API Canvas : public ui::Widget {
private:
    VGC_OBJECT(Canvas, ui::Widget)

protected:
    /// This is an implementation details. Please use
    /// Canvas::create() instead.
    ///
    Canvas(CreateKey, workspace::Workspace* workspace);

public:
    /// Creates a Canvas.
    ///
    static CanvasPtr create(workspace::Workspace* workspace);

    /// Returns the observed document workspace.
    ///
    workspace::Workspace* workspace() const {
        return workspace_;
    }

    /// Returns the current display mode.
    ///
    DisplayMode displayMode() const {
        return displayMode_;
    }

    /// Sets the current display mode.
    ///
    void setDisplayMode(DisplayMode displayMode);

    /// Sets the observed document workspace.
    ///
    void setWorkspace(workspace::Workspace* workspace);

    /// This signal is emitted whenever this `Canvas` changes which
    /// `workspace()` it is observing. In other words, this is emitted when
    /// `setWorkspace()` is called with a different workspace.
    ///
    /// The new `workspace()` may be null, which means that the canvas doesn't
    /// observe anymore a workspace.
    ///
    /// Note that this signal is different from `workspace()->changed()`, which
    /// is emitted when the inner content of the `workspace()` changes.
    ///
    VGC_SIGNAL(workspaceReplaced)

    /// Creates and manages new performance logs as children of the given \p
    /// parent.
    ///
    void startLoggingUnder(core::PerformanceLog* parent);

    /// Destroys the currently managed logs whose parent is the given \p
    /// parent, if any.
    ///
    void stopLoggingUnder(core::PerformanceLog* parent);

    /// Returns the current requested tesselation mode.
    ///
    geometry::CurveSamplingQuality requestedTesselationMode() const {
        return requestedTesselationMode_;
    }

    /// Returns the camera used to view the workspace.
    ///
    const geometry::Camera2d& camera() const {
        return camera_;
    }

    core::AnimTime currentTime() const {
        return {};
    }

    //----------------------- Manage selection --------------------------------
    //
    // For now, we keep this in Canvas, but we may want to move this to a
    // separate class (CanvasSelection? SelectionManager? WorkspaceSelection?)
    // since the "current selection" can be shared across several canvases
    // in case of split view, etc.
    //

    /// Returns the list of current selected elements.
    ///
    core::Array<core::Id> selection() const;

    /// Sets the current selected elements.
    ///
    void setSelection(const core::ConstSpan<core::Id>& elementIds);

    /// Deselects all selected elements.
    ///
    void clearSelection();

    /// This signal is emitted whenever the selection changes.
    ///
    VGC_SIGNAL(selectionChanged)

    /// Computes candidate elements for selection at `position`.
    ///
    /// Returns a list of pairs (element id, distance from position) with
    /// one pair for each candidate. This list sorted from closest to
    /// farthest from `position` and from foreground to background for
    /// candidates at equal distances from `position`.
    ///
    core::Array<SelectionCandidate>
    computeSelectionCandidates(const geometry::Vec2f& position) const;

    /// Computes candidate elements for selection in the axis-aligned
    /// rectangle with opposite corners `a` and `b`.
    ///
    /// Returns a list of element ids. This list sorted from foreground
    /// to background.
    ///
    core::Array<core::Id> computeRectangleSelectionCandidates(
        const geometry::Vec2f& a,
        const geometry::Vec2f& b) const;

protected:
    // Reimplementation of Widget virtual methods
    bool onKeyPress(ui::KeyPressEvent* event) override;
    bool onMouseMove(ui::MouseMoveEvent* event) override;
    bool onMousePress(ui::MousePressEvent* event) override;
    bool onMouseRelease(ui::MouseReleaseEvent* event) override;
    bool onMouseScroll(ui::ScrollEvent* event) override;
    void onMouseEnter() override;
    void onMouseLeave() override;
    void onVisible() override;
    void onHidden() override;
    void onResize() override;
    geometry::Vec2f computePreferredSize() const override;
    void onPaintCreate(graphics::Engine* engine) override;
    void onPaintDraw(graphics::Engine* engine, ui::PaintOptions options) override;
    void onPaintDestroy(graphics::Engine* engine) override;
    void updateChildrenGeometry() override;

private:
    // Flags
    bool reload_ = true;

    // Camera (provides view matrix + projection matrix)
    geometry::Camera2d camera_;

    // Scene
    workspace::Workspace* workspace_ = nullptr;
    DisplayMode displayMode_ = DisplayMode::Normal;

    void onWorkspaceChanged_();
    void onDocumentChanged_(const dom::Diff& diff);

    VGC_SLOT(onWorkspaceChanged, onWorkspaceChanged_)
    VGC_SLOT(onDocumentChanged, onDocumentChanged_)

    // Moving camera
    bool isPanning_ = false;
    bool isRotating_ = false;
    bool isZooming_ = false;
    geometry::Vec2f mousePosAtPress_ = {};
    geometry::Camera2d cameraAtPress_;
    double timeAtPress_ = 0;
    bool isDragging_ = false;

    // Selection
    core::Array<core::Id> selectedElementIds_;
    core::Array<workspace::Element*> selectedElements_() const;

    // Graphics resources
    // VgcGraph
    //   -> hit tests (tesselation as needed)
    //   -> render 2d (tesselation as needed)
    //   -> topo changes
    //   -> connected to dom (forward some changes and reflects them)
    //   -> has components instead of elements
    //   -> a square dom element has components in the graph
    //   -> components that have no 1-to-1 relationship with an
    //      element should be recognizable as such.

    graphics::GeometryViewPtr bgGeometry_;

    // Make sure to disallow concurrent usage of the mouse and the tablet to
    // avoid conflicts. This also acts as a work around the following Qt bugs:
    // 1. At least in Linus/X11, mouse events are generated even when tablet
    //    events are accepted.
    // 2. At least in Linux/X11, a TabletRelease is sometimes followed by both a
    //    MouseMove and a MouseRelease, see https://github.com/vgc/vgc/issues/9.
    //
    // We also disallow concurrent usage of different mouse buttons, in
    // particular:
    // 1. We ignore mousePressEvent() if there has already been a
    //    mousePressEvent() with another event->button() and no matching
    //    mouseReleaseEvent().
    // 2. We ignore mouseReleaseEvent() if the value of event->button() is
    //    different from its value in mousePressEvent().
    //
    bool mousePressed_ = false; // whether there's been a mouse press event
    // with no matching mouse release event.
    bool tabletPressed_ = false; // whether there's been a tablet press event
    // with no matching tablet release event.
    ui::MouseButton mouseButtonAtPress_ =
        ui::MouseButton::None; // value of event->button at press

    // Polygon mode. This is selected with the n/t/f keys.
    // XXX This is a temporary quick method to switch between
    // render modes. A more engineered method will come later.
    int polygonMode_ = 0; // 0: fill; 1: lines (i.e., not exactly like OpenGL)
    graphics::RasterizerStatePtr fillRS_;
    graphics::RasterizerStatePtr wireframeRS_;

    // Show control points. This is toggled with the "p" key.
    // XXX This is a temporary quick method to switch between
    // render modes. A more engineered method will come later.
    bool showControlPoints_ = false;

    // Tesselation mode. This is selected with the i/u/a keys.
    // XXX This is a temporary quick method to switch between
    // tesselation modes. A more engineered method will come later.
    geometry::CurveSamplingQuality requestedTesselationMode_ =
        geometry::CurveSamplingQuality::AdaptiveHigh;
    bool reTesselate = false;

    // Performance logging
    core::PerformanceLogTask renderTask_;
    core::PerformanceLogTask updateTask_;
    core::PerformanceLogTask drawTask_;

    void onFrameContent_();
    VGC_SLOT(onFrameContentSlot_, onFrameContent_)

    void cycleDisplayMode_();
    VGC_SLOT(cycleDisplayModeSlot_, cycleDisplayMode_)
};

} // namespace vgc::canvas

#endif // VGC_CANVAS_CANVAS_H
