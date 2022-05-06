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

#include <vgc/core/color.h>
#include <vgc/graphics/text.h>
#include <vgc/ui/widget.h>

namespace vgc {
namespace ui {

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
    LineEdit(const std::string& text);

public:
    /// Creates a LineEdit.
    ///
    static LineEditPtr create();

    /// Creates a LineEdit with the given text.
    ///
    static LineEditPtr create(const std::string& text);

    /// Returns the LineEdit's text.
    ///
    const std::string& text() const {
        return text_;
    }

    /// Sets the LineEdit's text.
    ///
    void setText(const std::string& text);

    // reimpl
    void onResize() override;
    void onPaintCreate(graphics::Engine* engine) override;
    void onPaintDraw(graphics::Engine* engine) override;
    void onPaintDestroy(graphics::Engine* engine) override;
    bool onMouseMove(MouseEvent* event) override;
    bool onMousePress(MouseEvent* event) override;
    bool onMouseRelease(MouseEvent* event) override;
    bool onMouseEnter() override;
    bool onMouseLeave() override;
    bool onFocusIn() override;
    bool onFocusOut() override;
    bool onKeyPress(QKeyEvent* event) override;

protected:
    core::Vec2f computePreferredSize() const override;

private:
    void updateBytePosition_(const core::Vec2f& mousePosition);
    Int bytePosition_(const core::Vec2f& mousePosition);
    void updateScroll_(float textWidth);
    std::string text_;
    graphics::ShapedText shapedText_;
    graphics::TextCursor textCursor_;
    float scrollLeft_ = 0.0f;
    graphics::TrianglesBufferPtr triangles_;
    bool reload_;
    bool isHovered_;
    bool isMousePressed_;
};

} // namespace ui
} // namespace vgc

#endif // VGC_UI_LINEEDIT_H
