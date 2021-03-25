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

#include <vgc/core/floatarray.h>
#include <vgc/core/paths.h>
#include <vgc/core/random.h>
#include <vgc/graphics/font.h>
#include <vgc/ui/strings.h>
#include <vgc/ui/style.h>

namespace vgc {
namespace ui {

Button::Button(const std::string& text) :
    Widget(),
    text_(text),
    trianglesId_(-1),
    reload_(true),
    isHovered_(false)
{
    addClass(strings::Button);
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
    trianglesId_ = engine->createTriangles();
}

namespace {

void insertTriangle(
        core::FloatArray& a,
        float r, float g, float b,
        float x1, float y1,
        float x2, float y2,
        float x3, float y3)
{
    a.insert(a.end(), {
        x1, y1, r, g, b,
        x2, y2, r, g, b,
        x3, y3, r, g, b});
}

void insertRect(
        core::FloatArray& a,
        float r, float g, float b,
        float x1, float y1, float x2, float y2)
{
    a.insert(a.end(), {
        x1, y1, r, g, b,
        x2, y1, r, g, b,
        x1, y2, r, g, b,
        x2, y1, r, g, b,
        x2, y2, r, g, b,
        x1, y2, r, g, b});
}

void insertRect(
        core::FloatArray& a,
        const core::Color& c,
        float x1, float y1, float x2, float y2,
        float borderRadius)
{
    float r = static_cast<float>(c[0]);
    float g = static_cast<float>(c[1]);
    float b = static_cast<float>(c[2]);
    float maxBorderRadius = 0.5 * std::min(std::abs(x2-x1), std::abs(y2-y1));
    borderRadius = core::clamp(borderRadius, 0.0f, maxBorderRadius);
    Int32 numCornerTriangles = core::ifloor<Int32>(borderRadius);
    if (numCornerTriangles < 1) {
        a.insert(a.end(), {
            x1, y1, r, g, b,
            x2, y1, r, g, b,
            x1, y2, r, g, b,
            x2, y1, r, g, b,
            x2, y2, r, g, b,
            x1, y2, r, g, b});
    }
    else {
        float x1_ = x1 + borderRadius;
        float x2_ = x2 - borderRadius;
        float y1_ = y1 + borderRadius;
        float y2_ = y2 - borderRadius;
        // center rectangle
        insertRect(a, r, g, b, x1_, y1_, x2_, y2_);
        // side rectangles
        insertRect(a, r, g, b, x1_, y1, x2_, y1_);
        insertRect(a, r, g, b, x2_, y1_, x2, y2_);
        insertRect(a, r, g, b, x1_, y2_, x2_, y2);
        insertRect(a, r, g, b, x1, y1_, x1_, y2_);
        // rounded corners
        float rcost_ = borderRadius;
        float rsint_ = 0;
        float dt = (0.5 * core::pi) / numCornerTriangles;
        for (Int32 i = 1; i <= numCornerTriangles; ++i) {
            float t = i * dt;
            float rcost = borderRadius * std::cos(t);
            float rsint = borderRadius * std::sin(t);
            insertTriangle(a, r, g, b, x1_, y1_, x1_ - rcost_, y1_ - rsint_, x1_ - rcost, y1_ - rsint);
            insertTriangle(a, r, g, b, x2_, y1_, x2_ + rsint_, y1_ - rcost_, x2_ + rsint, y1_ - rcost);
            insertTriangle(a, r, g, b, x2_, y2_, x2_ + rcost_, y2_ + rsint_, x2_ + rcost, y2_ + rsint);
            insertTriangle(a, r, g, b, x1_, y2_, x1_ - rsint_, y2_ + rcost_, x1_ - rsint, y2_ + rcost);
            rcost_ = rcost;
            rsint_ = rsint;
        }
    }
}

// x1, y1, x2, y2 is the text box to center the text into
void insertText(
        core::FloatArray& a,
        const core::Color& c,
        float /*x1*/, float y1, float /*x2*/, float y2,
        const std::string& text,
        bool hinting)
{
    if (text.length() > 0) {
        float r = static_cast<float>(c[0]);
        float g = static_cast<float>(c[1]);
        float b = static_cast<float>(c[2]);

        // Get FontFace.
        // TODO: we should use a persistent font library rather than
        // creating a new one each time
        std::string facePath = core::resourcePath("graphics/fonts/SourceSansPro/TTF/SourceSansPro-Regular.ttf");
        graphics::FontLibraryPtr fontLibrary = graphics::FontLibrary::create();
        graphics::FontFace* fontFace = fontLibrary->addFace(facePath); // XXX can this be nullptr?

        // Vertical centering
        float ascent = static_cast<float>(fontFace->ascent());
        float descent = static_cast<float>(fontFace->descent());
        float height = ascent - descent;
        float textTop = y1 + 0.5 * (y2-y1-height);
        float baseline = textTop + ascent;
        if (hinting) {
            baseline = std::round(baseline);
        }

        // Note: we currently disable per-letter hinting (which can be seen as
        // a component of horizontal hinting) because it looked worse, at least
        // with the current implementation. It produced uneven spacing between
        // letters, making kerning look bad.
        constexpr bool perLetterHinting = false;

        // Shape and triangulate text
        core::DoubleArray buf;
        for (const graphics::FontItem& item : fontFace->shape(text)) {
            float xoffset = item.offset()[0];
            float yoffset = item.offset()[1];
            if (perLetterHinting) {
                xoffset = std::round(xoffset);
                yoffset = std::round(yoffset);
            }
            buf.clear();
            item.glyph()->outline().fill(buf);
            for (Int i = 0; 2*i+1 < buf.length(); ++i) {
                float x = xoffset + static_cast<float>(buf[2*i]);
                float y = yoffset + static_cast<float>(buf[2*i+1]);
                y = baseline - y; // [ascent, descent] -> [0, height]
                a.insert(a.end(), {x, y, r, g, b});
            }
        }
    }
}

core::Color getColor(const Widget* widget, core::StringId property)
{
    core::Color res;
    StyleValue value = widget->style(property);
    if (value.type() == StyleValueType::Color) {
        res = value.color();
    }
    return res;
}

float getLength(const Widget* widget, core::StringId property)
{
    float res = 0;
    StyleValue value = widget->style(property);
    if (value.type() == StyleValueType::Number) {
        res = value.number();
    }
    return res;
}

} // namespace

void Button::onPaintDraw(graphics::Engine* engine)
{
    if (reload_) {
        reload_ = false;
        core::FloatArray a;
        core::Color backgroundColor = getColor(this, isHovered_ ?
                        strings::background_color_on_hover :
                        strings::background_color);
        core::Color textColor = getColor(this, strings::text_color);
        float borderRadius = getLength(this, strings::border_radius);
        bool hinting = style(strings::pixel_hinting) == strings::normal;
        insertRect(a, backgroundColor, 0, 0, width(), height(), borderRadius);
        insertText(a, textColor, 0, 0, width(), height(), text_, hinting);
        engine->loadTriangles(trianglesId_, a.data(), a.length());
    }
    engine->drawTriangles(trianglesId_);
}

void Button::onPaintDestroy(graphics::Engine* engine)
{
    engine->destroyTriangles(trianglesId_);
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

core::Vec2f Button::computePreferredSize() const
{
    PreferredSizeType auto_ = PreferredSizeType::Auto;
    PreferredSize w = preferredWidth();
    PreferredSize h = preferredHeight();
    core::Vec2f res(0, 0);
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
