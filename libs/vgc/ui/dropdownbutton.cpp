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

#include <vgc/ui/dropdownbutton.h>

#include <vgc/core/paths.h>
#include <vgc/ui/menu.h>
#include <vgc/ui/strings.h>

namespace vgc::ui {

DropdownButton::DropdownButton(
    CreateKey key,
    Action* action,
    FlexDirection layoutDirection)

    : Button(key, action, layoutDirection) {

    addStyleClass(strings::DropdownButton);
    updateArrowIcon_();
}

DropdownButtonPtr DropdownButton::create(Action* action, FlexDirection layoutDirection) {
    return core::createObject<DropdownButton>(action, layoutDirection);
}

void DropdownButton::setDropDirection(DropDirection direction) {
    if (dropDirection_ != direction) {
        dropDirection_ = direction;
        updateArrowIcon_();
    }
}

bool DropdownButton::isArrowVisible() const {
    if (auto arrowIcon = arrowIcon_.lock()) {
        return arrowIcon_->visibility() == Visibility::Inherit;
    }
    else {
        return false;
    }
}

void DropdownButton::setArrowVisible(bool visible) {
    if (auto arrowIcon = arrowIcon_.lock()) {
        arrowIcon->setVisibility(visible ? Visibility::Inherit : Visibility::Invisible);
    }
}

void DropdownButton::closePopupMenu() {
    if (Menu* menu = popupMenu()) {
        menu->close();
    }
}

namespace {

std::string getArrowIconPath(DropDirection direction) {
    switch (direction) {
    case DropDirection::Horizontal:
        return core::resourcePath("ui/icons/button-right-arrow.svg");
    case DropDirection::Vertical:
        return core::resourcePath("ui/icons/button-down-arrow.svg");
    }
    return core::resourcePath("");
}

} // namespace

void DropdownButton::updateArrowIcon_() {
    if (auto arrowIcon = arrowIcon_.lock()) {
        arrowIcon->reparent(nullptr);
        arrowIcon_ = nullptr;
    }
    std::string iconPath = getArrowIconPath(dropDirection());
    if (!iconPath.empty()) {
        arrowIcon_ = createChild<IconWidget>(iconPath);
        arrowIcon_->addStyleClass(strings::arrow);
    }
}

void DropdownButton::onMenuPopupOpened_(Menu* menu) {
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

void DropdownButton::onMenuPopupClosed_(bool recursive) {
    setActive(false);
    popupMenu_->popupClosed().disconnect(onMenuPopupClosedSlot_());
    popupMenu_ = nullptr;
    menuPopupClosed().emit(recursive);
}

} // namespace vgc::ui
