// Copyright 2023 The VGC Developers
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

#include <vgc/ui/tabbar.h>

#include <vgc/core/paths.h>
#include <vgc/style/strings.h>
#include <vgc/ui/button.h>
#include <vgc/ui/detail/paintutil.h>
#include <vgc/ui/iconwidget.h>
#include <vgc/ui/label.h>
#include <vgc/ui/preferredsizecalculator.h>

namespace vgc::ui {

namespace {

namespace commands {

VGC_UI_DEFINE_TRIGGER_COMMAND( //
    closeTab,
    "ui.tabBar.closeTab",
    "Close Tab",
    Key::None,
    "ui/icons/close.svg")

} // namespace commands

} // namespace

TabBar::TabBar(CreateKey key)
    : Widget(key) {

    Action* closeTabAction = createTriggerAction(commands::closeTab());
    closeTabAction->triggered().connect(onCloseTabTriggeredSlot_());

    tabs_ = createChild<Flex>(FlexDirection::Row);

    ButtonPtr closeTabButton = createChild<Button>(closeTabAction);
    closeTabButton->setTextVisible(false);
    closeTabButton->setTooltipEnabled(false);
    closeTabButton->setIconVisible(true);

    close_ = closeTabButton;
    close_->hide();

    addStyleClass(strings::TabBar);
    tabs_->addStyleClass(strings::tabs);
    close_->addStyleClass(strings::close);
}

TabBarPtr TabBar::create() {
    return core::createObject<TabBar>();
}

void TabBar::addTab(std::string_view label, bool isClosable) {
    tabSpecs_.append({isClosable});
    if (tabs_) {
        tabs_->createChild<Label>(label);
    }
}

Int TabBar::numTabs() const {
    if (tabs_) {
        return tabs_->numChildren();
    }
    else {
        return 0;
    }
}

void TabBar::onMouseEnter() {
    // TODO: actually handle having several tabs.
    // TODO: one close icon per tab (or not, to save space?).
    bool isActiveTabClosable = false;
    Int activeTab = tabSpecs_.isEmpty() ? -1 : 0;
    if (activeTab != -1) {
        isActiveTabClosable = tabSpecs_[activeTab].isClosable;
    }
    if (isActiveTabClosable && close_) {
        close_->show();
    }
}

void TabBar::onMouseLeave() {
    if (close_) {
        close_->hide();
    }
}

geometry::Vec2f TabBar::computePreferredSize() const {
    // The preferred size is determined entirely by the preferred
    // size of the tabs (that is, we ignore the close icon).
    PreferredSizeCalculator calc(this);
    if (tabs_) {
        calc.add(tabs_->preferredSize());
    }
    calc.addPaddingAndBorder();
    return calc.compute();
}

void TabBar::updateChildrenGeometry() {

    geometry::Rect2f rect = contentRect();

    // Update geometry of tabs
    if (tabs_) {
        tabs_->updateGeometry(rect);
    }

    // Update geometry of close icon
    if (close_) {

        using detail::getLengthOrPercentageInPx;
        namespace ss = style::strings;

        geometry::Vec2f size = close_->preferredSize();
        geometry::Vec2f position;

        // Align to the right
        position[0] = rect.xMax()
                      - getLengthOrPercentageInPx(close_.get(), ss::margin_right, size[0])
                      - size[0];

        // Center vertically
        position[1] = rect.yMin() + 0.5f * (rect.height() - size[1]);
        close_->updateGeometry(position, size);
    }
}

void TabBar::onCloseTabTriggered_() {
    Widget* activeTab = nullptr;
    Int tabIndex = 0;
    if (tabs_) {
        // For now we support only one tab.
        activeTab = tabs_->firstChild();
        tabIndex = 0;
    }
    if (activeTab) {
        activeTab->destroy();
        tabClosed().emit(tabIndex);
    }
}

} // namespace vgc::ui
