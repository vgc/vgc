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

namespace vgc::ui::internal {

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
    float maxBorderRadius = 0.5f * std::min(std::abs(x2-x1), std::abs(y2-y1));
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
        float pi = static_cast<float>(core::pi);
        float dt = (0.5f * pi) / numCornerTriangles;
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

graphics::ShapedText shapeText(const std::string& text)
{
    graphics::SizedFont* sizedFont = getDefaultSizedFont();
    return graphics::ShapedText(sizedFont, text);
}

// x1, y1, x2, y2 is the text box to center the text into
void insertText(
        core::FloatArray& a,
        const core::Color& c,
        float x1, float y1, float x2, float y2,
        float paddingLeft, float paddingRight, float paddingTop, float paddingBottom,
        const graphics::ShapedText& shapedText,
        const graphics::TextProperties& textProperties,
        const graphics::TextCursor& textCursor,
        bool hinting,
        float scrollLeft)
{
    graphics::SizedFont* sizedFont = shapedText.sizedFont();
    if (shapedText.text().length() > 0 || textCursor.isVisible()) {
        float r = static_cast<float>(c[0]);
        float g = static_cast<float>(c[1]);
        float b = static_cast<float>(c[2]);

        // Vertical centering
        float height = (y2 - paddingBottom) - (y1 + paddingTop);
        float ascent = sizedFont->ascent();
        float descent = sizedFont->descent();
        if (hinting) {
            ascent = std::round(ascent);
            descent = std::round(descent);
        }
        float textHeight = ascent - descent;
        float textTop = 0;
        switch (textProperties.verticalAlign()) {
        case graphics::TextVerticalAlign::Top:
            textTop = y1 + paddingTop;
            break;
        case graphics::TextVerticalAlign::Middle:
            textTop = y1 + paddingTop + 0.5f * (height - textHeight);
            break;
        case graphics::TextVerticalAlign::Bottom:
            textTop = y1 + paddingTop + (height - textHeight);
            break;
        }
        if (hinting) {
            textTop = std::round(textTop);
        }
        float baseline = textTop + ascent;

        // Horizontal centering. Note: we intentionally don't perform hinting
        // on the horizontal direction.
        float width = (x2 - paddingRight) - (x1 + paddingLeft);
        float advance = static_cast<float>(shapedText.advance()[0]);
        float textLeft = 0;
        switch (textProperties.horizontalAlign()) {
        case graphics::TextHorizontalAlign::Left:
            textLeft = x1 + paddingLeft;
            break;
        case graphics::TextHorizontalAlign::Center:
            textLeft = x1 + paddingLeft + 0.5f * (width - advance);
            break;
        case graphics::TextHorizontalAlign::Right:
            textLeft = x1 + paddingLeft + (width - advance);
            break;
        }
        textLeft -= scrollLeft;

        // Triangulate and clip the text.
        //
        // Note that we clip the text at the given padding. This is often
        // appropriate for LineEdits, but not necessarily for TextEdits, where
        // we may want to clip up to the border of the given text box instead.
        //
        constexpr bool clipAtPadding = true;
        geometry::Vec2f origin(textLeft, baseline);
        float clipLeft   = x1 + (clipAtPadding ? paddingLeft   : 0);
        float clipRight  = x2 - (clipAtPadding ? paddingRight  : 0);
        float clipTop    = y1 + (clipAtPadding ? paddingTop    : 0);
        float clipBottom = y2 - (clipAtPadding ? paddingBottom : 0);
        shapedText.fill(a, origin, r, g, b, clipLeft, clipRight, clipTop, clipBottom);

        // Draw cursor
        if (textCursor.isVisible()) {
            const graphics::ShapedGraphemeArray& graphemes = shapedText.graphemes();
            Int cursorBytePosition = textCursor.bytePosition();
            float cursorAdvance = - scrollLeft;
            for (const graphics::ShapedGrapheme& grapheme : graphemes) {
                if (grapheme.bytePosition() >= cursorBytePosition) {
                    break;
                }
                cursorAdvance += grapheme.advance()[0];
            }
            float cursorX = x1 + paddingLeft + cursorAdvance;
            float cursorW = 1.0f;
            if (hinting) {
                // Note: while we don't perform horizontal hinting for letters,
                // we do perform horizontal hinting for the cursor.
                cursorX = std::round(cursorX);
            }
            if constexpr (clipAtPadding) {
                // Ensure that we still draw the cursor when it is just barely
                // in the clipped padding (typically, when the cursor is at the
                // end of the text)
                clipLeft -= cursorW;
                clipRight += cursorW;
            }
            // Clip and draw cursor. Note that whenever the cursor is at least
            // partially visible in the horizontal direction, we draw it full-legth
            if (clipLeft <= cursorX && cursorX <= clipRight) {
                float cursorY = textTop;
                float cursorH = textHeight;
                float cursorY1 = std::max(cursorY, clipTop);
                float cursorY2 = std::min(cursorY + cursorH, clipBottom);
                if (cursorY2 > cursorY1) {
                    insertRect(a, c, cursorX, cursorY1, cursorX + cursorW, cursorY2);
                }
            }
        }
    }
}

// x1, y1, x2, y2 is the text box to center the text into
void insertText(
        core::FloatArray& a,
        const core::Color& c,
        float x1, float y1, float x2, float y2,
        float paddingLeft, float paddingRight, float paddingTop, float paddingBottom,
        const std::string& text,
        const graphics::TextProperties& textProperties,
        const graphics::TextCursor& textCursor,
        bool hinting,
        float scrollLeft)
{
    graphics::ShapedText shapedText = shapeText(text);
    insertText(a, c, x1, y1, x2, y2,
               paddingLeft, paddingRight, paddingTop, paddingBottom,
               shapedText, textProperties, textCursor, hinting, scrollLeft);
}

core::Color getColor(const Widget* widget, core::StringId property)
{
    core::Color res;
    style::StyleValue value = widget->style(property);
    if (value.has<core::Color>()) {
        res = value.to<core::Color>();
    }
    return res;
}

float getLength(const Widget* widget, core::StringId property)
{
    float res = 0;
    style::StyleValue value = widget->style(property);
    if (value.type() == style::StyleValueType::Number) {
        res = value.toFloat();
    }
    return res;
}

graphics::SizedFont* getDefaultSizedFont()
{
    return getDefaultSizedFont(15, graphics::FontHinting::Native);
}

graphics::SizedFont* getDefaultSizedFont(Int ppem)
{
    return getDefaultSizedFont(ppem, graphics::FontHinting::Native);
}

graphics::SizedFont* getDefaultSizedFont(Int ppem, graphics::FontHinting hinting)
{
    graphics::Font* font = graphics::fontLibrary()->defaultFont();
    return font->getSizedFont({ppem, hinting});
}

} // namespace vgc::ui::internal
