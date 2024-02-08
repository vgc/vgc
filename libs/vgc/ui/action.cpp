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

#include <vgc/ui/action.h>

#include <vgc/ui/logcategories.h>
#include <vgc/ui/menu.h>

namespace vgc::ui {

Action::Action(CreateKey key, core::StringId id)
    : Object(key)
    , command_(&CommandRegistry::find(id))
    , text_(command_->name()) {
}

Action::Action(CreateKey key, core::StringId id, std::string_view text)
    : Object(key)
    , command_(&CommandRegistry::find(id))
    , text_(text) {
}

ActionPtr Action::create(core::StringId id) {
    return core::createObject<Action>(id);
}

ActionPtr Action::create(core::StringId id, std::string_view text) {
    return core::createObject<Action>(id, text);
}

void Action::setText(std::string_view text) {
    if (text_ == text) {
        return;
    }
    text_ = text;
    propertiesChanged().emit();
}

void Action::setCheckMode(CheckMode newMode) {
    if (checkMode_ != newMode) {
        checkMode_ = newMode;

        // Update state if current state is now unsupported
        if (!supportsCheckState(checkState_)) {
            setCheckStateNoEmit_(CheckState::Unchecked);
        }

        // Update other actions in the same group
        if (group()) {
            group()->enforcePolicyNoEmit_(this);
        }

        // Prevent destructing object then emit state changes
        ActionPtr thisPtr(this);
        if (group()) {
            group()->emitPendingCheckStates_();
        }
        else {
            emitPendingCheckState_();
        }

        // Emit properties changed
        propertiesChanged().emit();
    }
}

void Action::setGroup(ActionGroup* group) {
    if (group) {
        // Add action to new group, automatically removing it from current group
        group->addAction(this);
    }
    else if (group_) {
        // Remove action from current group
        group_->removeAction(this);
    }
}

void Action::setEnabled(bool enabled) {
    if (isEnabled_ == enabled) {
        return;
    }
    isEnabled_ = enabled;
    this->enabledChanged().emit(enabled);
}

void Action::setCheckState(CheckState newState) {
    if (checkState_ != newState) {
        if (!supportsCheckState(newState)) {
            VGC_WARNING(
                LogVgcUi,
                "Cannot assign {} state to {} action.",
                detail::stateToStringId(newState),
                detail::modeToStringId(checkMode_));
            return;
        }
        ActionGroup::setCheckState_(group(), this, newState);
    }
}

bool Action::toggle() {
    return ActionGroup::toggle_(group(), this);
}

bool Action::trigger(Widget* from) {
    if (!isEnabled_) {
        return false;
    }
    if (isCheckable()) {
        toggle();
        // Note: even if toggle() returns false (i.e., the check state
        // didn't change), we still want trigger() to return true, because
        // the trigger was indeed performed.
    }
    triggered().emit(from);
    return true;
}

void Action::onMouseClick(MouseEvent*) {
}

void Action::onMouseDragStart(MouseEvent*) {
}

void Action::onMouseDragMove(MouseEvent*) {
}

void Action::onMouseDragConfirm(MouseEvent*) {
}

void Action::onMouseDragCancel(MouseEvent*) {
}

void Action::setOwningWidget_(Widget* owningWidget) {
    owningWidget_ = owningWidget;
    owningWidgetChanged().emit(owningWidget);
}

void Action::setCheckStateNoEmit_(CheckState newState) {
    checkState_ = newState;
}

void Action::emitPendingCheckState_() {
    if (lastEmittedCheckState_ != checkState_) {
        bool wasChecked = (lastEmittedCheckState_ == CheckState::Checked);
        bool isChecked = (checkState_ == CheckState::Checked);
        lastEmittedCheckState_ = checkState_;
        checkStateChanged().emit(this, checkState_);
        if (wasChecked != isChecked) {
            toggled().emit(isChecked);
        }
    }
}

} // namespace vgc::ui
