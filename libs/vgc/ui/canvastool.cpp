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

#include <vgc/ui/canvastool.h>

#include <QBitmap>
#include <QCursor>
#include <QPainter>

#include <vgc/core/stringid.h>
#include <vgc/graphics/strings.h>
#include <vgc/ui/qtutil.h>
#include <vgc/ui/strings.h>

namespace vgc::ui {

CanvasToolPtr CanvasTool::create() {
    CanvasToolPtr res = new CanvasTool();
    res->optionsWidget_ = res->createOptionsWidget();
    return res;
}

CanvasTool::CanvasTool()
    : Widget() {

    // Set Click+Sticky focus policy by default, so that:
    // - clicking on the canvas gives the keyboard focus to the tool, and
    // - the tool keeps the keyboard when clicking on widgets that don't
    //   accept focus, such as a button.
    //
    // For example, sketching gives the focus to the sketch tool, and clicking
    // on a button keeps the focus on the sketch tool.
    //
    // When clicking on a tool button to switch tools, the tool manager can
    // then query if the current tool has the focus, in which case it transfers
    // the focus to the new tool.
    //
    // Note: in the future, it's not clear that all tools (or any) really need
    // the Click focus policy. It is currently necessary because a lot of the
    // Canvas shortcuts (e.g., T to show the tessellation) are implemented
    // directly in Canvas::onKeyPress(). Instead, these might be registered
    // Action with ShortcutContext::Window (or ShortcutContext::Application),
    // so that they can be trigerred with their shortcut even if the canvas
    // doesn't have the focus.
    //
    setFocusPolicy(FocusPolicy::Click | FocusPolicy::Sticky);

    // Enable clipping, so that tools don't draw outside the canvas.
    //
    setClippingEnabled(true);
}

ui::Widget* CanvasTool::optionsWidget() const {
    if (!optionsWidget_) {
        // Create options widget if not already created.
        // Note that we cannot do this in the constructor, since the
        // virtual method would not call the derived implementation,
        // which is why we defer creating the options widget until here.
        optionsWidget_ = createOptionsWidget();
    }
    return optionsWidget_.get();
}

ui::WidgetPtr CanvasTool::createOptionsWidget() const {
    return ui::Widget::create();
}

// Reimplementation of Widget virtual methods

void CanvasTool::onParentWidgetChanged(Widget* newParent) {
    canvas_ = dynamic_cast<ui::Canvas*>(newParent);
}

void CanvasTool::preMouseMove(MouseEvent* event) {
    if (event->isTablet()) {
        if (pressedMouseButtons_) {
            event->stopPropagation();
        }
    }
    else {
        if (pressedTabletButtons_) {
            event->stopPropagation();
        }
    }
}

void CanvasTool::preMousePress(MouseEvent* event) {
    if (event->isTablet()) {
        if (pressedMouseButtons_) {
            event->stopPropagation();
        }
        else {
            pressedTabletButtons_.set(event->button());
        }
    }
    else {
        if (pressedTabletButtons_) {
            event->stopPropagation();
        }
        else {
            pressedMouseButtons_.set(event->button());
        }
    }
}

void CanvasTool::preMouseRelease(MouseEvent* event) {
    if (event->isTablet()) {
        if (!pressedTabletButtons_.has(event->button())) {
            event->stopPropagation();
        }
        else {
            pressedTabletButtons_.unset(event->button());
        }
    }
    else {
        if (!pressedMouseButtons_.has(event->button())) {
            event->stopPropagation();
        }
        else {
            pressedMouseButtons_.unset(event->button());
        }
    }
}

geometry::Vec2f CanvasTool::computePreferredSize() const {
    return geometry::Vec2f(0, 0);
}

} // namespace vgc::ui
