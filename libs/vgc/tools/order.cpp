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

#include <vgc/tools/order.h>

#include <algorithm> // max

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
using ui::modifierkeys::shift;

VGC_UI_DEFINE_WINDOW_COMMAND( //
    bringForward,
    "tools.order.bringForward",
    "Bring Forward",
    Shortcut(ctrl, Key::RightSquareBracket));

VGC_UI_DEFINE_WINDOW_COMMAND( //
    sendBackward,
    "tools.order.sendBackward",
    "Send Backward",
    Shortcut(ctrl, Key::LeftSquareBracket));

VGC_UI_DEFINE_WINDOW_COMMAND( //
    bringToFront,
    "tools.order.bringToFront",
    "Bring to Front",
    Shortcut(ctrl | shift, Key::RightSquareBracket));

VGC_UI_DEFINE_WINDOW_COMMAND( //
    sendToBack,
    "tools.order.sendToBack",
    "Send to Back",
    Shortcut(ctrl | shift, Key::LeftSquareBracket));

namespace {

// Secondary shortcuts for bring forward/backward
//
VGC_UI_ADD_DEFAULT_SHORTCUT(bringForward(), Shortcut(Key::PageUp))
VGC_UI_ADD_DEFAULT_SHORTCUT(sendBackward(), Shortcut(Key::PageDown))
VGC_UI_ADD_DEFAULT_SHORTCUT(bringToFront(), Shortcut(ctrl, Key::PageUp))
VGC_UI_ADD_DEFAULT_SHORTCUT(sendToBack(), Shortcut(ctrl, Key::PageDown))

} // namespace

} // namespace commands

OrderModule::OrderModule(CreateKey key, const ui::ModuleContext& context)
    : Module(key, context) {

    canvasManager_ = importModule<canvas::CanvasManager>();

    ui::MenuWeakPtr orderMenu;
    if (auto standardMenus = importModule<ui::StandardMenus>().lock()) {
        if (auto menuBar = standardMenus->menuBar().lock()) {
            Int index = std::max<Int>(0, menuBar->numItems() - 1);
            orderMenu = menuBar->createSubMenuAt(index, "Order");
        }
    }

    using namespace commands;
    ui::ModuleActionCreator c(this);
    c.setMenu(orderMenu);

    c.addAction(bringToFront(), onBringToFront_Slot());
    c.addAction(bringForward(), onBringForward_Slot());
    c.addAction(sendBackward(), onSendBackward_Slot());
    c.addAction(sendToBack(), onSendToBack_Slot());
}

OrderModulePtr OrderModule::create(const ui::ModuleContext& context) {
    return core::createObject<OrderModule>(context);
}

namespace {

class OrderContextLock {
public:
    OrderContextLock(
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

    ~OrderContextLock() {

        // Close history group
        if (auto undoGroup = undoGroup_.lock()) {
            undoGroup->close();
        }
    }

    explicit operator bool() const {
        return static_cast<bool>(workspace_) // implies canvas_ also true
               && !selection_.isEmpty();     // avoid doing work if nothing is selected
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

void OrderModule::onBringForward_() {
    if (auto context = OrderContextLock(canvasManager_, commands::bringForward())) {
        context.workspace()->bringForward(context.selection(), context.time());
    }
}

void OrderModule::onSendBackward_() {
    if (auto context = OrderContextLock(canvasManager_, commands::sendBackward())) {
        context.workspace()->sendBackward(context.selection(), context.time());
    }
}

void OrderModule::onBringToFront_() {
    if (auto context = OrderContextLock(canvasManager_, commands::bringToFront())) {
        context.workspace()->bringToFront(context.selection(), context.time());
    }
}

void OrderModule::onSendToBack_() {
    if (auto context = OrderContextLock(canvasManager_, commands::sendBackward())) {
        context.workspace()->sendToBack(context.selection(), context.time());
    }
}

} // namespace vgc::tools
