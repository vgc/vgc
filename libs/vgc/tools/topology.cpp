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

#include <vgc/tools/topology.h>

namespace vgc::tools::commands {

using ui::Key;
using ui::Shortcut;

constexpr ui::ModifierKey alt = ui::ModifierKey::Alt;

VGC_UI_DEFINE_TRIGGER_COMMAND( //
    glue,
    "tools.topology.glue",
    "Glue",
    Shortcut(alt, Key::G));

VGC_UI_DEFINE_TRIGGER_COMMAND( //
    unglue,
    "tools.topology.unglue",
    "Unglue",
    Shortcut(alt, Key::U));

VGC_UI_DEFINE_TRIGGER_COMMAND( //
    simplify,
    "tools.topology.simplify",
    "Simplify",
    Shortcut(alt, Key::S));

} // namespace vgc::tools::commands
