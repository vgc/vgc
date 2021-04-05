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

#include <vgc/ui/internal/paintutil.h>

#include <vgc/graphics/font.h>
#include <vgc/graphics/text.h>

namespace vgc {
namespace ui {
namespace internal {

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
        float x1, float y1, float x2, float y2,
        const std::string& text,
        bool hinting)
{
    if (text.length() > 0) {
        float r = static_cast<float>(c[0]);
        float g = static_cast<float>(c[1]);
        float b = static_cast<float>(c[2]);

        // Get FontFace.
        graphics::FontFace* fontFace = graphics::fontLibrary()->defaultFace();

        // Shape text
        graphics::ShapedText shapedText(fontFace, text);

        // Vertical centering
        float ascent = static_cast<float>(fontFace->ascent());
        float descent = static_cast<float>(fontFace->descent());
        float height = ascent - descent;
        float textTop = y1 + 0.5 * (y2-y1-height);
        float baseline = textTop + ascent;
        if (hinting) {
            baseline = std::round(baseline);
        }

        // Horizontal centering. Note: we intentionally don't perform hinting
        // on the horizontal direction.
        const graphics::ShapedGlyphArray& glyphs = shapedText.glyphs();
        Int start = 0;
        Int end = glyphs.length();
        float width = x2-x1;
        float advance = shapedText.advance()[0];
        std::unique_ptr<graphics::ShapedText> ellipsis;
        if (advance > width) {
            ellipsis = std::make_unique<graphics::ShapedText>(fontFace, "...");
            advance = ellipsis->advance()[0];
            end = 0;
            for (const graphics::ShapedGlyph& glyph : glyphs) {
                float newAdvance = advance + glyph.advance()[0];
                if (newAdvance > width) {
                    break;
                }
                advance = newAdvance;
                end += 1;
            }
        }
        float textLeft = x1 + 0.5 * (width-advance);

        // Triangulate text
        core::Vec2d origin(textLeft, baseline);
        shapedText.fill(a, origin, r, g, b, start, end);
        if (ellipsis) {
            core::Vec2d ellipsisOrigin = origin + core::Vec2d(advance, 0) - ellipsis->advance();
            ellipsis->fill(a, ellipsisOrigin, r, g, b);
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

} // namespace internal
} // namespace ui
} // namespace vgc
