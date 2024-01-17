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
    "canvas.switchToNormalDisplayMode",
    "Display Mode: Normal",
    Shortcut());

VGC_UI_DEFINE_WINDOW_COMMAND( //
    switchToOutlineOverlayDisplayMode,
    "canvas.switchToOutlineOverlayDisplayMode",
    "Display Mode: Outline Overlay",
    Shortcut());

VGC_UI_DEFINE_WINDOW_COMMAND( //
    switchToOutlineOnlyDisplayMode,
    "canvas.switchToOutlineOnlyDisplayMode",
    "Display Mode: Outline Only",
    Shortcut());

VGC_UI_DEFINE_WINDOW_COMMAND( //
    toggleLastTwoDisplayModes,
    "canvas.toggleLastTwoDisplayModes",
    "Toggle Last Two Display Modes",
    Shortcut(Key::D));

VGC_UI_DEFINE_WINDOW_COMMAND( //
    cycleDisplayModes,
    "canvas.cycleDisplayModes",
    "Cycle Display Modes",
    Shortcut(shift, Key::D));

VGC_UI_DEFINE_WINDOW_COMMAND( //
    fitViewToSelection,
    "canvas.fitViewToSelection",
    "Fit View to Selection",
    Key::F);

VGC_UI_DEFINE_WINDOW_COMMAND( //
    fitViewToDocument,
    "canvas.fitViewToDocument",
    "Fit View to Document",
    Shortcut(shift, Key::F));

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

    h.addAction(fitViewToSelection(), onFitViewToSelection_Slot());
    h.addAction(fitViewToDocument(), onFitViewToDocument_Slot());
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

namespace {

void fitViewToRect_(Canvas& canvas, const geometry::Rect2d& rect) {

    if (rect.isDegenerate()) {
        return;
    }

    // Get current camera, viewport size, and rotation
    geometry::Camera2d camera = canvas.camera();
    geometry::Vec2d viewportSize = camera.viewportSize();
    double viewportWidth = viewportSize[0];
    double viewportHeight = viewportSize[1];
    double rotation = camera.rotation();

    // Compute new zoom, keeping a little margin around the rect
    static constexpr double marginFactor = 1.1;
    geometry::Vec2d boundingCircleDiameter(rect.width(), rect.height());
    double a = boundingCircleDiameter.length();
    double b = viewportWidth / viewportHeight;
    double z = 1;
    if (b <= 1) {
        z = viewportWidth / (a * marginFactor);
    }
    else {
        z = viewportHeight / (a * marginFactor);
    }

    // Compute new center
    geometry::Vec2d bboxCenter = 0.5 * (rect.pMin() + rect.pMax());

    // Set camera assuming no rotation
    camera.setRotation(0);
    camera.setZoom(z);
    camera.setCenter(z * bboxCenter);

    // Set rotation
    // TODO: improve Camera2d API to make this easier.
    geometry::Vec2d c0 = 0.5 * viewportSize;
    geometry::Vec2d c1 = camera.viewMatrix().inverted().transformPointAffine(c0);
    camera.setRotation(rotation);
    geometry::Vec2d c2 = camera.viewMatrix().transformPointAffine(c1);
    camera.setCenter(camera.center() - c0 + c2);

    canvas.setCamera(camera);
}

// TODO: have both const and non-const versions of Workspace::visit...() so
// that we can use a `const Workspace&` argument here.
//
void FitViewToDocument_(Canvas& canvas, workspace::Workspace& workspace) {

    // TODO: implement Workspace::boundingBox().
    geometry::Rect2d rect = geometry::Rect2d::empty;
    workspace.visitDepthFirstPreOrder( //
        [&rect](workspace::Element* e, Int /*depth*/) {
            rect.uniteWith(e->boundingBox());
        });
    fitViewToRect_(canvas, rect);
}

void FitViewToSelection_(Canvas& canvas, workspace::Workspace& workspace) {

    core::Array<core::Id> selection = canvas.selection();
    if (selection.isEmpty()) {
        FitViewToDocument_(canvas, workspace);
    }
    else {
        geometry::Rect2d rect = geometry::Rect2d::empty;
        for (core::Id id : selection) {
            workspace::Element* e = workspace.find(id);
            if (e) {
                rect.uniteWith(e->boundingBox());
            }
        }
        fitViewToRect_(canvas, rect);
    }
}

} // namespace

void CanvasManager::onFitViewToSelection_() {
    if (auto canvas = activeCanvas().lock()) {
        if (auto workspace = workspace::WorkspaceWeakPtr(canvas->workspace()).lock()) {
            FitViewToSelection_(*canvas, *workspace);
        }
    }
}

void CanvasManager::onFitViewToDocument_() {
    if (auto canvas = activeCanvas().lock()) {
        if (auto workspace = workspace::WorkspaceWeakPtr(canvas->workspace()).lock()) {
            FitViewToDocument_(*canvas, *workspace);
        }
    }
}

} // namespace vgc::canvas
