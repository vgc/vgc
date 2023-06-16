// Copyright 2022 The VGC Developers
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

#include <vgc/ui/shortcut.h>

#include <unordered_set>

#include <vgc/core/os.h>
#include <vgc/ui/command.h>
#include <vgc/ui/logcategories.h>

namespace vgc::ui {

VGC_DEFINE_ENUM(
    ShortcutContext,
    (Application, "Application"),
    (Window, "Window"),
    (Widget, "Widget"))

VGC_DEFINE_ENUM( //
    ShortcutType,
    (None, "None"),
    (Keyboard, "Keyboard"),
    (Mouse, "Mouse"))

void Shortcut::setKey(Key key) {
    key_ = key;
    mouseButton_ = MouseButton::None;
    if (key_ == Key::None) {
        type_ = ShortcutType::None;
    }
    else {
        type_ = ShortcutType::Keyboard;
    }
}

void Shortcut::setMouseButton(MouseButton button) {
    mouseButton_ = button;
    key_ = Key::None;
    if (mouseButton_ == MouseButton::None) {
        type_ = ShortcutType::None;
    }
    else {
        type_ = ShortcutType::Mouse;
    }
}

namespace {

void appendString(std::string& res, std::string& separator, std::string_view str) {
    res += separator;
    res += str;
#ifndef VGC_CORE_OS_MACOS
    separator = "+";
#else
    separator = " ";
#endif
}

} // namespace

namespace detail {

std::string toString(const Shortcut& shortcut) {
    if (shortcut.type() == ShortcutType::None) {
        return "";
    }
    ModifierKeys modifierKeys = shortcut.modifierKeys();
    std::string res;
    std::string separator = "";
    // Note:
    // - on Windows, we want the order Ctrl + Alt + Shift + Meta
    // - on macOS, on Windows, we want the order Control + Alt + Shift + Command
#ifndef VGC_CORE_OS_MACOS
    if (modifierKeys.has(ModifierKey::Ctrl)) {
        appendString(res, separator, "Ctrl");
    }
    if (modifierKeys.has(ModifierKey::Alt)) {
        appendString(res, separator, "Alt");
    }
    if (modifierKeys.has(ModifierKey::Shift)) {
        appendString(res, separator, "Shift");
    }
    if (modifierKeys.has(ModifierKey::Meta)) {
        appendString(res, separator, "Meta");
    }
#else
    if (modifierKeys.has(ModifierKey::Meta)) {
        appendString(res, separator, "⌃"); // U+2303 Control / Ctrl
    }
    if (modifierKeys.has(ModifierKey::Alt)) {
        appendString(res, separator, "⌥"); // U+2325 Option / Alt
    }
    if (modifierKeys.has(ModifierKey::Shift)) {
        appendString(res, separator, "⇧"); // U+21E7 Shift
    }
    if (modifierKeys.has(ModifierKey::Ctrl)) {
        appendString(res, separator, "⌘"); // U+2318 Command / Cmd
    }
#endif
    if (shortcut.type() == ShortcutType::Keyboard) {
        appendString(res, separator, core::Enum::prettyName(shortcut.key()));
    }
    else if (shortcut.type() == ShortcutType::Mouse) {
        appendString(res, separator, "Mouse ");
        res += core::Enum::prettyName(shortcut.mouseButton());
    }
    return res;
}

} // namespace detail

ShortcutMap::ShortcutMap(const ShortcutMap* inheritedMap)
    : inheritedMap_(inheritedMap) {

    if (inheritedMap_) {
        inheritedMap_->changed().connect(changed());
    }
}

ShortcutMapPtr ShortcutMap::create(const ShortcutMap* inheritedMap) {
    return ShortcutMapPtr(new ShortcutMap(inheritedMap));
}

bool ShortcutMap::isSet(core::StringId commandId) const {
    auto it = shortcuts_.find(commandId);
    bool found = (it != shortcuts_.end());
    return found;
}

bool ShortcutMap::contains(core::StringId commandId) const {
    if (isSet(commandId)) {
        return true;
    }
    else if (inheritedMap_) {
        return inheritedMap_->contains(commandId);
    }
    else {
        return false;
    }
}

const core::Array<Shortcut>& ShortcutMap::get(core::StringId commandId) const {
    auto it = shortcuts_.find(commandId);
    bool found = (it != shortcuts_.end());
    if (found) {
        return it->second;
    }
    else {
        return noShortcuts_;
    }
}

const core::Array<Shortcut>& ShortcutMap::shortcuts(core::StringId commandId) const {
    auto it = shortcuts_.find(commandId);
    bool found = (it != shortcuts_.end());
    if (found) {
        return it->second;
    }
    else if (inheritedMap_) {
        return inheritedMap_->shortcuts(commandId);
    }
    else {
        return noShortcuts_;
    }
}

void ShortcutMap::set(core::StringId commandId, ShortcutArray shortcuts) {
    auto it = shortcuts_.find(commandId);
    bool found = (it != shortcuts_.end());
    if (found) {
        ShortcutArray& array = it->second;
        if (shortcuts != array) {
            array = std::move(shortcuts);
            changed().emit();
        }
    }
    else {
        shortcuts_[commandId] = std::move(shortcuts);
        changed().emit();
    }
}

void ShortcutMap::restore(core::StringId commandId) {
    size_t numErased = shortcuts_.erase(commandId);
    if (numErased > 0) {
        changed().emit();
    }
}

void ShortcutMap::clear(core::StringId commandId) {
    set(commandId, {});
}

// Note: an alternative signature for this function would be to pass the
// Shortcut by copy instead of const ref and std::move it in the array. We
// chose not to do this because Shortcut is not faster to move than to copy,
// and the Shortcut will not necessarily be inserted anyway.
//
void ShortcutMap::add(core::StringId commandId, const Shortcut& shortcut) {
    auto it = shortcuts_.find(commandId);
    bool found = (it != shortcuts_.end());
    if (found) {
        ShortcutArray& array = it->second;
        if (!array.contains(shortcut)) {
            array.append(shortcut);
            changed().emit();
        }
    }
    else {
        ShortcutArray array = shortcuts(commandId);
        if (!array.contains(shortcut)) {
            array.append(shortcut);
        }
        shortcuts_[commandId] = std::move(array);
        changed().emit();
    }
}

void ShortcutMap::remove(core::StringId commandId, const Shortcut& shortcut) {
    auto it = shortcuts_.find(commandId);
    bool found = (it != shortcuts_.end());
    if (found) {
        ShortcutArray& array = it->second;
        Int numRemoved = array.removeAll(shortcut);
        if (numRemoved > 0) {
            changed().emit();
        }
        // Note: even if array becomes empty, we still keep it in the map
    }
    else {
        ShortcutArray array = shortcuts(commandId);
        array.removeAll(shortcut);
        shortcuts_[commandId] = std::move(array);
        changed().emit();
    }
}

core::Array<core::StringId> ShortcutMap::commands(bool withInheritance) const {

    core::Array<core::StringId> res;

    // In case of inheritance, we need to use a temporary set to
    // ensure O(n log n) complexity instead of O(n²). Otherwise,
    // we can directly transfer the commands IDs to the array.
    //
    if (withInheritance && inheritedMap_) {

        // Compute result as a set
        std::unordered_set<core::StringId> out;
        commands_(out);

        // Convert set to array
        res.reserve(out.size());
        for (const auto& it : out) {
            res.append(it);
        }
    }
    else {
        // Transfer map keys to array
        res.reserve(shortcuts_.size());
        for (const auto& it : shortcuts_) {
            res.append(it.first);
        }
    }

    // Sort alphabetically
    std::sort(res.begin(), res.end(), [](core::StringId s1, core::StringId s2) {
        return s1.string() < s2.string();
    });

    return res;
}

void ShortcutMap::commands_(std::unordered_set<core::StringId>& out) const {
    for (const auto& it : shortcuts_) {
        out.insert(it.first);
    }
    if (inheritedMap_) {
        inheritedMap_->commands_(out);
    }
}

ShortcutMap* defaultShortcuts() {
    static ShortcutMapPtr res = ShortcutMap::create();
    return res.get();
}

const ShortcutArray& defaultShortcuts(core::StringId commandId) {
    return defaultShortcuts()->shortcuts(commandId);
}

ShortcutMap* userShortcuts() {
    static ShortcutMapPtr res = ShortcutMap::create(defaultShortcuts());
    return res.get();
}

const ShortcutArray& userShortcuts(core::StringId commandId) {
    return userShortcuts()->shortcuts(commandId);
}

namespace detail {

ShortcutAdder::ShortcutAdder(
    ShortcutMap* map,
    core::StringId commandId,
    const Shortcut& shortcut) {

    map->add(commandId, shortcut);
}

} // namespace detail

} // namespace vgc::ui
