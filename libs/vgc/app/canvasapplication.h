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

#ifndef VGC_APP_CANVASAPPLICATION_H
#define VGC_APP_CANVASAPPLICATION_H

#include <QApplication>

#include <vgc/app/api.h>
#include <vgc/app/mainwindow.h>
#include <vgc/app/qtwidgetsapplication.h>
#include <vgc/dom/document.h>
#include <vgc/ui/action.h>
#include <vgc/ui/canvas.h>
#include <vgc/ui/colorpalette.h>
#include <vgc/ui/column.h>
#include <vgc/ui/menu.h>
#include <vgc/ui/panel.h>
#include <vgc/ui/panelarea.h>
#include <vgc/ui/sketchtool.h>
#include <vgc/workspace/workspace.h>

namespace vgc::app {

namespace detail {

VGC_APP_API
ui::Panel* createPanelWithPadding(
    ui::PanelArea* panelArea,
    std::string_view panelTitle = "Untitled");

} // namespace detail

VGC_DECLARE_OBJECT(CanvasApplication);

/// \class vgc::app::CanvasApplication
/// \brief A common class for applications with a DOM rendered in a Canvas.
///
/// This convenient class combines together:
/// - an `Application`
/// - a `MainWindow`
/// - a `Document`
/// - a `Canvas` with a ColorPalette and other basic drawing tools
/// - basic actions such as New, Open, Save, Quit, Undo, Redo, etc.
///
/// This class is used as a base for VGC Illustration but can also be used
/// for other test applications.
///
class VGC_APP_API CanvasApplication : public QtWidgetsApplication {
    VGC_OBJECT(CanvasApplication, QtWidgetsApplication)

protected:
    CanvasApplication(int argc, char* argv[], std::string_view applicationName);

public:
    /// Creates the `CanvasApplication`.
    ///
    static CanvasApplicationPtr create(
        int argc,
        char* argv[],
        std::string_view applicationName = "Canvas Application");

    /// Returns the `MainWindow` of this application.
    ///
    MainWindow* mainWindow() const {
        return window_.get();
    }

    /// Returns the `MainWidget` of this application.
    ///
    MainWidget* mainWidget() const {
        return mainWindow()->mainWidget();
    }

    /// Returns the menu bar of the `MainWidget` of this application.
    ///
    ui::Menu* menuBar() const {
        return mainWidget()->menuBar();
    }

    /// Returns the top-level panel area of the `MainWidget` of this application.
    ///
    ui::PanelArea* panelArea() const {
        return mainWidget()->panelArea();
    }

    /// Returns the active document.
    ///
    dom::Document* activeDocument() const {
        return document_;
    }

    /// Returns the current `CanvasTool`.
    ///
    ui::CanvasTool* currentTool() const {
        return currentTool_;
    }

    /// Quits the application.
    ///
    void quit();

protected:
    // Reimplementation
    void onUnhandledException(std::string_view errorMessage) override;
    void onSystemSignalReceived(std::string_view errorMessage, int sig) override;

private:
    MainWindowPtr window_;

    // ------------------------------------------------------------------------
    //                       Crash recovery

    bool recoverySave_();
    void showCrashPopup_(std::string_view errorMessage, bool wasRecoverySaved);
    void crashHandler_(std::string_view errorMessage);

    // ------------------------------------------------------------------------
    //                       Document management

    dom::Document* document_;
    core::Id lastSavedDocumentVersionId = {};
    QString filename_;
    workspace::WorkspacePtr workspace_;
    core::ConnectionHandle documentHistoryHeadChangedConnectionHandle_;

    void openDocument_(QString filename);

    ui::Action* actionNew_ = nullptr;
    VGC_SLOT(onActionNewSlot_, onActionNew_)
    void onActionNew_();

    ui::Action* actionOpen_ = nullptr;
    VGC_SLOT(onActionOpenSlot_, onActionOpen_)
    void onActionOpen_();
    void doOpen_();

    ui::Action* actionSave_ = nullptr;
    VGC_SLOT(onActionSaveSlot_, onActionSave_)
    void onActionSave_();

    ui::Action* actionSaveAs_ = nullptr;
    VGC_SLOT(onActionSaveAsSlot_, onActionSaveAs_)
    void onActionSaveAs_();
    void doSaveAs_();
    void doSave_();

    ui::Action* actionQuit_ = nullptr;
    VGC_SLOT(onActionQuitSlot_, onActionQuit_);
    void onActionQuit_();

    ui::Action* actionUndo_ = nullptr;
    VGC_SLOT(onActionUndoSlot_, onActionUndo_);
    void onActionUndo_();

    ui::Action* actionRedo_ = nullptr;
    VGC_SLOT(onActionRedoSlot_, onActionRedo_);
    void onActionRedo_();

    void updateUndoRedoActionState_();
    VGC_SLOT(updateUndoRedoActionStateSlot_, updateUndoRedoActionState_)

    // ------------------------------------------------------------------------
    //                       Menu

    void createActions_(ui::Widget* parent);
    void createMenus_();

    // ------------------------------------------------------------------------
    //                       Canvas and tools

    void createWidgets_();

    // Canvas
    ui::Canvas* canvas_ = nullptr;

    void createCanvas_(ui::Widget* parent, workspace::Workspace* workspace);

    // Canvas Tools
    ui::ActionGroupPtr toolsActionGroup_;
    std::map<ui::Action*, ui::CanvasToolPtr> toolMap_;
    std::map<ui::CanvasTool*, ui::Action*> toolMapInv_;
    ui::CanvasTool* currentTool_ = nullptr;
    ui::SketchTool* sketchTool_ = nullptr;

    void createTools_(ui::Widget* parent);
    void registerTool_( //
        ui::Widget* parent,
        std::string_view toolName,
        ui::CanvasToolPtr tool);

    void clearCurrentTool_();
    void setCurrentTool_(ui::CanvasTool* canvasTool);

    void onToolCheckStateChanged_(ui::Action* toolAction, ui::CheckState checkState);
    VGC_SLOT(onToolCheckStateChangedSlot_, onToolCheckStateChanged_);

    // Palette
    ui::ColorPalette* palette_ = nullptr;

    void createColorPalette_(ui::Widget* parent);
    void onColorChanged_();
    VGC_SLOT(onColorChangedSlot_, onColorChanged_)

    // ------------------------------------------------------------------------
    //                       Misc

    ui::Action* actionDebugWidgetSizing_ = nullptr;
    VGC_SLOT(onActionDebugWidgetSizingSlot_, onActionDebugWidgetSizing_);
    void onActionDebugWidgetSizing_();
};

} // namespace vgc::app

#endif // VGC_APP_CANVASAPPLICATION_H
