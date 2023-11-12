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

#include <vgc/ui/module.h>

#include <vgc/ui/widget.h>

namespace vgc::ui {

Module::Module(CreateKey key, const ModuleContext& /*context*/)
    : Object(key) {
}

ModulePtr Module::create(const ModuleContext& context) {
    return core::createObject<Module>(context);
}

// XXX: do we need to override onDestroyed() and call removeAction()?

// Note: removing the action ensures that the action is now a root object, and
// therefore that our ObjPtr stored in actions_ keeps it alive (unless a parent
// is later re-added... for now we assume it doesn't happen and later things
// should be more robust with true shared ownership).
//
void Module::addAction(Action* action) {
    ActionPtr actionPtr = action;
    if (Widget* widget = action->owningWidget()) {
        widget->removeAction(action);
    }
    if (!actions_.contains(action)) {
        actions_.append(std::move(actionPtr));
        actionAdded().emit(action);
    }
}

void Module::removeAction(Action* action) {
    ActionPtr actionPtr = action;
    Int i = actions_.removeAll(actionPtr);
    if (i > 0) {
        actionRemoved().emit(action);
    }
}

void Module::clearActions() {
    while (!actions_.isEmpty()) {
        removeAction(actions_.last().get());
    }
}

} // namespace vgc::ui
