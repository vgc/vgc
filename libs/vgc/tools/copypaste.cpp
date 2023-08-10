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

#include <vgc/tools/copypaste.h>

namespace vgc::tools {

namespace commands {

using ui::Key;
using ui::Shortcut;

constexpr ui::ModifierKey ctrl = ui::ModifierKey::Ctrl;

VGC_UI_DEFINE_WINDOW_COMMAND( //
    copy,
    "tools.copypaste.copy",
    "Copy",
    Shortcut(ctrl, Key::C));

VGC_UI_DEFINE_WINDOW_COMMAND( //
    paste,
    "tools.copypaste.paste",
    "Paste",
    Shortcut(ctrl, Key::V));

VGC_UI_DEFINE_WINDOW_COMMAND( //
    duplicate,
    "tools.copypaste.duplicate",
    "Duplicate",
    Shortcut(ctrl, Key::D));

} // namespace commands

VGC_TOOLS_API
core::StringId copyCommandId() {
    return commands::copy;
}

VGC_TOOLS_API
core::StringId pasteCommandId() {
    return commands::paste;
}

VGC_TOOLS_API
core::StringId duplicateCommandId() {
    return commands::duplicate;
}

} // namespace vgc::tools
