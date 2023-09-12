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

#ifndef VGC_UI_CHECKENUMS_H
#define VGC_UI_CHECKENUMS_H

#include <vgc/core/arithmetic.h>
#include <vgc/core/enum.h>
#include <vgc/ui/api.h>

namespace vgc::ui {

/// \class vgc::ui::CheckMode
/// \brief Whether an action is uncheckable or checkable (possibly tristate).
///
enum class CheckMode : UInt8 {

    /// The action is not checkable.
    ///
    Uncheckable,

    /// The action is checkable with two possible states: `Unchecked` and
    /// `Checked`.
    ///
    Bistate,

    /// The action is checkable with three possible states: `Unchecked`,
    /// `Checked`, and `Indeterminate`.
    ///
    Tristate
};

VGC_UI_API
VGC_DECLARE_ENUM(CheckMode)

/// \class vgc::ui::CheckState
/// \brief The possible check states of an action.
///
enum class CheckState : UInt8 {
    Unchecked,
    Checked,
    Indeterminate
};

VGC_UI_API
VGC_DECLARE_ENUM(CheckState)

/// Returns whether the given `checkState` is supported by the given
/// `checkMode`.
///
/// If the mode is `Uncheckable`, the only supported state is `Unchecked`.
///
/// If the mode is `Bistate`, the supported states are `Unchecked` and
/// `Checked`.
///
/// If the mode is `Tristate`, the supported states are `Unchecked`, `Checked`,
/// and `Indeterminate`.
///
/// \sa `checkState()`, `checkMode()`.
///
constexpr bool supportsCheckState(CheckMode checkMode, CheckState checkState) {
    if (checkMode == CheckMode::Uncheckable) {
        return checkState == CheckState::Unchecked;
    }
    else if (checkMode == CheckMode::Bistate) {
        return checkState != CheckState::Indeterminate;
    }
    else {
        return true;
    }
}

/// \class vgc::ui::CheckPolicy
/// \brief How many actions in a group can be checked at a time.
///
/// In a group of checkable actions, this specifies how many actions can be
/// checked (`isChecked() == true`) at any given time.
///
/// Note that an `Indeterminate` action is considered not checked for the
/// policy. For example, a group whose policy is `ExactlyOne` must have exactly
/// one action whose state is `Checked`, but can have zero or more actions
/// whose state is `Indeterminate` or `Unchecked`.
///
enum class CheckPolicy : UInt8 {
    ZeroOrMore,
    // OneOrMore,
    // ZeroOrOne,
    ExactlyOne
};

VGC_UI_API
VGC_DECLARE_ENUM(CheckPolicy)

namespace detail {

VGC_UI_API
core::StringId modeToStringId(CheckMode mode);

VGC_UI_API
core::StringId stateToStringId(CheckState state);

} // namespace detail

} // namespace vgc::ui

#endif // VGC_UI_CHECKENUMS_H
