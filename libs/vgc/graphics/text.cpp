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

#include <array>
#include <functional> // std::less, std::greater

#include <hb.h>
#include <hb-ft.h>

#include <QTextBoundaryFinder>

#include <vgc/core/vec2f.h>

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
    Int oldLength = data.length();

    // Get position data
    fill(data, origin);

    // Add colors: [x1, y1, x2, y2] -> [x1, x1, r, g, b, x2, x2, r, g, b]
    Int newLength = data.length();
    Int numVertices = (newLength - oldLength) / 2;
    data.resize(oldLength + 5*numVertices);
    for (Int i = numVertices - 1; i >= 0; --i) {
        data[oldLength + 5*i]   = data[oldLength + 2*i];
        data[oldLength + 5*i+1] = data[oldLength + 2*i+1];
        data[oldLength + 5*i+2] = r;
        data[oldLength + 5*i+3] = g;
        data[oldLength + 5*i+4] = b;
    }
}

void ShapedGlyph::fill(core::FloatArray& data,
                       const core::Vec2d& origin) const
{
    // Note: we currently disable per-letter hinting (which can be seen as a
    // component of horizontal hinting) because it looked worse, at least with
    // the current implementation. It produced uneven spacing between letters,
    // making kerning look bad.
    constexpr bool hinting = false;

    // Per-letter hinting
    core::Vec2d p_ = origin + position();
    core::Vec2f p(static_cast<float>(p_[0]), static_cast<float>(p_[1]));
    if (hinting) {
        p[0] = std::round(p[0]);
        p[1] = std::round(p[1]);
    }

    // Transform from local glyph coordinates to requested coordinates.
    core::Mat3f transform = core::Mat3f::identity;
    transform.translate(p[0], p[1]); // TODO: Mat3f::translate(const Vec2f&)
    transform.scale(1, -1);

    // Get the glyph triangulation
    fontGlyph()->fill(data, transform);
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

core::Vec2d ShapedText::advance(Int bytePosition) const
{
    core::Vec2d res(0, 0);
    for (const graphics::ShapedGrapheme& grapheme : graphemes()) {
        if (grapheme.bytePosition() >= bytePosition) {
            break;
        }
        res += grapheme.advance();
    }
    return res;
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

class Triangle2f {
    std::array<core::Vec2f, 3> d_;

public:
    Triangle2f(const core::Vec2f& a, const core::Vec2f& b, const core::Vec2f& c) :
        d_({a, b, c}) {}

    Triangle2f(float ax, float ay, float bx, float by, float cx, float cy) :
        d_({core::Vec2f(ax, ay), core::Vec2f(bx, by), core::Vec2f(cx, cy)}) {}

    core::Vec2f& operator[](Int i) {
        return d_[i];
    }

    const core::Vec2f& operator[](Int i) const {
        return d_[i];
    }
};

using Triangle2fArray = core::Array<Triangle2f>;

void initTriangles(const core::FloatArray& data, Triangle2fArray& out)
{
    out.clear();
    Int numTriangles = data.length() / 6;
    out.reserve(numTriangles);
    const float* begin = data.begin();
    const float* end = begin + 6 * numTriangles;
    for (const float* f = begin; f < end; f += 6) {
        out.emplace(out.end(), f[0], f[1], f[2], f[3], f[4], f[5]);
    }
}

void addTriangles(
        core::FloatArray& data,
        const Triangle2fArray& triangles,
        float r, float g, float b)
{
    data.reserve(triangles.length() * 15);
    for (const Triangle2f& t : triangles) {
        data.extend({t[0][0], t[0][1], r, g, b,
                     t[1][0], t[1][1], r, g, b,
                     t[2][0], t[2][1], r, g, b});
    }
}

// Helper macro to perform pre-clipping:
// - Assumes that (a, b, c) are sorted relative to the i-th coordinate.
// - If the triangle is entirely inside or outside the clipping half-plane, the
//   triangle is either kept or discarded entirely, then the function returns.
// - Otherwise, assigns (a, b, c) to (A, B, C) for further processing.
//
#define PRECLIP_(a, b, c)                                        \
    if      (cmp(c[i], clip2)) { return; }                       \
    else if (cmp(clip1, a[i])) { out.append(triangle); return; } \
    else                       { A = a; B = b; C = c; }

// Clips the given triangle along the given `clip` line. Appends the resulting
// triangles (either zero, one, or two triangles) to the given out parameter.
//
// See documentation of clipTriangles_() for an explanation of the template
// parameters.
//
// The two values `clip1` and `clip2` are used for numerical precision and must
// specify a narrow band around the `clip` value, satisfying:
//
//      LessOrGreater(clip1, clip) and LessOrGreater(clip, clip2)
//
// The vertices within this band are considered to be exactly at the clip line
// and are clipped accordingly.
//
template<int i, template<typename> typename LessOrGreater>
void clipTriangle_(Triangle2fArray& out,
                   const Triangle2f& triangle,
                   float clip, float clip1, float clip2)
{
    constexpr auto cmp = LessOrGreater<float>();

    // Sort by i-th coordinate and returns early in trivial cases
    core::Vec2f A, B, C;
    bool mirrored;
    const core::Vec2f& a = triangle[0];
    const core::Vec2f& b = triangle[1];
    const core::Vec2f& c = triangle[2];
    if (cmp(a[i], b[i])) {
        if      (cmp(b[i], c[i])) { PRECLIP_(a, b, c); mirrored = false; }
        else if (cmp(a[i], c[i])) { PRECLIP_(a, c, b); mirrored = true;  }
        else                      { PRECLIP_(c, a, b); mirrored = false; }
    }
    else {
        if      (cmp(a[i], c[i])) { PRECLIP_(b, a, c); mirrored = true;  }
        else if (cmp(b[i], c[i])) { PRECLIP_(b, c, a); mirrored = false; }
        else                      { PRECLIP_(c, b, a); mirrored = true;  }
    }

    // If we're still here, then (A[i], B[i], C[i]) are sorted and:
    //
    //   A[i] < clip1 < clip < clip2 < C[i]   (where "x < y" means `cmp(x, y)`)
    //
    // We just need to check whether B[i] is within [clip1, clip2],
    // or before this range, or after this range.
    //
    if (cmp(B[i], clip1)) {
        A += (clip-A[i])/(C[i]-A[i]) * (C-A);
        B += (clip-B[i])/(C[i]-B[i]) * (C-B);
        if (mirrored) {
            out.emplace(out.end(), B, A, C);
        }
        else {
            out.emplace(out.end(), A, B, C);
        }
    }
    else if (cmp(B[i], clip2)) {
        A += (clip-A[i])/(C[i]-A[i]) * (C-A);
        if (mirrored) {
            out.emplace(out.end(), B, A, C);
        }
        else {
            out.emplace(out.end(), A, B, C);
        }
    }
    else {
        core::Vec2f B_ = A + (clip-A[i])/(B[i]-A[i]) * (B-A);
        core::Vec2f C_ = A + (clip-A[i])/(C[i]-A[i]) * (C-A);
        if (mirrored) {
            out.emplace(out.end(), B, B_, C);
            out.emplace(out.end(), C, B_, C_);
        }
        else {
            out.emplace(out.end(), B_, B, C);
            out.emplace(out.end(), B_, C, C_);
        }
    }
}

#undef PRECLIP_

// Clips the given triangles along the given `clip` line. The clipping is
// performed in-place, that is, the given array `triangles` is used both as
// input and output. The given `buffer` is used for temporary computation: it
// is emptied at the beginning of this function, and its final value is
// unspecified.
//
// The template parameter `i` represents the chosen coordinate:
// - 0 for the x-coordinate
// - 1 for the y-coordinate
//
// The template parameter `LessOrGreater` should be either:
// - std::less: if you'd like to clip (= remove) every vertex whose
//   i-th coordinate is less than the given clip line
// - std::greater: if you'd like to clip (= remove) every vertex whose
//   i-th coordinate is greater than the given clip line
//
template<int i, template<typename> typename LessOrGreater>
void clipTriangles_(Triangle2fArray& data,
                    Triangle2fArray& buffer,
                    float clip)
{
    constexpr auto cmp = LessOrGreater<float>();
    constexpr float eps = 1e-6f;
    constexpr bool isLess = cmp(0, eps);
    const float clip1 = isLess ? clip - eps : clip + eps;
    const float clip2 = isLess ? clip + eps : clip - eps;

    buffer.clear();
    for (const Triangle2f& t : data) {
        clipTriangle_<i, LessOrGreater>(buffer, t, clip, clip1, clip2);
    }
    std::swap(data, buffer);
}

}

void ShapedText::fill(core::FloatArray& data,
                      const core::Vec2d& origin,
                      float r, float g, float b,
                      float clipLeft, float clipRight,
                      float clipTop, float clipBottom) const
{
    core::FloatArray floatBuffer;
    Triangle2fArray trianglesBuffer;
    Triangle2fArray triangles;

    for (const ShapedGlyph& glyph : glyphs()) {

        // TODO: Implement ShapedGlyph::boundingBox() and use it as initial
        // early clipping: we don't want to call glyph.fill() and/or
        // clipTriangles_() if not necessary.

        // Get unclipped glyph as triangles
        floatBuffer.clear();
        glyph.fill(floatBuffer, origin);
        initTriangles(floatBuffer, triangles);

        // Clip in all directions
        clipTriangles_<0, std::less>(triangles, trianglesBuffer, clipLeft);
        clipTriangles_<1, std::less>(triangles, trianglesBuffer, clipTop);
        clipTriangles_<0, std::greater>(triangles, trianglesBuffer, clipRight);
        clipTriangles_<1, std::greater>(triangles, trianglesBuffer, clipBottom);

        // Add triangles
        addTriangles(data, triangles, r, g, b);
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
        u8_max_ = u8_to_u16_.length() - 1;
        u16_min_ = 0;
        u16_max_ = u16_to_u8_.length() - 1;

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

    Int u8_length() const {
        return u8_max_;
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

Int TextBoundaryIterator::numBytes() const
{
    return impl_->u8_length();
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
