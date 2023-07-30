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

#ifndef VGC_UI_SETTING_H
#define VGC_UI_SETTING_H

#include <vgc/core/object.h>
#include <vgc/ui/api.h>
#include <vgc/ui/settings.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(Setting);
VGC_DECLARE_OBJECT(Settings);

/// \class vgc::ui::Setting
/// \brief Represents an individual setting in a given `Settings`.
///
class VGC_UI_API Setting : public core::Object {
private:
    VGC_OBJECT(Setting, core::Object)

protected:
    Setting(CreateKey, Settings* settings, std::string_view key, std::string_view label);

public:
    /// Creates a `Setting`.
    ///
    static SettingPtr
    create(Settings* settings, std::string_view key, std::string_view label);

    /// Returns the `Settings` object this `SettingEdit` is synced to.
    ///
    Settings* settings() const {
        return settings_.get();
    }

    /// Returns the key corresponding to the value that his this `SettingEdit`
    /// controls.
    ///
    std::string_view key() const {
        return key_;
    }

    /// Returns the label of the setting, that is, a short human-readable
    /// description for display purposes in the UI.
    ///
    std::string_view label() const {
        return label_;
    }

private:
    SettingsPtr settings_;
    std::string key_;
    std::string label_;
};

} // namespace vgc::ui

#endif // VGC_UI_SETTING_H
