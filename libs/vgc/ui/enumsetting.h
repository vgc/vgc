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

#ifndef VGC_UI_ENUMSETTING_H
#define VGC_UI_ENUMSETTING_H

#include <vgc/core/enum.h>
#include <vgc/ui/api.h>
#include <vgc/ui/setting.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(EnumSetting);

/// \class vgc::ui::EnumSetting
/// \brief A `Setting` subclass for enumeration values.
///
class VGC_UI_API EnumSetting : public Setting {
private:
    VGC_OBJECT(EnumSetting, Setting)

protected:
    EnumSetting(
        CreateKey createKey,
        Settings* settings,
        std::string_view key,
        std::string_view label,
        core::EnumValue defaultValue);

public:
    /// Creates an `EnumSetting`.
    ///
    static EnumSettingPtr create(
        Settings* settings,
        std::string_view key,
        std::string_view label,
        core::EnumValue defaultValue);

    /// Returns the default value of this `EnumSetting`.
    ///
    core::EnumValue defaultValue() const {
        return defaultValue_;
    }

    /// Returns the `TypeId` of the enum type of this `EnumSetting`.
    ///
    core::EnumType enumType() const {
        return defaultValue_.type();
    }

    /// Returns the current value of this `EnumSetting`.
    ///
    /// \sa `setValue()`.
    ///
    core::EnumValue value() const;

    /// Sets the value of this `EnumSetting`.
    ///
    /// \sa `value()`.
    ///
    void setValue(core::EnumValue value);
    VGC_SLOT(setValue)

    /// This signal is emitted whenever `value()` changes.
    ///
    VGC_SIGNAL(valueChanged, (core::EnumValue, value))

private:
    core::EnumValue defaultValue_;
    std::string defaultValueString_;
};

} // namespace vgc::ui

#endif // VGC_UI_ENUMSETTING_H
