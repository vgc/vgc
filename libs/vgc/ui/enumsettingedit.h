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

#ifndef VGC_UI_ENUMSETTINGEDIT_H
#define VGC_UI_ENUMSETTINGEDIT_H

#include <vgc/ui/combobox.h>
#include <vgc/ui/enumsetting.h>
#include <vgc/ui/settingedit.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(EnumSettingEdit);

/// \class vgc::ui::EnumSettingEdit
/// \brief A `SettingEdit` subclass for editing enum settings
///
class VGC_UI_API EnumSettingEdit : public SettingEdit {
private:
    VGC_OBJECT(EnumSettingEdit, SettingEdit)

protected:
    EnumSettingEdit(CreateKey, EnumSettingPtr setting);

public:
    /// Creates a `EnumSettingEdit`.
    ///
    static EnumSettingEditPtr create(EnumSettingPtr setting);

    /// Returns the `ComboBox` widget of this `EnumSettingEdit`.
    ///
    ComboBoxWeakPtr comboBox() const {
        return comboBox_;
    };

    /// Returns the current value of this `EnumSettingEdit`.
    ///
    core::EnumValue value() const {
        return enumSetting_->value();
    }

private:
    EnumSettingSharedPtr enumSetting_;
    ComboBoxSharedPtr comboBox_;

    void onComboBoxIndexChanged_();
    VGC_SLOT(onComboBoxIndexChanged_);

    void onEnumSettingValueChanged_(core::EnumValue value);
    VGC_SLOT(onEnumSettingValueChanged_);
};

} // namespace vgc::ui

#endif // VGC_UI_ENUMSETTINGEDIT_H
