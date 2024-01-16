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

#include <vgc/canvas/canvasmanager.h>

#include <vgc/canvas/canvas.h>
#include <vgc/canvas/documentmanager.h>
#include <vgc/ui/menu.h>
#include <vgc/ui/standardmenus.h>

namespace vgc::canvas {

namespace commands {

using ui::Key;
using ui::Shortcut;
using ui::modifierkeys::shift;

VGC_UI_DEFINE_WINDOW_COMMAND( //
    switchToNormalDisplayMode,
    "tools.canvas.switchToNormalDisplayMode",
    "Display Mode: Normal",
    Shortcut());

VGC_UI_DEFINE_WINDOW_COMMAND( //
    switchToOutlineOverlayDisplayMode,
    "tools.canvas.switchToOutlineOverlayDisplayMode",
    "Display Mode: Outline Overlay",
    Shortcut());

VGC_UI_DEFINE_WINDOW_COMMAND( //
    switchToOutlineOnlyDisplayMode,
    "tools.canvas.switchToOutlineOnlyDisplayMode",
    "Display Mode: Outline Only",
    Shortcut());

VGC_UI_DEFINE_WINDOW_COMMAND( //
    toggleLastTwoDisplayModes,
    "tools.canvas.toggleLastTwoDisplayModes",
    "Toggle Last Two Display Modes",
    Shortcut(Key::D));

VGC_UI_DEFINE_WINDOW_COMMAND( //
    cycleDisplayModes,
    "tools.canvas.cycleDisplayModes",
    "Cycle Display Modes",
    Shortcut(shift, Key::D));

} // namespace commands

namespace {

struct ActionHelper {
public:
    ActionHelper(ui::Module& module, ui::MenuWeakPtr menu)
        : module_(module)
        , menu_(menu) {
    }

    template<typename Slot>
    ui::Action* addAction(core::StringId commandName, Slot slot) {
        ui::Action* action = module_.createTriggerAction(commandName);
        if (auto menu = menu_.lock()) {
            menu->addItem(action);
        }
        action->triggered().connect(slot);
        return action;
    }

    void addSeparator() {
        if (auto menu = menu_.lock()) {
            menu->addSeparator();
        }
    }

private:
    ui::Module& module_;
    ui::MenuWeakPtr menu_;
};

} // namespace

CanvasManager::CanvasManager(CreateKey key, const ui::ModuleContext& context)
    : Module(key, context) {

    documentManager_ = context.importModule<DocumentManager>();
    if (auto documentManager = documentManager_.lock()) {
        documentManager->currentWorkspaceChanged().connect(
            onCurrentWorkspaceChanged_Slot());
    }

    ui::MenuWeakPtr viewMenu;
    if (auto standardMenus = importModule<ui::StandardMenus>().lock()) {
        viewMenu = standardMenus->getOrCreateViewMenu();
    }

    using namespace commands;
    ActionHelper h(*this, viewMenu);

    h.addAction(switchToNormalDisplayMode(), onSwitchToNormalDisplayMode_Slot());
    h.addAction(
        switchToOutlineOverlayDisplayMode(), onSwitchToOutlineOverlayDisplayMode_Slot());
    h.addAction(
        switchToOutlineOnlyDisplayMode(), onSwitchToOutlineOnlyDisplayMode_Slot());
    h.addSeparator();

    h.addAction(toggleLastTwoDisplayModes(), onToggleLastTwoDisplayModes_Slot());
    h.addAction(cycleDisplayModes(), onCycleDisplayModes_Slot());
    h.addSeparator();
}

CanvasManagerPtr CanvasManager::create(const ui::ModuleContext& context) {
    return core::createObject<CanvasManager>(context);
}

namespace {

void setCanvasWorkspace(
    CanvasWeakPtr& canvas_,
    DocumentManagerWeakPtr& documentManager_) {

    if (auto canvas = canvas_.lock()) {
        if (auto documentManager = documentManager_.lock()) {
            canvas->setWorkspace(documentManager->currentWorkspace());
        }
    }
}

} // namespace

void CanvasManager::setActiveCanvas(CanvasWeakPtr canvas) {
    if (activeCanvas_ == canvas) {
        return;
    }
    activeCanvas_ = canvas;
    setCanvasWorkspace(activeCanvas_, documentManager_);
}

void CanvasManager::onCurrentWorkspaceChanged_() {
    setCanvasWorkspace(activeCanvas_, documentManager_);
}

void CanvasManager::switchToDisplayMode_(CanvasWeakPtr canvas_, DisplayMode mode) {
    if (auto canvas = canvas_.lock()) {
        DisplayMode previousDisplayMode = canvas->displayMode();
        if (previousDisplayMode != mode) {
            previousDisplayModes_[canvas_] = previousDisplayMode;
            canvas->setDisplayMode(mode);
        }
    }
}

void CanvasManager::onSwitchToNormalDisplayMode_() {
    switchToDisplayMode_(activeCanvas(), DisplayMode::Normal);
}

void CanvasManager::onSwitchToOutlineOverlayDisplayMode_() {
    switchToDisplayMode_(activeCanvas(), DisplayMode::OutlineOverlay);
}

void CanvasManager::onSwitchToOutlineOnlyDisplayMode_() {
    switchToDisplayMode_(activeCanvas(), DisplayMode::OutlineOnly);
}

void CanvasManager::onToggleLastTwoDisplayModes_() {
    CanvasWeakPtr canvas = activeCanvas();
    DisplayMode mode = defaultSecondDisplayMode;
    auto [it, inserted] = previousDisplayModes_.try_emplace(canvas, mode);
    if (!inserted) {
        mode = it->second;
    }
    switchToDisplayMode_(canvas, mode);
}

void CanvasManager::onCycleDisplayModes_() {
    if (auto canvas = activeCanvas().lock()) {
        switch (canvas->displayMode()) {
        case DisplayMode::Normal:
            switchToDisplayMode_(canvas, DisplayMode::OutlineOverlay);
            break;
        case DisplayMode::OutlineOverlay:
            switchToDisplayMode_(canvas, DisplayMode::OutlineOnly);
            break;
        case DisplayMode::OutlineOnly:
            switchToDisplayMode_(canvas, DisplayMode::Normal);
            break;
        }
    }
}

} // namespace vgc::canvas
