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

#include <vgc/ui/boolsettingedit.h>

#include <vgc/ui/strings.h>

namespace vgc::ui {

VGC_DEFINE_ENUM( //
    BoolSettingStyle,
    (Toggle, "Toggle"),
    (Checkbox, "Checkbox"))

BoolSettingEdit::BoolSettingEdit(
    CreateKey key,
    BoolSettingPtr setting,
    BoolSettingStyle style)

    : SettingEdit(key, setting)
    , boolSetting_(setting) {

    addStyleClass(strings::BoolSettingEdit);

    setStyle(style);

    boolSetting_->valueChanged().connect(onBoolSettingValueChangedSlot_());
}

BoolSettingEditPtr
BoolSettingEdit::create(BoolSettingPtr setting, BoolSettingStyle style) {
    return core::createObject<BoolSettingEdit>(setting, style);
}

BoolSettingStyle BoolSettingEdit::style() {
    if (checkbox_) {
        return BoolSettingStyle::Checkbox;
    }
    else {
        return BoolSettingStyle::Toggle;
    }
}

void BoolSettingEdit::setStyle(BoolSettingStyle style) {
    switch (style) {
    case BoolSettingStyle::Toggle:
        if (checkbox_) {
            checkbox_->destroy();
        }
        if (!toggle_) {
            toggle_ = createChild<Toggle>();
            toggle_->setState(boolSetting_->value());
            toggle_->toggled().connect(onToggleToggledSlot_());
        }
        break;
    case BoolSettingStyle::Checkbox:
        if (toggle_) {
            toggle_->destroy();
        }
        if (!checkbox_) {
            checkbox_ = createChildBefore<Checkbox>(firstChild());
            checkbox_->setChecked(boolSetting_->value());
            checkbox_->checkStateChanged().connect(onCheckboxCheckStateChangedSlot_());
        }
        break;
    }
}

void BoolSettingEdit::onToggleToggled_(bool state) {
    boolSetting_->setValue(state);
}

void BoolSettingEdit::onCheckboxCheckStateChanged_(Checkbox*, CheckState state) {
    bool value = (state == CheckState::Checked) ? true : false;
    boolSetting_->setValue(value);
}

void BoolSettingEdit::onBoolSettingValueChanged_(bool value) {
    if (toggle_) {
        toggle_->setState(value);
    }
    if (checkbox_) {
        checkbox_->setChecked(value);
    }
}

} // namespace vgc::ui
