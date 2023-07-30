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

#include <vgc/ui/numbersettingedit.h>

#include <vgc/ui/strings.h>

namespace vgc::ui {

NumberSettingEdit::NumberSettingEdit(CreateKey key, NumberSettingPtr setting)

    : SettingEdit(key, setting)
    , numberSetting_(setting) {

    addStyleClass(strings::NumberSettingEdit);

    numberEdit_ = createChild<NumberEdit>();
    numberEdit_->setStep(setting->step());
    numberEdit_->setMinimum(setting->minimum());
    numberEdit_->setMaximum(setting->maximum());
    numberEdit_->setPrecision(setting->precision());
    numberEdit_->setValue(setting->value());
    numberEdit_->valueChanged().connect(onNumberEditValueChangedSlot_());

    numberSetting_->valueChanged().connect(onNumberSettingValueChangedSlot_());
}

NumberSettingEditPtr NumberSettingEdit::create(NumberSettingPtr setting) {

    return core::createObject<NumberSettingEdit>(setting);
}

void NumberSettingEdit::onNumberEditValueChanged_(double value) {
    numberSetting_->setValue(value);
}

void NumberSettingEdit::onNumberSettingValueChanged_(double value) {
    numberEdit_->setValue(value);
}

} // namespace vgc::ui
