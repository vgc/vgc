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

namespace vgc::ui {

/// Pushes a mouse cursor to the cursor stack. This cursor becomes the
/// currently active cursor.
///
/// A typical usage is for example to push a cursor when the mouse is hovering
/// a certain UI element, and pop it back when the mouse is leaving the
/// element, thus restoring the previously active cursor.
///
/// \sa changeCursor() and popCursor()
///
void pushCursor(const QCursor& cursor);

/// Changes the currently active cursor, that is, the one on top of the cursor
/// stack.
///
/// Note that in most cases, you should use pushCursor() and popCursor() rather
/// than this function.
///
/// \sa pushCursor() and popCursor()
///
void changeCursor(const QCursor& cursor);

/// Pops the currently active cursor from the cursor stack.
///
/// \sa pushCursor() and changeCursor()
///
void popCursor();

} // namespace vgc::ui

#endif // VGC_UI_CURSOR_H
