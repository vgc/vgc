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

PopupLayer::PopupLayer(CreateKey key, WidgetWeakPtr passthrough)
    : Widget(key)
    , passthrough_(passthrough) {
}

PopupLayerPtr PopupLayer::create() {
    return core::createObject<PopupLayer>(nullptr);
}

PopupLayerPtr PopupLayer::create(WidgetWeakPtr passthrough) {
    return core::createObject<PopupLayer>(passthrough);
}

void PopupLayer::setPassthrough(WidgetWeakPtr passthrough) {
    passthrough_ = passthrough;
}

Widget* PopupLayer::computeHoverChainChild(MouseHoverEvent* event) const {
    if (auto passthrough = passthrough_.lock()) {
        geometry::Vec2f posInTarget = mapTo(passthrough.get(), event->position());
        if (passthrough->rect().contains(posInTarget)) {
            return passthrough.get();
        }
    }
    return nullptr;
}

bool PopupLayer::onMousePress(MousePressEvent* event) {
    bool handled = Widget::onMousePress(event);
    if (!handled && !hoverChainChild()) {
        clicked().emit();
    }
    return handled;
}

} // namespace vgc::ui
