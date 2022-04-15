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

#include <vgc/graphics/text.h>

#include <hb.h>
#include <hb-ft.h>

#include <QTextBoundaryFinder>

namespace vgc {
namespace graphics {

namespace internal {

class ShapedTextImpl {
public:
    // Input of shaping.
    //
    // Note that that we store a FontFacePtr to keep both the FontFace and the
    // FontLibrary alive. Note that the FontLibrary never destroys its
    // children: a created FontFace stays in memory forever until the library
    // itself is destroyed().
    //
    // XXX thread-safety? What if two threads create two ShapedTexts at the
    //     same time, wouldn't this cause a race condition when increasing
    //     the reference counts? Shouldn't we make the reference counts atomic,
    //     or else protect the assignment to this facePtr by a mutex?
    //
    FontFacePtr facePtr;
    std::string text;

    // Output of shaping
    //
    ShapedGlyphArray glyphs;
    ShapedGraphemeArray graphemes;
    core::Vec2d advance;

    // HarbBuzz buffer
    //
    hb_buffer_t* buf;

    ShapedTextImpl(FontFace* face, const std::string& text) :
        facePtr(face),
        text(),
        glyphs(),
        graphemes(),
        advance(0, 0),
        buf(hb_buffer_create())
    {
        setText(text);
    }

    ShapedTextImpl(const ShapedTextImpl& other) :
        facePtr(other.facePtr),
        text(other.text),
        glyphs(other.glyphs),
        graphemes(other.graphemes),
        advance(other.advance),
        buf(hb_buffer_create())
    {

    }

    ShapedTextImpl& operator=(const ShapedTextImpl& other)
    {
        if (this != &other) {
            facePtr = other.facePtr;
            text = other.text;
            glyphs = other.glyphs;
            graphemes = other.graphemes;
            advance = other.advance;
        }
        return *this;
    }

    ~ShapedTextImpl()
    {
        hb_buffer_destroy(buf);
    }

    void setText(const std::string& text_)
    {
        // HarfBuzz input
        text = text_;
        const char* data = text.data();
        int dataLength = core::int_cast<int>(text.size());
        unsigned int firstChar = 0;
        int numChars = dataLength;

        // Shape
        hb_buffer_reset(buf);
        hb_buffer_set_cluster_level(buf, HB_BUFFER_CLUSTER_LEVEL_MONOTONE_CHARACTERS);
        hb_buffer_add_utf8(buf, data, dataLength, firstChar, numChars);
        hb_buffer_guess_segment_properties(buf);
        hb_shape(facePtr->impl_->hbFont, buf, nullptr, 0);

        // HarfBuzz output
        unsigned int n;
        hb_glyph_info_t* infos = hb_buffer_get_glyph_infos(buf, &n);
        hb_glyph_position_t* positions = hb_buffer_get_glyph_positions(buf, &n);

        // Convert to ShapedGlyph elements
        glyphs.clear();
        advance = core::Vec2d(0, 0);
        for (unsigned int i = 0; i < n; ++i) {
            hb_glyph_info_t& info = infos[i];
            hb_glyph_position_t& pos = positions[i];
            FontGlyph* glyph = facePtr->getGlyphFromIndex(info.codepoint);
            Int bytePosition = core::int_cast<Int>(info.cluster);
            core::Vec2d glyphOffset = internal::f266ToVec2d(pos.x_offset, pos.y_offset);
            core::Vec2d glyphAdvance = internal::f266ToVec2d(pos.x_advance, pos.y_advance);
            core::Vec2d glyphPosition = advance + glyphOffset;
            if (glyph) {
                glyphs.append(ShapedGlyph(glyph, glyphOffset, glyphAdvance, glyphPosition, bytePosition));
            }
            advance += glyphAdvance;
        }

        // Compute graphemes and correspondence with glyphs
        // First pass: create graphemes with corresponding byte position
        graphemes.clear();
        TextBoundaryIterator it(TextBoundaryType::Grapheme, text);
        Int bytePositionBefore = 0;
        Int bytePositionAfter = it.toNextBoundary();
        while (bytePositionAfter != -1) {
            graphemes.append(ShapedGrapheme(0, core::Vec2d(0, 0), core::Vec2d(0, 0), bytePositionBefore));
            bytePositionBefore = bytePositionAfter;
            bytePositionAfter = it.toNextBoundary();
        }
        // Second pass: compute the smallest glyph corresponding to each grapheme
        Int numGraphemes = graphemes.length();
        Int numGlyphs = glyphs.length();
        if (numGraphemes > 0 && numGlyphs > 0) {
            Int graphemeIndex = 0;
            Int glyphIndex = 0;
            Int numBytes = core::int_cast<Int>(text.size());
            for (Int p = 0; p < numBytes; ++p) {
                while (glyphs[glyphIndex].bytePosition() < p &&
                       glyphIndex + 1 < numGlyphs &&
                       glyphs[glyphIndex + 1].bytePosition() <= p) {
                    glyphIndex += 1;
                }
                while (graphemes[graphemeIndex].bytePosition() < p &&
                       graphemeIndex + 1 < numGraphemes &&
                       graphemes[graphemeIndex + 1].bytePosition() <= p) {
                    graphemeIndex += 1;
                    graphemes[graphemeIndex].glyphIndex_ = glyphIndex;
                 }
            }
        }
        // Third pass: compute the grapheme advance by computing the sum of the
        // glyph advances (if several glyphs for one grapheme, e.g., accents),
        // or uniformly dividing the glyph advance by the number of graphemes
        // (if one glyph for several graphemes, e.g., ligatures).
        for (Int graphemeIndex = 0; graphemeIndex < numGraphemes; ++graphemeIndex) {
            Int glyphIndexBegin = graphemes[graphemeIndex].glyphIndex();
            Int glyphIndexEnd = (graphemeIndex + 1 < numGraphemes) ?
                                graphemes[graphemeIndex + 1].glyphIndex() :
                                numGlyphs;
            if (glyphIndexBegin == glyphIndexEnd) {
                // one glyph for several graphemes
                Int glyphIndex = glyphIndexBegin;
                Int graphemeIndexBegin = graphemeIndex;
                while (graphemeIndex + 1 < numGraphemes &&
                       graphemes[graphemeIndex + 1].glyphIndex() == glyphIndex) {
                    graphemeIndex += 1;
                }
                core::Vec2d glyphAdvance = glyphs[glyphIndexBegin].advance();
                Int numGraphemesForGlyph = graphemeIndex - graphemeIndexBegin + 1;
                core::Vec2d graphemeAdvance = glyphAdvance / static_cast<double>(numGraphemesForGlyph);
                for (Int k = graphemeIndexBegin; k <= graphemeIndex; ++k) {
                    graphemes[k].advance_ = graphemeAdvance;
                }
            }
            else {
                // one grapheme for one or several glyphs
                core::Vec2d graphemeAdvance(0, 0);
                for (Int i = glyphIndexBegin; i < glyphIndexEnd; ++i) {
                    graphemeAdvance += glyphs[i].advance();
                }
                graphemes[graphemeIndex].advance_ = graphemeAdvance;
            }
        }
        // Fourth pass: compute position based on advance
        core::Vec2d graphemePosition(0, 0);
        for (Int graphemeIndex = 0; graphemeIndex < numGraphemes; ++graphemeIndex) {
            graphemes[graphemeIndex].position_ = graphemePosition;
            graphemePosition += graphemes[graphemeIndex].advance();
        }
    }

private:
    friend class graphics::ShapedText;
};

} // namespace internal

void ShapedGlyph::fill(core::FloatArray& data,
                       const core::Vec2d& origin,
                       float r, float g, float b) const
{
    // Note: we currently disable per-letter hinting (which can be seen as a
    // component of horizontal hinting) because it looked worse, at least with
    // the current implementation. It produced uneven spacing between letters,
    // making kerning look bad.
    constexpr bool hinting = false;

    // Triangulate the glyph in local glyph coordinates.
    //
    // Note: for performance, it would be better to avoid dynamic allocations
    // by using a unique buffer when calling ShapedText::fill(), rather than
    // creating a new one for each ShapedGlyph. For now, it probably doesn't
    // matter much though, since the current implementation of Curves2d::fill()
    // does a lot of dynamic allocations anyway (for curve resampling and
    // triangulation). It might be a good idea to pass to all functions a
    // generic "geometry::Buffer", or perhaps a "core::Allocator".
    //
    // Another option would be to simply cache the triangulation within the
    // FontGlyph.
    //
    core::DoubleArray localFill;
    fontGlyph()->outline().fill(localFill);

    // Apply local-to-global transform and populate the output array.
    //
    // XXX: Is the Y-axis of position() and offset() currently pointing up or
    // down? Do we want it be pointing up or down? In our current tested usage,
    // all these vertical values are always zero, so it doesn't matter, but we
    // need to clarify this.
    //
    core::Vec2d p = origin + position();
    if (hinting) {
        p[0] = std::round(p[0]);
        p[1] = std::round(p[1]);
    }
    for (Int i = 0; 2*i+1 < localFill.length(); ++i) {
        float x = static_cast<float>(p[0] + localFill[2*i]);
        float y = static_cast<float>(p[1] - localFill[2*i+1]); // revert Y axis
        data.insert(data.end(), {x, y, r, g, b});
    }
}

ShapedText::ShapedText(FontFace* face, const std::string& text) :
    impl_()
{
    impl_ = new internal::ShapedTextImpl(face, text);
}

ShapedText::ShapedText(const ShapedText& other)
{
    impl_ = new internal::ShapedTextImpl(*(other.impl_));
}

ShapedText::ShapedText(ShapedText&& other)
{
    impl_ = other.impl_;
    other.impl_ = nullptr;
}

ShapedText& ShapedText::operator=(const ShapedText& other)
{
    if (this != &other) {
        *impl_ = *(other.impl_);
    }
    return *this;
}

ShapedText& ShapedText::operator=(ShapedText&& other)
{
    if (this != &other) {
        impl_ = other.impl_;
        other.impl_ = nullptr;
    }
    return *this;
}

ShapedText::~ShapedText()
{
    delete impl_;
}

FontFace* ShapedText::fontFace() const
{
    return impl_->facePtr.get();
}

const std::string& ShapedText::text() const
{
    return impl_->text;
}

void ShapedText::setText(const std::string& text)
{
    impl_->setText(text);
}

const ShapedGlyphArray& ShapedText::glyphs() const
{
    return impl_->glyphs;
}

const ShapedGraphemeArray& ShapedText::graphemes() const
{
    return impl_->graphemes;
}

core::Vec2d ShapedText::advance() const
{
    return impl_->advance;
}

void ShapedText::fill(core::FloatArray& data,
                      const core::Vec2d& origin,
                      float r, float g, float b) const
{
    for (const ShapedGlyph& glyph : glyphs()) {
        glyph.fill(data, origin, r, g, b);
    }
}

void ShapedText::fill(core::FloatArray& data,
                      const core::Vec2d& origin,
                      float r, float g, float b,
                      Int start, Int end) const
{
    const ShapedGlyphArray& glyphs = impl_->glyphs;
    for (Int i = start; i < end; ++i) {
        glyphs[i].fill(data, origin, r, g, b);
    }
}

namespace {

double pos_(const ShapedGrapheme& grapheme) {
    return grapheme.position()[0];
}

double adv_(const ShapedGrapheme& grapheme) {
    return grapheme.advance()[0];
}

} // namespace

Int ShapedText::bytePosition(const core::Vec2d& mousePosition)
{
    const ShapedGraphemeArray& g = graphemes();
    double x = mousePosition[0];
    Int numGraphemes = g.length();
    if (numGraphemes == 0) {
        return 0;
    }
    else {
        if (x < 0) {
            return 0;
        }
        else {
            const ShapedGrapheme& last = g.last();
            if (x > pos_(last) + adv_(last)) {
                return core::int_cast<Int>(text().size());
            }
            else {
                // Binary search: find grapheme hovered by mouse cursor.
                // i1 = index of grapheme s.t. mouse is after start of grapheme.
                // i2 = index of grapheme s.t. mouse is before end of grapheme.
                // At end of loop: i1 == i2 == grapheme hovered by mouse position
                Int i1 = 0;
                Int i2 = g.length() - 1;
                while (i1 != i2) {
                    if (i2 == i1 + 1) {
                        if (x < pos_(g[i2])) {
                            i2 = i1;
                        }
                        else {
                            i1 = i2;
                        }
                    }
                    else {
                        Int i3 = (i1 + i2) / 2;
                        if (x < pos_(g[i3])) {
                            i2 = i3;
                        }
                        else {
                            i1 = i3;
                        }
                    }
                }
                // Determine whether the cursor is closer to the beginning
                // of the grapheme or the end of the grapheme.
                const ShapedGrapheme& grapheme = g[i1];
                if (x - pos_(grapheme) < 0.5 * adv_(grapheme)) {
                    return grapheme.bytePosition();
                }
                else {
                    i2 = i1 + 1;
                    if (i2 >= g.length()) {
                        return core::int_cast<Int>(text().size());
                    }
                    else {
                        return g[i2].bytePosition();
                    }
                }
            }
        }
    }
}

// Convenient macro for checking assertions and failing with a LogicError.
// We should eventually add this to vgc::core API
#define VGC_EXPECT_EQ(a, b) \
    if (!((a) == (b))) { \
        std::string error = core::format( \
            "Failed to satisfy condition `" #a " == " #b "`. Actual values are {} and {}.", (a), (b)); \
        throw core::LogicError(error); \
    } else (void)0

namespace internal {

// Determines whether this byte is a continuation byte of a valid UTF-8 encoded
// stream. These have the form 10xxxxxx.
//
bool isUtf8ContinuationByte_(char c) {
    unsigned char c_ = static_cast<unsigned char>(c);
    return (c_ >> 6) == 2;
}

class TextBoundaryIteratorImpl {
public:
    TextBoundaryIteratorImpl(TextBoundaryType type, const std::string& string) :
        q(static_cast<QTextBoundaryFinder::BoundaryType>(type),
          QString::fromStdString(string))
    {
        QString qstring = q.string();

        // Compute number of UTF-8 positions and UTF-16 positions
        size_t u8_length = string.size();
        int u16_length = qstring.length();
        Int u8_numPositions = core::int_cast<Int>(u8_length) + 1;
        Int u16_numPositions = core::int_cast<Int>(u16_length) + 1;

        // Reserve array size
        u8_to_u16_.reserve(u8_numPositions);
        u16_to_u8_.reserve(u16_numPositions);

        // Compute UTF-8/UTF-16 index mapping
        size_t u8_index = 0;
        for (int i = 0; i < u16_length; ++i) {
            int u16_index = i;
            QChar c = qstring.at(i);
            u16_to_u8_.append(core::int_cast<Int>(u8_index));
            if (c.isHighSurrogate()) {
                u16_to_u8_.append(core::int_cast<Int>(u8_index));
                ++i;
            }
            u8_to_u16_.append(u16_index);
            u8_index += 1;
            while (u8_index < u8_length && isUtf8ContinuationByte_(string[u8_index])) {
                u8_to_u16_.append(u16_index);
                u8_index += 1;
            }
        }
        u8_to_u16_.append(u16_length);
        u16_to_u8_.append(core::int_cast<Int>(u8_length));
        u8_min_ = 0;
        u8_max_ = u8_to_u16_.length();
        u16_min_ = 0;
        u16_max_ = u16_to_u8_.length();

        // Check that the array sizes are what we expect
        VGC_EXPECT_EQ(u8_to_u16_.length(), u8_numPositions);
        VGC_EXPECT_EQ(u16_to_u8_.length(), u16_numPositions);
    }

    int u8_to_u16(Int p) const {
        p = core::clamp(p, u8_min_, u8_max_);
        return u8_to_u16_[p];
    }

    Int u16_to_u8(int p) const {
        Int p_ = core::int_cast<Int>(p);
        p_ = core::clamp(p_, u16_min_, u16_max_);
        return u16_to_u8_[p_];
    }

    QTextBoundaryFinder q;

private:
    core::Array<int> u8_to_u16_;
    Int u8_min_;
    Int u8_max_;

    core::Array<Int> u16_to_u8_;
    Int u16_min_;
    Int u16_max_;
};

} // namespace internal

TextBoundaryIterator::TextBoundaryIterator(TextBoundaryType type, const std::string& string)
{
    impl_ = new internal::TextBoundaryIteratorImpl(type, string);
}

TextBoundaryIterator::~TextBoundaryIterator()
{
    delete impl_;
}

bool TextBoundaryIterator::isAtBoundary() const
{
    return impl_->q.isAtBoundary();
}

bool TextBoundaryIterator::isValid() const
{
    return impl_->q.isValid();
}

Int TextBoundaryIterator::position() const
{
    int p = impl_->q.position();
    return impl_->u16_to_u8(p);
}

void TextBoundaryIterator::setPosition(Int position)
{
    int p = impl_->u8_to_u16(position);
    impl_->q.setPosition(p);
}

void TextBoundaryIterator::toEnd()
{
    impl_->q.toEnd();
}

Int TextBoundaryIterator::toNextBoundary()
{
    int p = impl_->q.toNextBoundary();
    return (p == -1) ? -1 : impl_->u16_to_u8(p);
}

Int TextBoundaryIterator::toPreviousBoundary()
{
    int p = impl_->q.toPreviousBoundary();
    return (p == -1) ? -1 : impl_->u16_to_u8(p);
}

void TextBoundaryIterator::toStart()
{
    impl_->q.toStart();
}

TextBoundaryType TextBoundaryIterator::type() const
{
    return static_cast<TextBoundaryType>(impl_->q.type());
}

} // namespace graphics
} // namespace vgc
