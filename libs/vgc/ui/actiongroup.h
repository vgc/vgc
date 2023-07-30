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

#ifndef VGC_UI_ACTIONGROUP_H
#define VGC_UI_ACTIONGROUP_H

#include <vgc/core/array.h>
#include <vgc/core/object.h>
#include <vgc/ui/api.h>
#include <vgc/ui/strings.h>

namespace vgc::ui {

/// \class vgc::ui::CheckMode
/// \brief Whether an action is uncheckable or checkable (possibly tristate).
///
enum class CheckMode : UInt8 {

    /// The action is not checkable.
    ///
    Uncheckable,

    /// The action is checkable with two possible states: `Unchecked` and
    /// `Checked`.
    ///
    Bistate,

    /// The action is checkable with three possible states: `Unchecked`,
    /// `Checked`, and `Indeterminate`.
    ///
    Tristate
};

/// \class vgc::ui::CheckState
/// \brief The possible check states of an action.
///
enum class CheckState : UInt8 {
    Unchecked,
    Checked,
    Indeterminate
};

/// \class vgc::ui::CheckPolicy
/// \brief How many actions in a group can be checked at a time.
///
/// In a group of checkable actions, this specifies how many actions can be
/// checked (`isChecked() == true`) at any given time.
///
/// Note that an `Indeterminate` action is considered not checked for the
/// policy. For example, a group whose policy is `ExactlyOne` must have exactly
/// one action whose state is `Checked`, but can have zero or more actions
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

VGC_DECLARE_OBJECT(ActionGroup);
VGC_DECLARE_OBJECT(Action);

/// \class vgc::ui::ActionGroup
/// \brief Allows to define mutually exclusive checkable actions.
///
class VGC_UI_API ActionGroup : public core::Object {
private:
    VGC_OBJECT(ActionGroup, core::Object)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

protected:
    ActionGroup(CreateKey, CheckPolicy checkPolicy);

public:
    /// Creates a non-exclusive `ActionGroup`, that is, a group with the policy
    /// `CheckPolicy::ZeroOrMore`.
    ///
    static ActionGroupPtr create();

    /// Creates an `ActionGroup` with the given `checkPolicy`.
    ///
    static ActionGroupPtr create(CheckPolicy checkPolicy);

    /// Removes all actions in this group.
    ///
    void clear();

    /// Adds an action to this group.
    ///
    /// Does nothing if the action is already in this group.
    ///
    /// If the action was already part of another group, it is automatically
    /// removed from the other group prior to being added to this group.
    ///
    void addAction(Action* action);

    /// Removes an action from this group.
    ///
    /// Does nothing if the action is not already in the group.
    ///
    void removeAction(Action* action);

    /// This signal is emitted whenever an action is added or removed from the
    /// group.
    ///
    VGC_SIGNAL(actionsChanged)

    /// Removes the list of all actions in the group, by order of insertion.
    ///
    const core::Array<Action*>& actions() const {
        return actions_;
    }

    /// Returns the number of actions in the group.
    ///
    Int numActions() const {
        return actions_.length();
    }

    /// Returns the number of checked actions in the group.
    ///
    Int numCheckedActions() const;

    /// Returns whether more than one action can be checked at a time.
    /// False by default.
    ///
    void setCheckPolicy(CheckPolicy checkPolicy);

    /// Sets whether more than one action can be checked at a time.
    ///
    CheckPolicy checkPolicy() const {
        return checkPolicy_;
    }

    /// Returns whether the `checkPolicy()` is satisfied.
    ///
    /// In most typical cases, this function will return true, since the
    /// `ActionGroup` tries its best to enforce the policy automatically. For
    /// example, if the policy is `ExactlyOne` (= "radio actions"), then
    /// checking an action automatically unchecks any other checked action.
    ///
    /// However, in some scenarios, the policy is impossible to satisfy. For
    /// example, if the policy is `ExactlyOne` and `numActions() == 0`, then
    /// the policy cannot be satisfied. A similar scenario is when
    /// `numActions() > 0`, but all actions are `CheckMode::Uncheckable`.
    /// In these cases, this function returns false.
    ///
    bool isCheckPolicySatisfied();

private:
    friend class Action;

    core::Array<Action*> actions_;
    CheckPolicy checkPolicy_;

    void connectAction_(Action* action);
    void disconnectAction_(Action* action);
    void onActionDestroyed_(Object* action);
    VGC_SLOT(onActionDestroyedSlot_, onActionDestroyed_)

    // Same as addAction() and removeAction(), but without emitting signals
    void addActionNoEmit_(Action* action);
    void removeActionNoEmit_(Action* action);

    // Implements toggle() logic. `group` can be null. `action` must be
    // non-null. Returns whether a change happens (nothing may happen if the
    // button is not checkable, or if is is already checked and part of an
    // exclusive group.
    static bool toggle_(ActionGroup* group, Action* action);

    // Implements setCheckState() logic.
    // `group` can be null. `action` must be non-null.
    // Assumes that the given action supports state.
    // Assumes that the current state of the action is different than newState.
    static void setCheckState_(ActionGroup* group, Action* action, CheckState state);

    // Tries to enforce the checked policy. If `newAction` is not `nullptr`, it
    // is assumed to be a newly added action and prioritize checking/unchecking
    // this action over other actions.
    void enforcePolicyNoEmit_(Action* newAction = nullptr);
    void enforcePolicy_(Action* newAction = nullptr);

    // Sets all `Checked` actions in this group (other than `action`) to `Unchecked`.
    // Leaves `Indeterminate` actions unchanged.
    void unckeckOthersNoEmit_(Action* action = nullptr);

    // Sets the first checkable action in this group to `Checked`.
    void checkFirstCheckable_();

    // Sets the first checkable action in this group (other than `action`) to `Checked`.
    // Note that this might be a previously `Indeterminate` action.
    void checkFirstOtherCheckableNoEmit_(Action* action = nullptr);

    // Informs the world about the new state:
    // - emits checkStateChanged
    // - updates style classes
    void emitPendingCheckStates_();
};

} // namespace vgc::ui

#endif // VGC_UI_ACTIONGROUP_H
