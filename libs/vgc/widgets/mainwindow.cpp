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

#include <QMenu>
#include <QMenuBar>
#include <QSplitter>
#include <vgc/core/python.h>
#include <vgc/scene/scene.h>
#include <vgc/widgets/console.h>
#include <vgc/widgets/openglviewer.h>

namespace vgc {
namespace widgets {

MainWindow::MainWindow(
    scene::Scene* scene,
    core::PythonInterpreter* interpreter,
    QWidget* parent) :

    QMainWindow(parent),
    scene_(scene)
{
    // Create OpenGLViewer
    OpenGLViewer* viewer = new OpenGLViewer(scene);

    // Create console
    Console* console = new Console(interpreter);

    // Create splitter and populate widget
    QSplitter* splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(viewer);
    splitter->addWidget(console);
    setCentralWidget(splitter);

    // Show maximized at startup
    // XXX This should be a user preference
    showMaximized();

    // Refresh viewer when scene changes
    scene->changed.connect(std::bind(
        (void (OpenGLViewer::*)()) &OpenGLViewer::update, viewer));

    // Don't refresh the view while the Python interpreter is running.
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
    interpreter->runStarted.connect(std::bind(
        &scene::Scene::pauseSignals, scene));
    interpreter->runFinished.connect(std::bind(
        &scene::Scene::resumeSignals, scene, true));

    createActions_();
    createMenus_();
}

MainWindow::~MainWindow()
{

}

void MainWindow::createActions_()
{
    actionQuit_ = new QAction(tr("&Quit"), this);
    actionQuit_->setStatusTip(tr("Quit VGC Illustration."));
    actionQuit_->setShortcut(QKeySequence::Quit);
    connect(actionQuit_, SIGNAL(triggered()), this, SLOT(close()));
}

void MainWindow::createMenus_()
{
    menuFile_ = new QMenu(tr("&File"));
    menuFile_->addAction(actionQuit_);
    menuBar()->addMenu(menuFile_);
}

} // namespace widgets
} // namespace vgc
