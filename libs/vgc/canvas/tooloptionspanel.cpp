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

#include <vgc/canvas/tooloptionspanel.h>

#include <vgc/canvas/toolmanager.h>
#include <vgc/ui/panelcontext.h>

namespace vgc::canvas {

namespace {

core::StringId s_with_padding("with-padding");
core::StringId s_tool_options("tool-options");

} // namespace

ToolOptionsPanel::ToolOptionsPanel(CreateKey key, const ui::PanelContext& context)
    : Panel(key, context, label) {

    if (auto toolManager = context.importModule<ToolManager>().lock()) {
        toolManager->currentToolChanged().connect(onCurrentToolChanged_Slot());
        onCurrentToolChanged_(toolManager->currentTool());
    }

    addStyleClass(s_with_padding);
    addStyleClass(s_tool_options);
}

ToolOptionsPanelPtr ToolOptionsPanel::create(const ui::PanelContext& context) {
    return core::createObject<ToolOptionsPanel>(context);
}

void ToolOptionsPanel::onCurrentToolChanged_(CanvasToolWeakPtr tool_) {
    if (auto tool = tool_.lock()) {
        ui::WidgetPtr optionsWidget = tool->createOptionsWidget();
        setBody(optionsWidget.get());
    }
    else {
        setBody(nullptr);
    }
}

} // namespace vgc::canvas
