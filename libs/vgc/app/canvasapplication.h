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
#include <vgc/canvas/canvas.h>
#include <vgc/canvas/toolmanager.h>
#include <vgc/core/colors.h>
#include <vgc/tools/colorpalette.h>
#include <vgc/tools/paintbucket.h>
#include <vgc/tools/sketch.h>
#include <vgc/ui/action.h>
#include <vgc/ui/column.h>
#include <vgc/ui/menu.h>
#include <vgc/ui/panel.h>
#include <vgc/ui/panelarea.h>
#include <vgc/ui/panelmanager.h>

namespace vgc::tools {

VGC_DECLARE_OBJECT(CurrentColor);
VGC_DECLARE_OBJECT(DocumentColorPalette);

} // namespace vgc::tools

namespace vgc::app {

namespace detail {

VGC_APP_API
ui::Panel* createPanelWithPadding(
    ui::PanelManager* panelManager,
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
    CanvasApplication(
        CreateKey,
        int argc,
        char* argv[],
        std::string_view applicationName);

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

    /// Returns the `PanelManager` of this application.
    ///
    ui::PanelManager* panelManager() const {
        return panelManager_.get();
    }

    /// Quits the application.
    ///
    void quit();
    VGC_SLOT(quit)

protected:
    // Reimplementation
    void onUnhandledException(std::string_view errorMessage) override;
    void onSystemSignalReceived(std::string_view errorMessage, int sig) override;

private:
    MainWindowPtr window_;

    // ------------------------------------------------------------------------
    //                       Crash recovery

    void crashHandler_(std::string_view errorMessage);

    // ------------------------------------------------------------------------
    //                       Menu

    void createActions_(ui::Widget* parent);

    // ------------------------------------------------------------------------
    //                       Panels

    ui::PanelManagerPtr panelManager_;
    ui::PanelAreaPtr mainPanelArea_;
    ui::PanelAreaPtr leftPanelArea_;

    void registerPanelTypes_();
    void createDefaultPanels_();
    ui::PanelArea* getOrCreateLeftPanelArea_();
    void onActionOpenPanel_(ui::PanelTypeId id);

    // Tools
    canvas::ToolManagerPtr toolManager_;
    tools::Sketch* sketchTool_ = nullptr;
    tools::PaintBucket* paintBucketTool_ = nullptr;
    void createTools_(canvas::Canvas* canvas);

    // Colors
    tools::CurrentColorPtr currentColor_;
    tools::DocumentColorPalettePtr documentColorPalette_;
    void onCurrentColorChanged_(const core::Color& color);
    VGC_SLOT(onCurrentColorChanged_)
};

} // namespace vgc::app

#endif // VGC_APP_CANVASAPPLICATION_H
