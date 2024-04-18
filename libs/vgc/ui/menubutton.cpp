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

#include <vgc/ui/menubutton.h>

#include <vgc/ui/menu.h>
#include <vgc/ui/strings.h>

namespace vgc::ui {

MenuButton::MenuButton(CreateKey key, Action* action, FlexDirection layoutDirection)
    : Button(key, action, layoutDirection) {

    addStyleClass(strings::MenuButton);
    setShortcutVisible(true);
}

MenuButtonPtr MenuButton::create(Action* action, FlexDirection layoutDirection) {
    return core::createObject<MenuButton>(action, layoutDirection);
}

void MenuButton::closePopupMenu() {
    if (Menu* menu = popupMenu()) {
        menu->close();
    }
}

void MenuButton::onMenuPopupOpened_(Menu* menu) {
    if (popupMenu_ == menu) {
        return;
    }
    if (popupMenu_) {
        closePopupMenu();
    }
    popupMenu_ = menu;
    popupMenu_->popupClosed().connect(onMenuPopupClosedSlot_());
    setActive(true);
    menuPopupOpened().emit();
}

void MenuButton::onMenuPopupClosed_(bool recursive) {
    setActive(false);
    popupMenu_->popupClosed().disconnect(onMenuPopupClosedSlot_());
    popupMenu_ = nullptr;
    menuPopupClosed().emit(recursive);
}

void MenuButton::onParentWidgetChanged(Widget* newParent) {
    SuperClass::onParentWidgetChanged(newParent);
    parentMenu_ = dynamic_cast<Menu*>(newParent);
}

} // namespace vgc::ui
