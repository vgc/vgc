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
#include <vgc/ui/api.h>

namespace vgc {
namespace ui {

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
    Shift = 0x01,
    Ctrl  = 0x02,
    Alt   = 0x04,
    Meta  = 0x08
};

/// \enum vgc::ui::ModifierKeys
/// \brief Stores a combination of Shift, Ctrl, Alt, or Meta.
///
/// \sa ModifierKey
///
// TODO: turn this design into a generic core::Flags type
//
class VGC_UI_API ModifierKeys {
public:
    using EnumType = ModifierKey;
    using IntType = std::underlying_type_t<EnumType>;

    /// Construct a ModifierKeys representing "no modifiers".
    ///
    ModifierKeys() : i_(0) {}

    /// Construct a ModifierKeys consisting of a single modifier.
    ///
    ModifierKeys(ModifierKey m) : i_(cast_(m)) {}

    /// Sets or unsets the given ModifierKey.
    ///
    void setFlag(ModifierKey m, bool on = true) {
        if (on) {
            i_ |= cast_(m);
        }
        else {
            i_ &= ~cast_(m);
        }
    }

    /// Returns whether this ModifierKeys has the given flag.
    ///
    bool hasFlag(ModifierKey m) const {
        return i_ & cast_(m);
    }

    /// Returns the OR combination of the given flags.
    ///
    ModifierKeys operator|(ModifierKeys other) {
        i_ |= other.i_;
    }

private:
    static IntType cast_(ModifierKey m) { return static_cast<IntType>(m); }
    IntType i_;
};

/// Returns the OR combination of the given flags.
///
ModifierKeys operator|(ModifierKey m1, ModifierKey m2) {
    ModifierKeys res = m1;
    res.setFlag(m2);
    return res;
}

/// Returns the OR combination of the given flags.
///
ModifierKeys operator|(ModifierKeys m1, ModifierKey m2) {
    m1.setFlag(m2);
    return m1;
}

} // namespace ui
} // namespace vgc

#endif // VGC_UI_MODIFIERKEY_H
