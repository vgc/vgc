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

#ifndef VGC_UI_SETTINGS_H
#define VGC_UI_SETTINGS_H

#include <map>
#include <variant>

#include <vgc/core/object.h>
#include <vgc/ui/api.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(Settings);

namespace detail {

// Types to use to store settings values and key-value map.
//
// Note: we use an ordered map to write keys in order. In the future, we may
// may want to use a combination of an Array (to choose the order / preserve
// the one in the existing settings file) together with an unordered_map (for
// fast read access to values). Also, we would like to support comments and
// preserve existing indentation / line breaks in the file.
//
using SettingsValue = std::variant<bool, double, std::string>;
using SettingsMap = std::map<std::string, SettingsValue>;

} // namespace detail

namespace settings {

/// Returns a global `Settings` object that has a `filePath()` pre-configured
/// to a location where it is suitable to read/write user preferences based on
/// the application name.
///
/// This cannot be called before an instance of `Application` has been created.
///
/// \sa `Application::applicationName()`.
///
VGC_UI_API
Settings* preferences();

/// Returns a global `Settings` object that has a `filePath()` pre-configured
/// to a location where it is suitable to read/write session settings (e.g.,
/// window size, opened panels, tools state, etc.) based on the application
/// name.
///
/// This cannot be called before an instance of `Application` has been created.
///
/// \sa `Application::applicationName()`.
///
VGC_UI_API
Settings* session();

} // namespace settings

/// \class vgc::ui::Settings
/// \brief Get and set user settings.
///
/// This class provides a mechanism to store user preferences, session state,
/// or other settings, by writing them to a file in the JSON format.
///
/// Convenient global `Settings` objects are available for typical use cases:
/// -`settings::preferences()`: for storing user preferences.
/// -`settings::session()`: for storing session state.
///
/// These global `Settings` objects use a `filePath()` pre-configured to a
/// standard location based on the application name.
///
class VGC_UI_API Settings : public core::Object {
private:
    VGC_OBJECT(Settings, core::Object)

protected:
    Settings(CreateKey, std::string_view filePath);

public:
    /// Creates a `Settings` object.
    ///
    /// If `filePath` is not empty and refers to an existing file, then the
    /// settings are initialized from the values in this file, and will be
    /// saved to this files when calling `writeToFile()` or destructing the
    /// `Settings` object.
    ///
    static SettingsPtr create(std::string_view filePath = "");

    /// Clears all the settings value.
    ///
    /// Note that this does not change the current `filePath()`, if any, so
    /// calling this function followed by `writeToFile()` would write an empty
    /// settings file, erasing previous settings.
    ///
    void clear();

    /// Returns the file path these settings will be saved to
    /// when calling `save()` or destructing this `Settings` object.
    ///
    std::string_view filePath() const {
        return filePath_;
    }

    /// Reads the settings from the file at `filePath()`, if any, overriding
    /// current settings.
    ///
    /// Any setting already in this `Settings` object but not in the given file
    /// is kept untouched. You can call `clear()` before calling this function
    /// if you want this `Settings` object to be re-initialized from the
    /// content of the file.
    ///
    /// A warning is emitted if `filePath()` is not empty and does not refer
    /// to a readable file.
    ///
    void readFromFile();

    /// Writes the current settings to the file at `filePath()`, if any.
    ///
    /// A warning is emitted if `filePath` is not empty and does not refer
    /// to a writable file.
    ///
    void writeToFile() const;

    /// Sets the `file path` that
    /// Returns whether the settings contains the given key.
    ///
    bool contains(std::string_view key);

    /// Returns the settings at the given `key`, as a boolean.
    ///
    /// Returns the given `fallback` if there is no value for the given `key`,
    /// or if the value isn't of type boolean.
    ///
    bool getBoolValue(std::string_view key, bool fallback = false) const;

    /// Assigns the given boolean `value` to the given `key`.
    ///
    void setBoolValue(std::string_view key, bool value);

    /// Returns the settings at the given `key`, as a boolean.
    ///
    /// If there is no current value for the given `key`, then this function
    /// sets its value to `fallback` and returns it.
    ///
    /// If there is a preexisting value for the given `key`, but this value is
    /// not a boolean, then this function returns `fallback` but does not
    /// overwrite the preexisting value.
    ///
    bool getOrSetBoolValue(std::string_view key, bool fallback);

    /// Returns the settings at the given `key`, as a double-precision floating point.
    ///
    /// Returns the given `fallback` if there is no value for the given `key`,
    /// or if the value isn't of type number.
    ///
    double getDoubleValue(std::string_view key, double fallback = 0) const;

    /// Assigns the given double-precision floating point `value` to the given `key`.
    ///
    void setDoubleValue(std::string_view key, double value);

    /// Returns the settings at the given `key`, as a double-precision floating point.
    ///
    /// If there is no current value for the given `key`, then this function
    /// sets its value to `fallback` and returns it.
    ///
    /// If there is a preexisting value for the given `key`, but this value is
    /// not a number, then this function returns `fallback` but does not
    /// overwrite the preexisting value.
    ///
    double getOrSetDoubleValue(std::string_view key, double fallback);

    /// Returns the settings at the given `key`, as a single-precision floating point.
    ///
    /// Returns the given `fallback` if there is no value for the given `key`,
    /// or if the value isn't of type number.
    ///
    float getFloatValue(std::string_view key, float fallback = 0) const;

    /// Assigns the given single-precision floating point `value` to the given `key`.
    ///
    void setFloatValue(std::string_view key, float value);

    /// Returns the settings at the given `key`, as a single-precision floating point.
    ///
    /// If there is no current value for the given `key`, then this function
    /// sets its value to `fallback` and returns it.
    ///
    /// If there is a preexisting value for the given `key`, but this value is
    /// not a number, then this function returns `fallback` but does not
    /// overwrite the preexisting value.
    ///
    float getOrSetFloatValue(std::string_view key, float fallback);

    /// Returns the settings at the given `key`, as an `Int`.
    ///
    /// If the stored number is not an integer, it is rounded to the closest integer.
    ///
    /// Returns the given `fallback` if there is no value for the given `key`,
    /// or if the value isn't of type number.
    ///
    Int getIntValue(std::string_view key, Int fallback = 0) const;

    /// Assigns the given integer `value` to the given `key`.
    ///
    void setIntValue(std::string_view key, Int value);

    /// Returns the settings at the given `key`, as an `Int`.
    ///
    /// If there is no current value for the given `key`, then this function
    /// sets its value to `fallback` and returns it.
    ///
    /// If there is a preexisting value for the given `key`, but this value is
    /// not a number, then this function returns `fallback` but does not
    /// overwrite the preexisting value.
    ///
    Int getOrSetIntValue(std::string_view key, Int fallback);

    /// Returns the settings at the given `key`, as a string.
    ///
    /// Returns the given `fallback` if there is no value for the given `key`,
    /// or if the value isn't of type string.
    ///
    std::string
    getStringValue(std::string_view key, std::string_view fallback = "") const;

    /// Assigns the given string `value` to the given `key`.
    ///
    void setStringValue(std::string_view key, std::string_view value);

    /// Returns the settings at the given `key`, as a string.
    ///
    /// If there is no preexisting value for the given `key`, then this
    /// function sets its value to `fallback` and returns it.
    ///
    /// If there is a preexisting value for the given `key`, but this value is
    /// not a string, then this function returns `fallback` but does not
    /// overwrite the preexisting value.
    ///
    std::string getOrSetStringValue(std::string_view key, std::string_view fallback);

protected:
    // reimpl
    void onDestroyed() override;

private:
    detail::SettingsMap map_;
    std::string filePath_;
};

} // namespace vgc::ui

#endif // VGC_UI_SETTINGS_H
