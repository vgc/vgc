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
#include <vgc/canvas/logcategories.h>
#include <vgc/canvas/workspaceselection.h>
#include <vgc/ui/menu.h>
#include <vgc/ui/standardmenus.h>

namespace vgc::canvas {

namespace commands {

using ui::Key;
using ui::Shortcut;
using ui::modifierkeys::alt;
using ui::modifierkeys::mod;
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

VGC_UI_DEFINE_WINDOW_COMMAND( //
    controlPoints,
    "canvas.controlPoints",
    "Show/Hide Control Points",
    Shortcut(alt, Key::P));

VGC_UI_DEFINE_WINDOW_COMMAND( //
    wireframe,
    "canvas.wireframe",
    "Show/Hide Wireframe",
    Shortcut(alt, Key::W));

VGC_UI_DEFINE_WINDOW_COMMAND( //
    adaptiveSampling,
    "canvas.adaptiveSampling",
    "Toggle Adaptive Sampling",
    Shortcut(mod, Key::A));

// Note: we can't use Shortcut(mod, Key::Q) (Q for "quality"), because
// Shift + Command + Q triggers macOS logout, which takes precedence.

VGC_UI_DEFINE_WINDOW_COMMAND( //
    decreaseSamplingQuality,
    "canvas.decreaseSamplingQuality",
    "Decrease Sampling Quality (Faster Rendering)",
    Shortcut(mod, Key::S));

VGC_UI_DEFINE_WINDOW_COMMAND( //
    increaseSamplingQuality,
    "canvas.increaseSamplingQuality",
    "Increase Sampling Quality (Slower Rendering)",
    Shortcut(mod, Key::D));

} // namespace commands

CanvasManager::CanvasManager(CreateKey key, const ui::ModuleContext& context)
    : Module(key, context) {

    documentManager_ = context.importModule<DocumentManager>();
    if (auto documentManager = documentManager_.lock()) {
        documentManager->currentWorkspaceReplaced().connect(
            onCurrentWorkspaceReplaced_Slot());
    }

    ui::MenuWeakPtr viewMenu;
    if (auto standardMenus = importModule<ui::StandardMenus>().lock()) {
        viewMenu = standardMenus->getOrCreateViewMenu();
    }

    using namespace commands;
    ui::ModuleActionCreator c(this);
    c.setMenu(viewMenu);

    c.addAction(switchToNormalDisplayMode(), onSwitchToNormalDisplayMode_Slot());
    c.addAction(
        switchToOutlineOverlayDisplayMode(), onSwitchToOutlineOverlayDisplayMode_Slot());
    c.addAction(
        switchToOutlineOnlyDisplayMode(), onSwitchToOutlineOnlyDisplayMode_Slot());

    c.addSeparator();
    c.addAction(toggleLastTwoDisplayModes(), onToggleLastTwoDisplayModes_Slot());
    c.addAction(cycleDisplayModes(), onCycleDisplayModes_Slot());

    c.addSeparator();
    c.addAction(fitViewToSelection(), onFitViewToSelection_Slot());
    c.addAction(fitViewToDocument(), onFitViewToDocument_Slot());

    c.addSeparator();
    c.addAction(controlPoints(), onControlPoints_Slot());
    c.addAction(wireframe(), onWireframe_Slot());

    c.addSeparator();
    c.addAction(adaptiveSampling(), onAdaptiveSampling_Slot());
    c.addAction(decreaseSamplingQuality(), onDecreaseSamplingQuality_Slot());
    c.addAction(increaseSamplingQuality(), onIncreaseSamplingQuality_Slot());

// Add separation with automatically generated "Enter Fullscreen" item on macOS
#ifdef VGC_OS_MACOS
    c.addSeparator();
#endif
}

CanvasManagerPtr CanvasManager::create(const ui::ModuleContext& context) {
    return core::createObject<CanvasManager>(context);
}

namespace {

void setCanvasWorkspace(
    const CanvasWeakPtr& canvas_,
    const DocumentManagerWeakPtr& documentManager_) {

    if (auto canvas = canvas_.lock()) {
        if (auto documentManager = documentManager_.lock()) {
            canvas->setWorkspace(documentManager->currentWorkspace());
            canvas->setWorkspaceSelection(documentManager->currentWorkspaceSelection());
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

void CanvasManager::onCurrentWorkspaceReplaced_() {
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
    geometry::Vec2d c1 = camera.viewMatrix().inverse().transformAffine(c0);
    camera.setRotation(rotation);
    geometry::Vec2d c2 = camera.viewMatrix().transformAffine(c1);
    camera.setCenter(camera.center() - c0 + c2);

    canvas.setCamera(camera);
}

// TODO: have both const and non-const versions of Workspace::visit...() so
// that we can use a `const Workspace&` argument here.
//
void fitViewToDocument_(Canvas& canvas, workspace::Workspace& workspace) {

    // TODO: implement Workspace::boundingBox().
    geometry::Rect2d rect = geometry::Rect2d::empty;
    workspace.visitDepthFirstPreOrder( //
        [&rect](workspace::Element* e, Int depth) {
            VGC_UNUSED(depth);
            rect.uniteWith(e->boundingBox());
        });
    fitViewToRect_(canvas, rect);
}

void fitViewToSelection_(
    Canvas& canvas,
    workspace::Workspace& workspace,
    WorkspaceSelection& selection) {

    const core::Array<core::Id>& itemIds = selection.itemIds();
    if (itemIds.isEmpty()) {
        fitViewToDocument_(canvas, workspace);
    }
    else {
        geometry::Rect2d rect = geometry::Rect2d::empty;
        for (core::Id id : itemIds) {
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
        if (auto workspace = canvas->workspace().lock()) {
            if (auto selection = canvas->workspaceSelection().lock()) {
                fitViewToSelection_(*canvas, *workspace, *selection);
            }
        }
    }
}

void CanvasManager::onFitViewToDocument_() {
    if (auto canvas = activeCanvas().lock()) {
        if (auto workspace = canvas->workspace().lock()) {
            fitViewToDocument_(*canvas, *workspace);
        }
    }
}

void CanvasManager::onControlPoints_() {
    if (auto canvas = activeCanvas().lock()) {
        canvas->setControlPointsVisible(!canvas->areControlPointsVisible());
    }
}

void CanvasManager::onWireframe_() {
    if (auto canvas = activeCanvas().lock()) {
        canvas->setWireframeMode(!canvas->isWireframeMode());
    }
}

namespace {

void setSamplingQuality_(
    vacomplex::Complex& complex,
    canvas::Canvas& canvas,
    geometry::CurveSamplingQuality quality) {

    VGC_INFO(
        LogVgcCanvas,
        "Switched sampling quality to: {}",
        core::Enum::prettyName(quality));

    complex.setSamplingQuality(quality);
    canvas.requestRepaint();
}

} // namespace

void CanvasManager::onAdaptiveSampling_() {
    if (auto canvas = activeCanvas().lock()) {
        if (auto workspace = canvas->workspace().lock()) {
            auto complex = workspace->vac().lock();
            if (!complex) {
                return;
            }
            geometry::CurveSamplingQuality quality = complex->samplingQuality();
            Int8 level = geometry::getSamplingQualityLevel(quality);
            bool oldIsAdaptive = geometry::isAdaptiveSampling(quality);
            bool newIsAdaptive = !oldIsAdaptive;
            quality = geometry::getSamplingQuality(level, newIsAdaptive);
            setSamplingQuality_(*complex, *canvas, quality);
        }
    }
}

void CanvasManager::onDecreaseSamplingQuality_() {
    if (auto canvas = activeCanvas().lock()) {
        if (auto workspace = canvas->workspace().lock()) {
            auto complex = workspace->vac().lock();
            if (!complex) {
                return;
            }
            geometry::CurveSamplingQuality quality = complex->samplingQuality();
            bool isAdaptive = geometry::isAdaptiveSampling(quality);
            Int8 oldLevel = geometry::getSamplingQualityLevel(quality);
            Int minLevel = isAdaptive ? 1 : 0;
            Int maxLevel = 5;
            Int8 newLevel = core::clamp(oldLevel - 1, minLevel, maxLevel);
            quality = geometry::getSamplingQuality(newLevel, isAdaptive);
            setSamplingQuality_(*complex, *canvas, quality);
        }
    }
}

void CanvasManager::onIncreaseSamplingQuality_() {
    if (auto canvas = activeCanvas().lock()) {
        if (auto workspace = canvas->workspace().lock()) {
            auto complex = workspace->vac().lock();
            if (!complex) {
                return;
            }
            geometry::CurveSamplingQuality quality = complex->samplingQuality();
            bool isAdaptive = geometry::isAdaptiveSampling(quality);
            Int8 oldLevel = geometry::getSamplingQualityLevel(quality);
            Int minLevel = isAdaptive ? 1 : 0;
            Int maxLevel = 5;
            Int8 newLevel = core::clamp(oldLevel + 1, minLevel, maxLevel);
            quality = geometry::getSamplingQuality(newLevel, isAdaptive);
            setSamplingQuality_(*complex, *canvas, quality);
        }
    }
}

} // namespace vgc::canvas
