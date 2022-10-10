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

#ifndef VGC_UI_CURSOR_H
#define VGC_UI_CURSOR_H

#include <QCursor>

#include <vgc/core/color.h>
#include <vgc/geometry/vec2f.h>

namespace vgc::ui {

/// Pushes a mouse cursor to the cursor stack. This cursor becomes the
/// currently active cursor. Returns a unique non-negative ID to be able to pop
/// it from the stack later (even if another cursor becomes the top-most
/// cursor).
///
/// A typical usage is for example to push a cursor when the mouse is hovering
/// a certain UI element, and pop it back when the mouse is leaving the
/// element, thus restoring the previously active cursor.
///
/// \sa popCursor(), CursorChanger
///
VGC_NODISCARD("You need to store the cursor id in order to be able to pop it later.")
Int pushCursor(const QCursor& cursor);

/// Pops the given cursor ID from the cursor stack.
///
/// \sa pushCursor(), CursorChanger
///
void popCursor(Int id);

/// \class vgc::ui::CursorChanger
/// \brief A helper class to push/pop cursors
///
class CursorChanger {
public:
    /// Changes the current cursor to the given cursor.
    ///
    /// This automatically pops any cursor previously pushed by this
    /// `CursorChanger`, then pushes a new cursor on the cursor stack.
    ///
    void set(const QCursor& cursor);

    /// Pops any cursor previously pushed by this `CursorChanger`.
    ///
    void clear();

private:
    Int id_ = -1;
};

/// Returns the global position of the mouse cursor in device-independent
/// pixels.
///
geometry::Vec2f globalCursorPosition();

/// Returns the color under the mouse cursor. Returns a black color in
/// case of errors (e.g., failed to queried which screen was under the cursor).
///
/// Warning: this can be an expensive operation.
///
core::Color colorUnderCursor();

} // namespace vgc::ui

#endif // VGC_UI_CURSOR_H
