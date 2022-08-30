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

#ifndef VGC_UI_BUTTON_H
#define VGC_UI_BUTTON_H

#include <string>
#include <string_view>

#include <vgc/geometry/vec2f.h>
#include <vgc/graphics/richtext.h>
#include <vgc/ui/widget.h>

namespace vgc::ui {

VGC_DECLARE_OBJECT(Button);

/// \class vgc::ui::Button
/// \brief A button widget.
///
class VGC_UI_API Button : public Widget {
private:
    VGC_OBJECT(Button, Widget)

protected:
    /// This is an implementation details. Please use
    /// Button::create(text) instead.
    ///
    Button(std::string_view text);

public:
    /// Creates a Button.
    ///
    static ButtonPtr create();

    /// Creates a Button with the given text.
    ///
    static ButtonPtr create(std::string_view text);

    /// Returns the Button's text.
    ///
    const std::string& text() const {
        return richText_->text();
    }

    /// Sets the Button's text.
    ///
    void setText(std::string_view text);

    /// Clicks the button at position `pos` in local coordinates.
    ///
    /// This will cause the clicked signals to be emitted.
    ///
    /// \sa clicked, clickedAt
    ///
    void click(const geometry::Vec2f& pos);

    /// This signal is emitted whenever the button is clicked by the user, or
    /// when the click() method is called.
    ///
    /// \sa click()
    ///
    VGC_SIGNAL(clicked);

    /// This signal is emitted whenever the button is clicked by the user, or
    /// when the click() method is called.
    ///
    /// \sa click()
    ///
    VGC_SIGNAL(clickedAt, (Button*, button), (const geometry::Vec2f&, pos));

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

protected:
    geometry::Vec2f computePreferredSize() const override;

private:
    graphics::RichTextPtr richText_;
    graphics::GeometryViewPtr triangles_;
    bool reload_ = true;
};

} // namespace vgc::ui

#endif // VGC_UI_BUTTON_H
