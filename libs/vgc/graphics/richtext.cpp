// Copyright 2022 The VGC Developers
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

#include <vgc/graphics/richtext.h>

#include <vgc/core/colors.h>
#include <vgc/graphics/strings.h>

namespace vgc::graphics {

RichTextSpan::RichTextSpan(RichTextSpan* parent)
    : StylableObject()
    , parent_(parent)
    , root_(parent ? parent->root() : this)
    , children_(RichTextSpanList::create(this))
{
}

RichTextSpanPtr RichTextSpan::createRoot()
{
    return RichTextSpanPtr(new RichTextSpan());
}

RichTextSpan* RichTextSpan::createChild()
{
    RichTextSpanPtr childPtr(new RichTextSpan(this));
    RichTextSpan* child = childPtr.get();
    children_->append(child);
    return child;
}

style::StylableObject* RichTextSpan::parentStylableObject() const
{
    return static_cast<style::StylableObject*>(parent());
}

style::StylableObject* RichTextSpan::firstChildStylableObject() const
{
    return static_cast<style::StylableObject*>(firstChild());
}

style::StylableObject* RichTextSpan::lastChildStylableObject() const
{
    return static_cast<style::StylableObject*>(lastChild());
}

style::StylableObject* RichTextSpan::previousSiblingStylableObject() const
{
    return static_cast<style::StylableObject*>(previousSibling());
}

style::StylableObject* RichTextSpan::nextSiblingStylableObject() const
{
    return static_cast<style::StylableObject*>(nextSibling());
}

namespace {

using style::StyleValue;
using style::StyleValueType;
using style::StyleTokenIterator;
using style::StyleTokenType;

StyleValue parseStyleColor(StyleTokenIterator begin, StyleTokenIterator end)
{
    try {
        std::string str(begin->begin, (end-1)->end);
        core::Color color = core::parse<core::Color>(str);
        return StyleValue::custom(color);
    } catch (const core::ParseError&) {
        return StyleValue::invalid();
    } catch (const core::RangeError&) {
        return StyleValue::invalid();
    }
}

StyleValue parseStyleLength(StyleTokenIterator begin, StyleTokenIterator end)
{
    // For now, we only support a unique Dimension token with a "dp" unit
    if (begin == end) {
        return StyleValue::invalid();
    }
    else if (begin->type == StyleTokenType::Dimension &&
             begin->codePointsValue == "dp" &&
             begin + 1 == end) {
        return StyleValue::number(begin->toFloat());
    }
    else {
        return StyleValue::invalid();
    }
}

StyleValue parseIdentifierAmong(StyleTokenIterator begin, StyleTokenIterator end,
                                std::initializer_list<core::StringId> list)
{
    if (end == begin + 1) {
        StyleTokenType t = begin->type;
        if (t == StyleTokenType::Identifier) {
            for (core::StringId id : list) {
                if (id == begin->codePointsValue) {
                    return StyleValue::identifier(begin->codePointsValue);
                }
            }
        }
    }
    return StyleValue::invalid();
}

StyleValue parsePixelHinting(StyleTokenIterator begin, StyleTokenIterator end)
{
    using namespace strings;
    return parseIdentifierAmong(begin, end, {off, normal});
}

StyleValue parseTextHorizontalAlign(StyleTokenIterator begin, StyleTokenIterator end)
{
    using namespace strings;
    return parseIdentifierAmong(begin, end, {left, center, right});
}

StyleValue parseTextVerticalAlign(StyleTokenIterator begin, StyleTokenIterator end)
{
    using namespace strings;
    return parseIdentifierAmong(begin, end, {top, middle, bottom});
}

style::StylePropertySpecTablePtr createGlobalStylePropertySpecTable_()
{
    // Reference: https://www.w3.org/TR/CSS21/propidx.html
    auto black       = StyleValue::custom(core::colors::black);
    auto transparent = StyleValue::custom(core::colors::transparent);
    auto zero        = StyleValue::number(0.0f);
    auto one         = StyleValue::number(1.0f);
    auto normal      = StyleValue::identifier(strings::normal);
    auto left        = StyleValue::identifier(strings::left);
    auto top         = StyleValue::identifier(strings::top);

    auto table = std::make_shared<style::StylePropertySpecTable>();
    table->insert("background-color",          transparent, false, &parseStyleColor);
    table->insert("background-color-on-hover", transparent, false, &parseStyleColor);
    table->insert("border-radius",             zero,        false, &parseStyleLength);
    table->insert("margin-bottom",             zero,        false, &parseStyleLength);
    table->insert("margin-left",               zero,        false, &parseStyleLength);
    table->insert("margin-right",              zero,        false, &parseStyleLength);
    table->insert("margin-top",                zero,        false, &parseStyleLength);
    table->insert("padding-bottom",            zero,        false, &parseStyleLength);
    table->insert("padding-left",              zero,        false, &parseStyleLength);
    table->insert("padding-right",             zero,        false, &parseStyleLength);
    table->insert("padding-top",               zero,        false, &parseStyleLength);
    table->insert("pixel-hinting",             normal,      false, &parsePixelHinting);
    table->insert("text-color",                black,       true,  &parseStyleColor);
    table->insert("text-horizontal-align",     left,        true,  &parseTextHorizontalAlign);
    table->insert("text-vertical-align",       top,         true,  &parseTextVerticalAlign);

    return table;
}

const style::StylePropertySpecTablePtr& stylePropertySpecTable_()
{
    static style::StylePropertySpecTablePtr table = createGlobalStylePropertySpecTable_();
    return table;
}

style::StyleSheetPtr createGlobalStyleSheet_() {
    return style::StyleSheet::create(stylePropertySpecTable_(), "");
}

} // namespace

const style::StyleSheet* RichTextSpan::defaultStyleSheet() const
{
    static style::StyleSheetPtr s = createGlobalStyleSheet_();
    return s.get();
}

const style::StylePropertySpecTable* RichTextSpan::stylePropertySpecs() {
    return stylePropertySpecTable_().get();
}

namespace {

graphics::SizedFont* getDefaultSizedFont_(Int ppem, graphics::FontHinting hinting)
{
    graphics::Font* font = graphics::fontLibrary()->defaultFont();
    return font->getSizedFont({ppem, hinting});
}

graphics::SizedFont* getDefaultSizedFont_()
{
    return getDefaultSizedFont_(15, graphics::FontHinting::Native);
}

} // namespace

RichText::RichText(std::string_view text)
    : parentStylableObject_(nullptr)
    , text_(text)
    , shapedText_(getDefaultSizedFont_(), text)
    , textCursor_(false, 0)
    , horizontalScroll_(0.0f)
{
}

RichTextPtr RichText::create()
{
    return RichTextPtr(new RichText(std::string_view()));
}

RichTextPtr RichText::create(std::string_view text)
{
    return RichTextPtr(new RichText(text));
}

void RichText::setText(std::string_view text)
{
    if (text_ != text) {
        text_ = text;
        shapedText_.setText(text);
        updateScroll_();
    }
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

void insertText(core::FloatArray& a,
                const graphics::ShapedText& shapedText,
                const core::Color& textColor,
                float x1, float y1, float x2, float y2,
                float paddingLeft, float paddingRight, float paddingTop, float paddingBottom,
                const graphics::TextProperties& textProperties,
                const graphics::TextCursor& textCursor,
                bool hinting,
                float scrollLeft)
{
    // Draw text
    graphics::SizedFont* sizedFont = shapedText.sizedFont();
    if (shapedText.text().length() > 0 || textCursor.isVisible()) {
        float r = static_cast<float>(textColor[0]);
        float g = static_cast<float>(textColor[1]);
        float b = static_cast<float>(textColor[2]);

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
                    insertRect(a, textColor, cursorX, cursorY1, cursorX + cursorW, cursorY2);
                }
            }
        }
    }
}

core::Color getColor(const RichTextSpan* span, core::StringId property)
{
    core::Color res;
    style::StyleValue value = span->style(property);
    if (value.has<core::Color>()) {
        res = value.to<core::Color>();
    }
    return res;
}

bool getHinting(const RichTextSpan* span, core::StringId property)
{
    return span->style(property) == strings::normal;
}

TextProperties getTextProperties(const RichTextSpan* span)
{
    style::StyleValue hAlign = span->style(strings::text_horizontal_align);
    style::StyleValue vAlign = span->style(strings::text_vertical_align);
    TextProperties properties; // default = (Left, Top)

    if (hAlign.type() == StyleValueType::Identifier) {
        core::StringId s = hAlign.toStringId();
        if (s == strings::left) {
            properties.setHorizontalAlign(TextHorizontalAlign::Left);
        }
        else if (s == strings::center) {
            properties.setHorizontalAlign(TextHorizontalAlign::Center);
        }
        else if (s == strings::right) {
            properties.setHorizontalAlign(TextHorizontalAlign::Right);
        }
    }

    if (vAlign.type() == StyleValueType::Identifier) {
        core::StringId s = vAlign.toStringId();
        if (s == strings::top) {
            properties.setVerticalAlign(TextVerticalAlign::Top);
        }
        else if (s == strings::middle) {
            properties.setVerticalAlign(TextVerticalAlign::Middle);
        }
        else if (s == strings::bottom) {
            properties.setVerticalAlign(TextVerticalAlign::Bottom);
        }
    }

    return properties;
}

} // namespace

void RichText::fill(core::FloatArray& a)
{
    core::Color textColor = getColor(this, strings::text_color);
    bool hinting = getHinting(this, strings::pixel_hinting);
    TextProperties textProperties = getTextProperties(this);
    insertText(a, shapedText_,
               textColor,
               rect_.xMin(), rect_.yMin(), rect_.xMax(), rect_.yMax(),
               0, 0, 0, 0,
               textProperties, textCursor_, hinting, horizontalScroll_);
}

Int RichText::bytePosition(const geometry::Vec2f& mousePosition)
{
    float x = mousePosition[0] + horizontalScroll_;
    float y = mousePosition[1];
    return shapedText_.bytePosition({x, y});
}

geometry::Vec2f RichText::graphemeAdvance_(Int bytePosition) const
{
    return shapedText_.advance(bytePosition);
}

float RichText::maxCursorHorizontalAdvance_() const
{
    return shapedText_.advance()[0];
}

void RichText::updateScroll_()
{
    float textWidth = rect_.width();
    float textEndAdvance = maxCursorHorizontalAdvance_();
    float currentTextEndPos = textEndAdvance - horizontalScroll_;
    if (currentTextEndPos < textWidth && horizontalScroll_ > 0) {
        if (textEndAdvance < textWidth) {
            horizontalScroll_ = 0;
        }
        else {
            horizontalScroll_ = textEndAdvance - textWidth;
        }
    }
    if (isCursorVisible()) {
        float cursorAdvance = graphemeAdvance_(cursorBytePosition())[0];
        float currentCursorPos = cursorAdvance - horizontalScroll_;
        if (currentCursorPos < 0) {
            horizontalScroll_ = cursorAdvance;
        }
        else if (currentCursorPos > textWidth) {
            horizontalScroll_ = cursorAdvance - textWidth;
        }
    }
}

} // namespace vgc::graphics
