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

#ifndef VGC_UI_MODIFIERKEY_H
#define VGC_UI_MODIFIERKEY_H

#include <vgc/core/arithmetic.h>
#include <vgc/core/flags.h>
#include <vgc/ui/api.h>

namespace vgc::ui {

// clang-format off

/// \enum vgc::ui::ModifierKey
/// \brief Represents Shift, Ctrl, Alt, or Meta.
///
/// Note that on macOS, ModifierKey::Ctrl corresponds to the Cmd key, and
/// ModifierKey::Meta modifier corresponds to the Ctrl key. This makes
/// cross-platform development easier, since users on macOS typically expect
/// shortcuts such as Cmd+A, while Windows and Linux users expect shortcuts
/// such as Ctrl+A.
///
/// On Windows, ModifierKey::Meta corresponds to the Windows key.
///
/// \sa ModifierKeys
///
enum class ModifierKey : UInt8 {
    None  = 0x00,
    Shift = 0x01,
    Ctrl  = 0x02,
    Alt   = 0x04,
    Meta  = 0x08
};
VGC_DEFINE_FLAGS(ModifierKeys, ModifierKey)

// clang-format on

} // namespace vgc::ui

#endif // VGC_UI_MODIFIERKEY_H
