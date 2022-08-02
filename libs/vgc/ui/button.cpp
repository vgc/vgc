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

#include <vgc/ui/button.h>

#include <vgc/core/array.h>
#include <vgc/ui/strings.h>

#include <vgc/ui/internal/paintutil.h>

namespace vgc {
namespace ui {

Button::Button(const std::string& text) :
    Widget(),
    text_(text),
    reload_(true),
    isHovered_(false)
{
    addStyleClass(strings::Button);
}

ButtonPtr Button::create()
{
    return ButtonPtr(new Button(""));
}

ButtonPtr Button::create(const std::string& text)
{
    return ButtonPtr(new Button(text));
}

void Button::setText(const std::string& text)
{
    if (text_ != text) {
        text_ = text;
        reload_ = true;
        repaint();
    }
}

void Button::onResize()
{
    reload_ = true;
}

void Button::onPaintCreate(graphics::Engine* engine)
{
    triangles_ = engine->createDynamicTriangleListView(graphics::BuiltinGeometryLayout::XYRGB);
}

void Button::onPaintDraw(graphics::Engine* engine, PaintFlags /*flags*/)
{
    if (reload_) {
        reload_ = false;
        core::FloatArray a = {};
        core::Color backgroundColor = internal::getColor(this, isHovered_ ?
                        strings::background_color_on_hover :
                        strings::background_color);
        core::Color textColor = internal::getColor(this, strings::text_color);
        float borderRadius = internal::getLength(this, strings::border_radius);
        graphics::TextProperties textProperties(
                    graphics::TextHorizontalAlign::Center,
                    graphics::TextVerticalAlign::Middle);
        graphics::TextCursor textCursor;
        bool hinting = style(strings::pixel_hinting) == strings::normal;
        internal::insertRect(a, backgroundColor, 0, 0, width(), height(), borderRadius);
        internal::insertText(a, textColor, 0, 0, width(), height(), 0, 0, 0, 0, text_, textProperties, textCursor, hinting);
        engine->updateVertexBufferData(triangles_, std::move(a));
    }
    engine->setProgram(graphics::BuiltinProgram::Simple);
    engine->draw(triangles_, -1, 0);
}

void Button::onPaintDestroy(graphics::Engine*)
{
    triangles_.reset();
}

bool Button::onMouseMove(MouseEvent* /*event*/)
{
    return true;
}

bool Button::onMousePress(MouseEvent* /*event*/)
{
    return true;
}

bool Button::onMouseRelease(MouseEvent* /*event*/)
{
    return true;
}

bool Button::onMouseEnter()
{
    isHovered_ = true;
    reload_ = true;
    repaint();
    return true;
}

bool Button::onMouseLeave()
{
    isHovered_ = false;
    reload_ = true;
    repaint();
    return true;
}

geometry::Vec2f Button::computePreferredSize() const
{
    PreferredSizeType auto_ = PreferredSizeType::Auto;
    PreferredSize w = preferredWidth();
    PreferredSize h = preferredHeight();
    geometry::Vec2f res(0, 0);
    if (w.type() == auto_) {
        res[0] = 100;
        // TODO: compute appropriate width based on text length
    }
    else {
        res[0] = w.value();
    }
    if (h.type() == auto_) {
        res[1] = 26;
        // TODO: compute appropriate height based on font size?
    }
    else {
        res[1] = h.value();
    }
    return res;
}

} // namespace ui
} // namespace vgc
