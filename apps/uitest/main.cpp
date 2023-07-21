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
#include <vgc/ui/grid.h>
#include <vgc/ui/iconwidget.h>
#include <vgc/ui/imagebox.h>
#include <vgc/ui/lineedit.h>
#include <vgc/ui/messagedialog.h>
#include <vgc/ui/numberedit.h>
#include <vgc/ui/overlayarea.h>
#include <vgc/ui/plot2d.h>
#include <vgc/ui/row.h>
#include <vgc/workspace/vertex.h>

namespace app = vgc::app;
namespace core = vgc::core;
namespace geometry = vgc::geometry;
namespace ui = vgc::ui;

using vgc::Int;
using vgc::UInt32;

VGC_DECLARE_OBJECT(UiTestApplication);

namespace {

namespace commands {

using Shortcut = ui::Shortcut;
using Key = ui::Key;

constexpr ui::ModifierKey ctrl = ui::ModifierKey::Ctrl;

VGC_UI_DEFINE_WINDOW_COMMAND(
    createAction,
    "uitest.createActionInTestMenu",
    "Create action in Test menu",
    Shortcut(ctrl, Key::A))

VGC_UI_DEFINE_WINDOW_COMMAND(
    createMenu,
    "uitest.createMenuInMenuBar",
    "Create menu in menubar",
    Shortcut(ctrl, Key::M))

VGC_UI_DEFINE_WINDOW_COMMAND(hello, "uitest.hello", "Hello")

VGC_UI_DEFINE_WINDOW_COMMAND(_1_1, "uitest.1.1", "Action #1.1", Shortcut(ctrl, Key::G))
VGC_UI_DEFINE_WINDOW_COMMAND(_1_2, "uitest.1.2", "Action #1.2", Shortcut(ctrl, Key::L))
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

VGC_UI_DEFINE_WINDOW_COMMAND(_2_1, "uitest.2.1", "Action #2.1", Shortcut(ctrl, Key::F))
VGC_UI_DEFINE_WINDOW_COMMAND(_2_2, "uitest.2.2", "Action #2.2", Shortcut(ctrl, Key::K))

VGC_UI_DEFINE_WINDOW_COMMAND(_3_1, "uitest.action.3.1", "Action #3.1")

VGC_UI_DEFINE_WINDOW_COMMAND(openPopup, "uitest.openPopup", "Open Popup")
VGC_UI_DEFINE_WINDOW_COMMAND(maybeQuit, "uitest.maybeQuit", "Maybe Quit")

} // namespace commands
} // namespace

class UiTestApplication : public app::CanvasApplication {
    VGC_OBJECT(UiTestApplication, app::CanvasApplication)

public:
    static UiTestApplicationPtr create(int argc, char* argv[]) {
        return UiTestApplicationPtr(new UiTestApplication(argc, argv));
    }

protected:
    UiTestApplication(int argc, char* argv[])
        : app::CanvasApplication(argc, argv, "VGC UI Test") {

        setOrganizationName("VGC Software");
        setOrganizationDomain("vgc.io");
        setWindowIconFromResource("apps/illustration/icons/512.png");
        createTestActionsAndMenus_();
        createTestWidgets_();
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

        testMenu_ = menuBar()->createSubMenu("Test");

        Action* actionCreateAction = parent->createTriggerAction(commands::createAction);
        actionCreateAction->triggered().connect([this]() {
            Action* action = this->testMenu_->createTriggerAction(commands::hello);
            this->testMenu_->addItem(action);
        });

        Action* actionCreateMenu = parent->createTriggerAction(commands::createMenu);
        actionCreateMenu->triggered().connect([this]() {
            Menu* menu = this->menuBar()->createSubMenu("Test 2");
            Action* action = menu->createTriggerAction(commands::hello);
            menu->addItem(action);
        });

        testMenu_->addItem(actionCreateAction);
        testMenu_->addItem(actionCreateMenu);
        Menu* menu1 = testMenu_->createSubMenu("Menu 1");
        Menu* menu2 = testMenu_->createSubMenu("Menu 2");
        Menu* menu3 = testMenu_->createSubMenu("Menu 3");

        menu1->addItem(parent->createTriggerAction(commands::_1_1));
        menu1->addItem(parent->createTriggerAction(commands::_1_2));
        menu1->addItem(parent->createTriggerAction(commands::_1_3));
        menu1->addItem(parent->createTriggerAction(commands::_1_4));
        menu1->addItem(parent->createTriggerAction(commands::_1_5));
        menu1->addItem(parent->createTriggerAction(commands::_1_6));
        menu1->addItem(parent->createTriggerAction(commands::_1_7));
        Menu* menu1b = menu1->createSubMenu("Menu 1.8");

        menu1b->addItem(parent->createTriggerAction(commands::_1_8_1));
        menu1b->addItem(parent->createTriggerAction(commands::_1_8_2));
        menu1b->addItem(parent->createTriggerAction(commands::_1_8_3));
        menu1b->addItem(parent->createTriggerAction(commands::_1_8_4));
        menu1b->addItem(parent->createTriggerAction(commands::_1_8_5));
        menu1b->addItem(parent->createTriggerAction(commands::_1_8_6));
        menu1b->addItem(parent->createTriggerAction(commands::_1_8_7));

        menu2->addItem(parent->createTriggerAction(commands::_2_1));
        menu2->addItem(parent->createTriggerAction(commands::_2_2));

        menu3->addItem(parent->createTriggerAction(commands::_3_1));
    }

    void createTestWidgets_() {

        using app::detail::createPanelWithPadding;

        // Create panel areas
        ui::PanelArea* mainArea = panelArea();
        ui::PanelArea* rightArea = ui::PanelArea::createVerticalSplit(mainArea);
        ui::PanelArea* rightTopArea = ui::PanelArea::createTabs(rightArea);
        ui::PanelArea* rightBottomArea = ui::PanelArea::createTabs(rightArea);

        // Create panels
        ui::Panel* rightTopPanel = createPanelWithPadding(rightTopArea, "Plot 2D");
        ui::Panel* rightBottomPanel =
            createPanelWithPadding(rightBottomArea, "Misc Tests");
        ui::Column* rightTopContent = rightTopPanel->createChild<ui::Column>();
        ui::Column* rightBottomContent = rightBottomPanel->createChild<ui::Column>();

        // Create widgets inside panels
        createPlot2d_(rightTopContent);
        createGrid_(rightBottomContent);
        createClickMePopups_(rightBottomContent);
        createMessageDialogButtons_(rightBottomContent);
        createLineEdits_(rightBottomContent);
        createNumberEdits_(rightBottomContent);
        ui::Row* imageAndIconRow = rightBottomContent->createChild<ui::Row>();
        createImageBox_(imageAndIconRow);
        createIconWidget_(imageAndIconRow);
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
                ui::Action* action =
                    parent->createTriggerAction(commands::openPopup, "click me");
                ui::ButtonPtr button = ui::Button::create(action);
                grid->setWidgetAt(button.get(), i, j);
                action->triggered().connect([=](ui::Widget* from) {
                    geometry::Vec2f p =
                        from->mapTo(overlayArea, geometry::Vec2f(0.f, 0.f));
                    label->updateGeometry(
                        p, geometry::Vec2f(120.f, 25.f)); // TODO Make dpi-independent
                });
            }
        }
    }

    void createMessageDialogButtons_(ui::Widget* parent) {

        ui::Flex* row = parent->createChild<ui::Flex>(ui::FlexDirection::Row);

        ui::Action* action = parent->createTriggerAction(commands::maybeQuit, "Quit?");
        ui::Button* button = row->createChild<ui::Button>(action);
        action->triggered().connect([=]() {
            auto dialog = ui::MessageDialog::create();
            dialog->setTitle("Quit");
            dialog->addText("Are you sure you want to quit the application?");
            dialog->addButton("No", [=]() { dialog->destroy(); });
            dialog->addButton("Yes", [=]() { quit(); });
            dialog->showAt(button);
        });
    }

    void createImageBox_(ui::Widget* parent) {
        std::string imagePath = core::resourcePath("apps/uitest/icons/512.png");
        parent->createChild<ui::ImageBox>(imagePath);
    }

    void createIconWidget_(ui::Widget* parent) {
        std::string iconPath = core::resourcePath("apps/uitest/icons/heart.svg");
        parent->createChild<ui::IconWidget>(iconPath);
    }
};

int main(int argc, char* argv[]) {
    vgc::workspace::detail::setMultiJoinEnabled(true);
    auto application = UiTestApplication::create(argc, argv);
    return application->exec();
}
