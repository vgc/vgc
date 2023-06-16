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

#include <unordered_map>
#include <unordered_set>

#include <vgc/core/format.h>
#include <vgc/core/object.h>
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

    /// Creates a Shortcut with no modifier keys and key.
    ///
    Shortcut(Key key) { // intentional implicit constructor
        setKey(key);
    }

    /// Creates a Shortcut with the given modifier keys and key.
    ///
    Shortcut(ModifierKeys modifierKeys, Key key) {
        setModifierKeys(modifierKeys);
        setKey(key);
    }

    /// Creates a Shortcut with no given modifier keys and mouse button.
    ///
    Shortcut(MouseButton button) { // intentional implicit constructor
        setMouseButton(button);
    }

    /// Creates a Shortcut with the given modifier keys and mouse button.
    ///
    Shortcut(ModifierKeys modifierKeys, MouseButton button) {
        setModifierKeys(modifierKeys);
        setMouseButton(button);
    }

    /// Returns the type of this shortcut.
    ///
    ShortcutType type() const {
        return type_;
    }

    /// Returns the modifier keys of this shortcut.
    ///
    ModifierKeys modifierKeys() const {
        return modifierKeys_;
    }

    /// Sets the modifier keys of this shortcut.
    ///
    void setModifierKeys(ModifierKeys modifierKeys) {
        modifierKeys_ = modifierKeys;
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
        return s1.type_ == s2.type_                    //
               && s1.modifierKeys_ == s2.modifierKeys_ //
               && s1.key_ == s2.key_                   //
               && s1.mouseButton_ == s2.mouseButton_;
    }

    /// Returns whether the two shortcuts `s1` and `s2` are different.
    ///
    friend constexpr bool operator!=(const Shortcut& s1, const Shortcut& s2) {
        return !(s1 == s2);
    }

private:
    ShortcutType type_ = ShortcutType::None;
    ModifierKeys modifierKeys_ = ModifierKey::None;
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
        vgc::UInt64 x = s.modifierKeys().toUnderlying();
        x <<= 32;
        x += vgc::core::toUnderlying(s.key());
        return std::hash<vgc::UInt64>()(x);
    }
};

namespace vgc::ui {

VGC_DECLARE_OBJECT(ShortcutMap);

using ShortcutArray = core::Array<Shortcut>;

/// \class vgc::ui::ShortcutMap
/// \brief Defines a mapping ("key bindings") between commands and shortcuts.
///
/// A `ShortcutMap` defines a mapping (often called "key bindings") between
/// `Command` objects and `Shortcut` objects. More precisely, it allows you to
/// query, for any command (given by its command ID), what is the list of
/// shortcuts that can be used to trigger the command.
///
/// A shortcut map `m2` can "inherit" the shortcuts from another shortcut map
/// `m1`. In this case, the shortcuts defined in `m1` are also available in
/// `m2`. If shortcuts for a given command are explicitly defined in `m2`, then
/// they override all the shortcuts defined in `m1`, and none of the shortcuts
/// in `m1` for this command are available.
///
class VGC_UI_API ShortcutMap : public core::Object {
private:
    VGC_OBJECT(ShortcutMap, core::Object)

protected:
    ShortcutMap(const ShortcutMap* inheritedMap);

public:
    /// Creates a `ShortcutMap` object. If `inheritedMap` is not null,
    /// then the created `ShortcutMap` will inherit from this other map.
    ///
    static ShortcutMapPtr create(const ShortcutMap* inheritedMap = nullptr);

    /// Returns the shortcut map that this shortcut map inherits from, if
    /// any.
    ///
    /// For example, the "user" shortcut map typically inherits from the
    /// "default" shortcut map, which means that unless explicitly overriden,
    /// the shortcuts for a given command in the context of this map are the
    /// same as its shortcuts in the context of the other map.
    ///
    const ShortcutMap* inheritedMap() const {
        return inheritedMap_.getIfAlive();
    }

    /// Returns whether this map contains the given command, ignoring
    /// inheritance.
    ///
    /// If this function returns `true`, you can use `get(commandId)` to
    /// get the corresponding shortcuts.
    ///
    /// If you want inheritance to be taken into account, use
    /// `contains(commandId)` instead.
    ///
    /// If `isSet(commandId)` returns `true`, then `contains(commandId)` also
    /// returns `true`.
    ///
    /// If `isSet(commandId)` returns `false`, then `contains(commandId)` may
    /// return either `false` or `true`, depending on the content of the
    /// inherited map, if any.
    ///
    /// \sa `get()`, `set()`, `contains()`.
    ///
    bool isSet(core::StringId commandId) const;

    /// Returns whether this map contains the given command, taking into
    /// account inheritance.
    ///
    /// If this function returns `true`, you can use `shortcut(commandId)` to
    /// get the corresponding shortcuts.
    ///
    /// If you want inheritance to be ignored, use `isSet(commandId)` instead.
    ///
    /// \sa `shortcuts()`, `isSet()`.
    ///
    bool contains(core::StringId commandId) const;

    /// Returns all the shortcuts bound to a given command, ignoring
    /// inheritance.
    ///
    /// If you want inheritance to be taken into account, use
    /// `shortcuts(commandId)` instead.
    ///
    /// \sa `set()`, `isSet()`, `shortcuts()`.
    ///
    const ShortcutArray& get(core::StringId commandId) const;

    /// Returns all the shortcuts bound to a given command, taking into account
    /// inheritance.
    ///
    /// If you want inheritance to be ignored, use `get(commandId)` instead.
    ///
    /// Note that if `isSet(commandId)` is `true`, then any shortcut defined in
    /// the inherited map are ignored: they are overriden by the shortcuts set
    /// in this map.
    ///
    /// Also note that if this function returns an empty array, this can either
    /// means that the command is not contained in the map, or that the command
    /// is explicitly mapped to "no shortcuts".
    ///
    /// \sa `get()`.
    ///
    const ShortcutArray& shortcuts(core::StringId commandId) const;

    /// Sets the shortcuts of the given command to be the given array of
    /// shortcuts.
    ///
    /// If this map inherits from another map, this means that all shortcuts
    /// assigned to the command in the other map are now inactive in the
    /// context of this map.
    ///
    /// After calling this function, `isSet(commandId)` returns `true` and both
    /// `get(commandId)` and `shortcuts(commandId)` return the given shortcuts.
    ///
    /// \sa `get()`, `isSet()`, `restore()`, `clear()`, `restore()`, `add()`, `remove()`.
    ///
    void set(core::StringId commandId, ShortcutArray shortcuts);

    /// This signal is emitted whenever some shortcuts have changed, whenever
    /// directly via `this->set()`, or indirectly via inheritance.
    ///
    VGC_SIGNAL(changed)

    /// Removes any previously `set()` shortcut on this map for the given
    /// command.
    ///
    /// If this map inherits from another map, this means that all shortcuts of
    /// the command in the context of this map are now the same than in the
    /// context of the other map.
    ///
    /// After calling this function, `isSet(commandId)` returns `false`,
    /// `get(commandId)` returns returns an empty array, and
    /// `shortcuts(commandId)` may or may not return an empty array depending
    /// on the content of the inherited map, if any.
    ///
    /// \sa `set()`.
    ///
    void restore(core::StringId commandId);

    /// Sets the shortcuts of the given command to be an empty array.
    ///
    /// This is equivalent to `set(commandId, {})`.
    ///
    /// If this map inherits from another map, this means that all shortcuts
    /// assigned to the command in the other map are now inactive in the
    /// context of this map.
    ///
    /// After calling this function, `isSet(commandId)` returns `true` and both
    /// `get(commandId)` and `shortcuts(commandId)` return an empty array.
    ///
    /// \sa `set()`.
    ///
    void clear(core::StringId commandId);

    /// Adds a shortcut for the given command.
    ///
    /// If this map inherits from another map, and the given command is not yet
    /// set in this map, then all shortcuts already bound to the command by
    /// inheritance are first copied into this map so that they are still
    /// active in the context of this map.
    ///
    /// After calling this function, `isSet(commandId)` returns `true` and
    /// `shortcuts(commandId)` returns the same array as it previously returned
    /// but with the given shortcut added.
    ///
    /// If the shortcut was already in `shortcuts(commandId)` then it is not
    /// added a second time (i.e., `shortcuts(commandId)` is unchanged),
    /// however this function may still have the side effect of changing
    /// `isSet(commandId)` from `false` to `true`
    ///
    /// \sa `set()`.
    ///
    void add(core::StringId commandId, const Shortcut& shortcut);

    /// Removes a shortcut for the given command.
    ///
    /// If this map inherits from another map, and the given command is not yet
    /// set in this map, then all shortcuts already bound to the command by
    /// inheritance are first copied into this map so that they are still
    /// active in the context of this map (except for the removed shortcut).
    ///
    /// After calling this function, `isSet(commandId)` return `true` and
    /// `shortcuts(commandId)` returns the same array as it previously returned
    /// but with the given shortcut removed.
    ///
    /// If the shortcut was not in `shortcuts(commandId)` then it is not
    /// removed (i.e., `shortcuts(commandId)` is unchanged), however this
    /// function may still have the side effect of changing `isSet(commandId)`
    /// from `false` to `true`
    ///
    /// \sa `set()`.
    ///
    void remove(core::StringId commandId, const Shortcut& shortcut);

    /// Returns the ID of all commands in this map.
    ///
    /// If `withInheritance` is `true` (the default), then inherited
    /// commands are also included.
    ///
    /// If `withInheritance` is `false`, then inherited
    /// commands are not included.
    ///
    core::Array<core::StringId> commands(bool withInheritance = true) const;

    // TODO:
    // searchByCommand(std::string_view pattern, bool withInheritance = true)
    // searchByShortcut(std::string_view pattern, bool withInheritance = true)

private:
    ShortcutMapConstPtr inheritedMap_;
    std::unordered_map<core::StringId, ShortcutArray> shortcuts_;
    ShortcutArray noShortcuts_;

    // Same as commands(true) but appends the results to a set for performance
    void commands_(std::unordered_set<core::StringId>& out) const;
};

/// Returns a global `ShortcutMap` object storing the default shortcuts.
///
/// \relates `ShortcutMap`
/// \relates `Shortcut`
///
VGC_UI_API
ShortcutMap* defaultShortcuts();

/// Returns all the default shortcuts bound to a given command.
///
/// This is equivalent to `defaultShortcuts()->shortcuts(commandId)`.
///
/// \relates `ShortcutMap`
/// \relates `Shortcut`
///
VGC_UI_API
const ShortcutArray& defaultShortcuts(core::StringId commandId);

/// Returns a global `ShortcutMap` object storing the user shortcuts.
///
/// \relates `ShortcutMap`
/// \relates `Shortcut`
///
VGC_UI_API
ShortcutMap* userShortcuts();

/// Returns all the user shortcuts bound to a given command.
///
/// This is equivalent to `userShortcuts()->shortcuts(commandId)`.
///
/// \relates `ShortcutMap`
/// \relates `Shortcut`
///
VGC_UI_API
const ShortcutArray& userShortcuts(core::StringId commandId);

class Command;

namespace detail {

struct VGC_UI_API ShortcutAdder {
    ShortcutAdder(ShortcutMap* map, core::StringId commandId, const Shortcut& shortcut);
};

} // namespace detail

} // namespace vgc::ui

/// Adds a shortcut to the `defaultShortcuts()` for the given command.
///
/// ```cpp
/// VGC_UI_ADD_DEFAULT_SHORTCUT(
///     save,
///     (vgc::ui::Key::S))
/// ```
///
/// ```cpp
/// VGC_UI_ADD_DEFAULT_SHORTCUT(
///     save,
///     (vgc::ui::ModifierKey::Ctrl, vgc::ui::Key::S))
/// ```
///
/// This can only be user in .cpp source files, not in header files.
///
/// Note that due to macro limitations for doing static initialization, the
/// first argument must be an unqualified variable identifier (that is, with no
/// explicit namespaces mentionnedprepended) of type `Command` or `const
/// Command&`. The macro creates a new variable name based on this name.
///
#define VGC_UI_ADD_DEFAULT_SHORTCUT(commandVariable, shortcut)                           \
    static const ::vgc::ui::detail::ShortcutAdder commandVariable##_shortcut_##__LINE__( \
        ::vgc::ui::defaultShortcuts(), commandVariable, shortcut);

#endif // VGC_UI_SHORTCUT_H
