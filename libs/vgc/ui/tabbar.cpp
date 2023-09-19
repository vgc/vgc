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
#include <vgc/ui/detail/paintutil.h>
#include <vgc/ui/iconwidget.h>
#include <vgc/ui/label.h>
#include <vgc/ui/preferredsizecalculator.h>

namespace vgc::ui {

TabBar::TabBar(CreateKey key)
    : Flex(key, FlexDirection::Row) {

    std::string iconPath = core::resourcePath("ui/icons/close.svg");

    tabs_ = createChild<Flex>();
    close_ = createChild<IconWidget>(iconPath);

    addStyleClass(strings::TabBar);
    tabs_->addStyleClass(strings::tabs);
    close_->addStyleClass(strings::close);

    // TODO: style classes for subwidgets
}

TabBarPtr TabBar::create() {
    return core::createObject<TabBar>();
}

void TabBar::addTab(std::string_view label) {
    if (tabs_) {
        tabs_->createChild<Label>(label);
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

} // namespace vgc::ui
