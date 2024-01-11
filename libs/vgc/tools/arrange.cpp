// Copyright 2024 The VGC Developers
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

#include <vgc/tools/arrange.h>

#include <algorithm> // min

#include <vgc/canvas/canvas.h>
#include <vgc/canvas/canvasmanager.h>
#include <vgc/ui/menu.h>
#include <vgc/ui/standardmenus.h>
#include <vgc/workspace/workspace.h>

namespace vgc::tools {

namespace commands {

using ui::Key;
using ui::Shortcut;
using ui::modifierkeys::ctrl;

VGC_UI_DEFINE_WINDOW_COMMAND( //
    bringForward,
    "tools.arrange.bringForward",
    "Bring Forward",
    Shortcut(ctrl, Key::RightSquareBracket));

VGC_UI_DEFINE_WINDOW_COMMAND( //
    sendBackward,
    "tools.arrange.sendBackward",
    "Send Backward",
    Shortcut(ctrl, Key::LeftSquareBracket));

namespace {

// Secondary shortcuts for bring forward/backward
//
VGC_UI_ADD_DEFAULT_SHORTCUT(bringForward(), Shortcut(Key::PageUp))
VGC_UI_ADD_DEFAULT_SHORTCUT(sendBackward(), Shortcut(Key::PageDown))

} // namespace

} // namespace commands

ArrangeModule::ArrangeModule(CreateKey key, const ui::ModuleContext& context)
    : Module(key, context) {

    canvasManager_ = importModule<canvas::CanvasManager>();

    ui::MenuWeakPtr arrangeMenu_;
    if (auto standardMenus = importModule<ui::StandardMenus>().lock()) {
        if (auto menuBar = standardMenus->menuBar().lock()) {
            Int index = std::min<Int>(2, menuBar->numItems());
            arrangeMenu_ = menuBar->createSubMenuAt(index, "Arrange");
        }
    }

    auto createAction = [this, arrangeMenu_](core::StringId commandName) {
        ui::Action* action = this->createTriggerAction(commandName);
        if (auto arrangeMenu = arrangeMenu_.lock()) {
            arrangeMenu->addItem(action);
        }
        return action;
    };

    ui::Action* bringForwardAction = createAction(commands::bringForward());
    bringForwardAction->triggered().connect(onBringForward_Slot());

    ui::Action* sendBackwardAction = createAction(commands::sendBackward());
    sendBackwardAction->triggered().connect(onSendBackward_Slot());
}

ArrangeModulePtr ArrangeModule::create(const ui::ModuleContext& context) {
    return core::createObject<ArrangeModule>(context);
}

namespace {

class ArrangeContextLock {
public:
    ArrangeContextLock(
        canvas::CanvasManagerWeakPtr canvasManager_,
        core::StringId commandName) {

        // Get canvas and workspace lock, then init if locks acquired
        if (auto canvasManager = canvasManager_.lock()) {
            canvas_ = canvasManager->activeCanvas().lock();
            if (canvas_) {
                workspace::WorkspaceWeakPtr workspaceWeak = canvas_->workspace();
                workspace_ = workspaceWeak.lock();
                if (workspace_) {

                    // Open history group
                    core::History* history = workspace_->history();
                    if (history) {
                        undoGroup_ = history->createUndoGroup(commandName);
                    }

                    // Get required data
                    selection_ = canvas_->selection();
                    time_ = canvas_->currentTime();
                }
            }
        }
    }

    ~ArrangeContextLock() {

        // Close history group
        if (auto undoGroup = undoGroup_.lock()) {
            undoGroup->close();
        }
    }

    explicit operator bool() const {
        return static_cast<bool>(workspace_); // implies canvas_ also true
    }

    canvas::Canvas* canvas() const {
        return canvas_.get();
    }

    workspace::Workspace* workspace() const {
        return workspace_.get();
    }

    const core::Array<core::Id>& selection() const {
        return selection_;
    }

    core::AnimTime time() const {
        return time_;
    }

private:
    canvas::CanvasLockPtr canvas_;
    workspace::WorkspaceLockPtr workspace_;

    core::Array<core::Id> selection_;
    core::AnimTime time_;

    core::UndoGroupWeakPtr undoGroup_;
};

} // namespace

void ArrangeModule::onBringForward_() {
    if (auto context = ArrangeContextLock(canvasManager_, commands::bringForward())) {
        context.workspace()->bringForward(context.selection(), context.time());
    }
}

void ArrangeModule::onSendBackward_() {
    if (auto context = ArrangeContextLock(canvasManager_, commands::sendBackward())) {
        context.workspace()->sendBackward(context.selection(), context.time());
    }
}

} // namespace vgc::tools
