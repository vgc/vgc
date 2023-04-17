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
#include <vgc/core/datetime.h>
#include <vgc/dom/strings.h>
#include <vgc/ui/qtutil.h>
#include <vgc/ui/selecttool.h>
#include <vgc/ui/shortcut.h>
#include <vgc/ui/tabbar.h>

namespace vgc::app {

namespace {

core::StringId s_left_sidebar("left-sidebar");
core::StringId s_with_padding("with-padding");
core::StringId s_user("user");
core::StringId s_colorpalette("colorpalette");
core::StringId s_colorpaletteitem("colorpaletteitem");
core::StringId s_color("color");
core::StringId s_tools("tools");
core::StringId s_tool_options("tool-options");

void loadColorPalette_(
    ui::ColorPalette* palette,
    const core::Array<core::Color>& colors) {

    if (!palette) {
        return;
    }
    ui::ColorListView* listView = palette->colorListView();
    if (!listView) {
        return;
    }
    listView->setColors(colors);
}

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

    ColorPaletteSaver(ui::ColorPalette* palette, dom::Document* doc)
        : isUndoOpened_(false)
        , doc_(doc) {

        if (!palette) {
            return;
        }

        ui::ColorListView* listView = palette->colorListView();
        if (!listView) {
            return;
        }

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
        for (Int i = 0; i < listView->numColors(); ++i) {
            const core::Color& color = listView->colorAt(i);
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
    ui::Panel* panel = panelArea->createPanel(panelTitle);
    panel->addStyleClass(s_with_padding);
    return panel;
}

} // namespace detail

CanvasApplication::CanvasApplication(
    int argc,
    char* argv[],
    std::string_view applicationName)

    : QtWidgetsApplication(argc, argv) {

    setApplicationName(applicationName);
    window_ = app::MainWindow::create(applicationName);
    window_->setBackgroundPainted(false);
    openDocument_("");
    createActions_(window_->mainWidget());
    createMenus_();
    createWidgets_();
}

CanvasApplicationPtr
CanvasApplication::create(int argc, char* argv[], std::string_view applicationName) {
    return CanvasApplicationPtr(new CanvasApplication(argc, argv, applicationName));
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

    loadColorPalette_(palette_, colors);
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
        ColorPaletteSaver saver(palette_, document_);
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

void CanvasApplication::createActions_(ui::Widget* parent) {

    using ui::Key;
    using ui::Shortcut;

    ui::ModifierKey ctrl = ui::ModifierKey::Ctrl;
    ui::ModifierKey shift = ui::ModifierKey::Shift;
    ui::ShortcutContext context = ui::ShortcutContext::Window;

    auto createAction = [=](std::string_view text, const Shortcut& shortcut) {
        return parent->createAction(text, shortcut, context);
    };

    actionNew_ = createAction("New", Shortcut(ctrl, Key::N));
    actionNew_->triggered().connect(onActionNewSlot_());

    actionOpen_ = createAction("Open", Shortcut(ctrl, Key::O));
    actionOpen_->triggered().connect(onActionOpenSlot_());

    actionSave_ = createAction("Save", Shortcut(ctrl, Key::S));
    actionSave_->triggered().connect(onActionSaveSlot_());

    actionSaveAs_ = createAction("Save As...", Shortcut(ctrl | shift, Key::S));
    actionSaveAs_->triggered().connect(onActionSaveAsSlot_());

    actionQuit_ = createAction("Quit", Shortcut(ctrl, Key::Q));
    actionQuit_->triggered().connect(onActionQuitSlot_());

    actionUndo_ = createAction("Undo", Shortcut(ctrl, Key::Z));
    actionUndo_->triggered().connect(onActionUndoSlot_());

    actionRedo_ = createAction("Redo", Shortcut(ctrl | shift, Key::Z));
    actionRedo_->triggered().connect(onActionRedoSlot_());

    actionDebugWidgetSizing_ =
        createAction("Debug Widget Sizing", Shortcut(ctrl | shift, Key::W));
    actionDebugWidgetSizing_->triggered().connect(onActionDebugWidgetSizingSlot_());

    updateUndoRedoActionState_();
}

void CanvasApplication::createMenus_() {

    ui::Menu* menuBar = window_->mainWidget()->menuBar();

    ui::Menu* fileMenu = menuBar->createSubMenu("File");
    fileMenu->addItem(actionNew_);
    fileMenu->addItem(actionOpen_);
    fileMenu->addItem(actionSave_);
    fileMenu->addItem(actionSaveAs_);
    fileMenu->addItem(actionQuit_);

    ui::Menu* editMenu = menuBar->createSubMenu("Edit");
    editMenu->addItem(actionUndo_);
    editMenu->addItem(actionRedo_);
}

void CanvasApplication::createWidgets_() {

    using detail::createPanelWithPadding;

    // Create panel areas
    ui::PanelArea* mainArea = window_->mainWidget()->panelArea();
    mainArea->setType(ui::PanelAreaType::HorizontalSplit);
    ui::PanelArea* leftArea = ui::PanelArea::createVerticalSplit(mainArea);
    ui::PanelArea* leftArea1 = ui::PanelArea::createTabs(leftArea);
    ui::PanelArea* leftArea2 = ui::PanelArea::createTabs(leftArea);
    ui::PanelArea* leftArea3 = ui::PanelArea::createTabs(leftArea);
    leftArea->addStyleClass(s_left_sidebar);
    leftArea1->addStyleClass(s_tools);
    leftArea2->addStyleClass(s_tool_options);
    ui::PanelArea* middleArea = ui::PanelArea::createTabs(mainArea);

    // Create panels
    ui::Panel* leftPanel1 = createPanelWithPadding(leftArea1, "Tools");
    toolOptionsPanel_ = createPanelWithPadding(leftArea2, "Tool Options");
    ui::Panel* leftPanel3 = createPanelWithPadding(leftArea3, "Colors");
    ui::Panel* middlePanel = middleArea->createPanel("Canvas");
    middleArea->tabBar()->hide();

    // Create widgets inside panels
    createCanvas_(middlePanel, workspace_.get());
    createTools_(leftPanel1);
    createColorPalette_(leftPanel3);
}

void CanvasApplication::createCanvas_(
    ui::Widget* parent,
    workspace::Workspace* workspace) {

    canvas_ = parent->createChild<ui::Canvas>(workspace);
}

void CanvasApplication::createTools_(ui::Widget* parent) {

    // Create action group ensuring only one tool is active at a time
    toolsActionGroup_ = ui::ActionGroup::create(ui::CheckPolicy::ExactlyOne);

    // Create and register all tools
    ui::Column* tools = parent->createChild<ui::Column>();
    ui::SelectToolPtr selectTool = ui::SelectTool::create();
    ui::SketchToolPtr sketchTool = ui::SketchTool::create();
    registerTool_(tools, "Select Tool", selectTool);
    registerTool_(tools, "Sketch Tool", sketchTool);

    // Keep pointer to sketch tool for handling color changes,
    // and set it as default tool.
    sketchTool_ = sketchTool.get();
    setCurrentTool_(sketchTool_);
}

void CanvasApplication::registerTool_(
    ui::Widget* parent,
    std::string_view toolName,
    ui::CanvasToolPtr tool) {

    // Create tool action and add it to the action group
    ui::Action* action = parent->createAction(toolName);
    action->setCheckable(true);
    action->checkStateChanged().connect(onToolCheckStateChangedSlot_());
    toolsActionGroup_->addAction(action);

    // Keeps the CanvasTool alive by storing it as an ObjPtr and
    // remembers which CanvasTool corresponds to which tool action.
    toolMap_[action] = tool;
    toolMapInv_[tool.get()] = action;

    // Create corresponding button in the Tools panel
    parent->createChild<ui::Button>(action);
}

void CanvasApplication::setCurrentTool_(ui::CanvasTool* canvasTool) {
    if (canvasTool != currentTool_) {
        bool wasFocusedWidget = false;
        if (currentTool_) {
            wasFocusedWidget = currentTool_->isFocusedWidget();
            currentTool_->clearFocus(ui::FocusReason::Other);
            currentTool_->reparent(nullptr);
            currentTool_->optionsWidget()->reparent(nullptr);
        }
        currentTool_ = canvasTool;
        if (currentTool_) {
            canvas_->addChild(canvasTool);
            toolOptionsPanel_->addChild(currentTool_->optionsWidget());
            if (wasFocusedWidget) {
                currentTool_->setFocus(ui::FocusReason::Other);
            }
            toolMapInv_[canvasTool]->setChecked(true);
        }
    }
}

void CanvasApplication::onToolCheckStateChanged_(
    ui::Action* toolAction,
    ui::CheckState checkState) {

    if (checkState == ui::CheckState::Checked) {
        ui::CanvasTool* canvasTool = toolMap_[toolAction].get();
        setCurrentTool_(canvasTool);
    }
}

void CanvasApplication::createColorPalette_(ui::Widget* parent) {
    palette_ = parent->createChild<ui::ColorPalette>();
    palette_->colorSelected().connect(onColorChangedSlot_());
    onColorChanged_();
}

void CanvasApplication::onColorChanged_() {
    if (canvas_ && palette_ && sketchTool_) {
        sketchTool_->setPenColor(palette_->selectedColor());
    }
}

namespace {

std::string widgetSizingInfo(ui::Widget* w, ui::Widget* root) {

    // TODO: have our class formatters support string format syntax (e.g.,
    // {:<12}) so that we don't need the intermediate calls to core::format().
    //
    return core::format(
        "{:<24} "
        "position = {:<12} "
        "size = {:<12} "
        "preferredSize = {:<12} "
        "margin = {:<16} "
        "padding = {:<16} "
        "border = {:<16}\n",
        w->className(),
        core::format("{}", w->mapTo(root, geometry::Vec2f(0, 0))),
        core::format("{}", w->size()),
        core::format("{}", w->preferredSize()),
        core::format("{}", w->margin()),
        core::format("{}", w->padding()),
        core::format("{}", w->border()));
}

} // namespace

void CanvasApplication::onActionDebugWidgetSizing_() {

    if (!mainWindow() || !mainWidget()) {
        return;
    }

    std::string out = "Position and size information about hovered widgets:\n";
    ui::Widget* root = mainWidget();
    ui::Widget* widget = root;
    while (widget) {
        out += widgetSizingInfo(widget, root);
        widget = widget->hoverChainChild();
    }
    VGC_DEBUG(LogVgcApp, out);
}

void CanvasApplication::createColorPalette_(ui::Widget* parent) {
    palette_ = parent->createChild<ui::ColorPalette>();
}

void CanvasApplication::onColorChanged_() {
    if (canvas_ && palette_) {
        core::Color color = palette_->selectedColor();
        tool_->setPenColor(color);
        canvas_->onColorChanged_(color);
    }
}

void CanvasApplication::createCanvas_(
    ui::Widget* parent,
    workspace::Workspace* workspace) {

    canvas_ = parent->createChild<ui::Canvas>(workspace);
    tool_ = canvas_->createChild<ui::SketchTool>();
    onColorChanged_();
    if (palette_) {
        palette_->colorSelected().connect(onColorChangedSlot_());
    }
}

} // namespace vgc::app
