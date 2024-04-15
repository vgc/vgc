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
    item,
    "ui.combobox.item",
    "ComboBox Item");

} // namespace

} // namespace commands_

ComboBox::ComboBox(CreateKey key, std::string_view title)
    : MenuButton(key, nullptr, FlexDirection::Row)
    , title_(title) {

    addStyleClass(strings::ComboBox);
    setMenuDropDirection(MenuDropDirection::Vertical);
    setArrowVisible(true);
    setShortcutVisible(false);

    menu_ = ComboBoxMenu::create(title, this);
    if (auto menu = menu_.lock()) {
        setAction(menu->menuAction());
    }
}

ComboBoxPtr ComboBox::create(std::string_view title) {
    return core::createObject<ComboBox>(title);
}

Int ComboBox::numItems() const {
    if (auto menu = menu_.lock()) {
        return menu->numItems();
    }
    else {
        return 0;
    }
}

void ComboBox::setCurrentIndex(Int index) {
    if (index != currentIndex_) {
        if (0 <= index && index < numItems()) {
            if (auto menu = menu_.lock()) {
                if (auto item = WidgetWeakPtr(menu->childAt(index)).lock()) {
                    setCurrentItem_(item.get(), index);
                    return;
                }
            }
            setCurrentItem_(nullptr, index);
        }
        else {
            setCurrentItem_(nullptr, -1);
        }
    }
}

void ComboBox::addItem(std::string_view text) {
    ActionWeakPtr itemAction_ = createTriggerAction(commands_::item());
    if (auto itemAction = itemAction_.lock()) {
        itemAction->setText(text);
        itemAction->triggered().connect(onItemActionTriggered_Slot());
        if (auto menu = menu_.lock()) {
            menu->addItem(itemAction.get());

            // Disable shortcut otherwise it adds an extra gap even if
            // the shortcut size itself is 0.
            Int index = menu->numItems() - 1;
            if (auto item = WidgetWeakPtr(menu->childAt(index)).lock()) {
                if (auto button = dynamic_cast<MenuButton*>(item.get())) {
                    button->setShortcutVisible(false);
                }
            }
        }
    }
}

void ComboBox::setText_(std::string_view text) {
    if (auto comboBoxAction = ActionWeakPtr(action()).lock()) {
        comboBoxAction->setText(text);
    }
}

void ComboBox::setCurrentItem_(Widget* item, Int index) {
    if (currentIndex_ != index) {
        currentIndex_ = index;
        if (Button* button = dynamic_cast<Button*>(item)) {
            if (auto action = ActionWeakPtr(button->action()).lock()) {
                setText_(action->text());
            }
            else {
                setText_(title_);
            }
        }
        else {
            setText_(title_);
        }
        currentIndexChanged().emit(index);
    }
}

void ComboBox::onItemActionTriggered_(Widget* from) {
    if (auto menu = menu_.lock()) {
        Int index = 0;
        for (Widget* child : menu->children()) {
            if (child == from) {
                setCurrentItem_(child, index);
            }
            ++index;
        }
    }
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
