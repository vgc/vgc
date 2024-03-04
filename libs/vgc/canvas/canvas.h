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
#include <vgc/canvas/displaymode.h>
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

enum class CoordinateSpace {
    Widget,
    Workspace
};

VGC_DECLARE_OBJECT(Canvas);
VGC_DECLARE_OBJECT(WorkspaceSelection);

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
    Canvas(CreateKey);

public:
    /// Creates a Canvas.
    ///
    static CanvasPtr create();

    /// Returns the observed document workspace.
    ///
    workspace::WorkspaceWeakPtr workspace() const {
        return workspace_;
    }

    /// Sets the `Workspace` that this `Canvas is operating on.
    ///
    void setWorkspace(workspace::WorkspaceWeakPtr workspace);

    /// This signal is emitted whenever this `Canvas` changes which
    /// `workspace()` it is operating on.
    ///
    /// The new `workspace()` may be null, which means that the canvas doesn't
    /// operate on any `Workspace` anymore.
    ///
    /// Note that this signal is different from `workspace()->changed()`, which
    /// is emitted when the inner content of the `workspace()` changes.
    ///
    VGC_SIGNAL(workspaceReplaced)

    /// Returns the `WorkspaceSelection` that this `Canvas` is operating on.
    ///
    WorkspaceSelectionWeakPtr workspaceSelection() const {
        return workspaceSelection_;
    }

    /// Sets the observed document workspace.
    ///
    void setWorkspaceSelection(WorkspaceSelectionWeakPtr workspaceSelection);

    /// This signal is emitted whenever this `Canvas` changes which
    /// `workspaceSelection()` it is operating on.
    ///
    /// The new `workspaceSelection()` may be null, which means that the canvas
    /// doesn't operate on any `WorkspaceSelection` anymore.
    ///
    /// Note that this signal is different from
    /// `workspaceSelection()->changed()`, which is emitted when the selected
    /// items of the `workspaceSelection()` changes.
    ///
    VGC_SIGNAL(workspaceSelectionReplaced)

    /// This signal is emitted whenever:
    ///
    /// - this `Canvas` changes which `workspaceSelection()` it is operating on, or
    ///
    /// - `workspaceSelection()->changed()` is emitted
    ///
    VGC_SIGNAL(workspaceSelectionChanged)

    /// Returns the current display mode.
    ///
    DisplayMode displayMode() const {
        return displayMode_;
    }

    /// Sets the current display mode.
    ///
    void setDisplayMode(DisplayMode displayMode);

    /// Creates and manages new performance logs as children of the given \p
    /// parent.
    ///
    void startLoggingUnder(core::PerformanceLog* parent);

    /// Destroys the currently managed logs whose parent is the given \p
    /// parent, if any.
    ///
    void stopLoggingUnder(core::PerformanceLog* parent);

    /// Returns the camera used to view the workspace.
    ///
    const geometry::Camera2d& camera() const {
        return camera_;
    }

    /// Sets the camera of this view.
    ///
    void setCamera(const geometry::Camera2d& camera);

    /// Returns the current time displayed by the canvas.
    ///
    core::AnimTime currentTime() const {
        return {};
    }

    /// Computes candidate elements for selection at `position`.
    ///
    /// Returns a list of pairs (element id, distance from position) with
    /// one pair for each candidate. This list sorted from closest to
    /// farthest from `position` and from foreground to background for
    /// candidates at equal distances from `position`.
    ///
    core::Array<SelectionCandidate> computeSelectionCandidates(
        const geometry::Vec2d& positionInWidgetSpace,
        style::Length toleranceInWidgetSpace =
            style::Length(7.0, style::LengthUnit::Dp)) const;

    core::Array<SelectionCandidate> computeSelectionCandidatesAboveOrAt(
        core::Id itemId,
        const geometry::Vec2d& position,
        double tolerance = 0,
        CoordinateSpace coordinateSpace = CoordinateSpace::Widget) const;

    /// Computes candidate elements for selection in the axis-aligned
    /// rectangle with opposite corners `a` and `b`.
    ///
    /// Returns a list of element ids. This list sorted from foreground
    /// to background.
    ///
    core::Array<core::Id> computeRectangleSelectionCandidates(
        const geometry::Vec2d& a,
        const geometry::Vec2d& b,
        CoordinateSpace coordinateSpace = CoordinateSpace::Widget) const;

    // TODO: Use enums and/or specific class such as WireframeMode,
    // ControlPointVisibility, CanvasSettings, etc?

    /// Returns whether the wireframe mode is enabled.
    ///
    bool isWireframeMode() const {
        return isWireframeMode_;
    }

    /// Sets whether the wireframe mode is enabled.
    ///
    void setWireframeMode(bool isWireframeMode);

    /// Returns whether the control points of all curves are visible.
    ///
    bool areControlPointsVisible() const {
        return areControlPointsVisible_;
    }

    /// Sets whether the wireframe mode is enabled.
    ///
    void setControlPointsVisible(bool areControlPointsVisible);

protected:
    // Reimplementation of Widget virtual methods
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
    workspace::WorkspaceWeakPtr workspace_;

    void onWorkspaceChanged_();
    VGC_SLOT(onWorkspaceChanged_)

    // Selection
    WorkspaceSelectionWeakPtr workspaceSelection_;

    void onWorkspaceSelectionChanged_();
    VGC_SLOT(onWorkspaceSelectionChanged_)

    core::Array<workspace::Element*> selectedElements_() const;

    // View Settings
    DisplayMode displayMode_ = DisplayMode::Normal;

    // Moving camera
    bool isPanning_ = false;
    bool isRotating_ = false;
    bool isZooming_ = false;
    geometry::Vec2f mousePosAtPress_ = {};
    geometry::Camera2d cameraAtPress_;
    double timeAtPress_ = 0;
    bool isDragging_ = false;

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

    // Wireframe mode
    bool isWireframeMode_ = false;
    graphics::RasterizerStatePtr fillRS_;
    graphics::RasterizerStatePtr wireframeRS_;

    // Show control points
    bool areControlPointsVisible_ = false;

    // Performance logging
    core::PerformanceLogTask renderTask_;
    core::PerformanceLogTask updateTask_;
    core::PerformanceLogTask drawTask_;
};

} // namespace vgc::canvas

#endif // VGC_CANVAS_CANVAS_H
