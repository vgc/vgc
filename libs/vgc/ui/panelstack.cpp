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

#include <vgc/ui/panelstack.h>

namespace vgc::ui {

PanelStack::PanelStack()
    : Widget() {

    addStyleClass(strings::PanelStack);
}

PanelStackPtr PanelStack::create() {
    return PanelStackPtr(new PanelStack());
}

void PanelStack::updateChildrenGeometry() {
    Panel* activePanel_ = activePanel();
    if (activePanel_) {
        activePanel_->updateGeometry(contentRect());
    }
}

void PanelStack::onWidgetAdded(Widget* child, bool) {
    Panel* panel = dynamic_cast<Panel*>(child);
    if (!panel) {
        throw LogicError(core::format(
            "Cannot add {} as child of {} : only widgets of type Panel are allowed.",
            ptr(child),
            ptr(this)));
    }
};

} // namespace vgc::ui
