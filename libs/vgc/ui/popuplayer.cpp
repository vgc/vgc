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

#include <vgc/ui/popuplayer.h>

#include <vgc/ui/menu.h>
#include <vgc/ui/overlayarea.h>

namespace vgc::ui {

PopupLayer::PopupLayer(Widget* underlyingWidget)
    : underlyingWidget_(underlyingWidget) {

    if (underlyingWidget_) {
        underlyingWidget_->aboutToBeDestroyed().connect(
            onUnderlyingWidgetAboutToBeDestroyedSlot_());
    }
}

PopupLayerPtr PopupLayer::create() {
    return PopupLayerPtr(new PopupLayer(nullptr));
}

PopupLayerPtr PopupLayer::create(Widget* underlyingWidget) {
    return PopupLayerPtr(new PopupLayer(underlyingWidget));
}

void PopupLayer::onWidgetAdded(Widget* child, bool /*wasOnlyReordered*/) {
    child->updateGeometry(0, 0, 0, 0);
}

void PopupLayer::onResize() {
    SuperClass::onResize();
    resized().emit();
}

Widget* PopupLayer::computeHoverChainChild(const geometry::Vec2f& position) const {
    if (underlyingWidget_) {
        geometry::Vec2f posInTarget = mapTo(underlyingWidget_, position);
        if (underlyingWidget_->rect().contains(posInTarget)) {
            return underlyingWidget_;
        }
    }
    return nullptr;
}

bool PopupLayer::onMousePress(MouseEvent* event) {
    bool handled = Widget::onMousePress(event);
    if (!handled && !hoverChainChild()) {
        backgroundPressed().emit();
    }
    return handled;
}

} // namespace vgc::ui
