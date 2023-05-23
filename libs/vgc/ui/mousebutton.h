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

#ifndef VGC_UI_MOUSEBUTTON_H
#define VGC_UI_MOUSEBUTTON_H

#include <vgc/core/arithmetic.h>
#include <vgc/core/enum.h>
#include <vgc/core/flags.h>
#include <vgc/ui/api.h>

namespace vgc::ui {

// clang-format off

/// \class vgc::ui::MouseButton
/// \brief Enumeration of all possible mouse buttons
///
enum class MouseButton : UInt32 {
    None    = 0x00000000,
    Left    = 0x00000001,
    Right   = 0x00000002,
    Middle  = 0x00000004,
    Button4  = 0x00000008,
    Button5  = 0x00000010,
    Button6  = 0x00000020,
    Button7  = 0x00000040,
    Button8  = 0x00000080,
    Button9  = 0x00000100,
    Button10  = 0x00000200,
    Button11  = 0x00000400,
    Button12  = 0x00000800,
    Button13 = 0x00001000,
    Button14 = 0x00002000,
    Button15 = 0x00004000,
    Button16 = 0x00008000,
    Button17 = 0x00010000,
    Button18 = 0x00020000,
    Button19 = 0x00040000,
    Button20 = 0x00080000,
    Button21 = 0x00100000,
    Button22 = 0x00200000,
    Button23 = 0x00400000,
    Button24 = 0x00800000,
    Button25 = 0x01000000,
    Button26 = 0x02000000,
    Button27 = 0x04000000
};
VGC_DEFINE_FLAGS(MouseButtons, MouseButton)

VGC_UI_API
VGC_DECLARE_ENUM(MouseButton)

// clang-format on

} // namespace vgc::ui

#endif // VGC_UI_MOUSEBUTTON_H
