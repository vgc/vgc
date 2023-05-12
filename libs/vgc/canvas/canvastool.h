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

#ifndef VGC_CANVAS_CANVASTOOL_H
#define VGC_CANVAS_CANVASTOOL_H

#include <variant>

#include <vgc/canvas/api.h>
#include <vgc/canvas/canvas.h>
#include <vgc/core/object.h>
#include <vgc/workspace/workspace.h>

namespace vgc::canvas {

VGC_DECLARE_OBJECT(CanvasTool);

/// \class vgc::canvas::CanvasTool
/// \brief An abstract canvas tool widget.
///
class VGC_CANVAS_API CanvasTool : public ui::Widget {
private:
    VGC_OBJECT(CanvasTool, ui::Widget)

protected:
    /// This is an implementation details. Please use
    /// CanvasTool::create() instead.
    ///
    CanvasTool();

public:
    /// Creates a CanvasTool.
    ///
    static CanvasToolPtr create();

    /// Returns the working document workspace.
    ///
    workspace::Workspace* workspace() const {
        return canvas_ ? canvas_->workspace() : nullptr;
    }

    Canvas* canvas() const {
        return canvas_;
    }

    /// Returns the widget that control options for this tool.
    ///
    /// \sa createOptionsWidget().
    ///
    ui::Widget* optionsWidget() const;

protected:
    /// Creates the widget that control options for this tool.
    ///
    /// This will be called automatically.
    ///
    /// \sa `optionsWidget()`.
    ///
    virtual ui::WidgetPtr createOptionsWidget() const;

    // Reimplementation of Widget virtual methods
    void onParentWidgetChanged(ui::Widget* newParent) override;
    void preMouseMove(ui::MouseEvent* event) override;
    void preMousePress(ui::MouseEvent* event) override;
    void preMouseRelease(ui::MouseEvent* event) override;
    geometry::Vec2f computePreferredSize() const override;

private:
    Canvas* canvas_ = nullptr;

    mutable ui::WidgetPtr optionsWidget_;

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
    ui::MouseButtons pressedMouseButtons_ =
        {}; // whether there's been a mouse press event
            // with no matching mouse release event.
    ui::MouseButtons pressedTabletButtons_ =
        {}; // whether there's been a tablet press event
            // with no matching tablet release event.
};

} // namespace vgc::canvas

#endif // VGC_CANVAS_CANVASTOOL_H
