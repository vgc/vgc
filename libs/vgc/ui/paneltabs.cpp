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

#include <vgc/ui/paneltabs.h>

#include <vgc/ui/panelstack.h>

namespace vgc::ui {

PanelTabs::PanelTabs(PanelStack* panels)
    : Label("temp")
    , panels_(panels) {

    addStyleClass(strings::PanelTabs);
    if (panels_) {
        panels_->aboutToBeDestroyed().connect(onPanelsDestroyedSlot_());
    }
}

PanelTabsPtr PanelTabs::create(PanelStack* stack) {
    return PanelTabsPtr(new PanelTabs(stack));
}

void PanelTabs::onPanelsDestroyed_() {
    panels_->disconnect(this);
    panels_ = nullptr;
}

} // namespace vgc::ui
