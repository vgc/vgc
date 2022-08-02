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

#include <vgc/core/color.h>
#include <vgc/ui/widget.h>

namespace vgc {
namespace ui {

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
    Button(const std::string& text);

public:
    /// Creates a Button.
    ///
    static ButtonPtr create();

    /// Creates a Button with the given text.
    ///
    static ButtonPtr create(const std::string& text);

    /// Returns the Button's text.
    ///
    const std::string& text() const {
        return text_;
    }

    /// Sets the Button's text.
    ///
    void setText(const std::string& text);

    // reimpl
    void onResize() override;
    void onPaintCreate(graphics::Engine* engine) override;
    void onPaintDraw(graphics::Engine* engine, PaintFlags flags) override;
    void onPaintDestroy(graphics::Engine* engine) override;
    bool onMouseMove(MouseEvent* event) override;
    bool onMousePress(MouseEvent* event) override;
    bool onMouseRelease(MouseEvent* event) override;
    bool onMouseEnter() override;
    bool onMouseLeave() override;

protected:
    geometry::Vec2f computePreferredSize() const override;

private:
    std::string text_;
    graphics::GeometryViewPtr triangles_;
    bool reload_;
    bool isHovered_;
};

} // namespace ui
} // namespace vgc

#endif // VGC_UI_BUTTON_H
