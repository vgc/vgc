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

#include <vgc/ui/genericcommands.h>

namespace vgc::ui::commands::generic {

using ui::modifierkeys::ctrl;

VGC_UI_DEFINE_WINDOW_COMMAND( //
    cut,
    "generic.cut",
    "Cut",
    Shortcut(ctrl, Key::X));

VGC_UI_DEFINE_WINDOW_COMMAND( //
    copy,
    "generic.copy",
    "Copy",
    Shortcut(ctrl, Key::C));

VGC_UI_DEFINE_WINDOW_COMMAND( //
    paste,
    "generic.paste",
    "Paste",
    Shortcut(ctrl, Key::V));

} // namespace vgc::ui::commands::generic
