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
#include <vgc/dom/strings.h>
#include <vgc/ui/qtutil.h>
#include <vgc/ui/shortcut.h>

namespace vgc::app {

namespace {

core::StringId left_sidebar("left-sidebar");
core::StringId with_padding("with-padding");
core::StringId user_("user");
core::StringId colorpalette_("colorpalette");
core::StringId colorpaletteitem_("colorpaletteitem");
core::StringId color_("color");

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
    for (dom::Element* user : root->childElements(user_)) {
        for (dom::Element* colorpalette : user->childElements(colorpalette_)) {
            for (dom::Element* item : colorpalette->childElements(colorpaletteitem_)) {
                core::Color color = item->getAttribute(color_).getColor();
                colors.append(color);
            }
        }
    }

    // Delete <user> element
    dom::Element* user = root->firstChildElement(user_);
    while (user) {
        dom::Element* nextUser = user->nextSiblingElement(user_);
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
        dom::Element* user = dom::Element::create(root, user_);
        dom::Element* colorpalette = dom::Element::create(user, colorpalette_);
        for (Int i = 0; i < listView->numColors(); ++i) {
            const core::Color& color = listView->colorAt(i);
            dom::Element* item = dom::Element::create(colorpalette, colorpaletteitem_);
            item->setAttribute(color_, color);
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

Panel* createPanel(ui::PanelArea* panelArea) {
    Panel* panel = panelArea->createChild<Panel>();
    panel->addStyleClass(ui::strings::Panel);
    return panel;
}

Panel* createPanelWithPadding(ui::PanelArea* panelArea) {
    Panel* panel = createPanel(panelArea);
    panel->addStyleClass(with_padding);
    return panel;
}

} // namespace detail

CanvasApplication::CanvasApplication(
    int argc,
    char* argv[],
    std::string_view applicationName)

    : Application(argc, argv) {

    window_ = app::MainWindow::create(applicationName);
    openDocument_("");
    createWidgets_();
}

CanvasApplicationPtr
CanvasApplication::create(int argc, char* argv[], std::string_view applicationName) {
    return CanvasApplicationPtr(new CanvasApplication(argc, argv, applicationName));
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

void CanvasApplication::createWidgets_() {

    using Panel = detail::Panel;
    using detail::createPanel;
    using detail::createPanelWithPadding;

    createActions_(window_->mainWidget());
    createMenus_();

    // Create panel areas
    ui::PanelArea* mainArea = window_->mainWidget()->panelArea();
    mainArea->setType(ui::PanelAreaType::HorizontalSplit);
    ui::PanelArea* leftArea = ui::PanelArea::createTabs(mainArea);
    leftArea->addStyleClass(left_sidebar);
    ui::PanelArea* middleArea = ui::PanelArea::createTabs(mainArea);

    // Create panels
    Panel* leftPanel = createPanelWithPadding(leftArea);
    Panel* middlePanel = createPanel(middleArea);

    // Create widgets inside panels
    createColorPalette_(leftPanel);
    createCanvas_(middlePanel, workspace_.get());
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

    updateUndoRedoActionState_();
}

void CanvasApplication::createMenus_() {

    ui::Menu* menuBar = window_->mainWidget()->menuBar();

    // XXX Do we need this? (was in UI Test)
    //menuBar->setPopupEnabled(false);

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

void CanvasApplication::onActionNew_() {

    // XXX TODO ask save current document

    // XXX Fix following bug:
    // - Draw something
    // - Save As
    // - New
    // - Save => this saves to the previous file (effectively erasing its content,
    //           with no possibility to undo and save again

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
    if (window_) {
        window_->close();
    }
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

void CanvasApplication::createColorPalette_(ui::Widget* parent) {
    palette_ = parent->createChild<ui::ColorPalette>();
}

void CanvasApplication::onColorChanged_() {
    if (canvas_ && palette_) {
        canvas_->setCurrentColor(palette_->selectedColor());
    }
}

void CanvasApplication::createCanvas_(
    ui::Widget* parent,
    workspace::Workspace* workspace) {

    canvas_ = parent->createChild<ui::Canvas>(workspace);
    onColorChanged_();
    if (palette_) {
        palette_->colorSelected().connect(onColorChangedSlot_());
    }
}

} // namespace vgc::app
