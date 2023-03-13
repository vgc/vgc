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
    return CanvasToolPtr(new CanvasTool());
}

CanvasTool::CanvasTool()
    : Widget() {

    setClippingEnabled(true);
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
