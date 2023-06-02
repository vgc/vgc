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

/// Defines a command and adds it to the `CommandRegistry`.
///
/// ```cpp
/// VGC_UI_DEFINE_COMMAND(
///     save,
///     "file.save"
///     vgc::ui::CommandType::Trigger,
///     "Save",
///     vgc::ui::ShortcutContext::Window,
///     vgc::ui::Shortcut(vgc::ui::ModifierKey::Ctrl, vgc::ui::Key::S))
/// ```
///
#define VGC_UI_DEFINE_COMMAND(variableName, id, type, name, shortcutContext, shortcut)   \
    static const ::vgc::core::StringId variableName(id);                                 \
    static const ::vgc::ui::detail::CommandRegistrer variableName##_detail(              \
        ::vgc::ui::Command(id, type, name, shortcutContext, shortcut));

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
        std::string_view name,
        ShortcutContext shortcutContext,
        Shortcut shortcut)

        : id_(id)
        , type_(type)
        , name_(name)
        , shortcutContext_(shortcutContext)
        , shortcut_(shortcut) {
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

    /// Returns the shortcut context of the command.
    ///
    /// This describes whether the `shortcut()` is active application-wide, or
    /// only when the action is in the active window, or only when the action
    /// is owned by a widget that has the keyboard focus.
    ///
    ShortcutContext shortcutContext() const {
        return shortcutContext_;
    }

    /// Returns the shortcut associated with the command. This can be an empty
    /// shortcut if this command has no associated shortcut.
    ///
    // TODO => the shortcut should be stored in a separate ShortcutMap, mapping
    // Commands with shortcuts, since this is not a static properties of actions
    // but a dynamic one (they can be changed at runtime).
    //
    // XXX Should shortcut context also be dynamic, and possibly each shortcut
    // have its own shortcut context (e.g.: "R" as WidgetContext, "Ctrl+Shift+R"
    // as WindowContext?)
    //
    const Shortcut& shortcut() const {
        return shortcut_;
    }

private:
    core::StringId id_;
    ui::CommandType type_;
    core::StringId name_;
    ShortcutContext shortcutContext_;
    Shortcut shortcut_;
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
    static void add(Command command);

private:
    std::unordered_map<core::StringId, Command> commands_;
    Command unknownCommand_;
    std::mutex mutex_;
};

namespace detail {

struct CommandRegistrer {
    CommandRegistrer(Command actionProperties) {
        CommandRegistry::add(actionProperties);
    }
};

} // namespace detail

} // namespace vgc::ui

#endif // VGC_UI_COMMAND_H
