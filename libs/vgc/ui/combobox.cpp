// Copyright 2024 The VGC Developers
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

#include <vgc/ui/combobox.h>

#include <vgc/ui/menu.h>

namespace vgc::ui {

namespace commands_ {

namespace {

VGC_UI_DEFINE_TRIGGER_COMMAND( //
    item1,
    "ui.combobox.test.item1",
    "Item 1");

VGC_UI_DEFINE_TRIGGER_COMMAND( //
    item2,
    "ui.combobox.test.item2",
    "Item 2");

} // namespace

} // namespace commands_

ComboBox::ComboBox(CreateKey key)
    : MenuButton(key, nullptr, FlexDirection::Row) {

    setMenuDropDirection(MenuDropDirection::Vertical);

    menu_ = ComboBoxMenu::create("My Combo Box", this);
    menu_->addItem(createTriggerAction(commands_::item1()));
    menu_->addItem(createTriggerAction(commands_::item2()));

    setAction(menu_->menuAction());

    addStyleClass(strings::ComboBox);
}

ComboBoxPtr ComboBox::create() {
    return core::createObject<ComboBox>();
}

ComboBoxMenu::ComboBoxMenu(CreateKey key, std::string_view title, Widget* comboBox)
    : Menu(key, title)
    , comboBox_(comboBox) {

    addStyleClass(strings::ComboBoxMenu);
}

ComboBoxMenuPtr ComboBoxMenu::create(std::string_view title, Widget* comboBox) {
    return core::createObject<ComboBoxMenu>(title, comboBox);
}

geometry::Vec2f ComboBoxMenu::computePreferredSize() const {
    geometry::Vec2f ret = Menu::computePreferredSize();
    if (auto comboBox = comboBox_.lock()) {
        ret[0] = (std::max)(comboBox->width(), ret[0]);
    }
    return ret;
}

} // namespace vgc::ui
