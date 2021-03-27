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

#include <vgc/ui/label.h>

#include <vgc/core/colors.h>
#include <vgc/core/paths.h>
#include <vgc/core/floatarray.h>
#include <vgc/graphics/font.h>
#include <vgc/ui/strings.h>

namespace vgc {
namespace ui {

Label::Label() :
    Widget(),
    text_(""),
    trianglesId_(-1),
    reload_(true)
{
    addClass(strings::Label);
}

Label::Label(const std::string& text) :
    Widget(),
    text_(text),
    trianglesId_(-1),
    reload_(true)
{

}

/* static */
LabelPtr Label::create()
{
    return LabelPtr(new Label());
}

/* static */
LabelPtr Label::create(const std::string& text)
{
    return LabelPtr(new Label(text));
}

void Label::setText(const std::string& text)
{
    if (text_ != text) {
        text_ = text;
        reload_ = true;
        repaint();
    }
}

void Label::onPaintCreate(graphics::Engine* engine)
{
    trianglesId_ = engine->createTriangles();
}

namespace {

void insertRect(
        core::FloatArray& a,
        const core::Color& c,
        float x1, float y1, float x2, float y2)
{
    float r = static_cast<float>(c[0]);
    float g = static_cast<float>(c[1]);
    float b = static_cast<float>(c[2]);
    a.insert(a.end(), {
        x1, y1, r, g, b,
        x2, y1, r, g, b,
        x1, y2, r, g, b,
        x2, y1, r, g, b,
        x2, y2, r, g, b,
        x1, y2, r, g, b});
}

// x1, y1, x2, y2 is the text box to center the text into
void insertText(
        core::FloatArray& a,
        const core::Color& c,
        float x1, float y1, float /*x2*/, float y2,
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
        core::Vec2d origin(x1, baseline);

        // Shape and triangulate text
        graphics::ShapedText shapedText = fontFace->shape(text);
        shapedText.fill(a, origin, r, g, b);
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

} // namespace

void Label::onPaintDraw(graphics::Engine* engine)
{
    if (reload_) {
        reload_ = false;
        core::FloatArray a;        
        core::Color textColor = getColor(this, strings::text_color);
        bool hinting = style(strings::pixel_hinting) == strings::normal;
        insertText(a, textColor, 0, 0, width(), height(), text_, hinting);
        engine->loadTriangles(trianglesId_, a.data(), a.length());
    }
    engine->drawTriangles(trianglesId_);
}

void Label::onPaintDestroy(graphics::Engine* engine)
{
    engine->destroyTriangles(trianglesId_);
}

} // namespace ui
} // namespace vgc
