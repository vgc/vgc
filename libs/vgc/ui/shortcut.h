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

#include <vgc/ui/api.h>
#include <vgc/ui/key.h>
#include <vgc/ui/modifierkey.h>

namespace vgc::ui {

/// \class vgc::ui::Shortcut
/// \brief Represents a combination of keys that can trigger an action.
///
/// A Shortcut is a combination of ModifiersKeys together with a Key.
///
class VGC_UI_API Shortcut {
public:
    /// Creates an empty shortcut, that is, a shortcut whose key is set
    /// as Key::None.
    ///
    Shortcut()
        : modifiers_()
        , key_(Key::None) {
    }

    /// Creates a Shortcut with the given modifier keys and key.
    ///
    Shortcut(ModifierKeys modifiers, Key key)
        : modifiers_(modifiers)
        , key_(key) {
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
    void setKey(Key key) {
        key_ = key;
    }

    /// Returns whether the shortcut is empty, that is, whether key() is
    /// Key::None.
    ///
    bool isEmpty() const {
        return key() == Key::None;
    }

private:
    ModifierKeys modifiers_;
    Key key_;
};

} // namespace vgc::ui

#endif // VGC_UI_SHORTCUT_H
