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

#include <vgc/ui/genericaction.h>

#include <vgc/ui/widget.h>

namespace vgc::ui {

GenericAction::GenericAction(CreateKey key, core::StringId id)
    : Action(key, id) {

    init_();
}

GenericAction::GenericAction(CreateKey key, core::StringId id, std::string_view text)
    : Action(key, id, text) {

    init_();
}

GenericActionPtr GenericAction::create(core::StringId id) {
    return core::createObject<GenericAction>(id);
}

GenericActionPtr GenericAction::create(core::StringId id, std::string_view text) {
    return core::createObject<GenericAction>(id, text);
}

void GenericAction::init_() {
    updateAction_();
    updateActionState_();
    owningWidgetChanged().connect(onOwningWidgetChangedSlot_());
    triggered().connect(onTriggeredSlot_());
}

namespace {

// Order of priority:
// 1. currently focused widget
// 2. ancestors of currently focused widget
// 3. previously focused widgets in focus stack and their ancestors
//
Action* findActionInFocusStack(Widget* owningWidget, core::StringId genericCommandId) {
    if (!owningWidget) {
        return nullptr;
    }
    core::Array<WidgetPtr> focusStack = owningWidget->focusStack();
    for (Int i = focusStack.length() - 1; i >= 0; --i) {
        Widget* widget = focusStack[i].get();
        while (widget) {
            for (Action* action : widget->actions()) {
                if (action->genericCommandId() == genericCommandId) {
                    return action;
                }
            }
            widget = widget->parent();
        }
    }
    return nullptr;
}

} // namespace

void GenericAction::updateAction_() {
    Action* oldAction = action_;
    action_ = findActionInFocusStack(owningWidget(), commandId());
    if (action_ != oldAction) {
        updateActionState_();
        if (oldAction) {
            oldAction->disconnect(this);
        }
        if (action_) {
            // Note: "properties" include text() and checkMode().
            action_->propertiesChanged().connect(updateActionStateSlot_());
            action_->enabledChanged().connect(updateActionStateSlot_());
            action_->checkStateChanged().connect(updateActionStateSlot_());

            action_->aboutToBeDestroyed().connect(updateActionSlot_());
            action_->owningWidgetChanged().connect(updateActionSlot_());
        }
    }
}

void GenericAction::updateActionState_() {
    if (action_) {
        setText(action_->text());
        setCheckMode(action_->checkMode());
        setEnabled(action_->isEnabled());
        setCheckState(action_->checkState());
    }
    else {
        setText(name());
        setEnabled(false);
        setCheckable(false); // implicit: setCheckedState(Unchecked)
    }

    // XXX Shouldn't checkMode() be part of the Command and be immutable?
    //
    //     We could emit a warning if a specific command doesn't have the
    //     same check mode as the generic command it refers to.
    //
    // XXX What about groups? Does it make sense for a generic action to
    //     have a group with exclusive policy? What if the policy of the
    //     group of the generic action is not compatible with the policy
    //     of the group of the specific action?
}

void GenericAction::onWidgetRootChanged_(Widget* widgetRoot) {
    if (widgetRoot_) {
        widgetRoot_->disconnect(this);
    }
    widgetRoot_ = widgetRoot;
    if (widgetRoot_) {
        widgetRoot_->focusCleared().connect(updateActionSlot_());
        widgetRoot_->focusSet().connect(updateActionSlot_());
    }
}

void GenericAction::onOwningWidgetChanged_(Widget* owningWidget) {

    onWidgetRootChanged_(owningWidget ? owningWidget->root() : nullptr);

    // TODO: make the above implementation simpler by adding Widget::rootChanged()
    // and/or Action::widgetRootChanged()?
}

void GenericAction::onTriggered_() {
    if (action_) {
        action_->trigger();
    }
}

} // namespace vgc::ui
