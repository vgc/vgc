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

#include <vgc/ui/enumsettingedit.h>

#include <vgc/ui/strings.h>

namespace vgc::ui {

EnumSettingEdit::EnumSettingEdit(CreateKey key, EnumSettingPtr setting)

    : SettingEdit(key, setting)
    , enumSetting_(setting) {

    addStyleClass(strings::EnumSettingEdit);

    comboBox_ = createChild<ComboBox>();

    auto enumSetting = enumSetting_.lock();
    if (!enumSetting) {
        return;
    }

    auto comboBox = comboBox_.lock();
    if (!comboBox) {
        return;
    }

    comboBox->setTitle(enumSetting->label());
    comboBox->setItemsFromEnum(enumSetting->enumType());
    comboBox->setEnumValue(enumSetting->value());
    comboBox->indexChanged().connect(onComboBoxIndexChanged_Slot());

    enumSetting->valueChanged().connect(onEnumSettingValueChanged_Slot());
}

EnumSettingEditPtr EnumSettingEdit::create(EnumSettingPtr setting) {

    return core::createObject<EnumSettingEdit>(setting);
}

void EnumSettingEdit::onComboBoxIndexChanged_() {
    if (auto enumSetting = enumSetting_.lock()) {
        if (auto comboBox = comboBox_.lock()) {
            if (auto value = comboBox->enumValue()) {
                enumSetting->setValue(*value);
            }
        }
    }
}

void EnumSettingEdit::onEnumSettingValueChanged_(core::EnumValue value) {
    if (auto comboBox = comboBox_.lock()) {
        comboBox->setEnumValue(value);
    }
}

} // namespace vgc::ui
