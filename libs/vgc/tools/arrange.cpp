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

#include <vgc/canvas/canvas.h>
#include <vgc/canvas/canvasmanager.h>
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
    bringBackward,
    "tools.arrange.bringBackward",
    "Bring Backward",
    Shortcut(ctrl, Key::LeftSquareBracket));

namespace {

// Secondary shortcuts for bring forward/backward
//
VGC_UI_ADD_DEFAULT_SHORTCUT(bringForward(), Shortcut(Key::PageUp))
VGC_UI_ADD_DEFAULT_SHORTCUT(bringBackward(), Shortcut(Key::PageDown))

} // namespace

} // namespace commands

ArrangeModule::ArrangeModule(CreateKey key, const ui::ModuleContext& context)
    : Module(key, context) {

    canvasManager_ = importModule<canvas::CanvasManager>();

    ui::Action* bringForwardAction = createTriggerAction(commands::bringForward());
    bringForwardAction->triggered().connect(onBringForward_Slot());

    ui::Action* bringBackwardAction = createTriggerAction(commands::bringBackward());
    bringBackwardAction->triggered().connect(onBringBackward_Slot());
}

ArrangeModulePtr ArrangeModule::create(const ui::ModuleContext& context) {
    return core::createObject<ArrangeModule>(context);
}

namespace {

struct ArrangeContextLock {
    canvas::CanvasLockPtr canvas;
    workspace::WorkspaceLockPtr workspace;

    ArrangeContextLock(
        canvas::CanvasManagerWeakPtr canvasManager_,
        core::StringId commandName) {

        // Get canvas and workspace lock
        if (auto canvasManager = canvasManager_.lock()) {
            canvas = canvasManager->activeCanvas().lock();
            if (canvas) {
                workspace::WorkspaceWeakPtr workspace_ = canvas->workspace();
                workspace = workspace_.lock();
            }
        }

        // Open history group
        if (workspace) {
            core::History* history = workspace->history();
            if (history) {
                undoGroup_ = history->createUndoGroup(commandName);
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
        return static_cast<bool>(workspace);
    }

private:
    core::UndoGroupWeakPtr undoGroup_;
};

} // namespace

void ArrangeModule::onBringForward_() {
    if (auto context = ArrangeContextLock(canvasManager_, commands::bringForward())) {
        core::Array<core::Id> selection = context.canvas->selection();
        context.workspace->raise(selection, context.canvas->currentTime());
    }
}

void ArrangeModule::onBringBackward_() {
    if (auto context = ArrangeContextLock(canvasManager_, commands::bringForward())) {
        core::Array<core::Id> selection = context.canvas->selection();
        context.workspace->lower(selection, context.canvas->currentTime());
    }
}

} // namespace vgc::tools
