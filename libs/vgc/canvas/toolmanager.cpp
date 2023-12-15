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

#include <vgc/canvas/toolmanager.h>

#include <vgc/ui/button.h>
#include <vgc/ui/panelarea.h>
#include <vgc/ui/panelmanager.h>
#include <vgc/ui/row.h>

namespace vgc::canvas {

ToolManager::ToolManager(CreateKey key, const ui::ModuleContext& context)
    : Module(key, context) {

    // Create action group ensuring only one tool is active at a time
    toolsActionGroup_ = ui::ActionGroup::create(ui::CheckPolicy::ExactlyOne);
}

ToolManagerPtr ToolManager::create(const ui::ModuleContext& context) {
    return core::createObject<ToolManager>(context);
}

void ToolManager::setCanvas(Canvas* canvas) {
    canvas_ = canvas;
}

void ToolManager::registerTool(core::StringId commandId, canvas::CanvasToolPtr tool) {

    // Create tool action and add it to the action group
    ui::Action* action = createTriggerAction(commandId);
    action->setCheckable(true);
    action->checkStateChanged().connect(onToolCheckStateChanged_Slot());
    toolsActionGroup_->addAction(action);

    // Keeps the CanvasTool alive by storing it as an ObjPtr and
    // remembers which CanvasTool corresponds to which tool action.
    toolMap_[action] = tool;
    toolMapInv_[tool.get()] = action;

    // Set first tool as current tool
    if (!currentTool()) {
        setCurrentTool(tool.get());
    }
}

void ToolManager::setCurrentTool(canvas::CanvasTool* canvasTool) {

    if (canvasTool != currentTool_) {
        bool hadFocusedWidget = false;
        if (currentTool_) {
            hadFocusedWidget = currentTool_->hasFocusedWidget();
            currentTool_->clearFocus(ui::FocusReason::Other);
            currentTool_->reparent(nullptr);
        }
        currentTool_ = canvasTool;
        if (currentTool_) {
            canvas_->addChild(canvasTool);
            if (hadFocusedWidget) {
                currentTool_->setFocus(ui::FocusReason::Other);
                // TODO: it would be even better to remember, for each tool,
                // which of its descendants was the focused widget and restore
                // this specific descendant as focused widget.
            }
            toolMapInv_[canvasTool]->setChecked(true);
        }
        currentToolChanged().emit(currentTool_);
    }
}

ui::Panel*
ToolManager::createToolsPanel(ui::PanelManager* panelManager, ui::PanelArea* panelArea) {
    ui::Panel* panel = panelManager->createPanelInstance_<ui::Panel>(panelArea, "Tools");
    ui::Row* row = panel->createChild<ui::Row>();
    // TODO: iterate with a different order than the map order, to ensure the tools
    //       appear in a specific, deterministic order.
    for (const auto& pair : toolMap_) {
        ui::Action* action = pair.first;
        ui::Button* button = row->createChild<ui::Button>(action);
        button->setIconVisible(true);
        button->setTextVisible(false);
    }
    return panel;
}

void ToolManager::onToolCheckStateChanged_(
    ui::Action* toolAction,
    ui::CheckState checkState) {

    if (checkState == ui::CheckState::Checked) {
        canvas::CanvasTool* canvasTool = toolMap_[toolAction].get();
        setCurrentTool(canvasTool);
    }
}

} // namespace vgc::canvas
