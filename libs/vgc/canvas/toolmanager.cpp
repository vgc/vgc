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

namespace vgc::canvas {

ToolManager::ToolManager(CreateKey key, const ui::ModuleContext& context)
    : Module(key, context) {

    // Create action group ensuring only one tool is active at a time
    toolsActionGroup_ = ui::ActionGroup::create(ui::CheckPolicy::ExactlyOne);
}

ToolManagerPtr ToolManager::create(const ui::ModuleContext& context) {
    return core::createObject<ToolManager>(context);
}

void ToolManager::setCanvas(CanvasWeakPtr canvas) {
    canvas_ = canvas;
}

void ToolManager::registerTool(
    core::StringId commandId,
    canvas::CanvasToolSharedPtr tool_) {

    auto tool = tool_.lock();
    if (!tool) {
        return;
    }

    // Prevent registering the same tool twice
    if (hasTool_(tool.get())) {
        return;
    }

    // Create tool action and add it to the action group
    ui::Action* action = createTriggerAction(commandId);
    action->setCheckable(true);
    action->checkStateChanged().connect(onToolCheckStateChanged_Slot());
    toolsActionGroup_->addAction(action);

    // Keep the CanvasTool alive by storing it as a SharedPtr and
    // remember which CanvasTool corresponds to which Action.
    tools_.append({action, tool});

    // Set first tool as current tool
    if (!currentTool_.isAlive()) {
        // ^ XXX The above would be more readable if we added to WeakPtr:
        //   - explicit bool conversion operator, or
        //   - Comparison with nullptr
        setCurrentTool(tool.get());
    }
}

void ToolManager::setCurrentTool(canvas::CanvasToolWeakPtr newCanvasTool) {
    if (newCanvasTool != currentTool_) {
        bool hadFocusedWidget = false;
        if (auto currentTool = currentTool_.lock()) {
            hadFocusedWidget = currentTool->hasFocusedWidget();
            currentTool->clearFocus(ui::FocusReason::Other);
            currentTool->reparent(nullptr);
        }
        currentTool_ = newCanvasTool;
        if (auto currentTool = currentTool_.lock()) {
            if (auto canvas = canvas_.lock()) {
                canvas->addChild(currentTool.get());
            }
            if (hadFocusedWidget) {
                currentTool->setFocus(ui::FocusReason::Other);
                // TODO: it would be even better to remember, for each tool,
                // which of its descendants was the focused widget and restore
                // this specific descendant as focused widget.
            }
            if (ui::Action* action = getActionFromTool_(currentTool.get())) {
                action->setChecked(true);
            }
        }
        currentToolChanged().emit(currentTool_);
    }
}

ui::Action* ToolManager::getActionFromTool_(canvas::CanvasTool* tool) const {
    auto res = tools_.search([tool](const RegisteredTool_& rt) { //
        return rt.tool.get() == tool;
    });
    return res ? res->action : nullptr;
}

canvas::CanvasToolWeakPtr ToolManager::getToolFromAction_(ui::Action* action) const {
    auto res = tools_.search([action](const RegisteredTool_& rt) { //
        return rt.action == action;
    });
    return res ? res->tool : nullptr;
}

bool ToolManager::hasTool_(canvas::CanvasTool* tool) {
    return getActionFromTool_(tool) != nullptr;
}

bool ToolManager::hasAction_(ui::Action* action) {
    return getToolFromAction_(action).isAlive();
}

void ToolManager::onToolCheckStateChanged_(
    ui::Action* toolAction,
    ui::CheckState checkState) {

    if (checkState == ui::CheckState::Checked) {
        canvas::CanvasToolWeakPtr tool = getToolFromAction_(toolAction);
        setCurrentTool(tool);
    }
}

} // namespace vgc::canvas
