// Copyright 2018 The VGC Developers
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
#include <QMessageBox>
#include <QStandardPaths>

#include <vgc/core/logging.h>
#include <vgc/widgets/menubar.h>
#include <vgc/widgets/qtutil.h>

namespace vgc {
namespace widgets {

MainWindow::MainWindow(
    /*dom::Document* document,*/
    core::PythonInterpreter* interpreter,
    QWidget* parent) :

    QMainWindow(parent),
    /*document_(document),*/
    interpreter_(interpreter)
{
    document_ = vgc::dom::Document::create();
    vgc::dom::Element::create(document_.get(), "vgc");

    setupWidgets_();
    setupActions_();
    setupMenus_();
    setupConnections_();

    viewer_->startLoggingUnder(performanceMonitor_->log());
}

MainWindow::~MainWindow()
{
    viewer_->stopLoggingUnder(performanceMonitor_->log());
}

void MainWindow::onColorChanged(const core::Color& newColor)
{
    viewer_->setCurrentColor(newColor);
}

void MainWindow::onRenderCompleted_()
{
    performanceMonitor_->refresh();
}

void MainWindow::open()
{
    // Get which directory the dialog should display first
    QString dir =
        filename_.isEmpty() ?
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation) :
        QFileInfo(filename_).dir().path();

    // Set which existing files to show in the dialog
    QString extension = ".vgci";
    QString filters = tr("VGC Illustration Files (*%1)").arg(extension);

    // Create the dialog
    QFileDialog dialog(this, tr("Open..."), dir, filters);

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
            core::warning() << "No file selected; file not opened";
        }
        if (selectedFiles.size() == 1) {
            QString selectedFile = selectedFiles.first();
            if (!selectedFile.isEmpty()) {
                // Open
                filename_ = selectedFile;
                open_();
            }
            else {
                core::warning() << "Empty file path selected; file not opened";
            }
        }
        else {
            core::warning() << "More than one file selected; file not opened";
        }
    }
    else {
        // User willfully cancelled the operation
        // => nothing to do, not even a warning.
    }
}

void MainWindow::save()
{
    if (filename_.isEmpty()) {
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
        filename_.isEmpty() ?
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation) :
        QFileInfo(filename_).dir().path();

    // Set which existing files to show in the dialog
    QString extension = ".vgci";
    QString filters = tr("VGC Illustration Files (*%1)").arg(extension);

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

void MainWindow::open_()
{
    // XXX TODO ask save current document

    try {
        document_ = dom::Document::open(fromQt(filename_));
        viewer_->setDocument(document());
    }
    catch (const dom::FileError& e) {
        QMessageBox::critical(this, "Error Opening File", e.what());
    }
}

void MainWindow::save_()
{
    try {
        document_->save(fromQt(filename_));
    }
    catch (const dom::FileError& e) {
        QMessageBox::critical(this, "Error Saving File", e.what());
    }
}

void MainWindow::setupWidgets_()
{
    viewer_ = new OpenGLViewer(document());
    console_ = new Console(interpreter_);
    console_->hide();
    performanceMonitor_ = new PerformanceMonitor();
    performanceMonitor_->hide();
    toolbar_ = new Toolbar();

    // Initialize and keep current color synced between widgets
    onColorChanged(toolbar_->color());
    connect(toolbar_, &Toolbar::colorChanged, this, &MainWindow::onColorChanged);

    // Layout
    centralWidget_ = new CentralWidget(viewer_, toolbar_, console_, performanceMonitor_);
    setCentralWidget(centralWidget_);
}

void MainWindow::setupActions_()
{
    actionOpen_ = new QAction(tr("&Open"), this);
    actionOpen_->setStatusTip(tr("Open an existing document."));
    actionOpen_->setShortcut(QKeySequence::Open);
    connect(actionOpen_, SIGNAL(triggered()), this, SLOT(open()));

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

    actionTogglePerformanceMonitorView_ = centralWidget_->panelToggleViewAction();
    actionTogglePerformanceMonitorView_->setText(tr("Performance Monitor"));
    actionTogglePerformanceMonitorView_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_M));

    actionToggleConsoleView_ = centralWidget_->consoleToggleViewAction();
    actionToggleConsoleView_->setText(tr("Python Console"));
    actionToggleConsoleView_->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_P));
}


void MainWindow::setupMenus_()
{
    MenuBar* menuBar_ = new MenuBar();

    menuFile_ = new QMenu(tr("&File"));
    menuFile_->addAction(actionOpen_);
    menuFile_->addAction(actionSave_);
    menuFile_->addAction(actionSaveAs_);
    menuFile_->addSeparator();
    menuFile_->addAction(actionQuit_);
    menuBar_->addMenu(menuFile_);

    menuView_ = new QMenu(tr("&View"));
    menuView_->addAction(actionTogglePerformanceMonitorView_);
    menuView_->addAction(actionToggleConsoleView_);
    menuBar_->addMenu(menuView_);

    setMenuBar(menuBar_);
}

void MainWindow::setupConnections_()
{
    connect(viewer_, &OpenGLViewer::renderCompleted,
            this, &MainWindow::onRenderCompleted_);

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
