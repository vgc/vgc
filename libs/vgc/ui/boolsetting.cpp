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

#include <vgc/ui/boolsetting.h>

namespace vgc::ui {

BoolSetting::BoolSetting(
    CreateKey createKey,
    Settings* settings,
    std::string_view key,
    std::string_view label,
    bool defaultValue)

    : Setting(createKey, settings, key, label)
    , defaultValue_(defaultValue) {
}

BoolSettingPtr BoolSetting::create(
    Settings* settings,
    std::string_view key,
    std::string_view label,
    bool defaultValue) {

    return core::createObject<BoolSetting>(settings, key, label, defaultValue);
}

bool BoolSetting::value() const {
    return settings()->getOrSetBoolValue(key(), defaultValue_);
}

void BoolSetting::setValue(bool newValue) {
    bool oldValue = value();
    if (oldValue != newValue) {
        settings()->setBoolValue(key(), newValue);
        valueChanged().emit(newValue);
    }
}

} // namespace vgc::ui
