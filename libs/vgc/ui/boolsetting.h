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

#ifndef VGC_UI_BOOLSETTING_H
#define VGC_UI_BOOLSETTING_H

#include <vgc/ui/action.h>
#include <vgc/ui/api.h>
#include <vgc/ui/setting.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(BoolSetting);

/// \class vgc::ui::BoolSetting
/// \brief A `Setting` subclass for boolean values.
///
class VGC_UI_API BoolSetting : public Setting {
private:
    VGC_OBJECT(BoolSetting, Setting)

protected:
    BoolSetting(
        CreateKey,
        Settings* settings,
        std::string_view key,
        std::string_view label,
        bool defaultValue);

public:
    /// Creates a `BoolSetting`.
    ///
    static BoolSettingPtr create(
        Settings* settings,
        std::string_view key,
        std::string_view label,
        bool defaultValue = false);

    /// Returns the default value of this `BoolSetting`.
    ///
    bool defaultValue() const;

    /// Returns the current value of this `BoolSetting`.
    ///
    /// \sa `setValue()`.
    ///
    bool value() const;

    /// Enables explicit conversion to bool.
    ///
    /// This returns the same as `value().
    ///
    explicit operator bool() const {
        return value();
    }

    /// Sets the value of this `BoolSetting`.
    ///
    /// \sa `value()`.
    ///
    void setValue(bool value);
    VGC_SLOT(setValue)

    /// This signal is emitted whenever `value()` changes.
    ///
    VGC_SIGNAL(valueChanged, (bool, value))

    /// Synchronizes this `BoolSetting` with the checked state of an action.
    ///
    /// \sa `unsynchronizeWith()`.
    ///
    // XXX: Should checkable commands automatically (or optionally) have an
    // associated synchronized bool setting, so that there is no need for the
    // developer to define both a command and a setting?
    //
    void synchronizeWith(Action& action);

    /// Stops the synchronization between this `BoolSetting` and the checked
    /// state of an action.
    ///
    /// \sa `synchronizeWith()`.
    ///
    void unsynchronizeWith(Action& action);

private:
    bool defaultValue_ = false;
};

} // namespace vgc::ui

#endif // VGC_UI_BOOLSETTING_H
