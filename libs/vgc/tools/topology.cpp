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

#include <vgc/canvas/documentmanager.h>
#include <vgc/canvas/workspaceselection.h>
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
    softDelete,
    "tools.topology.softDelete",
    "Soft Delete",
    Shortcut(Key::Backspace));

VGC_UI_DEFINE_WINDOW_COMMAND( //
    hardDelete,
    "tools.topology.hardDelete",
    "Hard Delete",
    Shortcut(ctrl, Key::Backspace));

namespace {

// Secondary shortcuts
//
VGC_UI_ADD_DEFAULT_SHORTCUT(softDelete(), Shortcut(Key::Delete))
VGC_UI_ADD_DEFAULT_SHORTCUT(hardDelete(), Shortcut(ctrl, Key::Delete))

} // namespace

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

    documentManager_ = importModule<canvas::DocumentManager>();

    ui::MenuWeakPtr editMenu;
    ui::MenuWeakPtr topologyMenu;
    if (auto standardMenus = importModule<ui::StandardMenus>().lock()) {
        editMenu = standardMenus->getOrCreateEditMenu();
        if (auto menuBar = standardMenus->menuBar().lock()) {
            Int index = std::max<Int>(0, menuBar->numItems() - 1);
            topologyMenu = menuBar->createSubMenuAt(index, "Topology");
        }
    }

    using namespace commands;
    ui::ModuleActionCreator c(this);

    c.setMenu(editMenu);
    c.addSeparator();
    c.addAction(softDelete(), onSoftDelete_Slot());
    c.addAction(hardDelete(), onHardDelete_Slot());

    c.setMenu(topologyMenu);
    c.addAction(glue(), onGlue_Slot());
    c.addAction(explode(), onExplode_Slot());
    c.addAction(simplify(), onSimplify_Slot());
    c.addAction(cutFaceWithEdge(), onCutFaceWithEdge_Slot());
}

TopologyModulePtr TopologyModule::create(const ui::ModuleContext& context) {
    return core::createObject<TopologyModule>(context);
}

namespace {

class TopologyContextLock {
public:
    TopologyContextLock(
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

    ~TopologyContextLock() {

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

void TopologyModule::onSoftDelete_() {
    if (auto context = TopologyContextLock(documentManager_, commands::softDelete())) {
        context.workspace()->softDelete(context.itemIds());
        context.workspaceSelection()->clear();
    }
}

void TopologyModule::onHardDelete_() {
    if (auto context = TopologyContextLock(documentManager_, commands::hardDelete())) {
        context.workspace()->hardDelete(context.itemIds());
        context.workspaceSelection()->clear();
    }
}

void TopologyModule::onGlue_() {
    if (auto context = TopologyContextLock(documentManager_, commands::glue())) {
        core::Id gluedId = context.workspace()->glue(context.itemIds());
        if (gluedId >= 0) {
            context.workspaceSelection()->setItemIds(std::array{gluedId});
        }
    }
}

void TopologyModule::onExplode_() {
    if (auto context = TopologyContextLock(documentManager_, commands::explode())) {
        core::Array<core::Id> ungluedIds = context.workspace()->unglue(context.itemIds());
        context.workspaceSelection()->setItemIds(std::move(ungluedIds));
    }
}

void TopologyModule::onSimplify_() {
    if (auto context = TopologyContextLock(documentManager_, commands::simplify())) {
        bool smoothJoins = false;
        core::Array<core::Id> uncutIds =
            context.workspace()->simplify(context.itemIds(), smoothJoins);
        context.workspaceSelection()->setItemIds(std::move(uncutIds));
    }
}

void TopologyModule::onCutFaceWithEdge_() {
    if (auto context =
            TopologyContextLock(documentManager_, commands::cutFaceWithEdge())) {
        bool success = context.workspace()->cutGlueFace(context.itemIds());
        if (success) {
            context.workspaceSelection()->clear();
        }
    }
}

} // namespace vgc::tools
