// Copyright 2021 The VGC Developers
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

#ifndef VGC_WIDGETS_MAINWINDOW_H
#define VGC_WIDGETS_MAINWINDOW_H

#include <QDockWidget>
#include <QMainWindow>
#include <QMenu>
#include <QString>

#include <vgc/core/object.h>
#include <vgc/core/python.h>
#include <vgc/dom/document.h>
#include <vgc/widgets/api.h>
#include <vgc/widgets/centralwidget.h>
#include <vgc/widgets/console.h>
#include <vgc/widgets/openglviewer.h>
#include <vgc/widgets/panel.h>
#include <vgc/widgets/performancemonitor.h>
#include <vgc/widgets/toolbar.h>

namespace vgc::widgets {

class VGC_WIDGETS_API MainWindow : public QMainWindow {
private:
    Q_OBJECT

public:
    MainWindow(core::PythonInterpreter* interpreter, QWidget* parent = nullptr);

    ~MainWindow();

    dom::Document* document() const {
        return document_.get();
    }

private Q_SLOTS:
    void onColorChanged(const core::Color& newColor);
    void onRenderCompleted_();
    void open();
    void save();
    void saveAs();
    void undo();
    void redo();
    void new_();

private:
    dom::DocumentPtr document_;
    core::PythonInterpreter* interpreter_;

    // XXX move what's below out of MainWindow to keep it generic.
    // Specific content within the MainWindow should be in a
    // class such as "VgcIllustrationMainWindow".

    void setupWidgets_();
    CentralWidget* centralWidget_;
    Toolbar* toolbar_;
    OpenGLViewer* viewer_;
    Console* console_;
    PerformanceMonitor* performanceMonitor_;
    Panel* performanceMonitorPanel_;

    void setupActions_();
    QAction* actionNew_;
    QAction* actionOpen_;
    QAction* actionSave_;
    QAction* actionSaveAs_;
    QAction* actionQuit_;
    QAction* actionUndo_;
    QAction* actionRedo_;
    QAction* actionToggleConsoleView_;
    QAction* actionTogglePerformanceMonitorView_;

    void setupMenus_();
    QMenu* menuFile_;
    QMenu* menuEdit_;
    QMenu* menuView_;

    void setupConnections_();

    // Saves or opens the document at the given filename
    void open_();
    void save_();
    QString filename_;

    void updateUndoRedoActionState_();
    core::ConnectionHandle headChangedConnectionHandle_;
};

} // namespace vgc::widgets

#endif // VGC_WIDGETS_MAINWINDOW_H
