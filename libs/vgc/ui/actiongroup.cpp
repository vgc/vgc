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

#include <vgc/ui/actiongroup.h>

#include <vgc/ui/action.h>
#include <vgc/ui/logcategories.h>

namespace vgc::ui {

ActionGroup::ActionGroup(CreateKey key, CheckPolicy checkPolicy)
    : Object(key)
    , checkPolicy_(checkPolicy) {
}

ActionGroupPtr ActionGroup::create() {
    return core::createObject<ActionGroup>(CheckPolicy::ZeroOrMore);
}

ActionGroupPtr ActionGroup::create(CheckPolicy checkPolicy) {
    return core::createObject<ActionGroup>(checkPolicy);
}

void ActionGroup::clear() {
    for (Action* action : actions_) {
        disconnectAction_(action);
    }
    actions_.clear();
}

void ActionGroup::addAction(Action* action) {
    if (!action) {
        VGC_WARNING(LogVgcUi, "Attempting to add a null action to an ActionGroup.");
        return;
    }
    if (action->group() == this) {
        return;
    }
    ActionGroup* oldGroup = action->group();
    addActionNoEmit_(action);
    action->groupChanged().emit(this);
    if (oldGroup) {
        oldGroup->actionsChanged().emit();
        oldGroup->emitPendingCheckStates_();
    }
    actionsChanged().emit();
    emitPendingCheckStates_();
}

void ActionGroup::removeAction(Action* action) {
    if (!action) {
        VGC_WARNING(LogVgcUi, "Attempting to remove a null action from an ActionGroup.");
        return;
    }
    if (action->group() != this) {
        return;
    }
    removeActionNoEmit_(action);
    action->groupChanged().emit(nullptr);
    actionsChanged().emit();
    emitPendingCheckStates_();
}

Int ActionGroup::numCheckedActions() const {
    Int res = 0;
    for (Action* action : actions()) {
        if (action->isChecked()) {
            ++res;
        }
    }
    return res;
}

void ActionGroup::setCheckPolicy(CheckPolicy checkPolicy) {
    if (checkPolicy_ == checkPolicy) {
        return;
    }
    checkPolicy_ = checkPolicy;
    enforcePolicy_();
}

bool ActionGroup::isCheckPolicySatisfied() {
    switch (checkPolicy()) {
    case CheckPolicy::ZeroOrMore:
        return true;
    case CheckPolicy::ExactlyOne:
        return numCheckedActions() == 1;
    }
    return true; // silence warning
}

void ActionGroup::connectAction_(Action* action) {
    action->group_ = this;
    action->aboutToBeDestroyed().connect(onActionDestroyedSlot_());
}

void ActionGroup::disconnectAction_(Action* action) {
    action->group_ = nullptr;
    action->aboutToBeDestroyed().disconnect(onActionDestroyedSlot_());
}

void ActionGroup::onActionDestroyed_(Object* action) {
    actions_.removeOne(static_cast<Action*>(action));
    enforcePolicy_();
}

void ActionGroup::addActionNoEmit_(Action* action) {
    if (!actions_.contains(action)) {
        if (action->group_) {
            action->group_->removeActionNoEmit_(action);
        }
        actions_.append(action);
        connectAction_(action);
        enforcePolicyNoEmit_(action);
    }
}

void ActionGroup::removeActionNoEmit_(Action* action) {
    Int oldNumActions = numActions();
    actions_.removeOne(action);
    if (oldNumActions != numActions()) {
        disconnectAction_(action);
        enforcePolicyNoEmit_();
    }
}

bool ActionGroup::toggle_(ActionGroup* group, Action* action) {
    if (!action->isEnabled()) {
        return false;
    }
    CheckPolicy policy = group ? group->checkPolicy() : CheckPolicy::ZeroOrMore;
    if (policy == CheckPolicy::ExactlyOne) {
        if (action->isCheckable() && !action->isChecked()) {
            action->setCheckStateNoEmit_(CheckState::Checked);
            group->unckeckOthersNoEmit_(action);
            group->emitPendingCheckStates_();
        }
        else {
            // action is uncheckable or already checked => do nothing
            return false;
        }
    }
    else {
        if (action->isCheckable()) {
            if (action->isChecked()) {
                action->setCheckStateNoEmit_(CheckState::Unchecked);
            }
            else {
                action->setCheckStateNoEmit_(CheckState::Checked);
            }
            action->emitPendingCheckState_();
        }
        else {
            // action is uncheckable => do nothing
            return false;
        }
    }
    return true;
}

void ActionGroup::setCheckState_(ActionGroup* group, Action* action, CheckState state) {
    CheckPolicy policy = group ? group->checkPolicy() : CheckPolicy::ZeroOrMore;
    if (policy == CheckPolicy::ExactlyOne) {
        if (action->isChecked()) {
            // we're about to uncheck this action, so we need to check another one
            group->checkFirstOtherCheckableNoEmit_(action);
        }
        action->setCheckStateNoEmit_(state);
        if (action->isChecked()) {
            group->unckeckOthersNoEmit_(action);
            group->emitPendingCheckStates_();
        }
        else {
            action->emitPendingCheckState_();
        }
    }
    else {
        action->setCheckStateNoEmit_(state);
        action->emitPendingCheckState_();
    }
}

void ActionGroup::enforcePolicy_(Action* newAction) {
    enforcePolicyNoEmit_(newAction);
    emitPendingCheckStates_();
}

void ActionGroup::enforcePolicyNoEmit_(Action* newAction) {
    if (checkPolicy_ == CheckPolicy::ExactlyOne) {
        Int numChecked = numCheckedActions();
        if (numChecked == 0) {
            if (newAction && newAction->isCheckable()) {
                newAction->setCheckStateNoEmit_(CheckState::Checked);
            }
            else {
                checkFirstOtherCheckableNoEmit_(nullptr);
            }
        }
        else if (numChecked >= 2) {
            if (newAction && newAction->isChecked()) {
                --numChecked;
                newAction->setCheckStateNoEmit_(CheckState::Unchecked);
            }
            if (numChecked >= 2) {
                bool foundChecked = false;
                for (Action* action : actions()) {
                    if (action->isChecked()) {
                        if (foundChecked) {
                            action->setCheckStateNoEmit_(CheckState::Unchecked);
                        }
                        else {
                            foundChecked = true;
                        }
                    }
                }
            }
        }
    }
}

void ActionGroup::unckeckOthersNoEmit_(Action* checkedAction) {
    for (Action* action : actions()) {
        if (action != checkedAction && action->isChecked()) {
            action->setCheckStateNoEmit_(CheckState::Unchecked);
        }
    }
}

void ActionGroup::checkFirstCheckable_() {
    for (Action* action : actions()) {
        if (action->isCheckable()) {
            action->setCheckState(CheckState::Checked);
            return;
        }
    }
}

void ActionGroup::checkFirstOtherCheckableNoEmit_(Action* uncheckedAction) {
    for (Action* action : actions()) {
        if (action != uncheckedAction && action->isCheckable()) {
            action->setCheckStateNoEmit_(CheckState::Checked);
            return;
        }
    }
}

void ActionGroup::emitPendingCheckStates_() {
    // Note: signal listeners may cause modifications to actions_ during the
    // iteration, so it is unsafe to use a range-for loop here
    for (Int i = 0; i < numActions(); ++i) {
        actions_[i]->emitPendingCheckState_();
    }
}

} // namespace vgc::ui
