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
    , children_(RichTextSpanList::create(this)) {
}

RichTextSpanPtr RichTextSpan::createRoot() {
    return RichTextSpanPtr(new RichTextSpan());
}

RichTextSpan* RichTextSpan::createChild() {
    RichTextSpanPtr childPtr(new RichTextSpan(this));
    RichTextSpan* child = childPtr.get();
    children_->append(child);
    return child;
}

style::StylableObject* RichTextSpan::parentStylableObject() const {
    return static_cast<style::StylableObject*>(parent());
}

style::StylableObject* RichTextSpan::firstChildStylableObject() const {
    return static_cast<style::StylableObject*>(firstChild());
}

style::StylableObject* RichTextSpan::lastChildStylableObject() const {
    return static_cast<style::StylableObject*>(lastChild());
}

style::StylableObject* RichTextSpan::previousSiblingStylableObject() const {
    return static_cast<style::StylableObject*>(previousSibling());
}

style::StylableObject* RichTextSpan::nextSiblingStylableObject() const {
    return static_cast<style::StylableObject*>(nextSibling());
}

namespace {

using style::StyleTokenIterator;
using style::StyleTokenType;
using style::StyleValue;
using style::StyleValueType;

StyleValue parseStyleColor(StyleTokenIterator begin, StyleTokenIterator end) {
    try {
        std::string str(begin->begin, (end - 1)->end);
        core::Color color = core::parse<core::Color>(str);
        return StyleValue::custom(color);
    }
    catch (const core::ParseError&) {
        return StyleValue::invalid();
    }
    catch (const core::RangeError&) {
        return StyleValue::invalid();
    }
}

StyleValue parseStyleLength(StyleTokenIterator begin, StyleTokenIterator end) {
    // For now, we only support a unique Dimension token with a "dp" unit
    if (begin == end) {
        return StyleValue::invalid();
    }
    else if (
        begin->type == StyleTokenType::Dimension //
        && begin->codePointsValue == "dp"        //
        && begin + 1 == end) {

        return StyleValue::number(begin->toFloat());
    }
    else {
        return StyleValue::invalid();
    }
}

StyleValue parseIdentifierAmong(
    StyleTokenIterator begin,
    StyleTokenIterator end,
    std::initializer_list<core::StringId> list) {

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

StyleValue parsePixelHinting(StyleTokenIterator begin, StyleTokenIterator end) {
    using namespace strings;
    return parseIdentifierAmong(begin, end, {off, normal});
}

StyleValue parseTextHorizontalAlign(StyleTokenIterator begin, StyleTokenIterator end) {
    using namespace strings;
    return parseIdentifierAmong(begin, end, {left, center, right});
}

StyleValue parseTextVerticalAlign(StyleTokenIterator begin, StyleTokenIterator end) {
    using namespace strings;
    return parseIdentifierAmong(begin, end, {top, middle, bottom});
}

// clang-format off

style::StylePropertySpecTablePtr createGlobalStylePropertySpecTable_() {

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
    table->insert("border-color",               black,       false, &parseStyleColor);
    table->insert("border-radius",              zero,        false, &parseStyleLength);
    table->insert("border-width",               zero,        false, &parseStyleLength);
    table->insert("caret-color",                black,       true,  &parseStyleColor);
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

// clang-format on

const style::StylePropertySpecTablePtr& stylePropertySpecTable_() {
    static style::StylePropertySpecTablePtr table = createGlobalStylePropertySpecTable_();
    return table;
}

style::StyleSheetPtr createGlobalStyleSheet_() {
    return style::StyleSheet::create(stylePropertySpecTable_(), "");
}

} // namespace

const style::StyleSheet* RichTextSpan::defaultStyleSheet() const {
    static style::StyleSheetPtr s = createGlobalStyleSheet_();
    return s.get();
}

const style::StylePropertySpecTable* RichTextSpan::stylePropertySpecs() {
    return stylePropertySpecTable_().get();
}

namespace {

graphics::SizedFont* getDefaultSizedFont_(Int ppem, graphics::FontHinting hinting) {
    graphics::Font* font = graphics::fontLibrary()->defaultFont();
    return font->getSizedFont({ppem, hinting});
}

graphics::SizedFont* getDefaultSizedFont_() {
    return getDefaultSizedFont_(15, graphics::FontHinting::Native);
}

} // namespace

RichText::RichText()
    : RichTextSpan()
    , parentStylableObject_(nullptr)
    , text_()
    , shapedText_(getDefaultSizedFont_(), "")
    , isSelectionVisible_(false)
    , isCursorVisible_(false)
    , selectionStart_(0)
    , selectionEnd_(0)
    , horizontalScroll_(0.0f) {
}

RichText::RichText(std::string_view text)
    : RichText() {

    insertText_(text);
    selectionStart_ = 0;
    selectionEnd_ = 0;
    updateScroll_();
}

RichTextPtr RichText::create() {
    return RichTextPtr(new RichText());
}

RichTextPtr RichText::create(std::string_view text) {
    return RichTextPtr(new RichText(text));
}

void RichText::setText(std::string_view text) {
    if (text_ != text) {
        Int oldSelectionEnd = selectionEnd_;
        text_.clear();
        insertText_(text);
        Int minPosition = shapedText_.minPosition();
        Int maxPosition = shapedText_.maxPosition();
        selectionEnd_ = core::clamp(oldSelectionEnd, minPosition, maxPosition);
        selectionStart_ = selectionEnd_;
        updateScroll_();
    }
}

namespace {

// clang-format off

void insertRect(
        core::FloatArray& a,
        const core::Color& c,
        float x1, float y1, float x2, float y2) {

    float r = static_cast<float>(c[0]);
    float g = static_cast<float>(c[1]);
    float b = static_cast<float>(c[2]);

    a.extend({
        x1, y1, r, g, b,
        x2, y1, r, g, b,
        x1, y2, r, g, b,
        x2, y1, r, g, b,
        x2, y2, r, g, b,
        x1, y2, r, g, b});
}

// clang-format on

void insertRect(core::FloatArray& a, const core::Color& c, const geometry::Rect2f& r) {
    insertRect(a, c, r.xMin(), r.yMin(), r.xMax(), r.yMax());
}

core::Color getColor(const RichTextSpan* span, core::StringId property) {
    core::Color res;
    style::StyleValue value = span->style(property);
    if (value.has<core::Color>()) {
        res = value.to<core::Color>();
    }
    return res;
}

bool getHinting(const RichTextSpan* span, core::StringId property) {
    return span->style(property) == strings::normal;
}

TextProperties getTextProperties(const RichTextSpan* span) {

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

void RichText::fill(core::FloatArray& a) {

    // Early return if nothing to draw
    if (shapedText_.text().length() == 0 && !isCursorVisible_) {
        return;
    }

    // Get style attributes
    // TODO: cache this on style change
    core::Color caretColor = getColor(this, strings::caret_color);
    core::Color textColor = getColor(this, strings::text_color);
    core::Color selectionBackgroundColor =
        getColor(this, strings::selection_background_color);
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
    float clipLeft = clipRect.xMin();
    float clipRight = clipRect.xMax();
    float clipTop = clipRect.yMin();
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
    bool hasSelection = selectionStart_ != selectionEnd_;
    bool hasVisibleSelection = isSelectionVisible_ && hasSelection;
    Int selectionBeginGlyph = -1;
    Int selectionEndGlyph = -1;
    float selectionBeginAdvance = 0;
    float selectionEndAdvance = 0;
    if (isCursorVisible_ || hasVisibleSelection) {
        using Info = graphics::ShapedTextPositionInfo;
        Info infoBegin = shapedText_.positionInfo(selectionStart_);
        Info infoEnd = shapedText_.positionInfo(selectionEnd_);
        selectionBeginGlyph = infoBegin.glyphIndex();
        selectionEndGlyph = infoEnd.glyphIndex();
        selectionBeginAdvance = infoBegin.advance()[0];
        selectionEndAdvance = infoEnd.advance()[0];
    }

    // Draw selection background
    if (hasVisibleSelection) {
        geometry::Rect2f selectionRect(
            textLeft + selectionBeginAdvance,
            textTop,
            textLeft + selectionEndAdvance,
            textTop + textHeight);
        selectionRect.normalize();
        selectionRect.intersectWith(clipRect);
        if (!selectionRect.isEmpty()) {
            insertRect(a, selectionBackgroundColor, selectionRect);
        }
    }

    // Draw text
    geometry::Vec2f origin(textLeft, baseline);
    if (hasVisibleSelection) {
        if (selectionBeginGlyph > selectionEndGlyph) {
            std::swap(selectionBeginGlyph, selectionEndGlyph);
        }
        // clang-format off
        shapedText_.fill(
            a, origin, r, g, b, 0, selectionBeginGlyph,
            clipLeft, clipRight, clipTop, clipBottom);
        shapedText_.fill(
            a, origin, sr, sg, sb, selectionBeginGlyph, selectionEndGlyph,
            clipLeft, clipRight, clipTop, clipBottom);
        shapedText_.fill(
            a, origin, r, g, b, selectionEndGlyph, shapedText_.glyphs().length(),
            clipLeft, clipRight, clipTop, clipBottom);
        // clang-format on
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
        // partially visible in the horizontal direction, we draw it full-length
        if (clipLeft <= cursorX && cursorX <= clipRight) {
            float cursorY = textTop;
            float cursorH = textHeight;
            float cursorY1 = (std::max)(cursorY, clipTop);
            float cursorY2 = (std::min)(cursorY + cursorH, clipBottom);
            if (cursorY2 > cursorY1) {
                insertRect(a, caretColor, cursorX, cursorY1, cursorX + cursorW, cursorY2);
            }
        }
    }
}

Int RichText::movedPosition(
    Int position,
    RichTextMoveOperation operation,
    Int /* selectionIndex */) {

    using Op = RichTextMoveOperation;

    switch (operation) {
    case Op::NoMove:
        return position;
    case Op::StartOfLine:
        // TODO: update this when adding support for multiline text
        return shapedText_.minPosition();
    case Op::StartOfText:
        return shapedText_.minPosition();
    case Op::StartOfSelection:
        return (std::min)(selectionStart_, selectionEnd_);
    case Op::EndOfLine:
        // TODO: update this when adding support for multiline text
        return shapedText_.maxPosition();
    case Op::EndOfText:
        return shapedText_.maxPosition();
    case Op::EndOfSelection:
        return (std::max)(selectionStart_, selectionEnd_);
    case Op::PreviousCharacter:
        return shapedText_.previousBoundary(position, TextBoundaryMarker::Grapheme);
    case Op::PreviousWord:
        return shapedText_.previousBoundary(position, TextBoundaryMarker::Word);
    case Op::NextCharacter:
        return shapedText_.nextBoundary(position, TextBoundaryMarker::Grapheme);
    case Op::NextWord:
        return shapedText_.nextBoundary(position, TextBoundaryMarker::Word);
    // TODO: map left/right to next/previous if text direction is rtl
    case Op::LeftOneCharacter:
        return shapedText_.previousBoundary(position, TextBoundaryMarker::Grapheme);
    case Op::LeftOneWord:
        return shapedText_.previousBoundary(position, TextBoundaryMarker::Word);
    case Op::LeftOfSelection:
        return (std::min)(selectionStart_, selectionEnd_);
    case Op::RightOneCharacter:
        return shapedText_.nextBoundary(position, TextBoundaryMarker::Grapheme);
    case Op::RightOneWord:
        return shapedText_.nextBoundary(position, TextBoundaryMarker::Word);
    case Op::RightOfSelection:
        return (std::max)(selectionStart_, selectionEnd_);
    default:
        // Silence compiler warning
        return position;
    }
}

void RichText::moveCursor(RichTextMoveOperation operation, bool select) {
    selectionEnd_ = movedPosition(selectionEnd_, operation);
    if (!select) {
        selectionStart_ = selectionEnd_;
    }
    updateScroll_();
}

std::string RichText::selectedText() const {
    return std::string(selectedTextView());
}

std::string_view RichText::selectedTextView() const {
    Int begin = shapedText_.positionInfo(selectionStart_).byteIndex();
    Int end = shapedText_.positionInfo(selectionEnd_).byteIndex();
    if (begin > end) {
        std::swap(begin, end);
    }
    size_t p1 = static_cast<size_t>(begin);
    size_t p2 = static_cast<size_t>(end);
    std::string_view res(text_);
    return res.substr(p1, p2 - p1);
}

void RichText::deleteSelectedText() {
    if (hasSelection()) {
        if (selectionStart_ > selectionEnd_) {
            std::swap(selectionStart_, selectionEnd_);
        }
        Int begin = shapedText_.positionInfo(selectionStart_).byteIndex();
        Int end = shapedText_.positionInfo(selectionEnd_).byteIndex();
        size_t p1 = static_cast<size_t>(begin);
        size_t p2 = static_cast<size_t>(end);
        text_.erase(p1, p2 - p1);
        shapedText_.setText(text_);
        selectionStart_ = core::clamp(
            selectionStart_, shapedText_.minPosition(), shapedText_.maxPosition());
        selectionEnd_ = selectionStart_;
        updateScroll_();
    }
}

void RichText::deleteFromCursor(RichTextMoveOperation operation) {
    if (!hasSelection()) {
        moveCursor(operation, true);
    }
    deleteSelectedText();
}

void RichText::insertText(std::string_view text) {
    if (hasSelection()) {
        deleteSelectedText();
    }
    insertText_(text);
    updateScroll_();
}

Int RichText::positionFromPoint(
    const geometry::Vec2f& point,
    TextBoundaryMarkers boundaryMarkers) {

    // TODO: take horizontal/vertical style alignment into account
    // (see implementation of fill())
    float x = point[0] + horizontalScroll_;
    float y = point[1];
    return shapedText_.positionFromPoint({x, y}, boundaryMarkers);
}

std::pair<Int, Int> RichText::positionPairFromPoint(
    const geometry::Vec2f& point,
    TextBoundaryMarkers boundaryMarkers) {
    // TODO: take horizontal/vertical style alignment into account
    // (see implementation of fill())
    float x = point[0] + horizontalScroll_;
    float y = point[1];
    return shapedText_.positionPairFromPoint({x, y}, boundaryMarkers);
}

geometry::Vec2f RichText::advance_(Int position) const {
    return shapedText_.advance(position);
}

float RichText::maxCursorHorizontalAdvance_() const {
    return shapedText_.advance()[0];
}

void RichText::updateScroll_() {
    float textWidth = rect_.width();
    if (horizontalScroll_ > 0) {
        float textEndAdvance = maxCursorHorizontalAdvance_();
        float currentTextEndPos = textEndAdvance - horizontalScroll_;
        if (currentTextEndPos < textWidth) {
            if (textEndAdvance < textWidth) {
                horizontalScroll_ = 0;
            }
            else {
                horizontalScroll_ = textEndAdvance - textWidth;
            }
        }
    }
    if (isCursorVisible()) {
        float cursorAdvance = advance_(selectionEnd_)[0];
        float currentCursorPos = cursorAdvance - horizontalScroll_;
        if (currentCursorPos < 0) {
            horizontalScroll_ = cursorAdvance;
        }
        else if (currentCursorPos > textWidth) {
            horizontalScroll_ = cursorAdvance - textWidth;
        }
    }
}

// Inserts text at the current cursor position. The new cursor position
// becomes the end of the inserted text and the selection is cleared.
//
void RichText::insertText_(std::string_view textToInsert) {

    // Get number of bytes to insert after removing unsupported characters
    size_t numBytesToInsert = 0;
    for (char c : textToInsert) {
        if (c < 0 || c >= 32) {
            ++numBytesToInsert;
        }
    }

    if (numBytesToInsert > 0) {

        // Get byte index where to insert the text
        Int oldByteIndex = shapedText_.positionInfo(selectionEnd_).byteIndex();
        size_t insertPos = static_cast<size_t>(oldByteIndex);

        // Resize text_, making space for the text to insert
        size_t oldSize = text_.size();
        size_t numBytesToShift = oldSize - insertPos;
        text_.resize(oldSize + numBytesToInsert);
        size_t newSize = text_.size();
        for (size_t i = 1; i <= numBytesToShift; ++i) {
            text_[newSize - i] = text_[oldSize - i];
        }

        // Insert the bytes
        char* cp = &text_[insertPos];
        for (char c : textToInsert) {
            if (c < 0 || c >= 32) {
                *cp = c;
                ++cp;
            }
        }

        // Update shaped text
        shapedText_.setText(text_);

        // Update cursor position
        selectionEnd_ = shapedText_.positionFromByte(oldByteIndex + numBytesToInsert);
    }

    // Clear selection
    selectionStart_ = selectionEnd_;
}

} // namespace vgc::graphics
