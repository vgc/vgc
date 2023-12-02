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

#ifndef VGC_APP_FILEMANAGER_H
#define VGC_APP_FILEMANAGER_H

#include <QString>

#include <vgc/app/api.h>
#include <vgc/dom/document.h>
#include <vgc/ui/module.h>
#include <vgc/workspace/workspace.h>

VGC_DECLARE_OBJECT(vgc::canvas, DocumentManager);
VGC_DECLARE_OBJECT(vgc::tools, DocumentColorPalette);

namespace vgc::app {

VGC_DECLARE_OBJECT(FileManager);

/// \class vgc::app::RecoverySaveInfo
/// \brief Information about a recovery save performed and where.
///
/// This is the return type of `FileManager::recoverySave()`.
///
class VGC_APP_API RecoverySaveInfo {
    RecoverySaveInfo(bool wasSaved, QString filename)
        : wasSaved_(wasSaved)
        , filename_(filename) {
    }

public:
    /// Constructs a `RecoverySaveInfo` indicating that no recovery file was
    /// saved.
    ///
    static RecoverySaveInfo notSaved() {
        return RecoverySaveInfo(false, "");
    }

    /// Constructs a `RecoverySaveInfo` indicating that a recovery file was
    /// saved to the given `filename`.
    ///
    static RecoverySaveInfo savedTo(QString filename) {
        return RecoverySaveInfo(true, filename);
    }

    /// Returns whether a recovery save was successfully done.
    ///
    bool wasSaved() const {
        return wasSaved_;
    }

    /// Returns the filename where the recovery saved was performed.
    ///
    QString filename() const {
        return filename_;
    }

private:
    friend FileManager;

    bool wasSaved_ = false;
    QString filename_;
};

/// \class vgc::app::FileManager
/// \brief A module providing the usual File functionality (New, Open, Save, etc.).
///
// Note: This is in the `app` library rather than the `tools` library because
// we use some of QtWidgets functionality here, such as QFileDialog and
// QMessageBox. In the future, we may want to abstract these away so that this
// class would be moved to the `tools` library where it probably makes more
// sense to be.
//
class VGC_APP_API FileManager : public ui::Module {
private:
    VGC_OBJECT(FileManager, ui::Module)

protected:
    FileManager(CreateKey, const ui::ModuleContext& context);

public:
    /// Creates the `CurrentColor` module.
    ///
    static FileManagerPtr create(const ui::ModuleContext& context);

    /// Performs a recovery save, that is, attempt to save the current document
    /// after a crash with a new name in a standard location, so that it can
    /// later be re-opened and potentially repaired, minimizing user data loss.
    ///
    RecoverySaveInfo recoverySave();

    /// The quit action was triggered
    ///
    // XXX Move this to StandardMenus? Better design allowing listeners to
    // cancel the quit?
    //
    VGC_SIGNAL(quitTriggered)

private:
    canvas::DocumentManagerWeakPtr documentManager_;

    // For now we need this. TODO: Have documentColorPalette_
    // listen to changes of documentManager itself
    tools::DocumentColorPaletteWeakPtr documentColorPalette_;

    core::Id lastSavedDocumentVersionId = {};
    QString filename_;

    // Closes current document.
    // Returns false if user answers "Cancel" to "Do you want to save?".
    bool maybeCloseCurrentDocument_();

    void openDocument_(QString filename);

    void onActionNew_();
    VGC_SLOT(onActionNew_)

    void onActionOpen_();
    void doOpen_();
    VGC_SLOT(onActionOpen_)

    void onActionSave_();
    VGC_SLOT(onActionSave_)

    void onActionSaveAs_();
    VGC_SLOT(onActionSaveAs_)

    void doSaveAs_();
    void doSave_();

    void onActionQuit_();
    VGC_SLOT(onActionQuit_)

    ui::ActionWeakPtr actionUndo_ = nullptr;
    void onActionUndo_();
    VGC_SLOT(onActionUndo_)

    ui::ActionWeakPtr actionRedo_ = nullptr;
    void onActionRedo_();
    VGC_SLOT(onActionRedo_)

    void updateUndoRedoActionState_();
    VGC_SLOT(updateUndoRedoActionState_)
};

} // namespace vgc::app

#endif // VGC_APP_FILEMANAGER_H
