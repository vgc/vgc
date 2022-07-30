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
    auto white       = StyleValue::custom(core::colors::white);
    auto blueish     = StyleValue::custom(core::Color(0.20, 0.56, 1.0));
    auto transparent = StyleValue::custom(core::colors::transparent);
    auto zero        = StyleValue::number(0.0f);
    auto one         = StyleValue::number(1.0f);
    auto normal      = StyleValue::identifier(strings::normal);
    auto left        = StyleValue::identifier(strings::left);
    auto top         = StyleValue::identifier(strings::top);

    auto table = std::make_shared<style::StylePropertySpecTable>();
    table->insert("background-color",           transparent, false, &parseStyleColor);
    table->insert("background-color-on-hover",  transparent, false, &parseStyleColor);
    table->insert("border-radius",              zero,        false, &parseStyleLength);
    table->insert("margin-bottom",              zero,        false, &parseStyleLength);
    table->insert("margin-left",                zero,        false, &parseStyleLength);
    table->insert("margin-right",               zero,        false, &parseStyleLength);
    table->insert("margin-top",                 zero,        false, &parseStyleLength);
    table->insert("padding-bottom",             zero,        false, &parseStyleLength);
    table->insert("padding-left",               zero,        false, &parseStyleLength);
    table->insert("padding-right",              zero,        false, &parseStyleLength);
    table->insert("padding-top",                zero,        false, &parseStyleLength);
    table->insert("pixel-hinting",              normal,      true,  &parsePixelHinting);
    table->insert("selection-background-color", blueish,     true,  &parseStyleColor);
    table->insert("selection-text-color",       white,       true,  &parseStyleColor);
    table->insert("text-color",                 black,       true,  &parseStyleColor);
    table->insert("text-horizontal-align",      left,        true,  &parseTextHorizontalAlign);
    table->insert("text-vertical-align",        top,         true,  &parseTextVerticalAlign);

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
    , isSelectionVisible_(false)
    , isCursorVisible_(false)
    , selectionBegin_(0)
    , selectionEnd_(0)
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
        Int n = core::int_cast<Int>(text.size());
        core::clamp(selectionBegin_, 0, n);
        core::clamp(selectionEnd_, 0, n);
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

void insertRect(
        core::FloatArray& a,
        const core::Color& c,
        const geometry::Rect2f& r)
{
    insertRect(a, c, r.xMin(), r.yMin(), r.xMax(), r.yMax());
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
    // Early return if nothing to draw
    if (shapedText_.text().length() == 0 && !isCursorVisible_) {
        return;
    }

    // Get style attributes
    // TODO: cache this on style change
    core::Color textColor = getColor(this, strings::text_color);
    core::Color selectionBackgroundColor = getColor(this, strings::selection_background_color);
    core::Color selectionTextColor = getColor(this, strings::selection_text_color);
    bool hinting = getHinting(this, strings::pixel_hinting);
    TextProperties textProperties = getTextProperties(this);
    float r = static_cast<float>(textColor[0]);
    float g = static_cast<float>(textColor[1]);
    float b = static_cast<float>(textColor[2]);
    float sr = static_cast<float>(selectionTextColor[0]);
    float sg = static_cast<float>(selectionTextColor[1]);
    float sb = static_cast<float>(selectionTextColor[2]);
    float paddingLeft = 0;
    float paddingRight = 0;
    float paddingBottom = 0;
    float paddingTop = 0;

    // Compute text geometry
    // TODO: cache this on text change, rect change, or style change
    // Vertical centering
    graphics::SizedFont* sizedFont = shapedText_.sizedFont();
    float height = (rect_.yMax() - paddingBottom) - (rect_.yMin() + paddingTop);
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
        textTop = rect_.yMin() + paddingTop;
        break;
    case graphics::TextVerticalAlign::Middle:
        textTop = rect_.yMin() + paddingTop + 0.5f * (height - textHeight);
        break;
    case graphics::TextVerticalAlign::Bottom:
        textTop = rect_.yMin() + paddingTop + (height - textHeight);
        break;
    }
    if (hinting) {
        textTop = std::round(textTop);
    }
    float baseline = textTop + ascent;
    // Horizontal centering. Note: we intentionally don't perform hinting
    // on the horizontal direction.
    float width = (rect_.xMax() - paddingRight) - (rect_.xMin() + paddingLeft);
    float advance = static_cast<float>(shapedText_.advance()[0]);
    float textLeft = 0;
    switch (textProperties.horizontalAlign()) {
    case graphics::TextHorizontalAlign::Left:
        textLeft = rect_.xMin() + paddingLeft;
        break;
    case graphics::TextHorizontalAlign::Center:
        textLeft = rect_.xMin() + paddingLeft + 0.5f * (width - advance);
        break;
    case graphics::TextHorizontalAlign::Right:
        textLeft = rect_.xMin() + paddingLeft + (width - advance);
        break;
    }
    textLeft -= horizontalScroll_;

    // Set clipping rectangle. For now, we clip at the rect.
    // This might be later disabled with overflow = true or other similar settings.
    geometry::Rect2f clipRect = rect_;
    float clipLeft   = clipRect.xMin();
    float clipRight  = clipRect.xMax();
    float clipTop    = clipRect.yMin();
    float clipBottom = clipRect.yMax();

    // Convert from byte positions to grapheme/glyphs indices and pixel offsets.
    //
    // Note: in some situations, a single glyph can be used to represent
    // multiple graphemes (e.g., an "ff" ligature is one glyph but two
    // graphemes). If the selection only covers some but not all of the glyph,
    // then we ideally need to draw some part of the glyph in textColor and
    // some other part in selectedTextColor. For now, we don't, but we should
    // do it in the future.
    //
    bool hasSelection = selectionBegin_ != selectionEnd_;
    bool hasVisibleSelection = isSelectionVisible_ && hasSelection;
    Int selectionBeginGlyph = -1;
    Int selectionEndGlyph = -1;
    float selectionBeginAdvance = 0;
    float selectionEndAdvance = 0;
    if (isCursorVisible_ || hasVisibleSelection) {
        const graphics::ShapedGraphemeArray& graphemes = shapedText_.graphemes();
        for (const graphics::ShapedGrapheme& grapheme : graphemes) {
            if (grapheme.bytePosition() < selectionBegin_) {
                selectionBeginAdvance += grapheme.advance()[0];
            }
            else if (selectionBeginGlyph == -1) {
                selectionBeginGlyph = grapheme.glyphIndex();
            }
            if (grapheme.bytePosition() < selectionEnd_) {
                selectionEndAdvance += grapheme.advance()[0];
            }
            else if (selectionEndGlyph == -1) {
                selectionEndGlyph = grapheme.glyphIndex();
            }
        }
    }
    if (selectionBeginGlyph == -1) {
        selectionBeginGlyph = shapedText_.glyphs().length();
    }
    if (selectionEndGlyph == -1) {
        selectionEndGlyph = shapedText_.glyphs().length();
    }

    // Draw selection background
    if (hasVisibleSelection) {
        geometry::Rect2f selectionRect(textLeft + selectionBeginAdvance, textTop,
                                       textLeft + selectionEndAdvance, textTop + textHeight);
        selectionRect.normalize();
        selectionRect.intersectWith(clipRect);
        if (!selectionRect.isEmpty()) {
            insertRect(a, selectionBackgroundColor, selectionRect);
        }
    }

    // Draw text
    // TODO: convert selectionBegin_/End_ from byte position to glyph index
    geometry::Vec2f origin(textLeft, baseline);
    if (isSelectionVisible_) {
        if (selectionBeginGlyph > selectionEndGlyph) {
            std::swap(selectionBeginGlyph, selectionEndGlyph);
        }
        shapedText_.fill(a, origin, r, g, b, 0, selectionBeginGlyph, clipLeft, clipRight, clipTop, clipBottom);
        shapedText_.fill(a, origin, sr, sg, sb, selectionBeginGlyph, selectionEndGlyph, clipLeft, clipRight, clipTop, clipBottom);
        shapedText_.fill(a, origin, r, g, b, selectionEndGlyph, shapedText_.glyphs().length(), clipLeft, clipRight, clipTop, clipBottom);
    }
    else {
        shapedText_.fill(a, origin, r, g, b, clipLeft, clipRight, clipTop, clipBottom);
    }

    // Draw cursor
    if (isCursorVisible_) {
        float cursorX = textLeft + selectionEndAdvance;
        float cursorW = 1.0f;
        if (hinting) {
            // Note: while we don't perform horizontal hinting for letters,
            // we do perform horizontal hinting for the cursor.
            cursorX = std::round(cursorX);
        }
        // Ensure that we still draw the cursor when it is just barely
        // in the clipped padding (typically, when the cursor is at the
        // end of the text)
        clipLeft -= cursorW;
        clipRight += cursorW;
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

std::string RichText::selectedText() const {
    return std::string(selectedTextView());
}

std::string_view RichText::selectedTextView() const {
    Int begin = selectionBegin_;
    Int end = selectionEnd_;
    if (begin > end) {
        std::swap(begin, end);
    }
    std::string_view res(text_);
    return res.substr(begin, end - begin);
}

void RichText::deleteSelectedText()
{
    if (hasSelection()) {
        if (selectionBegin_ > selectionEnd_) {
            std::swap(selectionBegin_, selectionEnd_);
        }
        size_t p1 = core::int_cast<size_t>(selectionBegin_);
        size_t p2 = core::int_cast<size_t>(selectionEnd_);
        selectionEnd_ = selectionBegin_;
        std::string newText;
        newText.reserve(text().size() - (p2 - p1));
        newText.append(text(), 0, p1);
        newText.append(text(), p2);
        setText(newText);
    }
}

void RichText::deleteNext(TextBoundaryType boundaryType)
{
    if (!hasSelection()) {
        graphics::TextBoundaryIterator it(boundaryType, text());
        it.setPosition(selectionBegin_);
        selectionEnd_ = it.toNextBoundary();
        if (selectionEnd_ == -1) {
            selectionEnd_ = selectionBegin_;
        }
    }
    deleteSelectedText();
}

void RichText::deletePrevious(TextBoundaryType boundaryType)
{
    if (!hasSelection()) {
        graphics::TextBoundaryIterator it(boundaryType, text());
        it.setPosition(selectionEnd_);
        selectionBegin_ = it.toPreviousBoundary();
        if (selectionBegin_ == -1) {
            selectionBegin_ = selectionEnd_;
        }
    }
    deleteSelectedText();
}

Int RichText::bytePosition(const geometry::Vec2f& mousePosition)
{
    // TODO: take horizontal/vertical style alignment into account
    // (see implementation of fill())
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
