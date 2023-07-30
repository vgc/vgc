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

#ifndef VGC_UI_BOOLSETTINGEDIT_H
#define VGC_UI_BOOLSETTINGEDIT_H

#include <vgc/ui/boolsetting.h>
#include <vgc/ui/button.h>
#include <vgc/ui/settingedit.h>
#include <vgc/ui/toggle.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(BoolSettingEdit);

/// \class vgc::ui::BoolSettingEdit
/// \brief A `SettingEdit` subclass for editing boolean values.
///
class VGC_UI_API BoolSettingEdit : public SettingEdit {
private:
    VGC_OBJECT(BoolSettingEdit, SettingEdit)

protected:
    BoolSettingEdit(CreateKey, BoolSettingPtr setting);

public:
    /// Creates a `BoolSettingEdit`.
    ///
    static BoolSettingEditPtr create(BoolSettingPtr setting);

    /// Returns the current value of this `BoolSettingEdit`.
    ///
    bool value() const {
        return boolSetting_->value();
    }

private:
    BoolSettingPtr boolSetting_;
    TogglePtr toggle_;

    void onToggleToggled_(bool state);
    VGC_SLOT(onToggleToggledSlot_, onToggleToggled_)

    void onBoolSettingValueChanged_(bool value);
    VGC_SLOT(onBoolSettingValueChangedSlot_, onBoolSettingValueChanged_);
};

} // namespace vgc::ui

#endif // VGC_UI_BOOLSETTINGEDIT_H
