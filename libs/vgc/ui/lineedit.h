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

#ifndef VGC_UI_LINEEDIT_H
#define VGC_UI_LINEEDIT_H

#include <string>
#include <string_view>

#include <vgc/core/stopwatch.h>
#include <vgc/graphics/richtext.h>
#include <vgc/ui/widget.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(LineEdit);

/// \class vgc::ui::LineEdit
/// \brief A widget to edit a line of text.
///
class VGC_UI_API LineEdit : public Widget {
private:
    VGC_OBJECT(LineEdit, Widget)

protected:
    /// This is an implementation details. Please use
    /// LineEdit::create(text) instead.
    ///
    LineEdit(std::string_view text);

public:
    /// Creates a LineEdit.
    ///
    static LineEditPtr create();

    /// Creates a LineEdit with the given text.
    ///
    static LineEditPtr create(std::string_view text);

    /// Returns the LineEdit's text.
    ///
    const std::string& text() const {
        return richText_->text();
    }

    /// Sets the LineEdit's text.
    ///
    void setText(std::string_view text);

    /// Moves the cursor according to the given operation. If `select` is false
    /// (the default), then the selection is cleared. If `select` is true, then
    /// the current selection is modified to integrate the given operation
    /// (typically, this mode is used when a user presses `Shift`).
    ///
    void moveCursor(graphics::RichTextMoveOperation operation, bool select = false);

    // Reimplementation of StylableObject virtual methods
    style::StylableObject* firstChildStylableObject() const override;
    style::StylableObject* lastChildStylableObject() const override;

    // Reimplementation of Widget virtual methods
    void onResize() override;
    void onPaintCreate(graphics::Engine* engine) override;
    void onPaintDraw(graphics::Engine* engine, PaintOptions options) override;
    void onPaintDestroy(graphics::Engine* engine) override;
    bool onMouseMove(MouseEvent* event) override;
    bool onMousePress(MouseEvent* event) override;
    bool onMouseRelease(MouseEvent* event) override;
    bool onMouseEnter() override;
    bool onMouseLeave() override;
    bool onFocusIn(FocusReason reason) override;
    bool onFocusOut(FocusReason reason) override;
    bool onKeyPress(QKeyEvent* event) override;

    /// This signal is emitted whenever the Enter or Return key is pressed or
    /// the line edit loses focus.
    ///
    VGC_SIGNAL(editingFinished)

    /// This signal is emitted whenever the text in the line edit has been
    /// edited graphically. This signal is not emitted when the text is changed
    /// programatically, for example via setText() or clear().
    ///
    VGC_SIGNAL(textEdited)

protected:
    geometry::Vec2f computePreferredSize() const override;

private:
    graphics::RichTextPtr richText_;
    graphics::GeometryViewPtr triangles_;
    bool reload_ = true;
    ui::MouseButton mouseButton_ = ui::MouseButton::None;

    // Handle double/triple clicks
    core::Stopwatch leftMouseButtonStopwatch_;
    Int numLeftMouseButtonClicks_ = 0;
    geometry::Vec2f mousePositionOnPress_;

    // Handle snapping to word/line boundaries on mouse move after double/triple-click
    // and extending the selection with shift+click
    void extendSelection_(const geometry::Vec2f& point);
    void resetSelectionInitialPair_();
    graphics::TextBoundaryMarkers mouseSelectionMarkers_ =
        graphics::TextBoundaryMarker::Grapheme;
    std::pair<Int, Int> mouseSelectionInitialPair_;
};

} // namespace vgc::ui

#endif // VGC_UI_LINEEDIT_H
