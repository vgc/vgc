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

#include <QApplication>
#include <QTimer>

#include <vgc/core/paths.h>
#include <vgc/core/python.h>
#include <vgc/dom/document.h>
#include <vgc/widgets/font.h>
#include <vgc/widgets/mainwindow.h>
#include <vgc/widgets/openglviewer.h>
#include <vgc/widgets/qtutil.h>
#include <vgc/widgets/stylesheets.h>

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
    // XXX We should create a vgc::???::Application class for code sharing
    // between the different VGC apps.
    QApplication application(argc, argv);

    // Set base path by removing "/bin" from the application dir path
    QString binPath = QCoreApplication::applicationDirPath();
    QString basePath = binPath.chopped(4);
    vgc::core::setBasePath(basePath.toStdString());

    // Create the python interpreter
    vgc::core::PythonInterpreter pythonInterpreter;

    // Create the document + root element
    // -> Let's have the MainWindow be the owner of the document for now.
    //    Later, it should be the VgcIllustrationApp, accessible from the
    //    MainWindow so that it could call app->setDocument(doc) on open.
    //auto doc = vgc::dom::Document::create();
    //vgc::dom::Element::create(doc.get(), "vgc");

    // Expose the Document instance to the Python console as a local Python
    // variable 'document'.
    //
    // XXX In the long term, we may not want to expose "document" directly, but:
    // 1. Have a class VgcIllustrationApp: public QApplication.
    // 2. Have an instance 'VgcIllustrationApp app'.
    // 3. Pass the app to python.
    // 4. Users can call things like:
    //      app.document() (or just app.document, which is more pythonic)
    //      app.currentDocument()
    //      app.documents()
    //      etc.
    //
    // One advantage is that the calls above can be made read-only.
    // Currently, users can do document = Document() and then are not able
    // to affect the actual document anymore...
    //
    //pythonInterpreter.run("import vgc.dom");
    //pythonInterpreter.setVariableValue("document", document);

    // Create the main window
    //vgc::widgets::MainWindow w(doc.get(), &pythonInterpreter);
    vgc::widgets::MainWindow w(&pythonInterpreter);
    w.setWindowTitle("VGC Illustration");

    // Set style
    vgc::widgets::addDefaultApplicationFonts();
    vgc::widgets::setApplicationStyleSheet("widgets/stylesheets/dark.qss");

    // Set window icon
    std::string iconPath = vgc::core::resourcePath("apps/illustration/icons/512.png");
    application.setWindowIcon(QIcon(vgc::widgets::toQt(iconPath)));

    // Show maximized.
    //
    // We must call showMaximized() after the event loop has started,
    // otherwise the QMenuBar's background won't extend to the full length of
    // the window. This is a known Qt bug:
    //
    //   https://bugreports.qt.io/browse/QTBUG-55690
    //
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, SIGNAL(timeout()), &w, SLOT(showMaximized()));
    timer.start(10);

    // Start event loop
    return application.exec();
}
