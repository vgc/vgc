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
#include <vgc/ui/imagebox.h>
#include <vgc/ui/lineedit.h>
#include <vgc/ui/overlayarea.h>
#include <vgc/ui/plot2d.h>
#include <vgc/ui/row.h>

namespace app = vgc::app;
namespace core = vgc::core;
namespace geometry = vgc::geometry;
namespace ui = vgc::ui;
namespace workspace = vgc::workspace;

using vgc::Int;
using vgc::UInt32;

VGC_DECLARE_OBJECT(UiTestApplication);

class UiTestApplication : public app::CanvasApplication {
    VGC_OBJECT(UiTestApplication, app::CanvasApplication)

public:
    static UiTestApplicationPtr create(int argc, char* argv[]) {
        return UiTestApplicationPtr(new UiTestApplication(argc, argv));
    }

protected:
    UiTestApplication(int argc, char* argv[])
        : app::CanvasApplication(argc, argv, "VGC UI Test") {

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

        Action* actionCreateAction = parent->createAction(
            "Create action in Test menu", Shortcut(ModifierKey::Ctrl, Key::A));
        actionCreateAction->triggered().connect([this]() {
            Action* action = this->testMenu_->createAction("Hello");
            this->testMenu_->addItem(action);
        });

        Action* actionCreateMenu = parent->createAction(
            "Create menu in menubar", Shortcut(ModifierKey::Ctrl, Key::M));
        actionCreateMenu->triggered().connect([this]() {
            Menu* menu = this->menuBar()->createSubMenu("Test 2");
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

    void createTestWidgets_() {

        using Panel = app::detail::Panel;
        using app::detail::createPanel;
        using app::detail::createPanelWithPadding;

        // Create panel areas
        ui::PanelArea* mainArea = panelArea();
        ui::PanelArea* rightArea = ui::PanelArea::createVerticalSplit(mainArea);
        ui::PanelArea* rightTopArea = ui::PanelArea::createTabs(rightArea);
        ui::PanelArea* rightBottomArea = ui::PanelArea::createTabs(rightArea);

        // Create panels
        Panel* rightTopPanel = createPanelWithPadding(rightTopArea);
        Panel* rightBottomPanel = createPanelWithPadding(rightBottomArea);

        // Create widgets inside panels
        createPlot2d_(rightTopPanel);
        createGrid_(rightBottomPanel);
        createClickMePopups_(rightBottomPanel);
        createLineEdits_(rightBottomPanel);
        createImageBox_(rightBottomPanel);
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
                    label->updateGeometry(
                        p, geometry::Vec2f(120.f, 25.f)); // TODO Make dpi-independent
                });
            }
        }
    }

    void createImageBox_(ui::Widget* parent) {
        std::string imagePath = core::resourcePath("apps/illustration/icons/512.png");
        parent->createChild<ui::ImageBox>(imagePath);
    }
};

int main(int argc, char* argv[]) {
    auto application = UiTestApplication::create(argc, argv);
    return application->exec();
}
