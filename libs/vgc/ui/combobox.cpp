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
    selectItem,
    "ui.combobox.selectItem",
    "ComboBox: Select Item");

VGC_UI_DEFINE_TRIGGER_COMMAND( //
    selectPreviousItem,
    "ui.combobox.selectPreviousItem",
    "ComboBox: Select Previous Item",
    Key::Up);

VGC_UI_DEFINE_TRIGGER_COMMAND( //
    selectNextItem,
    "ui.combobox.selectNextItem",
    "ComboBox: Select Next Item",
    Key::Down);

} // namespace

} // namespace commands_

ComboBox::ComboBox(CreateKey key, std::string_view title)
    : MenuButton(key, nullptr, FlexDirection::Row)
    , title_(title) {

    addStyleClass(strings::ComboBox);

    // Setup focus policy.
    //
    // Note: we intentionally don't give (single-)Click focus policy since we
    // believe this is more annoying than helpful in most of our use cases.
    //
    // Advantages of giving focus:
    // - Users can use arrow keys to quickly compare different results based on
    //   the combo box value.
    //
    // Disadvantages of giving focus (too easily):
    // - Clears the focus of other medium-strength widget (e.g., a line edit).
    // - Users cannot use anymore the arrow keys for their other purposes
    //   (translating objects, previous/next frame or keyframe, etc.), and
    //   become confused / not understand why the arrow keys don't work
    // - Users would have to make an extra click or pressing `Esc` to give
    //   the focus back to the previously high-strengh focused widget.
    // - Change of border/outline of the combo-box (or any other style choice
    //   to indicate that the combo box has focus) can be distracting.
    //
    // Basically, the disadvantages can be serious usability issues or/and
    // annoyances, while the only advantage is rarely useful. But in some
    // cases, the advantage can in fact be super useful (e.g., choosing a
    // font), so it's nice to be able to do it.
    //
    // For this reason, we choose to give focus on double-click, which is a
    // nice tradeoff, whose only issue is discoverability (perhaps we could add
    // in the tooltip: "double-click to focus"?). But since there is no focus
    // policy or action type for doing this yet, we detect it manually via
    // delay between opening and closing the menu.
    //
    setFocusPolicy(FocusPolicy::Tab);
    setFocusStrength(FocusStrength::Medium);

    setMenuDropDirection(MenuDropDirection::Vertical);
    setArrowVisible(true);
    setShortcutVisible(false);

    menu_ = ComboBoxMenu::create(title, this);
    if (auto menu = menu_.lock()) {
        setAction(menu->menuAction());
        menu->popupOpened().connect(onMenuOpened_Slot());
        menu->popupClosed().connect(onMenuClosed_Slot());
    }

    defineAction(commands_::selectPreviousItem(), onSelectPreviousItem_Slot());
    defineAction(commands_::selectNextItem(), onSelectNextItem_Slot());
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

void ComboBox::setIndex(Int index) {
    if (index != index_) {
        if (0 <= index && index < numItems()) {
            if (auto menu = menu_.lock()) {
                if (auto item = WidgetWeakPtr(menu->childAt(index)).lock()) {
                    setItem_(item.get(), index);
                    return;
                }
            }
            setItem_(nullptr, index);
        }
        else {
            setItem_(nullptr, -1);
        }
    }
}

void ComboBox::addItem(std::string_view text) {
    ActionWeakPtr actionWeak = createTriggerAction(commands_::selectItem());
    if (auto action = actionWeak.lock()) {
        action->setText(text);
        action->triggered().connect(onSelectItem_Slot());
        if (auto menu = menu_.lock()) {
            menu->addItem(action.get());

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

void ComboBox::setItem_(Widget* item, Int index) {
    if (index_ != index) {
        index_ = index;
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
        indexChanged().emit(index);
    }
}

void ComboBox::onSelectItem_(Widget* from) {
    wasItemSelectedSinceMenuOpened_ = true;
    if (auto menu = menu_.lock()) {
        Int index = 0;
        for (Widget* child : menu->children()) {
            if (child == from) {
                setItem_(child, index);
            }
            ++index;
        }
    }
}

void ComboBox::onMenuOpened_() {
    wasItemSelectedSinceMenuOpened_ = false;
    menuOpenedWatch_.start();
}

void ComboBox::onMenuClosed_() {

    constexpr float doubleClickMaxDuration = 0.3f;

    if (!wasItemSelectedSinceMenuOpened_
        && menuOpenedWatch_.elapsed() < doubleClickMaxDuration) {

        setFocus(FocusReason::Other);
    }
}

void ComboBox::onSelectPreviousItem_() {
    Int n = numItems();
    if (n > 0) {
        Int i = index();
        if (i < 0) {
            setIndex(n - 1);
        }
        else {
            setIndex((i - 1 + n) % n);
        }
    }
}

void ComboBox::onSelectNextItem_() {
    Int n = numItems();
    if (n > 0) {
        Int i = index();
        if (i < 0) {
            setIndex(0);
        }
        else {
            setIndex((i + 1) % n);
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
