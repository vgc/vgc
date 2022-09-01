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

#include <string>
#include <string_view>

#include <QApplication>
#include <QDir>
#include <QSettings>
#include <QTimer>

#include <vgc/core/paths.h>
#include <vgc/core/python.h>
#include <vgc/core/random.h>
#include <vgc/dom/document.h>
#include <vgc/ui/button.h>
#include <vgc/ui/column.h>
#include <vgc/ui/grid.h>
#include <vgc/ui/label.h>
#include <vgc/ui/lineedit.h>
#include <vgc/ui/overlayarea.h>
#include <vgc/ui/plot2d.h>
#include <vgc/ui/qtutil.h>
#include <vgc/ui/row.h>
#include <vgc/ui/window.h>
#include <vgc/widgets/font.h>
#include <vgc/widgets/mainwindow.h>
#include <vgc/widgets/openglviewer.h>
#include <vgc/widgets/stylesheets.h>

namespace py = pybind11;

#ifdef VGC_QOPENGL_EXPERIMENT
// test fix for white artefacts during Windows window resizing.
// https://bugreports.qt.io/browse/QTBUG-89688
// indicated commit does not seem to be enough to fix the bug
void runtimePatchQt() {
    auto hMod = LoadLibraryA("platforms/qwindowsd.dll");
    if (hMod) {
        char* base = reinterpret_cast<char*>(hMod);
        char* target = base + 0x0001BA61;
        DWORD oldProt{};
        char patch[] = {'\x90', '\x90'};
        VirtualProtect(target, sizeof(patch), PAGE_EXECUTE_READWRITE, &oldProt);
        memcpy(target, patch, sizeof(patch));
        VirtualProtect(target, sizeof(patch), oldProt, &oldProt);
    }
}
#endif

int main(int argc, char* argv[]) {

#ifdef VGC_QOPENGL_EXPERIMENT
    QGuiApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
#endif

    // Conversion between QString and std::string.
    using vgc::ui::fromQt;
    using vgc::ui::toQt;

    // Init OpenGL. Must be called before QApplication creation. See Qt doc:
    //
    // Calling QSurfaceFormat::setDefaultFormat() before constructing the
    // QApplication instance is mandatory on some platforms (for example,
    // macOS) when an OpenGL core profile context is requested. This is to
    // ensure that resource sharing between contexts stays functional as all
    // internal contexts are created using the correct version and profile.
    //
    //vgc::widgets::OpenGLViewer::init();

    // Creates the QApplication
    // XXX We should create a vgc::???::Application class for code sharing
    // between the different VGC apps.
    QApplication application(argc, argv);

    // Set runtime paths from vgc.conf, an optional configuration file to be
    // placed in the same folder as the executable.
    //
    // If vgc.conf exists, then the specified paths can be either absolute or
    // relative to the directory where vgc.conf lives (that is, relative to the
    // application dir path).
    //
    // If vgc.conf does not exist, or BasePath isn't specified, then BasePath
    // is assumed to be ".." (that is, one directory above the application dir
    // path).
    //
    // If vgc.conf does not exist, or PythonHome isn't specified, then
    // PythonHome is assumed to be equal to BasePath.
    //
    // Note: in the future, we would probably want this to be handled directly
    // by vgc::core, for example via a function vgc::core::init(argc, argv).
    // For now, we keep it here for the convenience of being able to use Qt's
    // applicationDirPath(), QDir, and QSettings. We don't want vgc::core to
    // depend on Qt.
    //
    QString binPath = QCoreApplication::applicationDirPath();
    QDir binDir(binPath);
    binDir.makeAbsolute();
    binDir.setPath(binDir.canonicalPath()); // resolve symlinks
    QDir baseDir = binDir;
    baseDir.cdUp();
    std::string basePath = fromQt(baseDir.path());
    if (binDir.exists("vgc.conf")) {
        QSettings conf(binDir.filePath("vgc.conf"), QSettings::IniFormat);
        if (conf.contains("BasePath")) {
            QString v = conf.value("BasePath").toString();
            if (!v.isEmpty()) {
                v = QDir::cleanPath(binDir.filePath(v));
                basePath = fromQt(v);
            }
        }
    }
    vgc::core::setBasePath(basePath);

    // Set style
    vgc::widgets::addDefaultApplicationFonts();
    vgc::widgets::setApplicationStyleSheet("widgets/stylesheets/dark.qss");

    // Set window icon
    std::string iconPath = vgc::core::resourcePath("apps/illustration/icons/512.png");
    application.setWindowIcon(QIcon(toQt(iconPath)));

    std::string lipsum =
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do "
        "eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim "
        "ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut "
        "aliquip ex ea commodo consequat. Duis aute irure dolor in "
        "reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla "
        "pariatur. Excepteur sint occaecat cupidatat non proident, sunt in "
        "culpa qui officia deserunt mollit anim id est laborum.";

    const vgc::UInt32 seed1 = 109283;
    const vgc::UInt32 seed2 = 981427;
    size_t lipsumSize = lipsum.size();
    vgc::UInt32 lipsumSize32 = static_cast<vgc::UInt32>(lipsumSize);
    vgc::core::PseudoRandomUniform<size_t> randomBegin(0, lipsumSize32, seed1);
    vgc::core::PseudoRandomUniform<size_t> randomCount(0, 100, seed2);

    vgc::ui::ColumnPtr col = vgc::ui::Column::create();

    //vgc::ui::MenuBar* menuBar = col->createChild<vgc::ui::MenuBar>();

    vgc::ui::Grid* gridTest = col->createChild<vgc::ui::Grid>();
    gridTest->setStyleSheet(".Grid { column-gap: 30dp; row-gap: 10dp; }");
    for (vgc::Int i = 0; i < 2; ++i) {
        for (vgc::Int j = 0; j < 3; ++j) {
            vgc::ui::LineEditPtr le = vgc::ui::LineEdit::create();
            std::string sheet(
                ".LineEdit { text-color: rgb(50, 232, 211); preferred-width: ");
            sheet += vgc::core::toString(1 + j);
            sheet += "00dp; horizontal-stretch: ";
            sheet += vgc::core::toString(j + 1);
            sheet += "; vertical-stretch: 0; }";
            le->setStyleSheet(sheet);
            le->setText("test");
            gridTest->setWidgetAt(le.get(), i, j);
        }
    }
    gridTest->widgetAt(0, 0)->setStyleSheet(
        ".LineEdit { text-color: rgb(255, 255, 50); vertical-stretch: 0; "
        "preferred-width: 127dp; padding-left: 30dp; margin-left: 80dp; "
        "horizontal-stretch: 0; horizontal-shrink: 1; }");
    gridTest->widgetAt(0, 1)->setStyleSheet(
        ".LineEdit { text-color: rgb(40, 255, 150); vertical-stretch: 0; "
        "preferred-width: 128dp; horizontal-stretch: 20; }");
    gridTest->widgetAt(1, 0)->setStyleSheet(
        ".LineEdit { text-color: rgb(40, 255, 150); vertical-stretch: 0; "
        "preferred-width: 127dp; horizontal-shrink: 1;}");
    gridTest->widgetAt(1, 1)->setStyleSheet(
        ".LineEdit { text-color: rgb(255, 255, 50); vertical-stretch: 0; "
        "preferred-width: 128dp; padding-left: 30dp; horizontal-stretch: 0; }");
    gridTest->widgetAt(0, 2)->setStyleSheet(
        ".LineEdit { text-color: rgb(255, 100, 80); vertical-stretch: 0; "
        "preferred-width: 231dp; horizontal-stretch: 2; }");
    gridTest->requestGeometryUpdate();

    vgc::ui::Plot2d* plot2d = col->createChild<vgc::ui::Plot2d>();
    plot2d->setNumYs(16);
    // clang-format off
    plot2d->appendDataPoint( 0.0f,  9.f,  9.f,  9.f,  9.f,  9.f,  9.f,  9.f,  9.f,  4.f,  5.f, 11.f,  4.f,  4.f,  5.f, 11.f,  4.f);
    plot2d->appendDataPoint( 1.0f,  5.f,  7.f,  2.f,  5.f,  5.f,  7.f,  2.f,  5.f,  7.f,  7.f,  8.f,  7.f,  7.f,  7.f,  8.f,  7.f);
    plot2d->appendDataPoint( 4.0f, 10.f,  1.f,  4.f, 10.f, 10.f,  1.f,  4.f, 10.f,  9.f,  8.f,  2.f,  9.f,  9.f,  8.f,  2.f,  9.f);
    plot2d->appendDataPoint( 5.0f,  5.f,  4.f,  6.f,  5.f,  5.f,  4.f,  6.f,  5.f,  5.f,  6.f,  4.f,  5.f,  5.f,  6.f,  4.f,  5.f);
    plot2d->appendDataPoint(10.0f,  8.f,  2.f,  7.f,  8.f,  8.f,  2.f,  7.f,  8.f, 10.f,  1.f,  1.f, 10.f, 10.f,  1.f,  1.f, 10.f);
    plot2d->appendDataPoint(11.0f,  4.f,  5.f, 11.f,  4.f,  4.f,  5.f, 11.f,  4.f,  9.f,  9.f,  9.f,  9.f,  9.f,  9.f,  9.f,  9.f);
    plot2d->appendDataPoint(12.0f,  7.f,  7.f,  8.f,  7.f,  7.f,  7.f,  8.f,  7.f,  5.f,  7.f,  2.f,  5.f,  5.f,  7.f,  2.f,  5.f);
    plot2d->appendDataPoint(13.0f,  9.f,  8.f,  2.f,  9.f,  9.f,  8.f,  2.f,  9.f, 10.f,  1.f,  4.f, 10.f, 10.f,  1.f,  4.f, 10.f);
    plot2d->appendDataPoint(20.0f,  5.f,  6.f,  4.f,  5.f,  5.f,  6.f,  4.f,  5.f,  5.f,  4.f,  6.f,  5.f,  5.f,  4.f,  6.f,  5.f);
    plot2d->appendDataPoint(21.0f, 10.f,  1.f,  1.f, 10.f, 10.f,  1.f,  1.f, 10.f,  8.f,  2.f,  7.f,  8.f,  8.f,  2.f,  7.f,  8.f);
    // clang-format on

    // tests OverlayArea, Button, mapTo()
    {
        vgc::ui::OverlayArea* overlayTest = col->createChild<vgc::ui::OverlayArea>();
        vgc::ui::Label* label =
            overlayTest->createOverlayWidget<vgc::ui::Label>("you clicked here!");
        label->setStyleSheet(".Label { background-color: rgb(20, 100, 100); "
                             "background-color-on-hover: rgb(20, 130, 130); }");
        vgc::ui::Grid* grid = overlayTest->createAreaWidget<vgc::ui::Grid>();
        grid->setStyleSheet(".Grid { column-gap: 10dp; row-gap: 10dp; }");
        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 4; ++j) {
                vgc::ui::ButtonPtr button = vgc::ui::Button::create("click me");
                grid->setWidgetAt(button.get(), i, j);
                button->clicked().connect(
                    [=](vgc::ui::Button* button, const vgc::geometry::Vec2f& pos) {
                        vgc::geometry::Vec2f p = button->mapTo(overlayTest, pos);
                        label->updateGeometry(p, vgc::geometry::Vec2f(120.f, 25.f));
                    });
            }
        }
    }

    int size = 10;
    for (int i = 0; i < size; ++i) {
        vgc::ui::Row* row = col->createChild<vgc::ui::Row>();
        row->addStyleClass(vgc::core::StringId("inner"));
        // Change style of first row
        if (i == 0) {
            row->setStyleSheet(".LineEdit { text-color: rgb(50, 232, 211); }");
        }
        for (int j = 0; j < size; ++j) {
            vgc::ui::LineEdit* lineEdit = row->createChild<vgc::ui::LineEdit>();
            size_t begin = randomBegin();
            size_t count = randomCount();
            begin = vgc::core::clamp(begin, 0, lipsumSize);
            size_t end = begin + count;
            end = vgc::core::clamp(end, 0, lipsumSize);
            count = end - begin;
            lineEdit->setText(std::string_view(lipsum).substr(begin, count));
        }
    }

    // XXX we need this until styles get better auto-update behavior
    col->addStyleClass(vgc::core::StringId("force-update-style"));

    vgc::ui::WindowPtr wnd = vgc::ui::Window::create(col);
    wnd->setTitle("VGC UI Test");
    wnd->resize(QSize(1100, 800));
    wnd->setVisible(true);

    // Start event loop
    return application.exec();
}
