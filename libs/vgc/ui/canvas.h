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

#ifndef VGC_UI_CANVAS_H
#define VGC_UI_CANVAS_H

#include <variant>

#include <vgc/core/array.h>
#include <vgc/core/color.h>
#include <vgc/core/object.h>
#include <vgc/core/performancelog.h>
#include <vgc/geometry/camera2d.h>
#include <vgc/geometry/vec2d.h>
#include <vgc/ui/api.h>
#include <vgc/ui/cursor.h>
#include <vgc/ui/widget.h>
#include <vgc/workspace/workspace.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(SelectionListHistory);
VGC_DECLARE_OBJECT(Canvas);

class VGC_UI_API SelectableItem {
    SelectableItem(dom::Element* element)
        : element_(element) {
    }

    const dom::Element* element() const {
        return element_.getIfAlive();
    }

private:
    dom::ElementPtr element_;
};

using SelectionList = core::Array<SelectableItem>;

class VGC_UI_API SelectionListHistory : public core::Object {
private:
    VGC_OBJECT(SelectionListHistory, core::Object)

    SelectionListHistory() = default;

public:
    SelectionListHistoryPtr create();

    const SelectionList& currentSelection() const {
        return lists_.last();
    }

    void setSelection(const SelectionList& list);
    void setSelection(SelectionList&& list);

    VGC_SIGNAL(selectionChanged)

private:
    core::Array<SelectionList> lists_;
};

/// \class vgc::ui::Canvas
/// \brief A document canvas widget.
///
class VGC_UI_API Canvas : public Widget {
private:
    VGC_OBJECT(Canvas, Widget)

protected:
    /// This is an implementation details. Please use
    /// Canvas::create() instead.
    ///
    Canvas(workspace::Workspace* workspace);

public:
    /// Creates a Canvas.
    ///
    static CanvasPtr create(workspace::Workspace* workspace);

    /// Returns the observed document workspace.
    ///
    workspace::Workspace* workspace() const {
        return workspace_;
    }

    // XXX temporary. Will be deferred to separate class.
    void setCurrentColor(const core::Color& color) {
        currentColor_ = color;
    }

    SelectionList getSelectableItemsAt(const geometry::Vec2f& position);

    /// Sets the observed document workspace.
    ///
    void setWorkspace(workspace::Workspace* workspace);

    /// Creates and manages new performance logs as children of the given \p
    /// parent.
    ///
    void startLoggingUnder(core::PerformanceLog* parent);

    /// Destroys the currently managed logs whose parent is the given \p
    /// parent, if any.
    ///
    void stopLoggingUnder(core::PerformanceLog* parent);

    VGC_SIGNAL(changed);

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
    geometry::Vec2f computePreferredSize() const override;
    void onPaintCreate(graphics::Engine* engine) override;
    void onPaintDraw(graphics::Engine* engine, PaintOptions options) override;
    void onPaintDestroy(graphics::Engine* engine) override;
    //

    VGC_SLOT(onWorkspaceChanged, onWorkspaceChanged_)
    VGC_SLOT(onDocumentChanged, onDocumentChanged_)

private:
    // Flags
    bool reload_ = true;

    // Cursor
    CursorChanger cursorChanger_;

    // Camera (provides view matrix + projection matrix)
    geometry::Camera2d camera_;

    // Scene
    workspace::Workspace* workspace_;

    void onWorkspaceChanged_();
    void onDocumentChanged_(const dom::Diff& diff);

    // Moving camera
    bool isSketching_ = false;
    bool isPanning_ = false;
    bool isRotating_ = false;
    bool isZooming_ = false;
    geometry::Vec2d mousePosAtPress_ = {};
    geometry::Camera2d cameraAtPress_;

    // Curve draw
    core::UndoGroup* drawCurveUndoGroup_ = nullptr;
    dom::Element* endVertex_ = nullptr;
    dom::Element* edge_ = nullptr;
    geometry::Vec2dArray points_;
    core::DoubleArray widths_;

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

    struct EdgeGraphics {
        explicit EdgeGraphics(dom::Element* element)
            : element(element) {
        }

        // Stroke
        graphics::GeometryViewPtr strokeGeometry_;

        // Fill
        //graphics::GeometryViewPtr fillGeometry_;

        // Control Points
        graphics::GeometryViewPtr pointsGeometry_;
        Int numPoints = 0;

        // Line
        graphics::GeometryViewPtr dispLineGeometry_;

        bool inited_ = false;
        dom::Element* element;
    };

    using EdgeGraphicsIterator = std::list<EdgeGraphics>::iterator;

    std::list<EdgeGraphics> edgeGraphics_; // in draw order
    std::list<EdgeGraphics> removedEdgeGraphics_;
    std::map<dom::Element*, EdgeGraphicsIterator> edgeGraphicsMap_;
    graphics::GeometryViewPtr bgGeometry_;

    struct unwrapped_less {
        template<typename It>
        bool operator()(const It& lh, const It& rh) const {
            return &*lh < &*rh;
        }
    };

    std::set<EdgeGraphicsIterator, unwrapped_less> toUpdate_;
    //bool needsSort_ = false;

    void clearGraphics_();
    void updateEdgeGraphics_(graphics::Engine* engine);
    EdgeGraphicsIterator getOrCreateEdgeGraphics_(dom::Element* element);
    void updateEdgeGraphics_(graphics::Engine* engine, EdgeGraphics& r);
    static void destroyEdgeGraphics_(EdgeGraphics& r);

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
    bool mousePressed_; // whether there's been a mouse press event
    // with no matching mouse release event.
    bool tabletPressed_; // whether there's been a tablet press event
    // with no matching tablet release event.
    MouseButton mouseButtonAtPress_; // value of event->button at press

    // Polygon mode. This is selected with the n/t/f keys.
    // XXX This is a temporary quick method to switch between
    // render modes. A more engineered method will come later.
    int polygonMode_; // 0: fill; 1: lines (i.e., not exactly like OpenGL)
    graphics::RasterizerStatePtr fillRS_;
    graphics::RasterizerStatePtr wireframeRS_;

    // Show control points. This is toggled with the "p" key.
    // XXX This is a temporary quick method to switch between
    // render modes. A more engineered method will come later.
    bool showControlPoints_;

    // Tesselation mode. This is selected with the i/u/a keys.
    // XXX This is a temporary quick method to switch between
    // tesselation modes. A more engineered method will come later.
    int requestedTesselationMode_; // 0: none; 1: uniform; 2: adaptive
    int currentTesselationMode_;

    // XXX This is a temporary test, will be deferred to separate classes. Here
    // is an example of how responsibilities could be separated:
    //
    // Widget: Creates an OpenGL context, receive graphical user input.
    // Renderer: Renders the workspace to the given OpenGL context.
    // Controller: Modifies the workspace based on user input (could be in the
    // form of "Action" instances).
    //
    void startCurve_(const geometry::Vec2d& p, double width = 1.0);
    void continueCurve_(const geometry::Vec2d& p, double width = 1.0);
    core::Color currentColor_;

    // Performance logging
    core::PerformanceLogTask renderTask_;
    core::PerformanceLogTask updateTask_;
    core::PerformanceLogTask drawTask_;
};

} // namespace vgc::ui

#endif // VGC_UI_CANVAS_H
