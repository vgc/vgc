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

#include <vgc/tools/topology.h>

#include <algorithm> // max

#include <vgc/canvas/canvas.h>
#include <vgc/canvas/canvasmanager.h>
#include <vgc/ui/menu.h>
#include <vgc/ui/standardmenus.h>
#include <vgc/workspace/workspace.h>

namespace vgc::tools {

namespace commands {

using ui::Key;
using ui::MouseButton;
using ui::Shortcut;
using ui::modifierkeys::alt;
using ui::modifierkeys::ctrl;

VGC_UI_DEFINE_WINDOW_COMMAND( //
    glue,
    "tools.topology.glue",
    "Glue",
    Shortcut(alt, Key::G));

VGC_UI_DEFINE_WINDOW_COMMAND( //
    explode,
    "tools.topology.explode",
    "Explode",
    Shortcut(alt, Key::E));

VGC_UI_DEFINE_WINDOW_COMMAND( //
    simplify,
    "tools.topology.simplify",
    "Simplify",
    Shortcut(alt, Key::S));

VGC_UI_DEFINE_WINDOW_COMMAND( //
    cutFaceWithEdge,
    "tools.topology.cutFaceWithEdge",
    "Cut Face With Edge",
    Shortcut(alt, Key::C));

VGC_UI_DEFINE_MOUSE_CLICK_COMMAND( //
    cutWithVertex,
    "tools.topology.cutWithVertex",
    "Cut with Vertex",
    Shortcut(ctrl, MouseButton::Right));

} // namespace commands

TopologyModule::TopologyModule(CreateKey key, const ui::ModuleContext& context)
    : Module(key, context) {

    canvasManager_ = importModule<canvas::CanvasManager>();

    ui::MenuWeakPtr topologyMenu_;
    if (auto standardMenus = importModule<ui::StandardMenus>().lock()) {
        if (auto menuBar = standardMenus->menuBar().lock()) {
            Int index = std::max<Int>(0, menuBar->numItems() - 1);
            topologyMenu_ = menuBar->createSubMenuAt(index, "Topology");
        }
    }

    auto createAction = [this, topologyMenu_](core::StringId commandName) {
        ui::Action* action = this->createTriggerAction(commandName);
        if (auto topologyMenu = topologyMenu_.lock()) {
            topologyMenu->addItem(action);
        }
        return action;
    };

    ui::Action* glueAction = createAction(commands::glue());
    glueAction->triggered().connect(onGlue_Slot());

    ui::Action* explodeAction = createAction(commands::explode());
    explodeAction->triggered().connect(onExplode_Slot());

    ui::Action* simplifyAction = createAction(commands::simplify());
    simplifyAction->triggered().connect(onSimplify_Slot());

    ui::Action* cutFaceWithEdgeAction = createAction(commands::cutFaceWithEdge());
    cutFaceWithEdgeAction->triggered().connect(onCutFaceWithEdge_Slot());
}

TopologyModulePtr TopologyModule::create(const ui::ModuleContext& context) {
    return core::createObject<TopologyModule>(context);
}

namespace {

class TopologyContextLock {
public:
    TopologyContextLock(
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

    ~TopologyContextLock() {

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

void TopologyModule::onGlue_() {
    if (auto context = TopologyContextLock(canvasManager_, commands::glue())) {
        core::Id gluedId = context.workspace()->glue(context.selection());
        if (gluedId >= 0) {
            context.canvas()->setSelection(std::array{gluedId});
        }
    }
}

void TopologyModule::onExplode_() {
    if (auto context = TopologyContextLock(canvasManager_, commands::explode())) {
        core::Array<core::Id> ungluedIds =
            context.workspace()->unglue(context.selection());
        context.canvas()->setSelection(std::move(ungluedIds));
    }
}

void TopologyModule::onSimplify_() {
    if (auto context = TopologyContextLock(canvasManager_, commands::simplify())) {
        bool smoothJoins = false;
        core::Array<core::Id> uncutIds =
            context.workspace()->simplify(context.selection(), smoothJoins);
        context.canvas()->setSelection(std::move(uncutIds));
    }
}

void TopologyModule::onCutFaceWithEdge_() {
    if (auto context = TopologyContextLock(canvasManager_, commands::cutFaceWithEdge())) {
        bool success = context.workspace()->cutGlueFace(context.selection());
        if (success) {
            context.canvas()->clearSelection();
        }
    }
}

} // namespace vgc::tools
