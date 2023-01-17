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

#include <vgc/ui/overlayarea.h>

#include <vgc/graphics/strings.h>
#include <vgc/ui/strings.h>

#include <vgc/ui/detail/paintutil.h>

namespace vgc::ui {

OverlayArea::OverlayArea()
    : Widget() {
}

OverlayAreaPtr OverlayArea::create() {
    return OverlayAreaPtr(new OverlayArea());
}

void OverlayArea::setAreaWidget(Widget* widget) {
    if (widget != areaWidget_) {
        if (areaWidget_) {
            areaWidget_->replace(widget);
        }
        areaWidget_ = widget;
    }
    insertChild(firstChild(), areaWidget_);
}

void OverlayArea::addOverlayWidget(Widget* widget, OverlayResizePolicy resizePolicy) {

    // move widget from area to overlay
    if (widget == areaWidget_) {
        areaWidget_ = nullptr;
    }

    // append in both lists
    addChild(widget);
    overlays_.removeIf([=](const OverlayDesc& od) { return od.widget() == widget; });
    overlays_.emplaceLast(widget, resizePolicy);

    switch (resizePolicy) {
    case OverlayResizePolicy::Stretch: {
        widget->updateGeometry(rect());
        break;
    }
    case OverlayResizePolicy::None:
    default:
        break;
    }

    requestRepaint();
}

void OverlayArea::onResize() {
    SuperClass::onResize();
}

void OverlayArea::onWidgetAdded(Widget* w, bool /*wasOnlyReordered*/) {
    // If area is no longer first, move to first.
    if (areaWidget_ && areaWidget_->previousSibling()) {
        insertChild(firstChild(), areaWidget_);
    }

    if (w == areaWidget_) {
        requestGeometryUpdate();
    }
    else {
        requestRepaint();
    }
}

void OverlayArea::onWidgetRemoved(Widget* w) {
    if (w == areaWidget_) {
        areaWidget_ = nullptr;
        requestGeometryUpdate();
    }
    else {
        overlays_.removeIf([=](const OverlayDesc& od) { return od.widget() == w; });
        requestRepaint();
    }
}

float OverlayArea::preferredWidthForHeight(float height) const {
    return areaWidget_ ? areaWidget_->preferredWidthForHeight(height) : 0;
}

float OverlayArea::preferredHeightForWidth(float width) const {
    return areaWidget_ ? areaWidget_->preferredHeightForWidth(width) : 0;
}

geometry::Vec2f OverlayArea::computePreferredSize() const {
    return areaWidget_ ? areaWidget_->preferredSize() : geometry::Vec2f();
}

void OverlayArea::updateChildrenGeometry() {
    geometry::Rect2f areaRect = rect();
    if (areaWidget_) {
        areaWidget_->updateGeometry(areaRect);
    }
    for (OverlayDesc& od : overlays_) {
        switch (od.resizePolicy()) {
        case OverlayResizePolicy::Stretch: {
            od.widget()->updateGeometry(areaRect);
            break;
        }
        case OverlayResizePolicy::None:
        default:
            od.widget()->updateGeometry();
            break;
        }
    }
    for (auto c : children()) {
        if (c != areaWidget_) {
            c->updateGeometry();
        }
    }
}

} // namespace vgc::ui
