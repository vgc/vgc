// Copyright 2021 The VGC Developers
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

#ifndef VGC_UI_SHORTCUT_H
#define VGC_UI_SHORTCUT_H

#include <vgc/core/format.h>
#include <vgc/ui/api.h>
#include <vgc/ui/key.h>
#include <vgc/ui/modifierkey.h>
#include <vgc/ui/mousebutton.h>

namespace vgc::ui {

/// \enum vgc::ui::ShortcutContext
/// \brief Describes in what context a shortcut is active.
///
/// This describes whether a shortcut is active application-wide, or only when
/// the action is in the active window, or only when the action is owned by a
/// widget that has the keyboard focus.
///
enum class ShortcutContext : UInt8 {

    /// The shortcut is active application-wide.
    ///
    Application,

    /// The shortcut is active if the action is owned by a widget inside the
    /// active window.
    ///
    Window,

    /// The shortcut is active if the action is owned by a widget which has the
    /// keyboard focus.
    ///
    Widget
};

VGC_UI_API
VGC_DECLARE_ENUM(ShortcutContext)

/// \enum vgc::ui::ShortcutType
/// \brief Describes whether a shortcut is a mouse button press, a keyboard key press, etc.
///
// TODO:
// - DoubleClick?
// - Activate on Press vs Release?
// - Should the key/button be kept pressed during a drag action?
//
enum class ShortcutType : UInt8 {

    /// There is no shortcut.
    ///
    None,

    /// The shortcut is activated by pressing a keyboard key.
    ///
    Keyboard,

    /// The shortcut is activated by pressing a mouse button.
    ///
    Mouse
};

VGC_UI_API
VGC_DECLARE_ENUM(ShortcutType)

/// \class vgc::ui::Shortcut
/// \brief Represents a combination of keys that can trigger an action.
///
/// A `Shortcut` is a combination of ModifiersKeys together with a `Key` or
/// `MouseButton`.
///
class VGC_UI_API Shortcut {
public:
    /// Creates a shortcut of type `ShortcutType::None`.
    ///
    Shortcut() {
    }

    /// Creates a Shortcut with the given modifier keys and key.
    ///
    Shortcut(ModifierKeys modifiers, Key key) {
        setModifiers(modifiers);
        setKey(key);
    }

    /// Creates a Shortcut with the given modifier keys and mouse button.
    ///
    Shortcut(ModifierKeys modifiers, MouseButton button) {
        setModifiers(modifiers);
        setMouseButton(button);
    }

    /// Returns the type of this shortcut.
    ///
    ShortcutType type() const {
        return type_;
    }

    /// Returns the modifier keys of this shortcut.
    ///
    ModifierKeys modifiers() const {
        return modifiers_;
    }

    /// Sets the modifier keys of this shortcut.
    ///
    void setModifiers(ModifierKeys modifiers) {
        modifiers_ = modifiers;
    }

    /// Returns the key of this shortcut.
    ///
    Key key() const {
        return key_;
    }

    /// Sets the key of this shortcut.
    ///
    /// This changes the `type()` of this shortcut to `Keyboard` (unless the
    /// given `key` is `None`, in which case the `type()` becomes `None`).
    ///
    /// This also changes the `mouseButton()` to `None`.
    ///
    void setKey(Key key);

    /// Returns the mouse button of this shortcut.
    ///
    MouseButton mouseButton() const {
        return mouseButton_;
    }

    /// Sets the mouse button of this shortcut.
    ///
    /// This changes the `type()` of this shortcut to `Mouse` (unless the
    /// given `button` is `None`, in which case the `type()` becomes `None`).
    ///
    /// This also changes the `key()` to `None`.
    ///
    void setMouseButton(MouseButton button);

    /// Returns whether the shortcut is empty, that is, whether both `key()`
    /// and `mouseButton()` are `None`.
    ///
    bool isEmpty() const {
        return key() == Key::None && mouseButton() == MouseButton::None;
    }

    /// Returns whether the two shortcuts `s1` and `s2` are equal.
    ///
    friend constexpr bool operator==(const Shortcut& s1, const Shortcut& s2) {
        return s1.modifiers_ == s2.modifiers_ //
               && s1.key_ == s2.key_;
    }

    /// Returns whether the two shortcuts `s1` and `s2` are different.
    ///
    friend constexpr bool operator!=(const Shortcut& s1, const Shortcut& s2) {
        return !(s1 == s2);
    }

private:
    ShortcutType type_ = ShortcutType::None;
    ModifierKeys modifiers_ = ModifierKey::None;
    Key key_ = Key::None;
    MouseButton mouseButton_ = MouseButton::None;
};

namespace detail {

std::string toString(const Shortcut& shortcut);

} // namespace detail

} // namespace vgc::ui

template<>
struct fmt::formatter<vgc::ui::Shortcut> : fmt::formatter<std::string_view> {
    template<typename FormatContext>
    auto format(const vgc::ui::Shortcut& shortcut, FormatContext& ctx) {
        std::string res = vgc::ui::detail::toString(shortcut);
        return fmt::formatter<std::string_view>::format(res, ctx);
    }
};

template<>
struct std::hash<vgc::ui::Shortcut> {
    std::size_t operator()(const vgc::ui::Shortcut& s) const noexcept {
        vgc::UInt64 x = s.modifiers().toUnderlying();
        x <<= 32;
        x += vgc::core::toUnderlying(s.key());
        return std::hash<vgc::UInt64>()(x);
    }
};

#endif // VGC_UI_SHORTCUT_H
