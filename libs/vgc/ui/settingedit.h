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

#ifndef VGC_UI_SETTINGEDIT_H
#define VGC_UI_SETTINGEDIT_H

#include <vgc/ui/api.h>
#include <vgc/ui/flex.h>
#include <vgc/ui/label.h>
#include <vgc/ui/setting.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(SettingEdit);

/// \class vgc::ui::SettingEdit
/// \brief A widget for editing an individual value in a `Settings`.
///
/// This class consists of a label and a field for editing an individual value
/// in a `Settings` given by its key.
///
class VGC_UI_API SettingEdit : public Flex {
private:
    VGC_OBJECT(SettingEdit, Flex)

protected:
    SettingEdit(CreateKey, SettingPtr setting);

public:
    /// Creates a `SettingEdit`.
    ///
    static SettingEditPtr create(SettingPtr setting);

    /// Returns the `Setting` object this `SettingEdit` is synced to.
    ///
    Setting* setting() const {
        return setting_.get();
    }

    /// Returns the `Label` widget of this `NumberSettingEdit`.
    ///
    Label* label() const {
        return label_.get();
    };

private:
    SettingPtr setting_;
    LabelPtr label_;
};

} // namespace vgc::ui

#endif // VGC_UI_SETTINGEDIT_H
