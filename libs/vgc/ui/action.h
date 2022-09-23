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

#ifndef VGC_UI_ACTION_H
#define VGC_UI_ACTION_H

#include <string>
#include <string_view>

#include <vgc/core/innercore.h>
#include <vgc/geometry/vec2f.h>
#include <vgc/ui/api.h>
#include <vgc/ui/shortcut.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(Action);
VGC_DECLARE_OBJECT(Widget);
VGC_DECLARE_OBJECT(Menu);

/// \class vgc::ui::Action
/// \brief Represents an action that can be triggered via menu items, shorctuts, etc.
///
class VGC_UI_API Action : public core::Object {
private:
    VGC_OBJECT(Action, core::Object)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS
    friend Menu;

protected:
    Action();
    explicit Action(const Shortcut& shortcut);
    explicit Action(const std::string& text);
    Action(const std::string& text, const Shortcut& shortcut);

public:
    /// Creates an action.
    ///
    static ActionPtr create();

    /// Creates an action with the given shortcut.
    ///
    static ActionPtr create(const Shortcut& shortcut);

    /// Creates an action with the given text.
    ///
    static ActionPtr create(const std::string& text);

    /// Creates an action with the given text and shortcut.
    ///
    static ActionPtr create(const std::string& text, const Shortcut& shortcut);

    /// This signal is emitted whenever the action properties are changed (shortcut,
    /// text, icon, ...).
    ///
    VGC_SIGNAL(changed, (Action*, action))

    /// Returns the descriptive text for this action.
    ///
    const std::string& text() const {
        return text_;
    }

    /// Sets the descriptive text for this action.
    ///
    void setText(std::string_view text) {
        text_ = text;
        changed().emit(this);
    }

    /// Returns the shortcut associated with this action. This can be an empty
    /// shortcut if this action has no associated shortcut.
    ///
    const Shortcut& shortcut() const {
        return shortcut_;
    }

    /// Sets the shortcut associated with this action.
    ///
    void setShortcut(const Shortcut& shortcut) {
        shortcut_ = shortcut;
        changed().emit(this);
    }

    VGC_SIGNAL(disabled, (Action*, action))

    /// Returns whether this action is disabled or not.
    ///
    bool isDisabled() const {
        return isDisabled_;
    }

    /// Sets the disabled state of this action.
    ///
    void setDisabled(bool disabled) {
        isDisabled_ = disabled;
        this->disabled().emit(this);
    }

    /// Returns whether this action is to open a menu.
    ///
    bool isMenu() const {
        return isMenu_;
    }

    /// If the action is not disabled, triggers the action and returns true.
    /// Otherwise returns false.
    ///
    /// This will cause the triggered signal to be emitted.
    ///
    /// \sa triggered
    ///
    bool trigger(Widget* from = nullptr);

    /// This signal is emitted whenever the action is activated by the user (for example,
    /// clicking on an associated button), or when the trigger() method is called.
    ///
    /// \sa trigger()
    ///
    VGC_SIGNAL(triggered, (Widget*, from))

private:
    std::string text_;
    Shortcut shortcut_;
    bool isDisabled_ = false;
    bool isMenu_ = false;
};

} // namespace vgc::ui

#endif // VGC_UI_ACTION_H
