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

#include <vgc/canvas/toolspanel.h>

#include <vgc/canvas/toolmanager.h>
#include <vgc/ui/button.h>
#include <vgc/ui/panelcontext.h>
#include <vgc/ui/row.h>

namespace vgc::canvas {

namespace {

core::StringId s_with_padding("with-padding");
core::StringId s_tools("tools");

} // namespace

ToolsPanel::ToolsPanel(CreateKey key, const ui::PanelContext& context)
    : Panel(key, context, label) {

    // Layout to which each tool button is added.
    //
    ui::Row* row = createChild<ui::Row>();

    // Iterate over all registered tools and add each one as a button.
    //
    // TODO: iterate with a different order than the map order, to ensure the
    // tools appear in a specific, deterministic order.
    //
    if (auto toolManager = context.importModule<ToolManager>().lock()) {
        for (const auto& pair : toolManager->toolMap_) {
            ui::Action* action = pair.first;
            ui::Button* button = row->createChild<ui::Button>(action);
            button->setIconVisible(true);
            button->setTextVisible(false);
        }
    }

    addStyleClass(s_with_padding);
    addStyleClass(s_tools);
}

ToolsPanelPtr ToolsPanel::create(const ui::PanelContext& context) {
    return core::createObject<ToolsPanel>(context);
}

} // namespace vgc::canvas
