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

#include <vgc/ui/buttongroup.h>

#include <vgc/ui/button.h>

namespace vgc::ui {

ButtonGroup::ButtonGroup() {
}

ButtonGroupPtr ButtonGroup::create() {
    return ButtonGroupPtr(new ButtonGroup());
}

void ButtonGroup::clear() {
    for (Button* button : buttons_) {
        disconnectButton_(button);
    }
    buttons_.clear();
}

void ButtonGroup::addButton(Button* button) {
    if (!buttons_.contains(button)) {
        if (button->group_) {
            button->group_->removeButton(button);
        }
        buttons_.append(button);
        connectButton_(button);
        enforcePolicy_(button);
    }
}

void ButtonGroup::removeButton(Button* button) {
    Int oldNumButtons = numButtons();
    buttons_.removeOne(button);
    if (oldNumButtons != numButtons()) {
        disconnectButton_(button);
        enforcePolicy_();
    }
}

Int ButtonGroup::numCheckedButtons() const {
    Int res = 0;
    for (Button* button : buttons()) {
        if (button->isChecked()) {
            ++res;
        }
    }
    return res;
}

void ButtonGroup::setCheckPolicy(CheckPolicy checkPolicy) {
    if (checkPolicy_ != checkPolicy) {
        checkPolicy_ = checkPolicy;
        enforcePolicy_();
    }
}

bool ButtonGroup::isCheckPolicySatisfied() {
    switch (checkPolicy()) {
    case CheckPolicy::ZeroOrMore:
        return true;
    case CheckPolicy::ExactlyOne:
        return numCheckedButtons() == 1;
    }
    return true; // silence warning
}

void ButtonGroup::connectButton_(Button* button) {
    button->group_ = this;
    button->aboutToBeDestroyed().connect(onButtonDestroyedSlot_());
}

void ButtonGroup::disconnectButton_(Button* button) {
    button->group_ = nullptr;
    button->aboutToBeDestroyed().disconnect(onButtonDestroyedSlot_());
}

void ButtonGroup::onButtonDestroyed_(Object* button) {
    buttons_.removeOne(static_cast<Button*>(button));
    enforcePolicy_();
}

void ButtonGroup::toggle_(ButtonGroup* group, Button* button) {
    CheckPolicy policy = group ? group->checkPolicy() : CheckPolicy::ZeroOrMore;
    if (policy == CheckPolicy::ExactlyOne) {
        if (button->isCheckable() && !button->isChecked()) {
            button->setCheckStateNoEmit_(CheckState::Checked);
            group->unckeckOthersNoEmit_(button);
            group->emitPendingCheckStates_();
        }
        else {
            // button is uncheckable or already checked => do nothing
            return;
        }
    }
    else {
        if (button->isCheckable()) {
            if (button->isChecked()) {
                button->setCheckStateNoEmit_(CheckState::Unchecked);
            }
            else {
                button->setCheckStateNoEmit_(CheckState::Checked);
            }
            button->emitPendingCheckState_();
        }
        else {
            // button is uncheckable => do nothing
            return;
        }
    }
}

void ButtonGroup::setCheckState_(ButtonGroup* group, Button* button, CheckState state) {
    CheckPolicy policy = group ? group->checkPolicy() : CheckPolicy::ZeroOrMore;
    if (policy == CheckPolicy::ExactlyOne) {
        if (button->isChecked()) {
            // we're about to uncheck this button, so we need to check another one
            group->checkFirstOtherCheckableNoEmit_(button);
        }
        button->setCheckStateNoEmit_(state);
        if (button->isChecked()) {
            group->unckeckOthersNoEmit_(button);
            group->emitPendingCheckStates_();
        }
        else {
            button->emitPendingCheckState_();
        }
    }
    else {
        button->setCheckStateNoEmit_(state);
        button->emitPendingCheckState_();
    }
}

void ButtonGroup::enforcePolicy_(Button* newButton) {
    enforcePolicyNoEmit_(newButton);
    emitPendingCheckStates_();
}

void ButtonGroup::enforcePolicyNoEmit_(Button* newButton) {
    if (checkPolicy_ == CheckPolicy::ExactlyOne) {
        Int numChecked = numCheckedButtons();
        if (numChecked == 0) {
            if (newButton && newButton->isCheckable()) {
                newButton->setCheckStateNoEmit_(CheckState::Checked);
            }
            else {
                checkFirstOtherCheckableNoEmit_(nullptr);
            }
        }
        else if (numChecked >= 2) {
            if (newButton && newButton->isChecked()) {
                --numChecked;
                newButton->setCheckStateNoEmit_(CheckState::Unchecked);
            }
            if (numChecked >= 2) {
                bool foundChecked = false;
                for (Button* button : buttons()) {
                    if (button->isChecked()) {
                        if (foundChecked) {
                            button->setCheckStateNoEmit_(CheckState::Unchecked);
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

void ButtonGroup::unckeckOthersNoEmit_(Button* checkedButton) {
    for (Button* button : buttons()) {
        if (button != checkedButton && button->isChecked()) {
            button->setCheckStateNoEmit_(CheckState::Unchecked);
        }
    }
}

void ButtonGroup::checkFirstCheckable_() {
    for (Button* button : buttons()) {
        if (button->isCheckable()) {
            button->setCheckState(CheckState::Checked);
            return;
        }
    }
}

void ButtonGroup::checkFirstOtherCheckableNoEmit_(Button* uncheckedButton) {
    for (Button* button : buttons()) {
        if (button != uncheckedButton && button->isCheckable()) {
            button->setCheckStateNoEmit_(CheckState::Checked);
            return;
        }
    }
}

void ButtonGroup::emitPendingCheckStates_() {
    // Note: signal listeners may cause modifications to buttons_ during the
    // iteration, so it is unsafe to use a range-for loop here
    for (Int i = 0; i < numButtons(); ++i) {
        buttons_[i]->emitPendingCheckState_();
    }
}

} // namespace vgc::ui
