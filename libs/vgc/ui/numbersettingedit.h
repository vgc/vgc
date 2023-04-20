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

#ifndef VGC_UI_NUMBERSETTINGEDIT_H
#define VGC_UI_NUMBERSETTINGEDIT_H

#include <vgc/ui/label.h>
#include <vgc/ui/numberedit.h>
#include <vgc/ui/numbersetting.h>
#include <vgc/ui/settingedit.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(NumberSettingEdit);

/// \class vgc::ui::NumberSettingEdit
/// \brief A `SettingEdit` subclass for editing numbers
///
class VGC_UI_API NumberSettingEdit : public SettingEdit {
private:
    VGC_OBJECT(NumberSettingEdit, SettingEdit)

protected:
    NumberSettingEdit(NumberSettingPtr setting);

public:
    /// Creates a `NumberSettingEdit`.
    ///
    static NumberSettingEditPtr create(NumberSettingPtr setting);

    /// Returns the `NumberEdit` widget of this `NumberSettingEdit`.
    ///
    NumberEdit* numberEdit() const {
        return numberEdit_.get();
    };

    /// Returns the current value of this `NumberSettingEdit`.
    ///
    double value() const {
        return numberEdit_->value();
    }

private:
    NumberSettingPtr numberSetting_;
    LabelPtr label_;
    NumberEditPtr numberEdit_;

    void onNumberEditValueChanged_(double value);
    VGC_SLOT(onNumberEditValueChangedSlot_, onNumberEditValueChanged_);
};

} // namespace vgc::ui

#endif // VGC_UI_NUMBERSETTINGEDIT_H
