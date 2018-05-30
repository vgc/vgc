// Copyright 2017 The VGC Developers
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

#include <vgc/widgets/mainwindow.h>

#include <QFileDialog>
#include <QFileInfo>
#include <QMenuBar>
#include <QStandardPaths>
#include <QToolBar>

#include <vgc/core/logging.h>
#include <vgc/widgets/qtutil.h>

namespace vgc {
namespace widgets {

MainWindow::MainWindow(
    dom::Document* document,
    core::PythonInterpreter* interpreter,
    QWidget* parent) :

    QMainWindow(parent),
    document_(document),
    interpreter_(interpreter)
{
    setupWidgets_();
    setupCentralWidget_();
    setupDocks_();
    setupActions_();
    setupMenus_();
    setupToolBars_();
    setupConnections_();

    // Show maximized at startup
    // XXX This should be a user preference
    showMaximized();
}

MainWindow::~MainWindow()
{

}

void MainWindow::onColorChanged(const core::Color& newColor)
{
    viewer_->setCurrentColor(newColor);
}

void MainWindow::save()
{
    if (saveFilename_.isEmpty()) {
        saveAs();
    }
    else {
        save_();
    }
}

void MainWindow::saveAs()
{
    // Get which directory the dialog should display first
    QString dir =
        saveFilename_.isEmpty() ?
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation) :
        QFileInfo(saveFilename_).dir().path();

    // Set which existing files to show in the dialog
    QString filters = tr("VGC Illustration Files (*.vgci)");

    // Create the dialog
    QFileDialog dialog(this, tr("Save As..."), dir, filters);

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
            core::warning() << "No file selected; file not saved";
        }
        if (selectedFiles.size() == 1) {
            QString selectedFile = selectedFiles.first();
            if (!selectedFile.isEmpty()) {
                saveFilename_ = selectedFile;
                save_();
            }
            else {
                core::warning() << "Empty file path selected; file not saved";
            }
        }
        else {
            core::warning() << "More than one file selected; file not saved";
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

void MainWindow::save_()
{
    document_->save(fromQt(saveFilename_));
}

void MainWindow::setupWidgets_()
{
    viewer_ = new OpenGLViewer(document_);
    console_ = new Console(interpreter_);
}

void MainWindow::setupCentralWidget_()
{
    setCentralWidget(viewer_);
}

void MainWindow::setupDocks_()
{
    dockConsole_ = new QDockWidget(tr("Python Console"));
    dockConsole_->setWidget(console_);
    dockConsole_->setFeatures(QDockWidget::DockWidgetClosable);
    dockConsole_->setTitleBarWidget(new QWidget());
    addDockWidget(Qt::BottomDockWidgetArea, dockConsole_);
    dockConsole_->hide();
}

void MainWindow::setupActions_()
{
    actionSave_ = new QAction(tr("&Save"), this);
    actionSave_->setStatusTip(tr("Save the current document."));
    actionSave_->setShortcut(QKeySequence::Save);
    connect(actionSave_, SIGNAL(triggered()), this, SLOT(save()));

    actionSaveAs_ = new QAction(tr("Save As..."), this);
    actionSaveAs_->setStatusTip(tr("Save the current document under a new name."));
    actionSaveAs_->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_S));
    connect(actionSaveAs_, SIGNAL(triggered()), this, SLOT(saveAs()));
    // Note: we don't use QKeySequence::SaveAs because it is undefined on
    // Windows and KDE. XXX TODO: Have a proper Shortcut manager. Might be
    // best to not use any of Qt default shortcuts.

    actionQuit_ = new QAction(tr("&Quit"), this);
    actionQuit_->setStatusTip(tr("Quit VGC Illustration."));
    actionQuit_->setShortcut(QKeySequence::Quit);
    connect(actionQuit_, SIGNAL(triggered()), this, SLOT(close()));

    actionToggleConsoleView_ = dockConsole_->toggleViewAction();
    actionToggleConsoleView_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_P));
}


void MainWindow::setupMenus_()
{
    menuFile_ = new QMenu(tr("&File"));
    menuFile_->addAction(actionSave_);
    menuFile_->addAction(actionSaveAs_);
    menuFile_->addSeparator();
    menuFile_->addAction(actionQuit_);
    menuBar()->addMenu(menuFile_);

    menuView_ = new QMenu(tr("&View"));
    menuView_->addAction(actionToggleConsoleView_);
    menuBar()->addMenu(menuView_);
}

void MainWindow::setupToolBars_()
{
    int iconWidth = 48;
    QSize iconSize(iconWidth,iconWidth);

    toolBar_ = addToolBar(tr("Toolbar"));
    toolBar_->setOrientation(Qt::Vertical);
    toolBar_->setMovable(false);
    toolBar_->setIconSize(iconSize);
    addToolBar(Qt::LeftToolBarArea, toolBar_);

    colorToolButton_ = new ColorToolButton();
    colorToolButton_->setToolTip(tr("Current color (C)"));
    colorToolButton_->setStatusTip(tr("Click to open the color selector"));
    colorToolButton_->setIconSize(iconSize);
    colorToolButton_->updateIcon();

    colorToolButtonAction_ = toolBar_->addWidget(colorToolButton_);
    colorToolButtonAction_->setText(tr("Color"));
    colorToolButtonAction_->setToolTip(tr("Color (C)"));
    colorToolButtonAction_->setStatusTip(tr("Click to open the color selector"));
    colorToolButtonAction_->setShortcut(QKeySequence(Qt::Key_C));
    colorToolButtonAction_->setShortcutContext(Qt::ApplicationShortcut);

    connect(colorToolButtonAction_, SIGNAL(triggered()), colorToolButton_, SLOT(click()));
    connect(colorToolButton_, &ColorToolButton::colorChanged, this, &MainWindow::onColorChanged);

    // Update current color of widgets created before the color selector
    onColorChanged(colorToolButton_->color());
}

void MainWindow::setupConnections_()
{
    // XXX TODO

    /*
    // Refresh the viewer when the scene changes
    scene_->changed.connect(std::bind(
        (void (OpenGLViewer::*)()) &OpenGLViewer::update, viewer_));

    // Prevent refreshing the viewer when the Python interpreter is running.
    //
    // Note 1:
    //
    // this could also be done by the owner of this widget. It is yet unclear
    // at this point which is preferable. In any case, we have to keep in mind
    // that this widget is only *one* observer of the scene. Maybe other
    // observers wouldn't want the signals of the scene to be aggregated? maybe
    // it's the role of all observers to aggreate the signals, though this
    // means duplicate work in case of simultaneous observers?
    //
    // I'd say both make sense in different scenarios. A flexible design would
    // be that by default, we do not call Scene::pauseSignals/resumeSignals,
    // and instead call Viewer::pauseRendering/resumeRendering. But since only
    // the scene library knows how to aggregate signals, there could be a
    // std::vector<Signals> scene::aggregateSignals(const
    // std::vector<Signals>&) helper method available to viewers.
    //
    // In case a manager knows that there are multiple viewers, then the
    // aggregation may be done by the manager, who will then pass the shared
    // aggregation to all viewers.
    //
    // Note 2:
    //
    // Maybe we do not want to pause the signals when the *interpreter* runs
    // but only when the *console* asks the interpreter to run something. It is
    // yet unclear at this point which is preferable.
    //
    interpreter_->runStarted.connect(std::bind(
        &scene::Scene::pauseSignals, scene_));
    interpreter_->runFinished.connect(std::bind(
        &scene::Scene::resumeSignals, scene_, true));
        */
}

} // namespace widgets
} // namespace vgc
