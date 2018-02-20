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

#include <QMenuBar>
#include <QToolBar>

namespace vgc {
namespace widgets {

MainWindow::MainWindow(
    scene::Scene* scene,
    core::PythonInterpreter* interpreter,
    QWidget* parent) :

    QMainWindow(parent),
    scene_(scene),
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
    scene_->setNewCurveColor(newColor);
}

void MainWindow::setupWidgets_()
{
    viewer_ = new OpenGLViewer(scene_);
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
    menuFile_->addAction(actionQuit_);
    menuBar()->addMenu(menuFile_);

    menuView_ = new QMenu(tr("&View"));
    menuView_->addAction(actionToggleConsoleView_);
    menuBar()->addMenu(menuView_);
}

void MainWindow::setupToolBars_()
{
    int iconWidth = 32;
    QSize iconSize(iconWidth,iconWidth);

    toolBar_ = addToolBar(tr("Toolbar"));
    toolBar_->setOrientation(Qt::Vertical);
    toolBar_->setMovable(false);
    toolBar_->setIconSize(iconSize);
    addToolBar(Qt::LeftToolBarArea, toolBar_);

    colorSelector_ = new ColorSelector();
    colorSelector_->setToolTip(tr("Current color (C)"));
    colorSelector_->setStatusTip(tr("Click to open the color selector"));
    colorSelector_->setIconSize(iconSize);
    colorSelector_->updateIcon();

    colorSelectorAction_ = toolBar_->addWidget(colorSelector_);
    colorSelectorAction_->setText(tr("Color"));
    colorSelectorAction_->setToolTip(tr("Color (C)"));
    colorSelectorAction_->setStatusTip(tr("Click to open the color selector"));
    colorSelectorAction_->setShortcut(QKeySequence(Qt::Key_C));
    colorSelectorAction_->setShortcutContext(Qt::ApplicationShortcut);

    connect(colorSelectorAction_, SIGNAL(triggered()), colorSelector_, SLOT(click()));
    connect(colorSelector_, &ColorSelector::colorChanged, this, &MainWindow::onColorChanged);
}

void MainWindow::setupConnections_()
{
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
}

} // namespace widgets
} // namespace vgc
