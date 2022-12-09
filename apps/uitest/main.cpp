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

#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QStandardPaths>

#include <vgc/app/application.h>
#include <vgc/app/logcategories.h>
#include <vgc/app/mainwindow.h>
#include <vgc/core/os.h>
#include <vgc/core/paths.h>
#include <vgc/core/random.h>
#include <vgc/dom/document.h>
#include <vgc/dom/strings.h>
#include <vgc/ui/action.h>
#include <vgc/ui/button.h>
#include <vgc/ui/canvas.h>
#include <vgc/ui/colorpalette.h>
#include <vgc/ui/column.h>
#include <vgc/ui/grid.h>
#include <vgc/ui/imagebox.h>
#include <vgc/ui/label.h>
#include <vgc/ui/lineedit.h>
#include <vgc/ui/menu.h>
#include <vgc/ui/overlayarea.h>
#include <vgc/ui/panel.h>
#include <vgc/ui/panelarea.h>
#include <vgc/ui/plot2d.h>
#include <vgc/ui/qtutil.h>
#include <vgc/ui/row.h>
#include <vgc/ui/shortcut.h>

namespace app = vgc::app;
namespace core = vgc::core;
namespace dom = vgc::dom;
namespace geometry = vgc::geometry;
namespace ui = vgc::ui;

using vgc::Int;
using vgc::UInt32;

using app::LogVgcApp;

namespace {

core::StringId left_sidebar("left-sidebar");
core::StringId with_padding("with-padding");
core::StringId user_("user");
core::StringId colorpalette_("colorpalette");
core::StringId colorpaletteitem_("colorpaletteitem");
core::StringId color_("color");

void loadColorPalette_(
    ui::ColorPalette* palette,
    const core::Array<core::Color>& colors) {

    if (!palette) {
        return;
    }
    ui::ColorListView* listView = palette->colorListView();
    if (!listView) {
        return;
    }
    listView->setColors(colors);
}

core::Array<core::Color> getColorPalette_(dom::Document* doc) {

    // Get colors
    core::Array<core::Color> colors;
    dom::Element* root = doc->rootElement();
    for (dom::Element* user : root->childElements(user_)) {
        for (dom::Element* colorpalette : user->childElements(colorpalette_)) {
            for (dom::Element* item : colorpalette->childElements(colorpaletteitem_)) {
                core::Color color = item->getAttribute(color_).getColor();
                colors.append(color);
            }
        }
    }

    // Delete <user> element
    dom::Element* user = root->firstChildElement(user_);
    while (user) {
        dom::Element* nextUser = user->nextSiblingElement(user_);
        user->remove();
        user = nextUser;
    }

    return colors;
}

class ColorPaletteSaver {
public:
    ColorPaletteSaver(const ColorPaletteSaver&) = delete;
    ColorPaletteSaver& operator=(const ColorPaletteSaver&) = delete;

    ColorPaletteSaver(ui::ColorPalette* palette, dom::Document* doc)
        : isUndoOpened_(false)
        , doc_(doc) {

        if (!palette) {
            return;
        }

        ui::ColorListView* listView = palette->colorListView();
        if (!listView) {
            return;
        }

        // The current implementation adds the colors to the DOM now, save, then
        // abort the "add color" operation so that it doesn't appear as an undo.
        //
        // Ideally, we should instead add the color to the DOM directly when the
        // user clicks the "add to palette" button (so it would be an undoable
        // action), and the color list view should listen to DOM changes to update
        // the color list. This way, even plugins could populate the color palette
        // by modifying the DOM.
        //
        static core::StringId Add_to_Palette("Add to Palette");
        doc->history()->createUndoGroup(Add_to_Palette);
        isUndoOpened_ = true;

        // TODO: reuse existing colorpalette element instead of creating new one.
        dom::Element* root = doc->rootElement();
        dom::Element* user = dom::Element::create(root, user_);
        dom::Element* colorpalette = dom::Element::create(user, colorpalette_);
        for (Int i = 0; i < listView->numColors(); ++i) {
            const core::Color& color = listView->colorAt(i);
            dom::Element* item = dom::Element::create(colorpalette, colorpaletteitem_);
            item->setAttribute(color_, color);
        }
    }

    ~ColorPaletteSaver() {
        if (isUndoOpened_) {
            doc_->history()->abort();
        }
    }

private:
    bool isUndoOpened_;
    dom::Document* doc_;
};

VGC_DECLARE_OBJECT(Application);

class Application : public app::Application {
    VGC_OBJECT(Application, app::Application)

public:
    static ApplicationPtr create(int argc, char* argv[]) {
        return ApplicationPtr(new Application(argc, argv));
    }

protected:
    Application(int argc, char* argv[])
        : app::Application(argc, argv) {

        setWindowIconFromResource("apps/illustration/icons/512.png");
        window_ = app::MainWindow::create("VGC UI Test");
        createDocument_();
        createWidgets_();
    }

private:
    app::MainWindowPtr window_;
    dom::DocumentPtr document_;
    core::ConnectionHandle documentHistoryHeadChangedConnectionHandle_;
    ui::ColorPalette* palette_ = nullptr;
    ui::Canvas* canvas_ = nullptr;

    VGC_SLOT(updateUndoRedoActionStateSlot_, updateUndoRedoActionState_)
    void updateUndoRedoActionState_() {
        if (actionUndo_ && actionRedo_ && document_ && document_->history()) {
            actionUndo_->setEnabled(document_->history()->canUndo());
            actionRedo_->setEnabled(document_->history()->canRedo());
        }
    }

    void createDocument_() {
        document_ = dom::Document::create();
        dom::Element::create(document_.get(), "vgc");
        document_->enableHistory(dom::strings::New_Document);
        document_->history()->headChanged().connect(updateUndoRedoActionStateSlot_());
        updateUndoRedoActionState_();
    }

    void createWidgets_() {

        createActions_(window_->mainWidget());
        createMenus_(window_->mainWidget());

        // Create panel areas
        ui::PanelArea* mainArea = window_->mainWidget()->panelArea();
        mainArea->setType(ui::PanelAreaType::HorizontalSplit);
        ui::PanelArea* leftArea = ui::PanelArea::createTabs(mainArea);
        leftArea->addStyleClass(left_sidebar);
        ui::PanelArea* middleArea = ui::PanelArea::createVerticalSplit(mainArea);
        ui::PanelArea* rightArea = ui::PanelArea::createVerticalSplit(mainArea);
        ui::PanelArea* middleTopArea = ui::PanelArea::createTabs(middleArea);
        ui::PanelArea* middleBottomArea = ui::PanelArea::createTabs(middleArea);
        ui::PanelArea* rightTopArea = ui::PanelArea::createTabs(rightArea);
        ui::PanelArea* rightBottomArea = ui::PanelArea::createTabs(rightArea);

        // Create panels
        Panel* leftPanel = createPanelWithPadding_(leftArea);
        Panel* middleTopPanel = createPanel_(middleTopArea);
        Panel* middleBottomPanel = createPanelWithPadding_(middleBottomArea);
        Panel* rightTopPanel = createPanelWithPadding_(rightTopArea);
        Panel* rightBottomPanel = createPanelWithPadding_(rightBottomArea);

        // Create widgets inside panels
        createColorPalette_(leftPanel);
        createGrid_(middleBottomPanel);
        createPlot2d_(rightTopPanel);
        createClickMePopups_(rightBottomPanel);
        createLineEdits_(rightBottomPanel);
        createImageBox_(rightBottomPanel);
        createCanvas_(middleTopPanel, document_.get());
    }

    // XXX actually create ui::Panel instances as children of Tabs areas
    using Panel = ui::Column;
    Panel* createPanel_(ui::PanelArea* panelArea) {
        Panel* panel = panelArea->createChild<Panel>();
        panel->addStyleClass(ui::strings::Panel);
        return panel;
    }

    Panel* createPanelWithPadding_(ui::PanelArea* panelArea) {
        Panel* panel = createPanel_(panelArea);
        panel->addStyleClass(with_padding);
        return panel;
    }

    ui::Action* actionNew_ = nullptr;
    VGC_SLOT(onActionNewSlot_, onActionNew_)
    void onActionNew_() {

        // XXX TODO ask save current document

        if (document_ && document_->history()) {
            document_->history()->disconnect(this);
        }

        try {
            dom::DocumentPtr tmp = dom::Document::create();
            dom::Element::create(tmp.get(), "vgc");
            tmp->enableHistory(dom::strings::New_Document);
            document_->history()->headChanged().connect(updateUndoRedoActionStateSlot_());
            canvas_->setDocument(tmp.get());
            document_ = tmp;
            updateUndoRedoActionState_();
        }
        catch (const dom::FileError& e) {
            // TODO: have our own message box instead of using QtWidgets
            QMessageBox::critical(nullptr, "Error Creating New File", e.what());
        }
    }

    QString filename_;
    ui::Action* actionOpen_ = nullptr;
    VGC_SLOT(onActionOpenSlot_, onActionOpen_)
    void onActionOpen_() {
        // Get which directory the dialog should display first
        QString dir = filename_.isEmpty()
                          ? QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
                          : QFileInfo(filename_).dir().path();

        // Set which existing files to show in the dialog
        QString filters = "VGC Illustration Files (*.vgci)";

        // Create the dialog.
        //
        // TODO: manually set position of dialog in screen (since we can't give
        // it a QWidget* parent). Same for all QMessageBox.
        //
        QWidget* parent = nullptr;
        QFileDialog dialog(parent, QString("Open..."), dir, filters);

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
                VGC_WARNING(LogVgcApp, "No file selected; file not opened.");
            }
            if (selectedFiles.size() == 1) {
                QString selectedFile = selectedFiles.first();
                if (!selectedFile.isEmpty()) {
                    // Open
                    filename_ = selectedFile;
                    doOpen_();
                }
                else {
                    VGC_WARNING(LogVgcApp, "Empty file path selected; file not opened.");
                }
            }
            else {
                VGC_WARNING(LogVgcApp, "More than one file selected; file not opened.");
            }
        }
        else {
            // User willfully cancelled the operation
            // => nothing to do, not even a warning.
        }
    }

    void doOpen_() {
        // XXX TODO ask save current document

        if (document_ && document_->history()) {
            document_->history()->disconnect(this);
        }

        try {
            // Open document from file, get its color palette, and remove it from
            // the DOM before enabling history
            dom::DocumentPtr doc = dom::Document::open(ui::fromQt(filename_));
            core::Array<core::Color> colors = getColorPalette_(doc.get());
            doc->enableHistory(dom::strings::Open_Document);
            doc->history()->headChanged().connect(updateUndoRedoActionStateSlot_());

            // Set viewer document (must happen before old document is destroyed)
            canvas_->setDocument(doc.get());

            // Destroy old document and set new document as current
            document_ = doc;
            updateUndoRedoActionState_();

            // Load color palette
            loadColorPalette_(palette_, colors);
        }
        catch (const dom::FileError& e) {
            QMessageBox::critical(nullptr, "Error Opening File", e.what());
        }
    }

    ui::Action* actionSave_ = nullptr;
    VGC_SLOT(onActionSaveSlot_, onActionSave_)
    void onActionSave_() {
        if (filename_.isEmpty()) {
            doSaveAs_();
        }
        else {
            doSave_();
        }
    }

    ui::Action* actionSaveAs_ = nullptr;
    VGC_SLOT(onActionSaveAsSlot_, onActionSaveAs_)
    void onActionSaveAs_() {
        doSaveAs_();
    }

    void doSaveAs_() {

        // Get which directory the dialog should display first
        QString dir = filename_.isEmpty()
                          ? QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
                          : QFileInfo(filename_).dir().path();

        // Set which existing files to show in the dialog
        QString extension = ".vgci";
        QString filters = QString("VGC Illustration Files (*") + extension + ")";

        // Create the dialog
        QFileDialog dialog(nullptr, "Save As...", dir, filters);

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
                VGC_WARNING(LogVgcApp, "No file selected; file not saved.");
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
                    doSave_();
                }
                else {
                    VGC_WARNING(LogVgcApp, "Empty file path selected; file not saved.");
                }
            }
            else {
                VGC_WARNING(LogVgcApp, "More than one file selected; file not saved.");
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

    void doSave_() {
        try {
            ColorPaletteSaver saver(palette_, document_.get());
            document_->save(ui::fromQt(filename_));
        }
        catch (const dom::FileError& e) {
            QMessageBox::critical(nullptr, "Error Saving File", e.what());
        }
    }

    ui::Action* actionQuit_ = nullptr;
    VGC_SLOT(onActionQuitSlot_, onActionQuit_);
    void onActionQuit_() {
        if (window_) {
            window_->close();
        }
    }

    ui::Action* actionUndo_ = nullptr;
    VGC_SLOT(onActionUndoSlot_, onActionUndo_);
    void onActionUndo_() {
        if (document_ && document_->history()) {
            document_->history()->undo();
        }
    }

    ui::Action* actionRedo_ = nullptr;
    VGC_SLOT(onActionRedoSlot_, onActionRedo_);
    void onActionRedo_() {
        if (document_ && document_->history()) {
            document_->history()->redo();
        }
    }

    void createActions_(ui::Widget* parent) {

        using ui::Key;
        using ui::Shortcut;

        ui::ModifierKey ctrl = ui::ModifierKey::Ctrl;
        ui::ModifierKey shift = ui::ModifierKey::Shift;
        ui::ShortcutContext context = ui::ShortcutContext::Window;

        auto createAction = [=](std::string_view text, const Shortcut& shortcut) {
            return parent->createAction(text, shortcut, context);
        };

        actionNew_ = createAction("New", Shortcut(ctrl, Key::N));
        actionNew_->triggered().connect(onActionNewSlot_());

        actionOpen_ = createAction("Open", Shortcut(ctrl, Key::O));
        actionOpen_->triggered().connect(onActionOpenSlot_());

        actionSave_ = createAction("Save", Shortcut(ctrl, Key::S));
        actionSave_->triggered().connect(onActionSaveSlot_());

        actionSaveAs_ = createAction("Save As...", Shortcut(ctrl | shift, Key::S));
        actionSaveAs_->triggered().connect(onActionSaveAsSlot_());

        actionQuit_ = createAction("Quit", Shortcut(ctrl, Key::Q));
        actionQuit_->triggered().connect(onActionQuitSlot_());

        actionUndo_ = createAction("Undo", Shortcut(ctrl, Key::Z));
        actionUndo_->triggered().connect(onActionUndoSlot_());

        actionRedo_ = createAction("Redo", Shortcut(ctrl | shift, Key::Z));
        actionRedo_->triggered().connect(onActionRedoSlot_());

        updateUndoRedoActionState_();
    }

    ui::Menu* testMenu_ = nullptr;
    void createMenus_(ui::Widget* parent) {

        using ui::Action;
        using ui::Key;
        using ui::Menu;
        using ui::ModifierKey;
        using ui::Shortcut;
        using ui::ShortcutContext;

        ui::Menu* menuBar = window_->mainWidget()->menuBar();

        Menu* fileMenu = menuBar->createSubMenu("File");
        Menu* editMenu = menuBar->createSubMenu("Edit");
        testMenu_ = menuBar->createSubMenu("Test");
        menuBar->setPopupEnabled(false);

        fileMenu->addItem(actionNew_);
        fileMenu->addItem(actionOpen_);
        fileMenu->addItem(actionSave_);
        fileMenu->addItem(actionSaveAs_);
        fileMenu->addItem(actionQuit_);

        editMenu->addItem(actionUndo_);
        editMenu->addItem(actionRedo_);

        Action* actionCreateAction = parent->createAction(
            "Create action in Test menu", Shortcut(ModifierKey::Ctrl, Key::A));
        actionCreateAction->triggered().connect([this]() {
            Action* action = this->testMenu_->createAction("Hello");
            this->testMenu_->addItem(action);
        });

        Action* actionCreateMenu = parent->createAction(
            "Create menu in menubar", Shortcut(ModifierKey::Ctrl, Key::M));
        actionCreateMenu->triggered().connect([this]() {
            Menu* menu = this->window_->mainWidget()->menuBar()->createSubMenu("Test 2");
            Action* action = menu->createAction("Hello");
            menu->addItem(action);
        });

        testMenu_->addItem(actionCreateAction);
        testMenu_->addItem(actionCreateMenu);
        Menu* menu1 = testMenu_->createSubMenu("Menu 1");
        Menu* menu2 = testMenu_->createSubMenu("Menu 2");
        Menu* menu3 = testMenu_->createSubMenu("Menu 3");

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
    }

    void createColorPalette_(ui::Widget* parent) {
        palette_ = parent->createChild<ui::ColorPalette>();
    }

    void onColorChanged_() {
        if (canvas_ && palette_) {
            canvas_->setCurrentColor(palette_->selectedColor());
        }
    }
    VGC_SLOT(onColorChangedSlot_, onColorChanged_)

    void createCanvas_(ui::Widget* parent, dom::Document* document) {
        canvas_ = parent->createChild<ui::Canvas>(document);
        onColorChanged_();
        if (palette_) {
            palette_->colorSelected().connect(onColorChangedSlot_());
        }
    }

    void createGrid_(ui::Widget* parent) {
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

    void createPlot2d_(ui::Widget* parent) {
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

    void createClickMePopups_(ui::Widget* parent) {

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
                    geometry::Vec2f p =
                        from->mapTo(overlayArea, geometry::Vec2f(0.f, 0.f));
                    label->updateGeometry(p, geometry::Vec2f(120.f, 25.f));
                });
            }
        }
    }

    void createLineEdits_(ui::Widget* parent) {

        std::string lipsum =
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do "
            "eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim "
            "ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut "
            "aliquip ex ea commodo consequat. Duis aute irure dolor in "
            "reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla "
            "pariatur. Excepteur sint occaecat cupidatat non proident, sunt in "
            "culpa qui officia deserunt mollit anim id est laborum.";

        const UInt32 seed1 = 109283;
        const UInt32 seed2 = 981427;
        size_t lipsumSize = lipsum.size();
        UInt32 lipsumSize32 = static_cast<UInt32>(lipsumSize);
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

    void createImageBox_(ui::Widget* parent) {
        std::string imagePath = core::resourcePath("apps/illustration/icons/512.png");
        parent->createChild<ui::ImageBox>(imagePath);
    }
};

} // namespace

int main(int argc, char* argv[]) {
    ApplicationPtr application = Application::create(argc, argv);
    return application->exec();
}
