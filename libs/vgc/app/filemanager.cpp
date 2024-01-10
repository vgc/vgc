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

#include <vgc/app/filemanager.h>

#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>

#include <vgc/app/logcategories.h>
#include <vgc/canvas/documentmanager.h>
#include <vgc/core/datetime.h>
#include <vgc/tools/documentcolorpalette.h>
#include <vgc/ui/key.h>
#include <vgc/ui/menu.h>
#include <vgc/ui/modifierkey.h>
#include <vgc/ui/qtutil.h>
#include <vgc/ui/shortcut.h>
#include <vgc/ui/standardmenus.h>

namespace vgc::app {

namespace {

namespace commands {

using ui::Key;
using ui::Shortcut;
using ui::modifierkeys::ctrl;
using ui::modifierkeys::shift;

VGC_UI_DEFINE_WINDOW_COMMAND( //
    _new,
    "file.new",
    "New",
    Shortcut(ctrl, Key::N))

VGC_UI_DEFINE_WINDOW_COMMAND(
    open, //
    "file.open",
    "Open...",
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

} // namespace commands

void addSeparator_(ui::Menu* menu) {
    if (menu) {
        menu->addSeparator();
    }
}

template<typename TSlot>
ui::Action*
createAction_(ui::Module& self, ui::Menu* menu, core::StringId commandId, TSlot slot) {
    ui::Action* action = self.createTriggerAction(commandId);
    action->triggered().connect(slot);
    if (menu) {
        menu->addItem(action);
    }
    return action;
}

} // namespace

FileManager::FileManager(CreateKey key, const ui::ModuleContext& context)
    : Module(key, context) {

    documentManager_ = context.importModule<canvas::DocumentManager>();
    documentColorPalette_ = context.importModule<tools::DocumentColorPalette>();

    ui::MenuLockPtr lFileMenu;
    ui::MenuLockPtr lEditMenu;
    if (auto standardMenus = context.importModule<ui::StandardMenus>().lock()) {
        lFileMenu = standardMenus->getOrCreateFileMenu().lock();
        lEditMenu = standardMenus->getOrCreateEditMenu().lock();
    }
    ui::Menu* fileMenu = lFileMenu.get();
    ui::Menu* editMenu = lEditMenu.get();

    createAction_(*this, fileMenu, commands::_new(), onActionNew_Slot());
    createAction_(*this, fileMenu, commands::open(), onActionOpen_Slot());
    addSeparator_(fileMenu);
    createAction_(*this, fileMenu, commands::save(), onActionSave_Slot());
    createAction_(*this, fileMenu, commands::saveAs(), onActionSaveAs_Slot());
    addSeparator_(fileMenu);
    createAction_(*this, fileMenu, commands::quit(), onActionQuit_Slot());

    // XXX: Make these generic actions? Note that we currently cannot: generic
    // actions rely on `Action::owningWidget` so they don't work in a module.
    //
    actionUndo_ = createAction_(*this, editMenu, commands::undo(), onActionUndo_Slot());
    actionRedo_ = createAction_(*this, editMenu, commands::redo(), onActionRedo_Slot());

    openDocument_("");
    updateUndoRedoActionState_();
}

FileManagerPtr FileManager::create(const ui::ModuleContext& context) {
    return core::createObject<FileManager>(context);
}

RecoverySaveInfo FileManager::recoverySave() {

    auto documentManager = documentManager_.lock();
    if (!documentManager) {
        return RecoverySaveInfo::notSaved();
    }
    dom::Document* document = documentManager->currentDocument();
    if (!document) {
        return RecoverySaveInfo::notSaved();
    }

    // It is risky to try to undo or abort the history since
    // it could cause another exception.
    // Thus we simply disable the history for the color palette
    // save operation.
    //
    core::History* history = document->history();
    if (history) {
        document->disableHistory();
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
            return RecoverySaveInfo::savedTo(filename_);
        }
    }

    // Failed to save.
    //
    return RecoverySaveInfo::notSaved();
}

namespace {

// XXX Move to core?
template<typename T>
[[nodiscard]] core::ObjLockPtr<T> lockAndThrowIfNull(const core::ObjWeakPtr<T>& ptr) {
    auto locked = ptr.lock();
    if (!locked) {
        throw core::NullError();
    }
    return locked;
}

} // namespace

bool FileManager::maybeCloseCurrentDocument_() {

    auto documentManager = lockAndThrowIfNull(documentManager_);
    auto documentColorPalette = lockAndThrowIfNull(documentColorPalette_);

    workspace::Workspace* workspace = documentManager->currentWorkspace();
    if (workspace) {
        workspace->sync();
        dom::Document* document = workspace->document();
        if (document && document->versionId() != lastSavedDocumentVersionId) {
            // XXX "Do you want to save ?" => return false if "Cancel" pressed.
        }
        core::History* history = workspace->history();
        if (history) {
            history->disconnect(this);
        }
        // There used to be `canvas_->setWorkspace(nullptr);` here
        // TODO: have canvas listen to workspace change via the DocumentManager
    }

    filename_.clear();
    documentManager->setCurrentWorkspace(nullptr);

    // TODO: have DocumentColorPalette listen to document changes via the DocumentManager
    documentColorPalette->setDocument(nullptr);

    return true;
}

namespace {

// Creates a new Document:
// - An empty document if filename is empty
// - Otherwise reads the file.
//
// In case there are errors while reading the file, we show a critical
// error to the user. XXX: Wouldn't a normal error be enough?
//
dom::DocumentSharedPtr createDocument_(QString filename) {
    if (filename.isEmpty()) {
        dom::DocumentSharedPtr document = dom::Document::create();
        dom::Element::create(document.get(), "vgc");
        return document;
    }
    else {
        try {
            return dom::Document::open(ui::fromQt(filename));
        }
        catch (const dom::FileError& e) {
            // TODO: have our own message box instead of using QtWidgets
            QMessageBox::critical(nullptr, "Error Opening File", e.what());
            return nullptr;
        }
    }
}

} // namespace

void FileManager::openDocument_(QString filename) {

    bool closed = maybeCloseCurrentDocument_();
    if (!closed) {
        return;
    }

    auto documentManager = lockAndThrowIfNull(documentManager_);
    auto documentColorPalette = lockAndThrowIfNull(documentColorPalette_);

    // Create an empty document (if filename is empty) or open from file.
    //
    // Note that if `document` is null (i.e., in case of errors opening the
    // file), we simply do nothing: indeed, the Workspace class does not
    // currently support having a null document. The above call to the function
    // `maybeCloseCurrentDocument_()` has already left the Manager in a valid
    // state via setWorkspace(nullptr).
    //
    dom::DocumentSharedPtr document = createDocument_(filename);
    if (!document) {
        return;
    }

    // Gets the <colorpalette> from the document, then deletes it as if it
    // never existed (we'll re-create it just before saving). This has to be
    // done before we enable the history on the document.
    //
    // TODO: Better design
    //
    documentColorPalette->setDocument(document.get());

    // Synchronize history with undo/redo action state.
    //
    core::History* history = document->enableHistory(dom::strings::New_Document);
    history->headChanged().connect(updateUndoRedoActionState_Slot());
    updateUndoRedoActionState_();

    // Create a Workspace based on the document.
    //
    auto workspace = workspace::Workspace::create(document);
    filename_ = filename;
    documentManager->setCurrentWorkspace(workspace);

    // There used to be `canvas_->setWorkspace(workspace_.get());` here
    // TODO: have canvas listen to workspace change via the DocumentManager
}

void FileManager::onActionNew_() {
    openDocument_("");
}

void FileManager::onActionOpen_() {
    doOpen_();
}

void FileManager::doOpen_() {

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

void FileManager::onActionSave_() {
    if (filename_.isEmpty()) {
        doSaveAs_();
    }
    else {
        doSave_();
    }
}

void FileManager::onActionSaveAs_() {
    doSaveAs_();
}

void FileManager::doSaveAs_() {

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

void FileManager::doSave_() {
    try {
        if (auto documentManager = documentManager_.lock()) {
            if (dom::Document* document = documentManager->currentDocument()) {
                if (auto documentColorPalette = documentColorPalette_.lock()) {
                    auto saver = documentColorPalette->saver();
                    document->save(ui::fromQt(filename_));
                }
                else {
                    document->save(ui::fromQt(filename_));
                }
            }
        }
    }
    catch (const dom::FileError& e) {
        QMessageBox::critical(nullptr, "Error Saving File", e.what());
    }
}

// XXX: is this better to have this logic here or in CanvasApplication?
//
void FileManager::onActionQuit_() {
    bool closed = maybeCloseCurrentDocument_();
    if (closed) {
        quitTriggered().emit();
    }
}

void FileManager::onActionUndo_() {
    if (auto documentManager = documentManager_.lock()) {
        if (workspace::Workspace* workspace = documentManager->currentWorkspace()) {
            if (core::History* history = workspace->history()) {
                history->undo();
            }
        }
    }
}

void FileManager::onActionRedo_() {
    if (auto documentManager = documentManager_.lock()) {
        if (workspace::Workspace* workspace = documentManager->currentWorkspace()) {
            if (core::History* history = workspace->history()) {
                history->redo();
            }
        }
    }
}

void FileManager::updateUndoRedoActionState_() {
    bool canUndo = false;
    bool canRedo = false;
    if (auto documentManager = documentManager_.lock()) {
        if (workspace::Workspace* workspace = documentManager->currentWorkspace()) {
            if (core::History* history = workspace->history()) {
                canUndo = history->canUndo();
                canRedo = history->canRedo();
            }
        }
    }
    if (auto actionUndo = actionUndo_.lock()) {
        actionUndo->setEnabled(canUndo);
    }
    if (auto actionRedo = actionRedo_.lock()) {
        actionRedo->setEnabled(canRedo);
    }
}

} // namespace vgc::app
