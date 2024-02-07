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

#include <vgc/canvas/documentmanager.h>
#include <vgc/canvas/workspaceselection.h>
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

    documentManager_ = importModule<canvas::DocumentManager>();

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
        canvas::DocumentManagerWeakPtr documentManager_,
        core::StringId commandName) {

        // TODO: get the current time from some TimeManager module
        time_ = core::AnimTime();

        // Acquires locks and initialize
        if (auto documentManager = documentManager_.lock()) {
            workspace_ = documentManager->currentWorkspace().lock();
            if (workspace_) {
                workspaceSelection_ = documentManager->currentWorkspaceSelection().lock();
                if (workspaceSelection_) {
                    itemIds_ = workspaceSelection_->itemIds();
                    if (!itemIds_.isEmpty()) {

                        // Open history group
                        if (core::History* history = workspace_->history()) {
                            undoGroup_ = history->createUndoGroup(commandName);
                        }
                    }
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

    // Returns whether all locks are acquired and the selection is non-empty.
    //
    explicit operator bool() const {
        return !itemIds_.isEmpty();
    }

    workspace::Workspace* workspace() const {
        return workspace_.get();
    }

    canvas::WorkspaceSelection* workspaceSelection() const {
        return workspaceSelection_.get();
    }

    const core::Array<core::Id>& itemIds() const {
        return itemIds_;
    }

    core::AnimTime time() const {
        return time_;
    }

private:
    workspace::WorkspaceLockPtr workspace_;
    canvas::WorkspaceSelectionLockPtr workspaceSelection_;
    core::UndoGroupWeakPtr undoGroup_;

    core::Array<core::Id> itemIds_;
    core::AnimTime time_;
};

} // namespace

void OrderModule::onBringForward_() {
    if (auto context = OrderContextLock(documentManager_, commands::bringForward())) {
        context.workspace()->bringForward(context.itemIds(), context.time());
    }
}

void OrderModule::onSendBackward_() {
    if (auto context = OrderContextLock(documentManager_, commands::sendBackward())) {
        context.workspace()->sendBackward(context.itemIds(), context.time());
    }
}

void OrderModule::onBringToFront_() {
    if (auto context = OrderContextLock(documentManager_, commands::bringToFront())) {
        context.workspace()->bringToFront(context.itemIds(), context.time());
    }
}

void OrderModule::onSendToBack_() {
    if (auto context = OrderContextLock(documentManager_, commands::sendBackward())) {
        context.workspace()->sendToBack(context.itemIds(), context.time());
    }
}

} // namespace vgc::tools
