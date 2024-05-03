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

#include <vgc/app/canvasapplication.h>
#include <vgc/core/paths.h>
#include <vgc/core/random.h>
#include <vgc/ui/column.h>
#include <vgc/ui/combobox.h>
#include <vgc/ui/grid.h>
#include <vgc/ui/iconwidget.h>
#include <vgc/ui/imagebox.h>
#include <vgc/ui/lineedit.h>
#include <vgc/ui/messagedialog.h>
#include <vgc/ui/mousebutton.h>
#include <vgc/ui/numberedit.h>
#include <vgc/ui/overlayarea.h>
#include <vgc/ui/panelmanager.h>
#include <vgc/ui/plot2d.h>
#include <vgc/ui/row.h>
#include <vgc/ui/standardmenus.h>
#include <vgc/workspace/vertex.h>

#include "resetcurrentcolor.h"

namespace app = vgc::app;
namespace core = vgc::core;
namespace geometry = vgc::geometry;
namespace ui = vgc::ui;
namespace uitest = vgc::apps::uitest;

using vgc::Int;
using vgc::UInt32;

VGC_DECLARE_OBJECT(UiTestApplication);

namespace {

namespace commands {

using ui::Key;
using ui::Shortcut;
using ui::modifierkeys::mod;

VGC_UI_DEFINE_WINDOW_COMMAND(
    createAction,
    "uitest.createActionInTestMenu",
    "Create action in Test menu",
    Shortcut(mod, Key::A))

VGC_UI_DEFINE_WINDOW_COMMAND(
    createMenu,
    "uitest.createMenuInMenuBar",
    "Create menu in menubar",
    Shortcut(mod, Key::M))

VGC_UI_DEFINE_WINDOW_COMMAND(hello, "uitest.hello", "Hello")

VGC_UI_DEFINE_WINDOW_COMMAND(_1_1, "uitest.1.1", "Action #1.1", Shortcut(mod, Key::G))
VGC_UI_DEFINE_WINDOW_COMMAND(_1_2, "uitest.1.2", "Action #1.2", Shortcut(mod, Key::L))
VGC_UI_DEFINE_WINDOW_COMMAND(_1_3, "uitest.1.3", "Action #1.3")
VGC_UI_DEFINE_WINDOW_COMMAND(_1_4, "uitest.1.4", "Action #1.4")
VGC_UI_DEFINE_WINDOW_COMMAND(_1_5, "uitest.1.5", "Action #1.5")
VGC_UI_DEFINE_WINDOW_COMMAND(_1_6, "uitest.1.6", "Action #1.6")
VGC_UI_DEFINE_WINDOW_COMMAND(_1_7, "uitest.1.7", "Action #1.7")

VGC_UI_DEFINE_WINDOW_COMMAND(_1_8_1, "uitest.1.8.1", "Action #1.8.1")
VGC_UI_DEFINE_WINDOW_COMMAND(_1_8_2, "uitest.1.8.2", "Action #1.8.2")
VGC_UI_DEFINE_WINDOW_COMMAND(_1_8_3, "uitest.1.8.3", "Action #1.8.3")
VGC_UI_DEFINE_WINDOW_COMMAND(_1_8_4, "uitest.1.8.4", "Action #1.8.4")
VGC_UI_DEFINE_WINDOW_COMMAND(_1_8_5, "uitest.1.8.5", "Action #1.8.5")
VGC_UI_DEFINE_WINDOW_COMMAND(_1_8_6, "uitest.1.8.6", "Action #1.8.6")
VGC_UI_DEFINE_WINDOW_COMMAND(_1_8_7, "uitest.1.8.7", "Action #1.8.7")

VGC_UI_DEFINE_WINDOW_COMMAND(_2_1, "uitest.2.1", "Action #2.1", Shortcut(mod, Key::F))
VGC_UI_DEFINE_WINDOW_COMMAND(_2_2, "uitest.2.2", "Action #2.2", Shortcut(mod, Key::K))

VGC_UI_DEFINE_WINDOW_COMMAND(_3_1, "uitest.action.3.1", "Action #3.1")

VGC_UI_DEFINE_WINDOW_COMMAND(openPopup, "uitest.openPopup", "Open Popup")
VGC_UI_DEFINE_WINDOW_COMMAND(maybeQuit, "uitest.maybeQuit", "Maybe Quit")

VGC_UI_DEFINE_WINDOW_COMMAND(
    cycleSvgIcon,
    "uitest.cycleSvgIcon",
    "Cycle between available SVG icons",
    Shortcut(mod, Key::S))

} // namespace commands

core::StringId s_with_padding("with-padding");

VGC_DECLARE_OBJECT(Plot2dPanel);

class Plot2dPanel : public ui::Panel {
private:
    VGC_OBJECT(Plot2dPanel, ui::Panel)

public:
    static constexpr std::string_view label = "Plot 2D";
    static constexpr std::string_view id = "vgc.uitest.plot2d";
    static constexpr ui::PanelDefaultArea defaultArea = ui::PanelDefaultArea::Right;

    static Plot2dPanelPtr create(const ui::PanelContext& context) {
        return core::createObject<Plot2dPanel>(context);
    }

protected:
    Plot2dPanel(CreateKey key, const ui::PanelContext& context)
        : Panel(key, context, label) {

        addStyleClass(s_with_padding);

        ui::Plot2d* plot2d = createChild<ui::Plot2d>();
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
};

VGC_DECLARE_OBJECT(MiscTestsPanel);

class MiscTestsPanel : public ui::Panel {
private:
    VGC_OBJECT(MiscTestsPanel, ui::Panel)

public:
    static constexpr std::string_view label = "Misc Tests";
    static constexpr std::string_view id = "vgc.uitest.miscTests";
    static constexpr ui::PanelDefaultArea defaultArea = ui::PanelDefaultArea::Right;

    static MiscTestsPanelPtr create(const ui::PanelContext& context) {
        return core::createObject<MiscTestsPanel>(context);
    }

protected:
    MiscTestsPanel(CreateKey key, const ui::PanelContext& context)
        : Panel(key, context, label) {

        addStyleClass(s_with_padding);

        ui::Column* layout = createChild<ui::Column>();
        createGrid_(layout);
        createClickMePopups_(layout);
        createMessageDialogButtons_(layout);
        createLineEdits_(layout);
        createNumberEdits_(layout);
        createComboBoxes_(layout);
    }

private:
    void createGrid_(ui::Widget* parent) {
        ui::Grid* gridTest = parent->createChild<ui::Grid>();
        gridTest->setStyleSheet(".Grid { column-gap: 30dp; row-gap: 10dp; }");
        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 3; ++j) {
                ui::LineEditPtr lineEdit = ui::LineEdit::create();
                std::string styleSheet(
                    ".LineEdit { text-color: rgb(50, 232, 211); preferred-width: ");
                styleSheet += core::toString(1 + j);
                styleSheet += "00dp; horizontal-stretch: ";
                styleSheet += core::toString(j + 1);
                styleSheet += "; vertical-stretch: 0; }";
                lineEdit->setStyleSheet(styleSheet);
                lineEdit->setText("test");
                gridTest->setWidgetAt(lineEdit.get(), i, j);
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

    void createNumberEdits_(ui::Widget* parent) {

        ui::Row* row = parent->createChild<ui::Row>();

        // Default NumberEdit: integer from 0 to 100
        row->createChild<ui::NumberEdit>();

        // NumberEdit in [12.5, 42] rounded to 1 decimal with 0.01 step.
        ui::NumberEdit* numberEdit2 = row->createChild<ui::NumberEdit>();
        numberEdit2->setDecimals(1);
        numberEdit2->setStep(0.01);
        numberEdit2->setMinimum(12.5);
        numberEdit2->setMaximum(42);

        // NumberEdit in [12.5, 42] rounded to 2 significant digits with 0.01 step.
        ui::NumberEdit* numberEdit3 = row->createChild<ui::NumberEdit>();
        numberEdit3->setSignificantDigits(2);
        numberEdit3->setStep(0.001);
        numberEdit3->setText("0.0000234");
    }

    static void setComboBoxLabelText_(
        ui::LabelWeakPtr label_,
        ui::ComboBoxWeakPtr comboBox_,
        Int index) {

        if (auto l = label_.lock()) {
            //   ^ Note: compiler warning if named `label` (hides Panel::label)
            if (auto comboBox = comboBox_.lock()) {
                if (index != comboBox->index()) {
                    throw core::LogicError(core::format(
                        "Indices don't match: {} != {}.", index, comboBox->index()));
                }
                l->setText(core::format("index={} text={}", index, comboBox->text()));
            }
        }
    }

    ui::ComboBoxWeakPtr createComboBox_(ui::Widget* parent, std::string_view title) {
        ui::Column* col = parent->createChild<ui::Column>();
        ui::LabelWeakPtr label_ = col->createChild<ui::Label>();
        ui::ComboBoxWeakPtr comboBox_ = col->createChild<ui::ComboBox>(title);
        setComboBoxLabelText_(label_, comboBox_, -1);
        if (auto comboBox = comboBox_.lock()) {
            comboBox->indexChanged().connect([label_, comboBox_](Int index) {
                setComboBoxLabelText_(label_, comboBox_, index);
            });
        }
        return comboBox_;
    }

    template<typename EnumType>
    ui::ComboBoxWeakPtr createComboBoxFromEnum_(ui::Widget* parent) {
        ui::Column* col = parent->createChild<ui::Column>();
        ui::LabelWeakPtr label_ = col->createChild<ui::Label>();
        auto comboBoxShared_ = ui::ComboBox::createFromEnum<EnumType>();
        ui::ComboBoxWeakPtr comboBox_ = comboBoxShared_;
        setComboBoxLabelText_(label_, comboBox_, -1);
        if (auto comboBox = comboBox_.lock()) {
            col->addChild(comboBox.get());
            comboBox->indexChanged().connect([label_, comboBox_](Int index) {
                setComboBoxLabelText_(label_, comboBox_, index);
            });
        }
        return comboBox_;
    }

    void createComboBoxes_(ui::Widget* parent) {

        using ui::ComboBox;
        using ui::ComboBoxWeakPtr;

        ui::Row* row = parent->createChild<ui::Row>();

        // Default ComboBox
        createComboBox_(row, "Combo Box 1");

        // ComboBox with manually set items, none set as current index
        if (auto comboBox = createComboBox_(row, "Combo Box 2").lock()) {
            comboBox->addItem("Item 1");
            comboBox->addItem("Item 2");
            comboBox->addItem("Item 3");
        }

        // ComboBox with manually set items, with the first set as current index
        if (auto comboBox = createComboBox_(row, "Combo Box 3").lock()) {
            comboBox->addItem("Item 1");
            comboBox->addItem("Item 2");
            comboBox->addItem("Item 3");
            comboBox->setIndex(0);
        }

        // ComboBox with items set from a registered enum
        if (auto comboBox = createComboBoxFromEnum_<ui::MouseButton>(row).lock()) {
            // nothing to do
        }
    }

    ui::OverlayAreaWeakPtr getClickMeOverlayArea_(ui::Widget& from) {
        static ui::OverlayAreaWeakPtr overlayArea_;
        if (!overlayArea_.isAlive()) { // XXX Add WeakPtr::operator bool()?
            overlayArea_ = from.topmostOverlayArea();
        }
        return overlayArea_;
    }

    ui::WidgetWeakPtr getClickMePopup_(ui::OverlayArea& overlayArea) {
        static ui::WidgetWeakPtr popup_;
        if (!popup_.isAlive()) {
            popup_ = overlayArea.createModelessOverlay<ui::Label>("you clicked here!");
            if (auto popup = popup_.lock()) {
                popup->setStyleSheet(".Label { background-color: rgb(20, 100, 100); "
                                     "background-color-on-hover: rgb(20, 130, 130); }");
            }
        }
        return popup_;
    }

    void onClickMe_(ui::Widget* from) {
        if (from) {
            if (auto overlayArea = getClickMeOverlayArea_(*from).lock()) {
                if (auto popup = getClickMePopup_(*overlayArea).lock()) {
                    geometry::Vec2f p =
                        from->mapTo(overlayArea.get(), geometry::Vec2f(0.f, 0.f));
                    popup->updateGeometry(
                        p, geometry::Vec2f(120.f, 25.f)); // TODO Make dpi-independent
                }
            }
        }
    }

    void createClickMePopups_(ui::Widget* parent) {

        // Create a grid of "click me" buttons, whose actions are to set the popup geometry.
        ui::Grid* grid = parent->createChild<ui::Grid>();
        grid->setStyleSheet(".Grid { column-gap: 10dp; row-gap: 10dp; }");
        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 4; ++j) {
                ui::Action* action =
                    parent->createTriggerAction(commands::openPopup(), "click me");
                ui::ButtonPtr button = ui::Button::create(action);
                grid->setWidgetAt(button.get(), i, j);
                action->triggered().connect([=](ui::Widget* from) { onClickMe_(from); });
            }
        }
    }

    void createMessageDialogButtons_(ui::Widget* parent) {

        ui::Flex* row = parent->createChild<ui::Flex>(ui::FlexDirection::Row);

        ui::Action* action = parent->createTriggerAction(commands::maybeQuit(), "Quit?");
        ui::Button* button = row->createChild<ui::Button>(action);
        action->triggered().connect([=]() {
            auto dialog = ui::MessageDialog::create();
            dialog->setTitle("Quit");
            dialog->addText("Are you sure you want to quit the application?");
            dialog->addButton("No", [=]() { dialog->destroy(); });
            dialog->addButton("Yes", []() {
                if (auto application =
                        dynamic_cast<app::CanvasApplication*>(ui::application())) {
                    application->quit();
                }
            });
            dialog->showAtWindow(button);
        });
    }
};

VGC_DECLARE_OBJECT(ImagesAndIconsPanel);

class ImagesAndIconsPanel : public ui::Panel {
private:
    VGC_OBJECT(ImagesAndIconsPanel, ui::Panel)

public:
    static constexpr std::string_view label = "Images and Icons";
    static constexpr std::string_view id = "vgc.uitest.imagesAndIcons";
    static constexpr ui::PanelDefaultArea defaultArea = ui::PanelDefaultArea::Right;

    static ImagesAndIconsPanelPtr create(const ui::PanelContext& context) {
        return core::createObject<ImagesAndIconsPanel>(context);
    }

protected:
    ImagesAndIconsPanel(CreateKey key, const ui::PanelContext& context)
        : Panel(key, context, label) {

        addStyleClass(s_with_padding);

        ui::Row* layout = createChild<ui::Row>();
        createImageBox_(layout);
        createIconWidget_(layout);
    }

private:
    void createImageBox_(ui::Widget* parent) {
        std::string imagePath = core::resourcePath("apps/uitest/icons/512.png");
        parent->createChild<ui::ImageBox>(imagePath);
    }

    core::Array<std::string_view> iconNames_ = {
        "apps/uitest/svg/samples/tiger.svg",
        "apps/uitest/svg/coords/InitialCoords-notext.svg",
        "apps/uitest/svg/coords/Nested-notext.svg",
        "apps/uitest/svg/coords/NewCoordSys-notext.svg",
        "apps/uitest/svg/coords/OrigCoordSys-notext.svg",
        "apps/uitest/svg/coords/PreserveAspectRatio-noentity-notext.svg",
        "apps/uitest/svg/coords/RotateScale-notext.svg",
        "apps/uitest/svg/coords/Skew-notext.svg",
        "apps/uitest/svg/coords/Units-notext.svg",
        "apps/uitest/svg/coords/Viewbox-notext.svg",

        "apps/uitest/svg/painting/fillrule-evenodd-nodefs.svg",
        "apps/uitest/svg/painting/fillrule-nonzero-nodefs.svg",
        "apps/uitest/svg/painting/inheritance-nogradient.svg",
        "apps/uitest/svg/painting/linecap-nostylesheet-nodefs-notext.svg",
        "apps/uitest/svg/painting/linejoin-nostylesheet-nodefs-notext.svg",
        "apps/uitest/svg/painting/marker-simulated.svg",
        "apps/uitest/svg/painting/marker.svg",
        "apps/uitest/svg/painting/miterlimit-notext.svg",

        "apps/uitest/svg/paths/arcs01.svg",
        "apps/uitest/svg/paths/arcs02-nodefs.svg",
        "apps/uitest/svg/paths/cubic01-nostylesheet.svg",
        "apps/uitest/svg/paths/cubic02-nostylesheet.svg",
        "apps/uitest/svg/paths/quad01.svg",
        "apps/uitest/svg/paths/triangle01.svg",
        "apps/uitest/svg/shapes/circle01.svg",
        "apps/uitest/svg/shapes/ellipse01.svg",
        "apps/uitest/svg/shapes/line01.svg",
        "apps/uitest/svg/shapes/polygon01.svg",
        "apps/uitest/svg/shapes/polyline01.svg",
        "apps/uitest/svg/shapes/rect01.svg",
        "apps/uitest/svg/shapes/rect02.svg"};

    Int iconIndex_ = 0;
    ui::IconWidgetPtr iconWidget_ = nullptr;

    void cycleSvgIcon_() {
        if (iconWidget_) {
            iconIndex_ = (iconIndex_ + 1) % iconNames_.length();
            iconWidget_->setFilePath(core::resourcePath(iconNames_[iconIndex_]));
        }
    }
    VGC_SLOT(cycleSvgIconSlot_, cycleSvgIcon_)

    void createIconWidget_(ui::Widget* parent) {
        std::string iconPath = core::resourcePath(iconNames_[iconIndex_]);
        iconWidget_ = parent->createChild<ui::IconWidget>(iconPath);
        parent->defineAction(commands::cycleSvgIcon(), cycleSvgIconSlot_());
    }
};

} // namespace

class UiTestApplication : public app::CanvasApplication {
    VGC_OBJECT(UiTestApplication, app::CanvasApplication)

public:
    static UiTestApplicationPtr create(int argc, char* argv[]) {
        return core::createObject<UiTestApplication>(argc, argv);
    }

protected:
    UiTestApplication(CreateKey key, int argc, char* argv[])
        : app::CanvasApplication(key, argc, argv, "VGC UI Test") {

        setOrganizationName("VGC Software");
        setOrganizationDomain("vgc.io");
        setWindowIconFromResource("apps/illustration/icons/512.png");
        createTestActionsAndMenus_();
        registerTestPanels_();

        importModule<uitest::ResetCurrentColor>();
    }

private:
    ui::Menu* testMenu_ = nullptr;
    void createTestActionsAndMenus_() {

        using ui::Action;
        using ui::Key;
        using ui::Menu;
        using ui::ModifierKey;
        using ui::Shortcut;

        ui::Widget* parent = mainWidget();

        if (auto standardMenus = importModule<ui::StandardMenus>().lock()) {
            if (auto menuBar = standardMenus->menuBar().lock()) {
                testMenu_ = menuBar->createSubMenu("Test");
            }
        }
        if (!testMenu_) {
            return;
        }

        Action* actionCreateAction =
            parent->defineAction(commands::createAction(), [this]() {
                Action* action = this->testMenu_->createTriggerAction(commands::hello());
                this->testMenu_->addItem(action);
            });

        Action* actionCreateMenu = parent->defineAction(commands::createMenu(), [this]() {
            if (auto standardMenus = importModule<ui::StandardMenus>().lock()) {
                if (auto menuBar = standardMenus->menuBar().lock()) {
                    Menu* menu = menuBar->createSubMenu("Test 2");
                    Action* action = menu->createTriggerAction(commands::hello());
                    menu->addItem(action);
                }
            }
        });

        testMenu_->addItem(actionCreateAction);
        testMenu_->addItem(actionCreateMenu);
        Menu* menu1 = testMenu_->createSubMenu("Menu 1");
        Menu* menu2 = testMenu_->createSubMenu("Menu 2");
        Menu* menu3 = testMenu_->createSubMenu("Menu 3");

        menu1->addItem(parent->createTriggerAction(commands::_1_1()));
        menu1->addItem(parent->createTriggerAction(commands::_1_2()));
        menu1->addItem(parent->createTriggerAction(commands::_1_3()));
        menu1->addItem(parent->createTriggerAction(commands::_1_4()));
        menu1->addItem(parent->createTriggerAction(commands::_1_5()));
        menu1->addItem(parent->createTriggerAction(commands::_1_6()));
        menu1->addItem(parent->createTriggerAction(commands::_1_7()));
        Menu* menu1b = menu1->createSubMenu("Menu 1.8");

        menu1b->addItem(parent->createTriggerAction(commands::_1_8_1()));
        menu1b->addItem(parent->createTriggerAction(commands::_1_8_2()));
        menu1b->addItem(parent->createTriggerAction(commands::_1_8_3()));
        menu1b->addItem(parent->createTriggerAction(commands::_1_8_4()));
        menu1b->addItem(parent->createTriggerAction(commands::_1_8_5()));
        menu1b->addItem(parent->createTriggerAction(commands::_1_8_6()));
        menu1b->addItem(parent->createTriggerAction(commands::_1_8_7()));

        menu2->addItem(parent->createTriggerAction(commands::_2_1()));
        menu2->addItem(parent->createTriggerAction(commands::_2_2()));

        menu3->addItem(parent->createTriggerAction(commands::_3_1()));
    }

    void registerTestPanels_() {
        if (auto panelManager = importModule<ui::PanelManager>().lock()) {
            panelManager->registerPanelType<Plot2dPanel>();
            panelManager->registerPanelType<MiscTestsPanel>();
            panelManager->registerPanelType<ImagesAndIconsPanel>();
        }
    }
};

int main(int argc, char* argv[]) {
    vgc::workspace::detail::setMultiJoinEnabled(true);
    auto application = UiTestApplication::create(argc, argv);
    return application->exec();
}
