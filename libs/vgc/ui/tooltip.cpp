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

#include <vgc/ui/tooltip.h>

#include <vgc/ui/strings.h>

namespace vgc::ui {

Tooltip::Tooltip(CreateKey key, std::string_view text)
    : Dialog(key) {

    addStyleClass(strings::Tooltip);
    content_ = createContent<Flex>(FlexDirection::Row);

    textLabel_ = content_->createChild<Label>();
    textLabel_->addStyleClass(strings::text);

    shortcutLabel_ = content_->createChild<Label>();
    shortcutLabel_->addStyleClass(strings::shortcut);

    setText(text);
    setShortcut({});
}

TooltipPtr Tooltip::create() {
    return core::createObject<Tooltip>("");
}

TooltipPtr Tooltip::create(std::string_view text) {
    return core::createObject<Tooltip>(text);
}

std::string_view Tooltip::text() const {
    if (textLabel_) {
        return textLabel_->text();
    }
    else {
        return "";
    }
}

void Tooltip::setText(std::string_view text) {
    if (textLabel_) {
        textLabel_->setText(text);
    }
}

void Tooltip::setShortcut(const Shortcut& shortcut) {
    shortcut_ = shortcut;
    if (shortcutLabel_) {
        shortcutLabel_->setText(core::format("{}", shortcut));
    }
}

bool Tooltip::isTextVisible() const {
    return textLabel_ ? textLabel_->visibility() == Visibility::Inherit : false;
}

void Tooltip::setTextVisible(bool visible) {
    if (textLabel_) {
        textLabel_->setVisibility(visible ? Visibility::Inherit : Visibility::Invisible);
    }
}

bool Tooltip::isShortcutVisible() const {
    return shortcutLabel_ ? shortcutLabel_->visibility() == Visibility::Inherit : false;
}

void Tooltip::setShortcutVisible(bool visible) {
    if (shortcutLabel_) {
        shortcutLabel_->setVisibility(
            visible ? Visibility::Inherit : Visibility::Invisible);
    }
}

bool Tooltip::computeIsHovered(MouseHoverEvent*) const {
    return false;
}

} // namespace vgc::ui
