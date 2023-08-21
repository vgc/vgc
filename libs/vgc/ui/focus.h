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

#ifndef VGC_UI_FOCUS_H
#define VGC_UI_FOCUS_H

#include <vgc/core/arithmetic.h>
#include <vgc/core/enum.h>
#include <vgc/core/flags.h>
#include <vgc/ui/api.h>

namespace vgc::ui {

/// \enum vgc::ui::FocusPolicy
/// \brief Specifies how a widget accepts keyboard focus.
///
enum class FocusPolicy : UInt8 {

    /// Specifies that the widget never accepts focus.
    ///
    Never = 0x00,

    /// Specifies that the widget accepts focus by clicking on it.
    ///
    Click = 0x01,

    /// Specifies that the widget accepts focus by using the mouse wheel over
    /// it.
    ///
    Wheel = 0x02,

    /// Specifies that the widget accepts focus cycling through widgets with
    /// the Tab key. it.
    ///
    Tab = 0x04,
};
VGC_DEFINE_FLAGS(FocusPolicyFlags, FocusPolicy)

VGC_UI_API
VGC_DECLARE_ENUM(FocusPolicy)

/// \enum vgc::ui::FocusStrength
/// \brief Specifies how strong a widget's focus is.
///
enum class FocusStrength : UInt8 {

    /// The widget will lose focus when clicking on empty space, and give it
    /// back to any previously focused widget with Medium or High strength.
    ///
    Low,

    /// The widget will lose focus when clicking on empty space, and give it
    /// back to any previously focused widget with High strength.
    ///
    Medium,

    /// The widget will not lose focus when clicking on empty space, and will
    /// never give it back to any other widget.
    ///
    High
};

VGC_UI_API
VGC_DECLARE_ENUM(FocusStrength)

/// \enum vgc::ui::FocusReason
/// \brief Specifies why a widget receives or loses keyboard focus.
///
enum class FocusReason : UInt8 {

    /// A mouse click or wheel event occurred.
    ///
    Mouse = 0,

    /// The user cycled through widgets with the Tab key.
    ///
    Tab = 1,

    /// The user cycled through widgets, in reverse order, with the Tab key
    /// (e.g., Shift+Tab).
    ///
    Backtab = 2,

    /// The window became active or inactive.
    ///
    Window = 3,

    /// A popup grabbed or released the keyboard focus.
    ///
    Popup = 4,

    /// A widget gained focus via its label's buddy shortcut.
    ///
    Shortcut = 5,

    /// A menu bar grabbed or released the keyboard focus.
    ///
    Menu = 6,

    /// Another reason.
    ///
    Other = 7,
};

VGC_UI_API
VGC_DECLARE_ENUM(FocusReason)

} // namespace vgc::ui

#endif // VGC_UI_FOCUS_H
