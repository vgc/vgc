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

Action::Action() {
}

Action::Action(const Shortcut& shortcut)
    : shortcut_(shortcut) {
}

Action::Action(const std::string_view& text)
    : text_(text) {
}

Action::Action(const std::string_view& text, const Shortcut& shortcut)
    : text_(text)
    , shortcut_(shortcut) {
}

ActionPtr Action::create() {
    return ActionPtr(new Action());
}

ActionPtr Action::create(const Shortcut& shortcut) {
    return ActionPtr(new Action(shortcut));
}

ActionPtr Action::create(const std::string_view& text) {
    return ActionPtr(new Action(text));
}

ActionPtr Action::create(const std::string_view& text, const Shortcut& shortcut) {
    return ActionPtr(new Action(text, shortcut));
}

void Action::setEnabled(bool enabled) {
    if (isEnabled_ == enabled) {
        return;
    }
    isEnabled_ = enabled;
    this->enabledChanged().emit(enabled);
}

void Action::setCheckMode(CheckMode newMode) {
    if (checkMode_ != newMode) {
        //CheckMode oldMode = checkMode_;
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

        // Update CheckMode style
        // XXX TODO: have Button listen to its underlying action and
        //           update its style classes appropriately
        /*
        if (oldMode == CheckMode::Uncheckable) {
            addStyleClass(strings::checkable);
        }
        else if (newMode == CheckMode::Uncheckable) {
            removeStyleClass(strings::checkable);
        }
        replaceStyleClass(
            detail::modeToStringId(oldMode), detail::modeToStringId(newMode));
        */
    }
}

bool Action::supportsCheckState(CheckState checkState) const {
    if (checkMode_ == CheckMode::Uncheckable) {
        return checkState == CheckState::Unchecked;
    }
    else if (checkMode_ == CheckMode::Bistate) {
        return checkState != CheckState::Indeterminate;
    }
    else {
        return true;
    }
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
    triggered().emit(from);
    return true;
}

void Action::setCheckStateNoEmit_(CheckState newState) {
    checkState_ = newState;
}

void Action::emitPendingCheckState_() {
    if (lastEmittedCheckState_ != checkState_) {

        // In case of nested emits, remember who is the first emitter. If we're
        // the first emitter, we're responsible for not destructing the action,
        // as well as updating the style once all nested emits are done.
        //
        bool isFirstEmitter = !emitting_;
        emitting_ = true;
        ActionPtr thisPtr;
        core::StringId oldStyleClass;
        if (isFirstEmitter) {
            thisPtr = this;
            oldStyleClass = detail::stateToStringId(lastEmittedCheckState_);
        }

        // The first emitter is responsible for updating the style, after all
        // nested emits are done. Here, we remember the current style.
        //
        if (isFirstEmitter) {
            oldStyleClass = detail::stateToStringId(lastEmittedCheckState_);
        }

        // Emit the checkStateChanged() signal. This may cause nested emits.
        //
        //reload_ = true;
        lastEmittedCheckState_ = checkState_;
        checkStateChanged().emit(this, checkState_);

        // Update style. Note that here, checkState_ may be different from the
        // one emitted above, but that's intended: we want to set the style of
        // the nested-most emit.
        //
        if (isFirstEmitter) {
            //const core::StringId newStyleClass = detail::stateToStringId(checkState_);
            emitting_ = false;
            //replaceStyleClass(oldStyleClass, newStyleClass);
        }
    }
}

} // namespace vgc::ui
