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

#include <vgc/ui/enumsetting.h>

#include <vgc/ui/logcategories.h>

namespace vgc::ui {

EnumSetting::EnumSetting(
    CreateKey createKey,
    Settings* settings,
    std::string_view key,
    std::string_view label,
    core::EnumValue defaultValue)

    : Setting(createKey, settings, key, label)
    , defaultValue_(defaultValue)
    , defaultValueString_(defaultValue.shortName()) {
}

EnumSettingPtr EnumSetting::create(
    Settings* settings,
    std::string_view key,
    std::string_view label,
    core::EnumValue defaultValue) {

    return core::createObject<EnumSetting>(settings, key, label, defaultValue);
}

// TODO: improve performance of `EnumSetting::value()` by avoiding conversion
// from string, that is, keep a pointer to the actual integer value (or at
// least to the EnumValue). This requires some revamp of the whole
// Settings/Setting architecture. Such revamp would also be useful anyway for
// other Setting types (BoolSetting, NumberSetting, etc.) that also currently
// are slower than necessary: their `value()` method still requires querying
// the Settings' internal map, which would not be necessary if the Setting was
// some sort of smart pointer to the actual data, and the Settings class was
// basically a map<string, SettingPtr>.
//
core::EnumValue EnumSetting::value() const {
    std::string name = settings()->getOrSetStringValue(key(), defaultValueString_);
    if (auto value = core::Enum::fromShortName(defaultValue_.typeId(), name)) {
        return *value;
    }
    else {
        return defaultValue_;
    }
}

void EnumSetting::setValue(core::EnumValue newValue) {
    core::EnumValue oldValue = value();
    if (oldValue != newValue) {
        if (oldValue.typeId() != newValue.typeId()) {
            VGC_WARNING(
                LogVgcUi,
                "Cannot set value '{}' to setting '{}': the value has a "
                "different enum type than the setting's default value ('{}').",
                newValue,
                key(),
                defaultValue_);
        }
        else {
            // XXX: Shouldn't shortName return an optional? For example, if it
            // is currently storing an unregistered value of a registered enum type,
            // it would return an empty optional, allowing us to issue a warning
            // and format it as an integer instead.
            settings()->setStringValue(key(), newValue.shortName());
            valueChanged().emit(newValue);
        }
    }
}

} // namespace vgc::ui
