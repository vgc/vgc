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

#include <vgc/core/innercore.h>
#include <vgc/ui/api.h>
#include <vgc/ui/shortcut.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(Action);

/// \class vgc::ui::Action
/// \brief Represents an action that can be triggered via menu items, shorctuts, etc.
///
class VGC_UI_API Action : public core::Object {
private:
    VGC_OBJECT(Action, core::Object)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

protected:
    Action();
    explicit Action(const Shortcut& shortcut);

public:
    /// Creates an action.
    ///
    static ActionPtr create();

    /// Creates an action with the given shortcut.
    ///
    static ActionPtr create(const Shortcut& shortcut);

    /// Triggers the action. This will cause the triggered signal to be emitted.
    ///
    /// \sa triggered
    ///
    void trigger();

    /// This signal is emitted whenever the action is activated by the user (for example, clicking
    /// on an associated button), or when the trigger() method is called.
    ///
    /// \sa trigger()
    ///
    VGC_SIGNAL(triggered);

    /// Returns the shortcut associated with this action. This can be an empty
    /// shortcut if this action has no associated shortcut.
    ///
    const Shortcut& shortcut() const {
        return shortcut_;
    }

private:
    Shortcut shortcut_;
};

} // namespace vgc::ui

#endif // VGC_UI_ACTION_H
