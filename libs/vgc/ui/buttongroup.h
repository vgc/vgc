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

#ifndef VGC_UI_BUTTONGROUP_H
#define VGC_UI_BUTTONGROUP_H

// Note: this is temporary design. Eventually, we probably
//       want to only have "action buttons". That is, any "Button"
//       always have an associate action, and the groups are defined
//       via ActionGroup instead of ButtonGroup.

#include <vgc/core/array.h>
#include <vgc/core/object.h>
#include <vgc/ui/api.h>
#include <vgc/ui/strings.h>

namespace vgc::ui {

/// \class vgc::ui::CheckMode
/// \brief Whether a button is uncheckable or checkable (possibly tristate).
///
enum class CheckMode : UInt8 {

    /// The button is not checkable.
    ///
    Uncheckable,

    /// The button is checkable with two possible states: `Unchecked` and
    /// `Checked`.
    ///
    Bistate,

    /// The button is checkable with three possible states: `Unchecked`,
    /// `Checked`, and `Indeterminate`.
    ///
    Tristate
};

/// \class vgc::ui::CheckState
/// \brief The possible check states of a button.
///
enum class CheckState : UInt8 {
    Unchecked,
    Checked,
    Indeterminate
};

/// \class vgc::ui::CheckPolicy
/// \brief How many buttons in a group can be checked at a time.
///
/// In a group of checkable buttons, this specifies how many buttons can be
/// checked (`isChecked() == true`) at any given time.
///
/// Note that an `Indeterminate` button is considered not checked for the
/// policy. For example, a group whose policy is `ExactlyOne` must have exactly
/// one button whose state is `Checked`, but can have zero or more buttons
/// whose state is `Indeterminate` or `Unchecked`.
///
enum class CheckPolicy : UInt8 {
    ZeroOrMore,
    // OneOrMore,
    // ZeroOrOne,
    ExactlyOne
};

namespace detail {

inline core::StringId modeToStringId(CheckMode mode) {
    switch (mode) {
    case CheckMode::Uncheckable:
        return strings::uncheckable;
    case CheckMode::Bistate:
        return strings::bistate;
    case CheckMode::Tristate:
        return strings::tristate;
    }
    return core::StringId(); // silence warning
}

inline core::StringId stateToStringId(CheckState state) {
    switch (state) {
    case CheckState::Unchecked:
        return strings::unchecked;
    case CheckState::Checked:
        return strings::checked;
    case CheckState::Indeterminate:
        return strings::indeterminate;
    }
    return core::StringId(); // silence warning
}

} // namespace detail

VGC_DECLARE_OBJECT(ButtonGroup);
VGC_DECLARE_OBJECT(Button);

class VGC_UI_API ButtonGroup : public core::Object {
private:
    VGC_OBJECT(ButtonGroup, core::Object)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

protected:
    ButtonGroup(CheckPolicy checkPolicy);

public:
    /// Creates a non-exclusive `ButtonGroup`, that is, a group with the policy
    /// `CheckPolicy::ZeroOrMore`.
    ///
    static ButtonGroupPtr create();

    /// Creates a `ButtonGroup` with the given `checkPolicy`.
    ///
    static ButtonGroupPtr create(CheckPolicy checkPolicy);

    /// Removes all buttons in the group.
    ///
    void clear();

    /// Adds a button to the group.
    ///
    /// Does nothing if the button is already in the group.
    ///
    /// If the button was already part of another group, it is first removed
    /// from this other group.
    ///
    void addButton(Button* button);

    /// Removes a button from the group.
    ///
    /// Does nothing if the button is not already in the group.
    ///
    void removeButton(Button* button);

    /// Removes the list of all buttons in the group, by order of insertion.
    ///
    const core::Array<Button*>& buttons() const {
        return buttons_;
    }

    /// Returns the number of buttons in the group.
    ///
    Int numButtons() const {
        return buttons_.length();
    }

    /// Returns the number of checked buttons in the group.
    ///
    Int numCheckedButtons() const;

    /// Returns whether more than one button can be checked at a time.
    /// False by default.
    ///
    void setCheckPolicy(CheckPolicy checkPolicy);

    /// Sets whether more than one button can be checked at a time.
    ///
    CheckPolicy checkPolicy() const {
        return checkPolicy_;
    }

    /// Returns whether the `checkPolicy()` is satisfied.
    ///
    /// In most typical cases, this function will return true, since the
    /// `ButtonGroup` tries its best to enforce the policy automatically. For
    /// example, if the policy is `ExactlyOne` (= "radio buttons"), then
    /// checking a button automatically unchecks any other checked button.
    ///
    /// However, in some scenarios, the policy is impossible to satisfy. For
    /// example, if the policy is `ExactlyOne` and `numButtons() == 0`, then
    /// the policy cannot be satisfied. A similar scenario is when
    /// `numButtons() > 0`, but all buttons are `CheckMode::Uncheckable`.
    /// In these cases, this function returns false.
    ///
    bool isCheckPolicySatisfied();

private:
    friend class Button;

    core::Array<Button*> buttons_;
    CheckPolicy checkPolicy_;

    void connectButton_(Button* button);
    void disconnectButton_(Button* button);
    void onButtonDestroyed_(Object* button);
    VGC_SLOT(onButtonDestroyedSlot_, onButtonDestroyed_)

    // Implements toggle() logic.
    // `group` can be null. `button` must be non-null.
    static void toggle_(ButtonGroup* group, Button* button);

    // Implements setCheckState() logic.
    // `group` can be null. `button` must be non-null.
    // Assumes that the given button supports state.
    // Assumes that the current state of the button is different than newState.
    static void setCheckState_(ButtonGroup* group, Button* button, CheckState state);

    // Tries to enforce the checked policy. If `newButton` is not `nullptr`, it
    // is assumed to be a newly added button and prioritize checking/unchecking
    // this button over other buttons.
    void enforcePolicyNoEmit_(Button* newButton = nullptr);
    void enforcePolicy_(Button* newButton = nullptr);

    // Sets all `Checked` buttons in this group (other than `button`) to `Unchecked`.
    // Leaves `Indeterminate` buttons unchanged.
    void unckeckOthersNoEmit_(Button* button = nullptr);

    // Sets the first checkable button in this group to `Checked`.
    void checkFirstCheckable_();

    // Sets the first checkable button in this group (other than `button`) to `Checked`.
    // Note that this might be a previously `Indeterminate` button.
    void checkFirstOtherCheckableNoEmit_(Button* button = nullptr);

    // Informs the world about the new state:
    // - emits checkStateChanged
    // - updates style classes
    void emitPendingCheckStates_();
};

} // namespace vgc::ui

#endif // VGC_UI_BUTTONGROUP_H
