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

#include <QDir>
#include <QGuiApplication>
#include <QSettings>
#include <QTimer>

#include <vgc/core/paths.h>
#include <vgc/core/python.h>
#include <vgc/core/random.h>
#include <vgc/dom/document.h>
#include <vgc/ui/action.h>
#include <vgc/ui/button.h>
#include <vgc/ui/colorpalette.h>
#include <vgc/ui/column.h>
#include <vgc/ui/grid.h>
#include <vgc/ui/imagebox.h>
#include <vgc/ui/label.h>
#include <vgc/ui/lineedit.h>
#include <vgc/ui/menu.h>
#include <vgc/ui/menubutton.h>
#include <vgc/ui/overlayarea.h>
#include <vgc/ui/panel.h>
#include <vgc/ui/panelarea.h>
#include <vgc/ui/plot2d.h>
#include <vgc/ui/qtutil.h>
#include <vgc/ui/row.h>
#include <vgc/ui/shortcut.h>
#include <vgc/ui/window.h>
#include <vgc/widgets/font.h>
#include <vgc/widgets/mainwindow.h>
#include <vgc/widgets/openglviewer.h>
#include <vgc/widgets/stylesheets.h>

namespace core = vgc::core;
namespace geometry = vgc::geometry;
namespace ui = vgc::ui;

namespace py = pybind11;

namespace {

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
// by vgc::core, for example via a function core::init(argc, argv).
// For now, we keep it here for the convenience of being able to use Qt's
// applicationDirPath(), QDir, and QSettings. We don't want vgc::core to
// depend on Qt.
//
void setBasePath() {
    QString binPath = QCoreApplication::applicationDirPath();
    QDir binDir(binPath);
    binDir.makeAbsolute();
    binDir.setPath(binDir.canonicalPath()); // resolve symlinks
    QDir baseDir = binDir;
    baseDir.cdUp();
    std::string basePath = ui::fromQt(baseDir.path());
    if (binDir.exists("vgc.conf")) {
        QSettings conf(binDir.filePath("vgc.conf"), QSettings::IniFormat);
        if (conf.contains("BasePath")) {
            QString v = conf.value("BasePath").toString();
            if (!v.isEmpty()) {
                v = QDir::cleanPath(binDir.filePath(v));
                basePath = ui::fromQt(v);
            }
        }
    }
    core::setBasePath(basePath);
}

void createMenu(ui::Widget* parent) {

    using ui::Key;
    using ui::Menu;
    using ui::ModifierKey;
    using ui::Shortcut;

    Menu* menu = parent->createChild<Menu>("Menu");
    menu->setDirection(ui::FlexDirection::Row);
    menu->addStyleClass(core::StringId("horizontal")); // TODO: move to Flex and/or Menu.

    Menu* menuA = menu->createSubMenu("Menu A");
    Menu* menuB = menu->createSubMenu("Menu B");
    menu->setPopupEnabled(false);

    menuA->addItem(parent->createAction("Action #A.1", Shortcut({}, Key::B)));
    Menu* menu1 = menuA->createSubMenu("Menu 1");
    Menu* menu2 = menuA->createSubMenu("Menu 2");
    Menu* menu3 = menuA->createSubMenu("Menu 3");

    menu1->addItem(parent->createAction("Action #1.1", Shortcut({}, Key::G)));
    menu1->addItem(
        parent->createAction("Action #1.2", Shortcut(ModifierKey::Ctrl, Key::L)));
    menu1->addItem(parent->createAction("Action #1.3"));
    menu1->addItem(parent->createAction("Action #1.4"));
    menu1->addItem(parent->createAction("Action #1.5"));
    menu1->addItem(parent->createAction("Action #1.6"));
    menu1->addItem(parent->createAction("Action #1.7"));
    Menu* menu1b = menu1->createSubMenu("Menu 1.8");

    menu1b->addItem(parent->createAction("Action #1.8.1"));
    menu1b->addItem(parent->createAction("Action #1.8.2"));
    menu1b->addItem(parent->createAction("Action #1.8.3"));
    menu1b->addItem(parent->createAction("Action #1.8.4"));
    menu1b->addItem(parent->createAction("Action #1.8.5"));
    menu1b->addItem(parent->createAction("Action #1.8.6"));
    menu1b->addItem(parent->createAction("Action #1.8.7"));

    menu2->addItem(parent->createAction("Action #2.1", Shortcut({}, Key::F)));
    menu2->addItem(
        parent->createAction("Action #2.2", Shortcut(ModifierKey::Ctrl, Key::K)));

    menu3->addItem(parent->createAction("Action #3.1"));

    menuB->addItem(parent->createAction("Action #B.1", Shortcut({}, Key::B)));

    // stretch at the right-side of the Flex
    // TODO: implement AlignLeft in Flex to achieve the same without the extra child
    menu->createChild<ui::Widget>();
}

void createColorPalette(ui::Widget* parent) {
    ui::ColorPalette* palette = parent->createChild<ui::ColorPalette>();
    palette->setStyleSheet(".ColorPalette { vertical-stretch: 0; }");
    parent->createChild<ui::Widget>(); // vertical-stretch: 1
}

void createGrid(ui::Widget* parent) {
    ui::Grid* gridTest = parent->createChild<ui::Grid>();
    gridTest->setStyleSheet(".Grid { column-gap: 30dp; row-gap: 10dp; }");
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 3; ++j) {
            ui::LineEditPtr le = ui::LineEdit::create();
            std::string sheet(
                ".LineEdit { text-color: rgb(50, 232, 211); preferred-width: ");
            sheet += core::toString(1 + j);
            sheet += "00dp; horizontal-stretch: ";
            sheet += core::toString(j + 1);
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
}

void createPlot2d(ui::Widget* parent) {
    ui::Plot2d* plot2d = parent->createChild<ui::Plot2d>();
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
}

void createClickMePopups(ui::Widget* parent) {

    // Get overlay area
    ui::OverlayArea* overlayArea = parent->topmostOverlayArea();

    // Create the popup. We can't see it at the beginning because its size is (0, 0).
    ui::Label* label = overlayArea->createOverlayWidget<ui::Label>(
        ui::OverlayResizePolicy::None, "you clicked here!");
    label->setStyleSheet(".Label { background-color: rgb(20, 100, 100); "
                         "background-color-on-hover: rgb(20, 130, 130); }");

    // Create a grid of "click me" buttons, whose actions are to set the popup geometry.
    ui::Grid* grid = parent->createChild<ui::Grid>();
    grid->setStyleSheet(".Grid { column-gap: 10dp; row-gap: 10dp; }");
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 4; ++j) {
            ui::Action* action = parent->createAction("click me");
            ui::ButtonPtr button = ui::Button::create(action);
            grid->setWidgetAt(button.get(), i, j);
            action->triggered().connect([=](ui::Widget* from) {
                geometry::Vec2f p = from->mapTo(overlayArea, geometry::Vec2f(0.f, 0.f));
                label->updateGeometry(p, geometry::Vec2f(120.f, 25.f));
            });
        }
    }
}

void createLineEdits(ui::Widget* parent) {

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
    core::PseudoRandomUniform<size_t> randomBegin(0, lipsumSize32, seed1);
    core::PseudoRandomUniform<size_t> randomCount(0, 100, seed2);

    int numRows = 3;
    int numColumns = 5;
    for (int i = 0; i < numRows; ++i) {
        ui::Row* row = parent->createChild<ui::Row>();
        row->addStyleClass(core::StringId("inner"));
        // Change style of first row
        if (i == 0) {
            row->setStyleSheet(".LineEdit { text-color: rgb(50, 232, 211); }");
        }
        for (int j = 0; j < numColumns; ++j) {
            ui::LineEdit* lineEdit = row->createChild<ui::LineEdit>();
            size_t begin = randomBegin();
            size_t count = randomCount();
            begin = core::clamp(begin, 0, lipsumSize);
            size_t end = begin + count;
            end = core::clamp(end, 0, lipsumSize);
            count = end - begin;
            lineEdit->setText(std::string_view(lipsum).substr(begin, count));
        }
    }
}

} // namespace

int main(int argc, char* argv[]) {

    using vgc::Int;
    using vgc::UInt32;

#ifdef VGC_QOPENGL_EXPERIMENT
    QGuiApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
#endif
    QGuiApplication::setAttribute(Qt::AA_SynthesizeMouseForUnhandledTabletEvents, false);

    // Various initializations
    QGuiApplication application(argc, argv);
    setBasePath();

    // Set window icon
    std::string iconPath = core::resourcePath("apps/illustration/icons/512.png");
    application.setWindowIcon(QIcon(ui::toQt(iconPath)));

    // Create overlay area and main layout
    ui::OverlayAreaPtr overlay = ui::OverlayArea::create();
    ui::Column* mainLayout = overlay->createChild<ui::Column>();
    overlay->setAreaWidget(mainLayout);
    mainLayout->addStyleClass(core::StringId("main-layout"));
    mainLayout->setStyleSheet(".main-layout { "
                              "    row-gap: 0dp; "
                              "    padding-top: 0dp; "
                              "    padding-right: 0dp; "
                              "    padding-bottom: 0dp; "
                              "    padding-left: 0dp; }");

    // Create menubar
    createMenu(mainLayout);

    // Create panel areas
    ui::PanelArea* mainArea = ui::PanelArea::createHorizontalSplit(mainLayout);
    ui::PanelArea* leftArea = ui::PanelArea::createTabs(mainArea);
    ui::PanelArea* middleArea = ui::PanelArea::createVerticalSplit(mainArea);
    ui::PanelArea* rightArea = ui::PanelArea::createVerticalSplit(mainArea);
    ui::PanelArea* middleTopArea = ui::PanelArea::createTabs(middleArea);
    ui::PanelArea* middleBottomArea = ui::PanelArea::createTabs(middleArea);
    ui::PanelArea* rightTopArea = ui::PanelArea::createTabs(rightArea);
    ui::PanelArea* rightBottomArea = ui::PanelArea::createTabs(rightArea);

    // Create panels
    // XXX actually create Panel instances as children of Tabs areas
    ui::Column* leftAreaWidget = leftArea->createChild<ui::Column>();
    ui::Column* middleTopAreaWidget = middleTopArea->createChild<ui::Column>();
    ui::Column* middleBottomAreaWidget = middleBottomArea->createChild<ui::Column>();
    ui::Column* rightTopAreaWidget = rightTopArea->createChild<ui::Column>();
    ui::Column* rightBottomAreaWidget = rightBottomArea->createChild<ui::Column>();

    // Create widgets
    createColorPalette(leftAreaWidget);
    createGrid(middleBottomAreaWidget);
    createPlot2d(rightTopAreaWidget);
    createClickMePopups(rightBottomAreaWidget);
    createLineEdits(rightBottomAreaWidget);
    rightBottomAreaWidget->createChild<ui::ImageBox>(iconPath);

    // XXX we need this until styles get better auto-update behavior
    overlay->addStyleClass(core::StringId("force-update-style"));

    // Create window
    ui::WindowPtr wnd = ui::Window::create(overlay);
    wnd->setTitle("VGC UI Test");
    wnd->resize(QSize(1100, 800));
    wnd->setVisible(true);

    application.installEventFilter(wnd.get());

    // Start event loop
    return application.exec();
}
