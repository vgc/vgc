// Copyright 2022 The VGC Developers
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

#include <vgc/app/canvasapplication.h>

#include <QDir>
#include <QMessageBox>

#include <vgc/app/filemanager.h>
#include <vgc/app/logcategories.h>
#include <vgc/canvas/canvasmanager.h>
#include <vgc/canvas/documentmanager.h>
#include <vgc/canvas/tooloptionspanel.h>
#include <vgc/canvas/toolspanel.h>
#include <vgc/dom/strings.h>
#include <vgc/tools/currentcolor.h>
#include <vgc/tools/documentcolorpalette.h>
#include <vgc/tools/paintbucket.h>
#include <vgc/tools/sculpt.h>
#include <vgc/tools/select.h>
#include <vgc/ui/genericaction.h>
#include <vgc/ui/genericcommands.h>
#include <vgc/ui/inspector.h>
#include <vgc/ui/qtutil.h>
#include <vgc/ui/row.h>
#include <vgc/ui/shortcut.h>
#include <vgc/ui/standardmenus.h>
#include <vgc/ui/tabbar.h>

namespace vgc::app {

namespace {

const core::Color initialColor(0.416f, 0.416f, 0.918f);

namespace paneltypes_ {

ui::PanelTypeId tools("vgc.common.tools");
ui::PanelTypeId toolOptions("vgc.common.toolOptions");
ui::PanelTypeId colors("vgc.common.colors");

} // namespace paneltypes_

core::StringId s_left_sidebar("left-sidebar");
core::StringId s_with_padding("with-padding");

} // namespace

namespace detail {

ui::Panel* createPanelWithPadding(
    ui::PanelManager* panelManager,
    ui::PanelArea* panelArea,
    std::string_view panelTitle) {

    ui::Panel* panel =
        panelManager->createPanelInstance_<ui::Panel>(panelArea, panelTitle);
    panel->addStyleClass(s_with_padding);
    return panel;
}

} // namespace detail

CanvasApplication::CanvasApplication(
    CreateKey key,
    int argc,
    char* argv[],
    std::string_view applicationName)

    : QtWidgetsApplication(key, argc, argv) {

    setApplicationName(applicationName);
    window_ = app::MainWindow::create(applicationName);
    window_->setBackgroundPainted(false);

    // Sets the window's menu bar as being the standard menu bar. This must be
    // done before creating other modules so that they can add their actions to
    // the menu bar.
    //
    if (auto standardMenus = importModule<ui::StandardMenus>().lock()) {
        standardMenus->setMenuBar(window_->mainWidget()->menuBar());
        standardMenus->createFileMenu();
        standardMenus->createEditMenu();
    }

    // CurrentColor module
    if (auto currentColor = importModule<tools::CurrentColor>().lock()) {
        currentColor_ = currentColor;
        currentColor->colorChanged().connect(onCurrentColorChanged_Slot());
        currentColor->setColor(initialColor);
    }

    // DocumentColorPalette module
    documentColorPalette_ = importModule<tools::DocumentColorPalette>();

    // FileManager module
    if (auto fileManager = importModule<FileManager>().lock()) {
        fileManager->quitTriggered().connect(quitSlot());
    }

    // Other actions (TODO: refactor these out of CanvasApplication)
    createActions_(window_->mainWidget());

    // Panels
    registerPanelTypes_();
    createDefaultPanels_();

    // Widget Inspector
    importModule<ui::Inspector>();
}

CanvasApplicationPtr
CanvasApplication::create(int argc, char* argv[], std::string_view applicationName) {
    return core::createObject<CanvasApplication>(argc, argv, applicationName);
}

void CanvasApplication::quit() {
    if (window_) {
        window_->close();
    }
}

void CanvasApplication::onUnhandledException(std::string_view errorMessage) {
    crashHandler_(errorMessage);
    SuperClass::onUnhandledException(errorMessage);
}

void CanvasApplication::onSystemSignalReceived(std::string_view errorMessage, int sig) {
    crashHandler_(errorMessage);
    SuperClass::onSystemSignalReceived(errorMessage, sig);
}

namespace {

#ifndef VGC_DEBUG_BUILD
void showCrashPopup_(
    std::string_view errorMessage,
    const RecoverySaveInfo& recoverySaveInfo) {

    // Construct error message to show to the user.
    //
    QString title = "Oops! Something went wrong";
    QString msg;
    msg += "<p>We're very sorry, a bug occured and the application will now be closed."
           " It's totally our fault, not yours.</p>";
    if (recoverySaveInfo.wasSaved()) {
        msg += "<p>Good news, we saved your work here:</p>";
        msg += "<p><b>";
        msg += QDir::toNativeSeparators(recoverySaveInfo.filename()).toHtmlEscaped();
        msg += "</b></p>";
    }
    msg += "<p>We would love to fix this bug. "
           "You can help us by describing what happened at:</p>"
           "<p><a href='https://github.com/vgc/vgc/issues/new/choose'>"
           "https://github.com/vgc/vgc/issues</a></p>"
           "<p>On behalf of all users, thank you.</p>";
    msg += "<p>More details:</p><p>";
    msg += ui::toQt(errorMessage).toHtmlEscaped();
    msg += "</p>";

    // Show error to the user.
    //
    QMessageBox messageBox(nullptr);
    messageBox.setWindowTitle(title);
    messageBox.setTextFormat(Qt::RichText); // makes the links clickable
    messageBox.setText(msg);
    messageBox.exec();
}
#endif

} // namespace

// In debug builds, we silently show the location of the saved file instead of
// using a popup, since having to close the popup each time when debugging is a
// bit annoying.
//
void CanvasApplication::crashHandler_([[maybe_unused]] std::string_view errorMessage) {
    auto info = RecoverySaveInfo::notSaved();
    if (auto fileManager = importModule<FileManager>().lock()) {
        info = fileManager->recoverySave();
    }
#ifdef VGC_DEBUG_BUILD
    if (info.wasSaved()) {
        VGC_INFO(LogVgcApp, "Recovery file saved to: {}.", ui::fromQt(info.filename()));
    }
#else
    showCrashPopup_(errorMessage, info);
#endif
}

namespace {

namespace commands {

using ui::Key;
using ui::Shortcut;
using ui::modifierkeys::ctrl;
using ui::modifierkeys::shift;

// TODO: one command per panel with specific shortcut?
VGC_UI_DEFINE_WINDOW_COMMAND( //
    openPanel,
    "panels.openPanel",
    "Open Panel",
    Shortcut())

} // namespace commands

template<typename TSlot>
ui::Action* createAction(ui::Widget* parent, core::StringId commandId, TSlot slot) {
    ui::Action* action = parent->createTriggerAction(commandId);
    action->triggered().connect(slot);
    return action;
}

void createGenericAction_(ui::Widget* parent, ui::Menu& menu, core::StringId commandId) {
    ui::Action* action = parent->createAction<ui::GenericAction>(commandId);
    menu.addItem(action);
}

} // namespace

void CanvasApplication::createActions_(ui::Widget* parent) {

    // For now, generic actions don't work if they are owned by a module, since
    // GenericAction works by using its `owningWidget`. Thus we define
    // cut-copy-paste actions here since we need a parent widget.
    //
    // TODO:
    // - make generic actions work in a module
    // - Implement something like StandardMenus::createGenericCutCopyPaste()
    //
    if (auto standardMenus = importModule<ui::StandardMenus>().lock()) {
        if (auto editMenu = standardMenus->getOrCreateEditMenu().lock()) {
            namespace generic = ui::commands::generic;
            editMenu->addSeparator();
            createGenericAction_(parent, *editMenu, generic::cut());
            createGenericAction_(parent, *editMenu, generic::copy());
            createGenericAction_(parent, *editMenu, generic::paste());
        }
    }
}

void CanvasApplication::registerPanelTypes_() {

    panelManager_ = ui::PanelManager::create(moduleManager());

    // Tools
    using canvas::ToolsPanel;
    panelManager_->registerPanelType(
        paneltypes_::tools, ToolsPanel::label, [=](ui::PanelArea* parent) {
            return this->panelManager_->createPanelInstance_<ToolsPanel>(parent);
        });

    // Tool Options
    using canvas::ToolOptionsPanel;
    panelManager_->registerPanelType(
        paneltypes_::toolOptions, ToolOptionsPanel::label, [=](ui::PanelArea* parent) {
            return this->panelManager_->createPanelInstance_<ToolOptionsPanel>(parent);
        });

    // Colors
    using tools::ColorsPanel;
    panelManager_->registerPanelType(
        paneltypes_::colors, ColorsPanel::label, [=](ui::PanelArea* parent) {
            return this->panelManager_->createPanelInstance_<ColorsPanel>(parent);
        });

    // Create Panels menu
    ui::Menu* panelsMenu = nullptr;
    if (auto standardMenus = importModule<ui::StandardMenus>().lock()) {
        if (auto menuBar = standardMenus->menuBar().lock()) {
            panelsMenu = menuBar->createSubMenu("Panels");
        }
    }

    // Populate Panels menu
    if (panelsMenu) {
        ui::Widget* actionParent = window_->mainWidget();
        for (ui::PanelTypeId id : panelManager_->registeredPanelTypeIds()) {
            ui::Action* action = actionParent->createTriggerAction(commands::openPanel());
            action->triggered().connect([=]() { this->onActionOpenPanel_(id); });
            action->setText(panelManager_->label(id));
            panelsMenu->addItem(action);
        }
    }
}

void CanvasApplication::createDefaultPanels_() {

    using detail::createPanelWithPadding;

    // Create main panel area
    mainPanelArea_ = window_->mainWidget()->panelArea();
    mainPanelArea_->setType(ui::PanelAreaType::HorizontalSplit);

    // Create Canvas (both the panel and the canvas itself)
    //
    // XXX This panel type is currently not registered with the PanelManager. Should it?
    //
    ui::PanelArea* canvasArea = ui::PanelArea::createTabs(mainPanelArea_.get());
    ui::Panel* canvasPanel =
        panelManager_->createPanelInstance_<ui::Panel>(canvasArea, "Canvas");
    canvasArea->tabBar()->hide();
    canvas::Canvas* canvas = canvasPanel->createChild<canvas::Canvas>(nullptr);
    if (auto canvasManager = importModule<canvas::CanvasManager>().lock()) {
        // Set the canvas as being the active canvas. This ensures that
        // canvas->setWorkspace() is called whenever the current workspace
        // changes, e.g., when opening a new file.
        canvasManager->setActiveCanvas(canvas);
    }

    // Create and populate the ToolManager.
    //
    // Note: for now, this requires the canvas to already be created and
    // outlive the tool manager. See comment in ToolManager for better design:
    // instead of having ToolManager depend on a Canvas, we should have each
    // Canvas listen to the/a (global? module?) ToolManager. Or having
    // CanvasManager make the link between the two.
    //
    createTools_(canvas);

    // Create other panels
    onActionOpenPanel_(paneltypes_::tools);
    onActionOpenPanel_(paneltypes_::toolOptions);
    onActionOpenPanel_(paneltypes_::colors);
}

ui::PanelArea* CanvasApplication::getOrCreateLeftPanelArea_() {

    if (!mainPanelArea_) {
        return nullptr;
    }

    if (!leftPanelArea_) {

        // Create panel
        leftPanelArea_ = ui::PanelArea::createVerticalSplit(mainPanelArea_.get());
        leftPanelArea_->addStyleClass(s_left_sidebar);

        // Move it as first child (i.e., at the left) of the main panel area
        mainPanelArea_->insertChild(mainPanelArea_->firstChild(), leftPanelArea_.get());

        // Set an appropriate size.
        // Note: This given size will be automatically increased to satisfy min-size.
        // TODO: Use a system to remember the last-used size.
        leftPanelArea_->setSplitSize(100);
    }
    return leftPanelArea_.get();
}

void CanvasApplication::onActionOpenPanel_(ui::PanelTypeId id) {

    // No possible action to do if there is no panel manager or the panel type is unknown.
    //
    if (!panelManager_ || !panelManager_->isRegistered(id)) {
        return;
    }

    // Prevent creating several instances of the same panel type when using the
    // Panels menu. This is not a technical limitation but a UX decision: the
    // panels are in fact implemented in a way that supports multiple instances
    // of the same panel type, and in the future we want to allow users to
    // create such multiple instances via a "+" menu in a panel area.
    //
    // For testing that multiple panels do indeed work, set the variable to
    // true.
    //
    constexpr bool allowMultipleInstances = false;
    if (!allowMultipleInstances && panelManager_->hasInstance(id)) {
        return;
    }

    ui::PanelArea* leftPanelArea = getOrCreateLeftPanelArea_();
    if (!leftPanelArea) {
        return;
    }

    ui::PanelArea* tabs = ui::PanelArea::createTabs(leftPanelArea);
    panelManager_->createPanelInstance(id, tabs);
}

namespace {

namespace commands {

// Note: These shortcuts are standards in existing software (except "S" for
// sculpt), and quite nice on QWERTY keyboards since they are all easy to
// access with the left hand.

VGC_UI_DEFINE_WINDOW_COMMAND( //
    selectTool,
    "tools.select",
    "Select Tool",
    Key::V,
    "tools/icons/select.svg")

VGC_UI_DEFINE_WINDOW_COMMAND( //
    sketchTool,
    "tools.sketch",
    "Sketch Tool",
    Key::B,
    "tools/icons/sketch.svg")

VGC_UI_DEFINE_WINDOW_COMMAND( //
    paintBucketTool,
    "tools.paintBucket",
    "Paint Bucket Tool",
    Key::G,
    "tools/icons/paintBucket.svg")

VGC_UI_DEFINE_WINDOW_COMMAND( //
    sculptTool,
    "tools.sculpt",
    "Sculpt Tool",
    Key::S,
    "tools/icons/sculpt.svg")

} // namespace commands

} // namespace

void CanvasApplication::createTools_(canvas::Canvas* canvas) {

    // Create the tool manager
    toolManager_ = importModule<canvas::ToolManager>();
    toolManager_->setCanvas(canvas);

    // Create and register all tools
    // TODO: add CanvasTool::command() and use a createAndRegisterTool() helper
    //       to only have half the number of lines here.
    tools::SelectPtr selectTool = tools::Select::create();
    tools::SketchPtr sketchTool = tools::Sketch::create();
    tools::PaintBucketPtr paintBucketTool = tools::PaintBucket::create();
    tools::SculptPtr sculptTool = tools::Sculpt::create();
    toolManager_->registerTool(commands::selectTool(), selectTool);
    toolManager_->registerTool(commands::sketchTool(), sketchTool);
    toolManager_->registerTool(commands::paintBucketTool(), paintBucketTool);
    toolManager_->registerTool(commands::sculptTool(), sculptTool);

    // Keep pointer to some tools for handling color changes
    // TODO: Delegate this to the tools themselves by providing the CurrentColor object.
    sketchTool_ = sketchTool.get();
    paintBucketTool_ = paintBucketTool.get();
    if (auto currentColor = currentColor_.lock()) {
        onCurrentColorChanged_(currentColor->color());
    }
}

void CanvasApplication::onCurrentColorChanged_(const core::Color& color) {

    // Update colors of other widgets / tools
    //
    // TODO: Delegate this to the tools themselves by providing the CurrentColor object.
    //
    if (sketchTool_) {
        sketchTool_->setPenColor(color);
    }
    if (paintBucketTool_) {
        paintBucketTool_->setColor(color);
    }
}

} // namespace vgc::app
