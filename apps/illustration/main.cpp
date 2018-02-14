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

#include <QApplication>
#include <vgc/core/python.h>
#include <vgc/scene/scene.h>
#include <vgc/widgets/mainwindow.h>
#include <vgc/widgets/openglviewer.h>

namespace py = pybind11;

int main(int argc, char *argv[])
{
    // Init OpenGL. Must be called before QApplication creation. See Qt doc:
    //
    // Calling QSurfaceFormat::setDefaultFormat() before constructing the
    // QApplication instance is mandatory on some platforms (for example,
    // macOS) when an OpenGL core profile context is requested. This is to
    // ensure that resource sharing between contexts stays functional as all
    // internal contexts are created using the correct version and profile.
    //
    vgc::widgets::OpenGLViewer::init();

    // Creates the QApplication
    QApplication a(argc, argv);

    // Create the python interpreter
    vgc::core::PythonInterpreter pythonInterpreter;

    // Create the scene
    vgc::scene::SceneSharedPtr scene = vgc::scene::Scene::make();

    // Expose the above Scene instance to the Python console as a local Python
    // variable 'scene'.
    //
    // XXX In the long term, we may not want to expose "scene" directly, but:
    // 1. Have a class VgcIllustrationApp: public QApplication.
    // 2. Have an instance 'VgcIllustrationApp app'.
    // 3. Pass the app to python.
    // 4. Users can call things like:
    //      app.scene() (or just app.scene, which is more pythonic)
    //      app.currentScene()
    //      app.scenes()
    //      etc.
    //
    // One advantage is that the calls above can be made read-only.
    // Currently, users can do scene = Scene() and then are not able
    // to affect the actual scene anymore...
    //
    pythonInterpreter.run("import vgc.scene");
    pythonInterpreter.setVariableValue("scene", scene);

    // Create and show the widget
    vgc::widgets::MainWindow w(scene.get(), &pythonInterpreter);
    w.show();

    return a.exec();
}
