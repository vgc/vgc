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

#include <vgc/ui/checkenums.h>

#include <vgc/ui/strings.h>

namespace vgc::ui {

VGC_DEFINE_ENUM(
    CheckMode,
    (Uncheckable, "Uncheckable"),
    (Bistate, "Bistate"),
    (Tristate, "Tristate"))

VGC_DEFINE_ENUM(
    CheckState,
    (Unchecked, "Unchecked"),
    (Checked, "Checked"),
    (Indeterminate, "Indeterminate"))

VGC_DEFINE_ENUM( //
    CheckPolicy,
    (ZeroOrMore, "Zero or More"),
    (ExactlyOne, "Exactly One"))

namespace detail {

core::StringId modeToStringId(CheckMode mode) {
    switch (mode) {
    case CheckMode::Uncheckable:
        return strings::uncheckable;
    case CheckMode::Bistate:
        return strings::bistate;
    case CheckMode::Tristate:
        return strings::tristate;
    }
    return core::StringId(); // silence warning
}

core::StringId stateToStringId(CheckState state) {
    switch (state) {
    case CheckState::Unchecked:
        return strings::unchecked;
    case CheckState::Checked:
        return strings::checked;
    case CheckState::Indeterminate:
        return strings::indeterminate;
    }
    return core::StringId(); // silence warning
}

} // namespace detail

} // namespace vgc::ui
