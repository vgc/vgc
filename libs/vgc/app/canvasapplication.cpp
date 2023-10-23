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

#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>

#include <vgc/app/logcategories.h>
#include <vgc/canvas/tooloptionspanel.h>
#include <vgc/core/datetime.h>
#include <vgc/dom/strings.h>
#include <vgc/tools/paintbucket.h>
#include <vgc/tools/sculpt.h>
#include <vgc/tools/select.h>
#include <vgc/ui/genericaction.h>
#include <vgc/ui/genericcommands.h>
#include <vgc/ui/qtutil.h>
#include <vgc/ui/row.h>
#include <vgc/ui/shortcut.h>
#include <vgc/ui/tabbar.h>

namespace vgc::app {

namespace {

const core::Color initialColor(0.416f, 0.416f, 0.918f);

namespace paneltypes_ {

ui::PanelTypeId tools("vgc.common.tools");
ui::PanelTypeId toolOptions("vgc.common.toolOptions");
ui::PanelTypeId colorPalette("vgc.common.colorPalette");

} // namespace paneltypes_

core::StringId s_left_sidebar("left-sidebar");
core::StringId s_with_padding("with-padding");
core::StringId s_user("user");
core::StringId s_colorpalette("colorpalette");
core::StringId s_colorpaletteitem("colorpaletteitem");
core::StringId s_color("color");
core::StringId s_tools("tools");
core::StringId s_tool_options("tool-options");

core::Array<core::Color> getColorPalette_(dom::Document* doc) {

    // Get colors
    core::Array<core::Color> colors;
    dom::Element* root = doc->rootElement();
    for (dom::Element* user : root->childElements(s_user)) {
        for (dom::Element* colorpalette : user->childElements(s_colorpalette)) {
            for (dom::Element* item : colorpalette->childElements(s_colorpaletteitem)) {
                core::Color color = item->getAttribute(s_color).getColor();
                colors.append(color);
            }
        }
    }

    // Delete <user> element
    dom::Element* user = root->firstChildElement(s_user);
    while (user) {
        dom::Element* nextUser = user->nextSiblingElement(s_user);
        user->remove();
        user = nextUser;
    }

    return colors;
}

class ColorPaletteSaver {
public:
    ColorPaletteSaver(const ColorPaletteSaver&) = delete;
    ColorPaletteSaver& operator=(const ColorPaletteSaver&) = delete;

    ColorPaletteSaver(const core::Array<core::Color>& colors, dom::Document* doc)
        : isUndoOpened_(false)
        , doc_(doc) {

        // The current implementation adds the colors to the DOM now, save, then
        // abort the "add color" operation so that it doesn't appear as an undo.
        //
        // Ideally, we should instead add the color to the DOM directly when the
        // user clicks the "add to palette" button (so it would be an undoable
        // action), and the color list view should listen to DOM changes to update
        // the color list. This way, even plugins could populate the color palette
        // by modifying the DOM.
        //
        static core::StringId Add_to_Palette("Add to Palette");
        core::History* history = doc->history();
        if (history) {
            history->createUndoGroup(Add_to_Palette);
            isUndoOpened_ = true;
        }

        // TODO: reuse existing colorpalette element instead of creating new one.
        dom::Element* root = doc->rootElement();
        dom::Element* user = dom::Element::create(root, s_user);
        dom::Element* colorpalette = dom::Element::create(user, s_colorpalette);
        for (const core::Color& color : colors) {
            dom::Element* item = dom::Element::create(colorpalette, s_colorpaletteitem);
            item->setAttribute(s_color, color);
        }
    }

    ~ColorPaletteSaver() {
        if (isUndoOpened_) {
            core::History* history = doc_->history();
            if (history) {
                history->abort();
            }
        }
    }

private:
    bool isUndoOpened_;
    dom::Document* doc_;
};

} // namespace

namespace detail {

ui::Panel* createPanelWithPadding(ui::PanelArea* panelArea, std::string_view panelTitle) {
    ui::Panel* panel = panelArea->createPanel<ui::Panel>(panelTitle);
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

    openDocument_("");
    createActions_(window_->mainWidget());
    createMenus_();
    registerPanelTypes_();
    createDefaultPanels_();

    setCurrentColor_(initialColor);
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

bool CanvasApplication::recoverySave_() {

    // Nothing to save if no document.
    //
    if (!document_) {
        return false;
    }

    // It is risky to try to undo or abort the history since
    // it could cause another exception.
    // Thus we simply disable the history for the color palette
    // save operation.
    //
    core::History* history = document_->history();
    if (history) {
        document_->disableHistory();
    }

    // Determine where to save the recovery file.
    //
    QDir dir;         // "/home/user/Documents/MyPictures"
    QString basename; // "cat"
    QString suffix;   // ".vgci"
    if (filename_.isEmpty()) {
        dir = QDir(QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
        core::DateTime now = core::DateTime::now().toLocalTime();
        basename = ui::toQt(core::format("vgc-recovered-file-{:%Y-%m-%d}", now));
        suffix = ".vgci";
    }
    else {
        QFileInfo info(filename_);
        dir = info.dir();
        basename = info.baseName();
        suffix = "." + info.completeSuffix();
    }

    // Try to append ~1, ~2, 3, etc. to the filename until we find a filename
    // that doesn't exist yet, and save the recovery file there.
    //
    int maxRecoverVersion = 10000;
    for (int i = 1; i <= maxRecoverVersion; ++i) {
        QString name = basename + "~" + QString::number(i) + suffix;
        if (!dir.exists(name)) {
            filename_ = dir.absoluteFilePath(name);
            doSave_();
            return true;
        }
    }

    // Failed to save.
    //
    return false;
}

void CanvasApplication::showCrashPopup_(
    std::string_view errorMessage,
    bool wasRecoverySaved) {

    // Construct error message to show to the user.
    //
    QString title = "Oops! Something went wrong";
    QString msg;
    msg += "<p>We're very sorry, a bug occured and the application will now be closed."
           " It's totally our fault, not yours.</p>";
    if (wasRecoverySaved) {
        msg += "<p>Good news, we saved your work here:</p>";
        msg += "<p><b>";
        msg += QDir::toNativeSeparators(filename_).toHtmlEscaped();
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

// In debug builds, we silently show the location of the saved file instead of
// using a popup, since having to close the popup each time when debugging is a
// bit annoying.
//
void CanvasApplication::crashHandler_([[maybe_unused]] std::string_view errorMessage) {
    bool wasRecoverySaved = recoverySave_();
#ifdef VGC_DEBUG_BUILD
    if (wasRecoverySaved) {
        VGC_INFO(LogVgcApp, "Recovery file saved to: {}.", ui::fromQt(filename_));
    }
#else
    showCrashPopup_(errorMessage, wasRecoverySaved);
#endif
}

void CanvasApplication::openDocument_(QString filename) {

    // clear previous workspace
    if (workspace_) {
        workspace_->sync();
        if (document_->versionId() != lastSavedDocumentVersionId) {
            // XXX "do you wanna save ?"
        }
        core::History* history = workspace_->history();
        if (history) {
            history->disconnect(this);
        }
        canvas_->setWorkspace(nullptr);
    }

    // clear document info
    filename_.clear();
    document_ = nullptr;

    core::Array<core::Color> colors = {};
    dom::DocumentPtr newDocument = {};
    if (filename.isEmpty()) {
        try {
            newDocument = dom::Document::create();
            dom::Element::create(newDocument.get(), "vgc");
        }
        catch (const dom::FileError& e) {
            // TODO: have our own message box instead of using QtWidgets
            QMessageBox::critical(nullptr, "Error Creating New File", e.what());
        }
    }
    else {
        try {
            newDocument = dom::Document::open(ui::fromQt(filename));
            colors = getColorPalette_(newDocument.get());
        }
        catch (const dom::FileError& e) {
            // TODO: have our own message box instead of using QtWidgets
            QMessageBox::critical(nullptr, "Error Opening File", e.what());
        }
    }
    setDocumentColorPalette_(colors);

    workspace_ = workspace::Workspace::create(newDocument);
    document_ = newDocument.get();
    filename_ = filename;

    if (canvas_) {
        canvas_->setWorkspace(workspace_.get());
    }

    core::History* history = document_->enableHistory(dom::strings::New_Document);
    history->headChanged().connect(updateUndoRedoActionStateSlot_());
    updateUndoRedoActionState_();
}

void CanvasApplication::onActionNew_() {
    openDocument_("");
}

void CanvasApplication::onActionOpen_() {
    doOpen_();
}

void CanvasApplication::doOpen_() {
    // Get which directory the dialog should display first
    QString dir = filename_.isEmpty()
                      ? QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
                      : QFileInfo(filename_).dir().path();

    // Set which existing files to show in the dialog
    QString filters = "VGC Illustration Files (*.vgci)";

    // Create the dialog.
    //
    // TODO: manually set position of dialog in screen (since we can't give
    // it a QWidget* parent). Same for all QMessageBox.
    //
    QWidget* parent = nullptr;
    QFileDialog dialog(parent, QString("Open..."), dir, filters);

    // Allow to select existing files only
    dialog.setFileMode(QFileDialog::ExistingFile);

    // Set acceptMode to "Open" (as opposed to "Save")
    dialog.setAcceptMode(QFileDialog::AcceptOpen);

    // Exec the dialog as modal
    int result = dialog.exec();

    // Actually open the file
    if (result == QDialog::Accepted) {
        QStringList selectedFiles = dialog.selectedFiles();
        if (selectedFiles.size() == 0) {
            VGC_WARNING(LogVgcApp, "No file selected; file not opened.");
        }
        if (selectedFiles.size() == 1) {
            QString selectedFile = selectedFiles.first();
            if (!selectedFile.isEmpty()) {
                // Open
                openDocument_(selectedFile);
            }
            else {
                VGC_WARNING(LogVgcApp, "Empty file path selected; file not opened.");
            }
        }
        else {
            VGC_WARNING(LogVgcApp, "More than one file selected; file not opened.");
        }
    }
    else {
        // User willfully cancelled the operation
        // => nothing to do, not even a warning.
    }
}

void CanvasApplication::onActionSave_() {
    if (filename_.isEmpty()) {
        doSaveAs_();
    }
    else {
        doSave_();
    }
}

void CanvasApplication::onActionSaveAs_() {
    doSaveAs_();
}

void CanvasApplication::doSaveAs_() {

    // Get which directory the dialog should display first
    QString dir = filename_.isEmpty()
                      ? QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
                      : QFileInfo(filename_).dir().path();

    // Set which existing files to show in the dialog
    QString extension = ".vgci";
    QString filters = QString("VGC Illustration Files (*") + extension + ")";

    // Create the dialog
    QFileDialog dialog(nullptr, "Save As...", dir, filters);

    // Allow to select non-existing files
    dialog.setFileMode(QFileDialog::AnyFile);

    // Set acceptMode to "Save" (as opposed to "Open")
    dialog.setAcceptMode(QFileDialog::AcceptSave);

    // Exec the dialog as modal
    int result = dialog.exec();

    // Actually save the file
    if (result == QDialog::Accepted) {
        QStringList selectedFiles = dialog.selectedFiles();
        if (selectedFiles.size() == 0) {
            VGC_WARNING(LogVgcApp, "No file selected; file not saved.");
        }
        if (selectedFiles.size() == 1) {
            QString selectedFile = selectedFiles.first();
            if (!selectedFile.isEmpty()) {
                // Append file extension if missing. Examples:
                //   drawing.vgci -> drawing.vgci
                //   drawing      -> drawing.vgci
                //   drawing.     -> drawing..vgci
                //   drawing.vgc  -> drawing.vgc.vgci
                //   drawingvgci  -> drawingvgci.vgci
                //   .vgci        -> .vgci
                if (!selectedFile.endsWith(extension)) {
                    selectedFile.append(extension);
                }

                // Save
                filename_ = selectedFile;
                doSave_();
            }
            else {
                VGC_WARNING(LogVgcApp, "Empty file path selected; file not saved.");
            }
        }
        else {
            VGC_WARNING(LogVgcApp, "More than one file selected; file not saved.");
        }
    }
    else {
        // User willfully cancelled the operation
        // => nothing to do, not even a warning.
    }

    // Note: On some window managers, modal dialogs such as this Save As dialog
    // causes "QXcbConnection: XCB error: 3 (BadWindow)" errors. See:
    //   https://github.com/vgc/vgc/issues/6
    //   https://bugreports.qt.io/browse/QTBUG-56893
}

void CanvasApplication::doSave_() {
    try {
        ColorPaletteSaver saver(documentColorPalette(), document_);
        document_->save(ui::fromQt(filename_));
    }
    catch (const dom::FileError& e) {
        QMessageBox::critical(nullptr, "Error Saving File", e.what());
    }
}

void CanvasApplication::onActionQuit_() {
    quit();
}

void CanvasApplication::onActionUndo_() {
    if (workspace_) {
        core::History* history = workspace_->history();
        if (history) {
            history->undo();
        }
    }
}

void CanvasApplication::onActionRedo_() {
    if (workspace_) {
        core::History* history = workspace_->history();
        if (history) {
            history->redo();
        }
    }
}

void CanvasApplication::updateUndoRedoActionState_() {
    core::History* history = workspace_ ? workspace_->history() : nullptr;
    if (actionUndo_) {
        actionUndo_->setEnabled(history ? history->canUndo() : false);
    }
    if (actionRedo_) {
        actionRedo_->setEnabled(history ? history->canRedo() : false);
    }
}

namespace {

namespace commands {

using Shortcut = ui::Shortcut;
using Key = ui::Key;

constexpr ui::ModifierKey ctrl = ui::ModifierKey::Ctrl;
constexpr ui::ModifierKey shift = ui::ModifierKey::Shift;

VGC_UI_DEFINE_WINDOW_COMMAND( //
    _new,
    "file.new",
    "New",
    Shortcut(ctrl, Key::N))

VGC_UI_DEFINE_WINDOW_COMMAND(
    open, //
    "file.open",
    "Open",
    Shortcut(ctrl, Key::O))

VGC_UI_DEFINE_WINDOW_COMMAND( //
    save,
    "file.save",
    "Save",
    Shortcut(ctrl, Key::S))

VGC_UI_DEFINE_WINDOW_COMMAND( //
    saveAs,
    "file.saveAs",
    "Save As...",
    Shortcut(ctrl | shift, Key::S))

VGC_UI_DEFINE_WINDOW_COMMAND( //
    quit,
    "file.quit",
    "Quit",
    Shortcut(ctrl, Key::Q))

VGC_UI_DEFINE_WINDOW_COMMAND( //
    undo,
    "edit.undo",
    "Undo",
    Shortcut(ctrl, Key::Z))

VGC_UI_DEFINE_WINDOW_COMMAND( //
    redo,
    "edit.redo",
    "Redo",
    Shortcut(ctrl | shift, Key::Z))

VGC_UI_DEFINE_WINDOW_COMMAND(
    debugWidgetStyle,
    "debug.widgetStyle",
    "Debug Widget Style",
    Shortcut(ctrl | shift, Key::W))

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

} // namespace

void CanvasApplication::createActions_(ui::Widget* parent) {

    namespace generic = ui::commands::generic;

    actionNew_ = createAction(parent, commands::_new(), onActionNewSlot_());
    actionOpen_ = createAction(parent, commands::open(), onActionOpenSlot_());
    actionSave_ = createAction(parent, commands::save(), onActionSaveSlot_());
    actionSaveAs_ = createAction(parent, commands::saveAs(), onActionSaveAsSlot_());
    actionQuit_ = createAction(parent, commands::quit(), onActionQuitSlot_());

    actionUndo_ = createAction(parent, commands::undo(), onActionUndoSlot_());
    actionRedo_ = createAction(parent, commands::redo(), onActionRedoSlot_());
    actionCut_ = parent->createAction<ui::GenericAction>(generic::cut());
    actionCopy_ = parent->createAction<ui::GenericAction>(generic::copy());
    actionPaste_ = parent->createAction<ui::GenericAction>(generic::paste());

    actionDebugWidgetStyle_ = createAction(
        parent, commands::debugWidgetStyle(), onActionDebugWidgetStyleSlot_());

    updateUndoRedoActionState_();
}

void CanvasApplication::createMenus_() {

    ui::Menu* menuBar = window_->mainWidget()->menuBar();

    ui::Menu* fileMenu = menuBar->createSubMenu("File");
    fileMenu->addItem(actionNew_);
    fileMenu->addItem(actionOpen_);
    fileMenu->addSeparator();
    fileMenu->addItem(actionSave_);
    fileMenu->addItem(actionSaveAs_);
    fileMenu->addSeparator();
    fileMenu->addItem(actionQuit_);

    ui::Menu* editMenu = menuBar->createSubMenu("Edit");
    editMenu->addItem(actionUndo_);
    editMenu->addItem(actionRedo_);
    editMenu->addSeparator();
    editMenu->addItem(actionCut_);
    editMenu->addItem(actionCopy_);
    editMenu->addItem(actionPaste_);

    panelsMenu_ = menuBar->createSubMenu("Panels");
}

void CanvasApplication::registerPanelTypes_() {

    panelManager_ = ui::PanelManager::create();

    // Tools
    std::string_view toolsLabel = "Tools";
    panelManager_->registerPanelType(
        paneltypes_::tools, toolsLabel, [=](ui::PanelArea* parent) {
            ui::Panel* panel = this->toolManager_->createToolsPanel(parent);
            panel->addStyleClass(s_with_padding);
            parent->addStyleClass(s_tools); // XXX Why not on the panel itself?
            return panel;
        });

    // Tool Options
    panelManager_->registerPanelType(
        paneltypes_::toolOptions,
        canvas::ToolOptionsPanel::label,
        [=](ui::PanelArea* parent) {
            canvas::ToolOptionsPanel* panel =
                parent->createPanel<canvas::ToolOptionsPanel>(this->toolManager_.get());
            panel->addStyleClass(s_with_padding);
            parent->addStyleClass(s_tool_options); // XXX Why not on the panel itself?
            return panel;
        });

    // Colors
    std::string_view colorPaletteLabel = "Colors";
    panelManager_->registerPanelType(
        paneltypes_::colorPalette, colorPaletteLabel, [=](ui::PanelArea* parent) {
            ui::Panel* panel = detail::createPanelWithPadding(parent, colorPaletteLabel);
            tools::ColorPalette* palette = panel->createChild<tools::ColorPalette>();
            palette->setSelectedColor(this->currentColor());
            palette->setColors(this->documentColorPalette());
            palette->colorSelected().connect(this->setCurrentColor_Slot());
            palette->colorsChanged().connect(this->setDocumentColorPalette_Slot());
            this->currentColorChanged_().connect(palette->setSelectedColorSlot());
            this->documentColorPaletteChanged_().connect(palette->setColorsSlot());
            return panel;
        });

    // Populate Panels menu
    ui::Widget* actionParent = window_->mainWidget();
    for (ui::PanelTypeId id : panelManager_->registeredPanelTypeIds()) {
        ui::Action* action = actionParent->createTriggerAction(commands::openPanel());
        action->triggered().connect([=]() { this->onActionOpenPanel_(id); });
        action->setText(panelManager_->label(id));
        panelsMenu_->addItem(action);
    }
}

void CanvasApplication::createDefaultPanels_() {

    using detail::createPanelWithPadding;

    // Create main panel area
    mainPanelArea_ = window_->mainWidget()->panelArea();
    mainPanelArea_->setType(ui::PanelAreaType::HorizontalSplit);

    // Create Canvas (both the panel and the canvas itself)
    ui::PanelArea* canvasArea = ui::PanelArea::createTabs(mainPanelArea_.get());
    ui::Panel* canvasPanel = canvasArea->createPanel<ui::Panel>("Canvas");
    canvasArea->tabBar()->hide();
    createCanvas_(canvasPanel, workspace_.get());

    // Create and populate the ToolManager.
    //
    // Note: for now, this requires the `canvas_` to already be created. See comment in
    // ToolManager for better design (not have ToolManager depend on a Canvas instance).
    // Once the better design is implemented, this function would be better called before
    // createDefaultPanels_().
    //
    createTools_();

    // Create other panels
    onActionOpenPanel_(paneltypes_::tools);
    onActionOpenPanel_(paneltypes_::toolOptions);
    onActionOpenPanel_(paneltypes_::colorPalette);
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
    }
    return leftPanelArea_.get();
}

void CanvasApplication::onActionOpenPanel_(ui::PanelTypeId id) {

    if (!panelManager_ || !panelManager_->isRegistered(id)) {
        return;
    }

    ui::PanelArea* leftPanelArea = getOrCreateLeftPanelArea_();
    if (!leftPanelArea) {
        return;
    }

    ui::PanelArea* tabs = ui::PanelArea::createTabs(leftPanelArea);
    panelManager_->createPanelInstance(id, tabs);
}

void CanvasApplication::createCanvas_(
    ui::Widget* parent,
    workspace::Workspace* workspace) {

    canvas_ = parent->createChild<canvas::Canvas>(workspace);
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

void CanvasApplication::createTools_() {

    // Create the tool manager
    ui::Widget* actionOwner = mainWidget();
    toolManager_ = canvas::ToolManager::create(canvas_, actionOwner);

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
    sketchTool_ = sketchTool.get();
    paintBucketTool_ = paintBucketTool.get();
}

void CanvasApplication::setCurrentColor_(const core::Color& color) {

    // Set data member
    if (currentColor() == color) {
        return;
    }
    currentColor_ = color;

    // Update colors of other widgets / tools
    if (sketchTool_) {
        sketchTool_->setPenColor(currentColor());
    }
    if (paintBucketTool_) {
        paintBucketTool_->setColor(currentColor());
    }

    // Emit
    currentColorChanged_().emit(currentColor());
}

void CanvasApplication::setDocumentColorPalette_(const core::Array<core::Color>& colors) {

    // Set data member
    if (documentColorPalette_ == colors) {
        return;
    }
    documentColorPalette_ = colors;

    // Emit
    documentColorPaletteChanged_().emit(documentColorPalette());
}

namespace {

void widgetSizingInfo(std::string& out, ui::Widget* widget, ui::Widget* root) {

    auto outB = std::back_inserter(out);
    out += widget->className();

    out += "\nStyle =";
    for (core::StringId styleClass : widget->styleClasses()) {
        out += " ";
        out += styleClass;
    }
    out += "\n";
    core::formatTo(
        outB, "\nPosition       = {}", widget->mapTo(root, geometry::Vec2f(0, 0)));
    core::formatTo(outB, "\nSize           = {}", widget->size());
    core::formatTo(outB, "\nPreferred Size = {}", widget->preferredSize());
    core::formatTo(outB, "\nMargin         = {}", widget->margin());
    core::formatTo(outB, "\nPadding        = {}", widget->padding());
    core::formatTo(outB, "\nBorder         = {}", widget->border());

    out += "\n\nMatching style rules:\n\n";
    core::StringWriter outS(out);
    widget->debugPrintStyle(outS);
}

} // namespace

void CanvasApplication::onActionDebugWidgetStyle_() {

    if (!mainWindow() || !mainWidget()) {
        return;
    }

    std::string out;

    using core::write;
    out.append(80, '=');
    out += "\nPosition and size information about hovered widgets:\n";
    ui::Widget* root = mainWidget();
    ui::Widget* widget = root;
    while (widget) {
        out.append(80, '-');
        out += "\n";
        widgetSizingInfo(out, widget, root);
        widget = widget->hoverChainChild();
    }
    VGC_DEBUG(LogVgcApp, out);
}

} // namespace vgc::app
