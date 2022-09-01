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

void OverlayArea::setAreaWidget(Widget* w) {
    Widget* oldAreaWidget = areaWidget_;
    areaWidget_ = w;
    if (oldAreaWidget) {
        w->replace(oldAreaWidget);
    }
    else {
        addChild(w);
    }
}

void OverlayArea::addOverlayWidget(Widget* w) {
    if (!areaWidget_) {
        WidgetPtr placeholder = Widget::create();
        areaWidget_ = placeholder.get();
        addChild(placeholder.get());
    }
    addChild(w);
}

void OverlayArea::onWidgetAdded(Widget* w) {
    if (w == areaWidget_) {
        requestGeometryUpdate();
    }
}

void OverlayArea::onWidgetRemoved(Widget* w) {
    if (w == areaWidget_) {
        areaWidget_ = nullptr;
    }
    requestGeometryUpdate();
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
    if (areaWidget_) {
        areaWidget_->updateGeometry(
            geometry::Vec2f(), geometry::Vec2f(width(), height()));
    }
}

} // namespace vgc::ui
