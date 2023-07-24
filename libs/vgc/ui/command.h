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

#ifndef VGC_UI_COMMAND_H
#define VGC_UI_COMMAND_H

#include <mutex>
#include <unordered_map>

#include <vgc/core/stringid.h>
#include <vgc/ui/api.h>
#include <vgc/ui/shortcut.h>

namespace vgc::ui {

/// \enum vgc::ui::CommandType
/// \brief Whether a command is a one-shot trigger or a mouse click/drag.
///
// XXX Add `Menu` type? There is currently `isMenu_` as `Action` data member.
//
enum class CommandType {

    /// Represents a command whose type is unknown, for example because it hasn't
    /// been properly registered.
    ///
    Unknown,

    /// Represents a single-step command that can be typically be triggered
    /// from anywhere and does not requires knowledge of the mouse cursor.
    ///
    Trigger,

    /// Represents a single-step command that requires the mouse cursor, and is
    /// typically performed either on mouse press or mouse release. However, it
    /// can also be initiated via a keyboard shortcut if preferred.
    ///
    MouseClick,

    /// Represents a multiple-steps command that requires the mouse cursor and
    /// typically performs something on mouse press, mouse move, and mouse
    /// release. However, it can also be initiated/completed via keyboard
    /// shortcuts if preferred.
    ///
    MouseDrag
};

VGC_UI_API
VGC_DECLARE_ENUM(CommandType);

/// \class vgc::ui::Command
/// \brief An abstract idea of user interaction, typically implemented as an Action.
///
/// A `Command` represents an abstract idea of a command the user can invoke,
/// for example, the "copy" command.
///
/// Such `Command` can be bound to key bindings, for example, `Ctrl + C`.
///
/// Then, subclasses of `Action` are responsible to actually implement what
/// happens when a given `Command` is invoked. In other words, `Action` objects
/// are the handlers of `Command` objects.
///
/// While there is only one instance of the "copy" `Command`, there can exist
/// several `Action` instances all implementing this command, and which one is
/// invoked depends on the context.
///
/// The easiest way to define a new command in C++ is to use the convenient
/// `VGC_UI_DEFINE_COMMAND` macro at namespace scope in a `*.cpp` file, which
/// creates a command, adds it to the `CommandRegistry`, and assign its ID
/// to the given variable name.
///
// TODO: User-facing categories ("Tools" > "Sculpt" > "Sculpt Grab")
//       to organize the shortcut editor
//
// TODO: Short/Long description (for tooltip, status bar, etc.)
//
// TODO: Extract shortcut out of the command definition: the bindings
//       between key/buttons and commands should be an external map
//       that can change dynamically.
//
// XXX: CheckMode should be in properties too?
// XXX: How to make name and categories translatable?
// XXX: Make it an Object and allow it to change while notifying its
//      implementer Action objects? (onCommandChanged())
//
// Possible i18n string keys for action names/categories/Description:
// - tools.sculpt.grab.actionName
// - tools.sculpt.grab.actionShortDescription
// - tools.sculpt.grab.actionLongDescription
// - tools.sculpt.actionCategoryName
// - tools.actionCategoryName
//
class VGC_UI_API Command {
public:
    /// Creates a `Command`.
    ///
    Command(
        std::string_view id,
        ui::CommandType type,
        ShortcutContext shortcutContext,
        std::string_view name,
        std::string_view icon = "")

        : id_(id)
        , type_(type)
        , shortcutContext_(shortcutContext)
        , name_(name)
        , icon_(icon) {
    }

    /// Returns the ID of the command, which is a string that uniquely identifies a
    /// command in the `CommandRegistry`.
    ///
    /// Example: `"tools.sculpt.grab"`.
    ///
    core::StringId id() const {
        return id_;
    }

    /// Returns the type of the command.
    ///
    /// This describes what type of user interaction is expected to perform the
    /// command, for example, a mouse click vs. a mouse drag.
    ///
    ui::CommandType type() const {
        return type_;
    }

    /// Returns the shortcut context of the command.
    ///
    /// This describes whether the `shortcut()` is active application-wide, or
    /// only when the action is in the active window, or only when the action
    /// is owned by a widget that has the keyboard focus.
    ///
    // XXX Should shortcut context be allowed to be defined per-shortcut,
    // instead of per-command? Example: "R" as WidgetContext, "Ctrl+Shift+R" as
    // WindowContext?
    //
    ShortcutContext shortcutContext() const {
        return shortcutContext_;
    }

    /// Returns the name of the command.
    ///
    /// This is a short user-facing string that appears for example in menus,
    /// buttons, or in the shortcut editor.
    ///
    /// Example: `"Sculpt Grab"`.
    ///
    core::StringId name() const {
        return name_;
    }

    /// Returns the icon URL of the command.
    ///
    /// Example: `"tools/icons/select.svg"`.
    ///
    core::StringId icon() const {
        return icon_;
    }

private:
    core::StringId id_;
    ui::CommandType type_;
    ShortcutContext shortcutContext_;
    core::StringId name_;
    core::StringId icon_;
};

} // namespace vgc::ui

namespace vgc::ui {

/// \class vgc::ui::CommandRegistry
/// \brief Stores all registered `Command` objects in the application.
///
///
class VGC_UI_API CommandRegistry {
private:
    CommandRegistry();
    static CommandRegistry* instance_();

public:
    /// Returns the `Command` corresponding to the given command ID.
    ///
    /// If there is no command with the given ID in the registry, then this
    /// returns a `Command` whose type is `ActionType::Unknown`.
    ///
    /// \sa `isRegistered()`.
    ///
    static const Command& find(core::StringId id);

    /// Returns whether a `Command` with the given ID exists in the registry.
    ///
    static bool contains(core::StringId id);

    /// Adds a `Command` to the registry. If there is already a command with
    /// the same ID in the registry, then the given command replaces the
    /// pre-existing command.
    ///
    static void add(const Command& command);

private:
    std::unordered_map<core::StringId, Command> commands_;
    Command unknownCommand_;
    std::mutex mutex_;
};

namespace detail {

struct CommandRegistrer {
    CommandRegistrer(const Command& command) {
        CommandRegistry::add(command);
    }
};

} // namespace detail

} // namespace vgc::ui

#define VGC_UI_DEFINE_COMMAND_BASE(variableName, id, ...)                                \
    static const ::vgc::core::StringId variableName(id);                                 \
    static const ::vgc::ui::detail::CommandRegistrer variableName##_detail(              \
        ::vgc::ui::Command(id, __VA_ARGS__));

#define VGC_UI_DEFINE_COMMAND_5(variableName, id, type, shortcutContext, name)           \
    VGC_UI_DEFINE_COMMAND_BASE(variableName, id, type, shortcutContext, name)

#define VGC_UI_DEFINE_COMMAND_6(variableName, id, type, shortcutContext, name, shortcut) \
    VGC_UI_DEFINE_COMMAND_BASE(variableName, id, type, shortcutContext, name)            \
    VGC_UI_ADD_DEFAULT_SHORTCUT(variableName, shortcut)

#define VGC_UI_DEFINE_COMMAND_7(vName, id, type, shortcutContext, name, shortcut, icon)  \
    VGC_UI_DEFINE_COMMAND_BASE(vName, id, type, shortcutContext, name, icon)             \
    VGC_UI_ADD_DEFAULT_SHORTCUT(vName, shortcut)

/// Defines a command and adds it to the `CommandRegistry`.
///
/// ```cpp
/// VGC_UI_DEFINE_COMMAND(
///     save,
///     "file.save"
///     CommandType::Trigger,
///     ShortcutContext::Window,
///     "Save",
///     )
/// ```
///
/// Optionally, it is also possible to add a default shortcut associated with the command:
///
/// ```cpp
/// VGC_UI_DEFINE_COMMAND(
///     save,
///     "file.save"
///     CommandType::Trigger,
///     ShortcutContext::Window,
///     "Save",
///     Shortcut(ModifierKey::Ctrl, Key::S)
/// )
/// ```
///
#define VGC_UI_DEFINE_COMMAND(...)                                                       \
    VGC_PP_EXPAND(VGC_PP_OVERLOAD(VGC_UI_DEFINE_COMMAND_, __VA_ARGS__)(__VA_ARGS__))

/// An overload of `VGC_UI_DEFINE_COMMAND()` that creates a command of type
/// `Trigger` and shortcut context `Widget`.
///
/// ```cpp
/// VGC_UI_DEFINE_TRIGGER_COMMAND(
///     openSubMenu,
///     "ui.menu.openVerticalSubMenu",
///     "Open Vertical Sub Menu"
///     Key::Right)
/// ```
///
#define VGC_UI_DEFINE_TRIGGER_COMMAND(var, id, ...)                                      \
    VGC_UI_DEFINE_COMMAND(                                                               \
        var, id, ui::CommandType::Trigger, ui::ShortcutContext::Widget, __VA_ARGS__)

/// An overload of `VGC_UI_DEFINE_COMMAND()` that creates a command of type
/// `MouseDrag` and shortcut context `Widget`.
///
/// ```cpp
/// VGC_UI_DEFINE_MOUSE_DRAG_COMMAND(
///     grab,
///     "tools.sculpt.grab",
///     "Sculpt Grab",
///     MouseButton::Left)
/// ```
///
#define VGC_UI_DEFINE_MOUSE_DRAG_COMMAND(var, id, ...)                                   \
    VGC_UI_DEFINE_COMMAND(                                                               \
        var, id, ui::CommandType::MouseDrag, ui::ShortcutContext::Widget, __VA_ARGS__)

/// An overload of `VGC_UI_DEFINE_COMMAND()` that creates a command of type
/// `MouseClick` and shortcut context `Widget`.
///
/// ```cpp
/// VGC_UI_DEFINE_MOUSE_CLICK_COMMAND(
///     cutEdge,
///     "tools.topology.cutEdgeAtNewVertex",
///     "Cut Edge at New Vertex",
///     MouseButton::Left)
/// ```
///
#define VGC_UI_DEFINE_MOUSE_CLICK_COMMAND(var, id, ...)                                  \
    VGC_UI_DEFINE_COMMAND(                                                               \
        var, id, ui::CommandType::MouseClick, ui::ShortcutContext::Widget, __VA_ARGS__)

/// An overload of `VGC_UI_DEFINE_COMMAND()` that creates a command of type
/// `Trigger` and shortcut context `Window`.
///
/// ```cpp
/// VGC_UI_DEFINE_WINDOW_COMMAND(
///     undo,
///     "edit.undo",
///     "Undo",
///     Shortcut(ModifierKey::Ctrl, Key::Z))
/// ```
///
#define VGC_UI_DEFINE_WINDOW_COMMAND(var, id, ...)                                       \
    VGC_UI_DEFINE_COMMAND(                                                               \
        var, id, ui::CommandType::Trigger, ui::ShortcutContext::Window, __VA_ARGS__)

/// An overload of `VGC_UI_DEFINE_COMMAND()` that creates a command of type
/// `Trigger` and shortcut context `Application`.
///
#define VGC_UI_DEFINE_APPLICATION_COMMAND(var, id, ...)                                  \
    VGC_UI_DEFINE_COMMAND(                                                               \
        var,                                                                             \
        id,                                                                              \
        ui::CommandType::Trigger,                                                        \
        ui::ShortcutContext::Application,                                                \
        __VA_ARGS__)

#endif // VGC_UI_COMMAND_H
