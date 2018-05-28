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
#include <vgc/dom/document.h>
#include <vgc/scene/scene.h>
#include <vgc/widgets/font.h>
#include <vgc/widgets/mainwindow.h>
#include <vgc/widgets/openglviewer.h>
#include <vgc/widgets/qtutil.h>
#include <vgc/widgets/stylesheets.h>

#include <iostream>
#include <vgc/core/color.h>
#include <vgc/core/doublearray.h>
#include <vgc/core/vec2darray.h>
#include <vgc/dom/vgc.h>
#include <vgc/dom/path.h>

namespace py = pybind11;

int main(int argc, char* argv[])
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
    QApplication application(argc, argv);

    // Create the python interpreter
    vgc::core::PythonInterpreter pythonInterpreter;

    // Create the scene
    //vgc::scene::SceneSharedPtr scene = vgc::scene::Scene::make();
    vgc::dom::DocumentSharedPtr document = vgc::dom::Document::make();
    document->setRootElement(vgc::dom::Vgc::make());

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
    //pythonInterpreter.run("import vgc.scene");
    //pythonInterpreter.setVariableValue("scene", scene);

    // Create and show the widget
    vgc::widgets::MainWindow w(document.get(), &pythonInterpreter);
    w.show();

    // Set style
    // XXX Create widgets::Application class with:
    // Application::addFont(resourceName)
    // Application::setStyleSheet(resourceName)
    vgc::widgets::addApplicationFont("widgets/fonts/SourceSansPro-Regular.ttf");
    vgc::widgets::addApplicationFont("widgets/fonts/SourceCodePro-Regular.ttf");
    vgc::widgets::setApplicationStyleSheet("widgets/stylesheets/dark.qss");

    // ---- Test dom ----

    /*
    // Create a document
    vgc::dom::DocumentSharedPtr doc = vgc::dom::Document::make();

    // Create the root element of the document
    vgc::dom::VgcSharedPtr root = vgc::dom::Vgc::make();
    doc->setRootElement(root);

    // Create two children elements
    vgc::dom::PathSharedPtr p1 = vgc::dom::Path::make();
    vgc::dom::PathSharedPtr p2 = vgc::dom::Path::make();
    root->appendChild(p1);
    root->appendChild(p2);

    // Author 'positions' attributes
    vgc::core::StringId positions("positions");
    vgc::core::Vec2dArray pos = {
        vgc::core::Vec2d(1, 2),
        vgc::core::Vec2d(12, 42)
    };
    p1->setAttribute(positions, pos);
    pos.append(vgc::core::Vec2d(5, 6));
    p2->setAttribute(positions, pos);

    // Author 'widths' attributes
    vgc::core::StringId widths("widths");
    vgc::core::DoubleArray ws1 = { 5, 5 };
    vgc::core::DoubleArray ws2 = { 5, 10, 15 };
    p1->setAttribute(widths, ws1);
    p2->setAttribute(widths, ws2);

    // Author 'color' attributes
    vgc::core::StringId color("color");
    vgc::core::Color c1(1, 0, 0);
    vgc::core::Color c2(0, 1, 0);
    p1->setAttribute(color, c1);
    p2->setAttribute(color, c2);

    // Save to XML file
    doc->save("test.vgc");
    */

    // Start event loop
    return application.exec();
}
