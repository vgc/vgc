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

#include <vgc/ui/command.h>

namespace vgc::ui {

VGC_DEFINE_ENUM(
    CommandType,
    (Trigger, "Trigger"),
    (MouseClick, "Mouse Click"),
    (MouseDrag, "Mouse Drag"))

CommandRegistry::CommandRegistry()
    : commands_()
    , unknownCommand_(
          "",
          CommandType::Unknown,
          "Unknown Command",
          ShortcutContext::Application,
          Shortcut())
    , mutex_() {
}

CommandRegistry* CommandRegistry::instance_() {
    // Trusty leaky singleton
    static CommandRegistry* registry = new CommandRegistry();
    return registry;
}

const Command& CommandRegistry::find(core::StringId id) {
    CommandRegistry* registry = CommandRegistry::instance_();
    std::lock_guard<std::mutex> lock(registry->mutex_);
    auto it = registry->commands_.find(id);
    if (it != registry->commands_.end()) {
        return it->second;
    }
    else {
        return registry->unknownCommand_;
    }
}

bool CommandRegistry::contains(core::StringId id) {
    CommandRegistry* registry = CommandRegistry::instance_();
    std::lock_guard<std::mutex> lock(registry->mutex_);
    auto it = registry->commands_.find(id);
    return it == registry->commands_.end();
}

void CommandRegistry::add(Command command) {
    CommandRegistry* registry = CommandRegistry::instance_();
    std::lock_guard<std::mutex> lock(registry->mutex_);
    registry->commands_.insert_or_assign(command.id(), std::move(command));
}

} // namespace vgc::ui
