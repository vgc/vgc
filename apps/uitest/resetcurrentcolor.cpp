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

#include "resetcurrentcolor.h"

#include <vgc/tools/currentcolor.h>
#include <vgc/ui/modifierkey.h>
#include <vgc/ui/modulecontext.h>

namespace vgc::apps::uitest {

namespace {

using ui::Key;
using ui::Shortcut;
using ui::modifierkeys::mod;

namespace commands {

VGC_UI_DEFINE_WINDOW_COMMAND(
    resetCurrentColor,
    "uitest.resetCurrentColor",
    "Resets the current color to the default color (black)",
    Shortcut(mod, Key::C))

} // namespace commands

} // namespace

ResetCurrentColor::ResetCurrentColor(CreateKey key, const ui::ModuleContext& context)
    : Module(key, context)
    , currentColor_(context.importModule<tools::CurrentColor>()) {

    ui::Action* action = createTriggerAction(commands::resetCurrentColor());
    action->triggered().connect(onActionTriggered_Slot());
}

ResetCurrentColorPtr ResetCurrentColor::create(const ui::ModuleContext& context) {
    return core::createObject<ResetCurrentColor>(context);
}

void ResetCurrentColor::onActionTriggered_() {
    currentColor_->setColor(core::colors::black);
}

} // namespace vgc::apps::uitest
