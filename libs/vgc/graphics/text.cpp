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

#include <vgc/geometry/vec2f.h>

namespace vgc {
namespace graphics {

namespace {

class Triangle2f {
    std::array<geometry::Vec2f, 3> d_;

public:
    Triangle2f(const geometry::Vec2f& a, const geometry::Vec2f& b, const geometry::Vec2f& c) :
        d_({a, b, c}) {}

    Triangle2f(float ax, float ay, float bx, float by, float cx, float cy) :
        d_({geometry::Vec2f(ax, ay), geometry::Vec2f(bx, by), geometry::Vec2f(cx, cy)}) {}

    geometry::Vec2f& operator[](Int i) {
        return d_[i];
    }

    const geometry::Vec2f& operator[](Int i) const {
        return d_[i];
    }
};

using Triangle2fArray = core::Array<Triangle2f>;

} // namespace

namespace internal {

class ShapedTextImpl {
public:
    // Input of shaping.
    //
    // Note that that we store a SizedFontPtr to keep both the SizedFont and the
    // FontLibrary alive. Note that the FontLibrary never destroys its
    // children: a created SizedFont stays in memory forever until the library
    // itself is destroyed().
    //
    // XXX thread-safety? What if two threads create two ShapedTexts at the
    //     same time, wouldn't this cause a race condition when increasing
    //     the reference counts? Shouldn't we make the reference counts atomic,
    //     or else protect the assignment to this facePtr by a mutex?
    //
    SizedFontPtr facePtr;
    std::string text;

    // Output of shaping
    //
    ShapedGlyphArray glyphs;
    ShapedGraphemeArray graphemes;
    ShapedTextPositionInfoArray positions;
    geometry::Vec2f advance;

    // Buffers to avoid dynamic allocations during filling.
    //
    core::FloatArray floatBuffer;
    Triangle2fArray trianglesBuffer1;
    Triangle2fArray trianglesBuffer2;

    // HarbBuzz buffer
    //
    hb_buffer_t* buf;

    ShapedTextImpl(SizedFont* face, std::string_view text) :
        facePtr(face),
        text(),
        glyphs(),
        graphemes(),
        positions(),
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
        positions(other.positions),
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
            positions = other.positions;
            advance = other.advance;
        }
        return *this;
    }

    ~ShapedTextImpl()
    {
        hb_buffer_destroy(buf);
    }

    void setText(std::string_view text_)
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
        hb_glyph_info_t* glyphInfos = hb_buffer_get_glyph_infos(buf, &n);
        hb_glyph_position_t* glyphPositions = hb_buffer_get_glyph_positions(buf, &n);

        // Convert to ShapedGlyph elements
        glyphs.clear();
        advance = geometry::Vec2f(0, 0);
        for (unsigned int i = 0; i < n; ++i) {
            hb_glyph_info_t& info = glyphInfos[i];
            hb_glyph_position_t& pos = glyphPositions[i];
            SizedGlyph* glyph = facePtr->getSizedGlyphFromIndex(info.codepoint);
            Int bytePosition = core::int_cast<Int>(info.cluster);
            geometry::Vec2f glyphOffset = internal::f266ToVec2f(pos.x_offset, pos.y_offset);
            geometry::Vec2f glyphAdvance = internal::f266ToVec2f(pos.x_advance, pos.y_advance);
            geometry::Vec2f glyphPosition = advance + glyphOffset;
            if (glyph) {
                glyphs.append(ShapedGlyph(glyph, glyphOffset, glyphAdvance, glyphPosition, bytePosition));
            }
            advance += glyphAdvance;

            // Note: so far, we have only tested cases where the y coords of
            // offset and advance (and thus position) was 0. The desired output
            // is to have the y axis pointing down. This might require to add
            // minus signs in front of pos.y_offset and pos.y_advance.
        }

        // Compute graphemes/positions and correspondence with glyphs
        // First pass: create graphemes/positions with corresponding byte index
        graphemes.clear();
        positions.clear();
        TextBoundaryIterator it(TextBoundaryType::Grapheme, text);
        Int byteIndexBefore = 0;
        Int byteIndexAfter = it.toNextBoundary();
        TextBoundaryMarkers markers = TextBoundaryMarker::Grapheme;
        while (byteIndexAfter != -1) {
            graphemes.append(ShapedGrapheme(0, geometry::Vec2f(0, 0), geometry::Vec2f(0, 0), byteIndexBefore));
            positions.append(ShapedTextPositionInfo(0, byteIndexBefore, geometry::Vec2f(0, 0), markers));
            byteIndexBefore = byteIndexAfter;
            byteIndexAfter = it.toNextBoundary();
        }
        positions.append(ShapedTextPositionInfo(-1, byteIndexBefore, geometry::Vec2f(0, 0), markers));

        // Second pass: compute correspondence between graphemes/positions and glyphs
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
                    positions[graphemeIndex].glyphIndex_ = glyphIndex;
                 }
            }
        }
        positions.last().glyphIndex_ = numGlyphs;

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
                geometry::Vec2f glyphAdvance = glyphs[glyphIndexBegin].advance();
                Int numGraphemesForGlyph = graphemeIndex - graphemeIndexBegin + 1;
                geometry::Vec2f graphemeAdvance = glyphAdvance / static_cast<float>(numGraphemesForGlyph);
                for (Int k = graphemeIndexBegin; k <= graphemeIndex; ++k) {
                    graphemes[k].advance_ = graphemeAdvance;
                }
            }
            else {
                // one grapheme for one or several glyphs
                geometry::Vec2f graphemeAdvance(0, 0);
                for (Int i = glyphIndexBegin; i < glyphIndexEnd; ++i) {
                    graphemeAdvance += glyphs[i].advance();
                }
                graphemes[graphemeIndex].advance_ = graphemeAdvance;
            }
        }

        // Fourth pass: compute position based on advance
        geometry::Vec2f graphemePosition(0, 0);
        for (Int graphemeIndex = 0; graphemeIndex < numGraphemes; ++graphemeIndex) {
            graphemes[graphemeIndex].position_ = graphemePosition;
            positions[graphemeIndex].advance_ = graphemePosition;
            graphemePosition += graphemes[graphemeIndex].advance();
        }
        positions.last().advance_ = graphemePosition;

        // Fifth pass: compute word boundaries
        TextBoundaryIterator itWord(TextBoundaryType::Word, text);
        Int position = 0;
        Int byteIndex = 0;
        Int maxPosition = positions.length() - 1;
        while (byteIndex != -1) {
            while(position < maxPosition
                  && positions[position].byteIndex() < byteIndex) {
                ++position;
            }
            positions[position].boundaryMarkers_ |= TextBoundaryMarker::Word;
            byteIndex = itWord.toNextBoundary();
        }

        // Set line boundaries
        positions.first().boundaryMarkers_ |= TextBoundaryMarker::Line;
        positions.last().boundaryMarkers_ |= TextBoundaryMarker::Line;
    }

    float horizontalAdvance(Int position) {
        return positions[position].advance()[0];
    }

private:
    friend class graphics::ShapedText;
};

} // namespace internal

void ShapedGlyph::fill(core::FloatArray& data,
                       const geometry::Vec2f& origin,
                       float r, float g, float b) const
{
    Int oldLength = data.length();

    // Get position data
    fill(data, origin);

    // Add colors: [x1, y1, x2, y2] -> [x1, x1, r, g, b, x2, x2, r, g, b]
    Int newLength = data.length();
    Int numVertices = (newLength - oldLength) / 2;
    data.resizeNoInit(oldLength + 5 * numVertices);
    float* out = data.begin() + (oldLength + 5 * numVertices);
    float* in = data.begin() + (oldLength + 2 * numVertices);
    while(out != in) {
        *(--out) = b;
        *(--out) = g;
        *(--out) = r;
        *(--out) = *(--in);
        *(--out) = *(--in);
    }
}

void ShapedGlyph::fill(core::FloatArray& data,
                       const geometry::Vec2f& origin) const
{
    sizedGlyph()->fillYMirrored(data, origin + position());
}

ShapedText::ShapedText(SizedFont* sizedFont, std::string_view text) :
    impl_()
{
    impl_ = new internal::ShapedTextImpl(sizedFont, text);
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

SizedFont* ShapedText::sizedFont() const
{
    return impl_->facePtr.get();
}

const std::string& ShapedText::text() const
{
    return impl_->text;
}

void ShapedText::setText(std::string_view text)
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

ShapedTextPositionInfo ShapedText::positionInfo(Int position) const
{
    if (position < 0 || position >= impl_->positions.length()) {
        return ShapedTextPositionInfo(-1, -1, geometry::Vec2f(), TextBoundaryMarker::None);
    }
    else {
        return impl_->positions.getUnchecked(position);
    }
}

Int ShapedText::numPositions() const
{
    return impl_->positions.length();
}

geometry::Vec2f ShapedText::advance() const
{
    return impl_->advance;
}

geometry::Vec2f ShapedText::advance(Int position) const
{
    if (position < 0 || position >= impl_->positions.length()) {
        return geometry::Vec2f();
    }
    else {
        return impl_->positions.getUnchecked(position).advance();
    }
}

void ShapedText::fill(core::FloatArray& data,
                      const geometry::Vec2f& origin,
                      float r, float g, float b) const
{
    for (const ShapedGlyph& glyph : glyphs()) {
        glyph.fill(data, origin, r, g, b);
    }
}

void ShapedText::fill(core::FloatArray& data,
                      const geometry::Vec2f& origin,
                      float r, float g, float b,
                      Int start, Int end) const
{
    const ShapedGlyphArray& glyphs = impl_->glyphs;
    for (Int i = start; i < end; ++i) {
        glyphs[i].fill(data, origin, r, g, b);
    }
}

namespace {

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
    if      (!cmp(clip, c[i])) { return; }                       \
    else if (!cmp(a[i], clip)) { out.append(triangle); return; } \
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
                   float clip)
{
    constexpr auto cmp = LessOrGreater<float>();

    // Sort by i-th coordinate and returns early in trivial cases
    geometry::Vec2f A, B, C;
    bool mirrored;
    const geometry::Vec2f& a = triangle[0];
    const geometry::Vec2f& b = triangle[1];
    const geometry::Vec2f& c = triangle[2];
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

    // If we're still here, then, we have:
    //
    //   A[i] <= B[i] <= C[i]   (where "x <= y" means `!cmp(y, x)`)
    //
    // and:
    //
    //   A[i] < clip < C[i]     (where "x < y" means `cmp(x, y)`)
    //
    // We now need to check whether B[i] is before or after the clip
    // line, and whether AB or BC are parallel to the clip line.
    //
    float eps = 1e-6f;
    float ac = C[i]-A[i];
    if (cmp(clip, B[i])) {
        float ab = B[i]-A[i];
        if (cmp(ab, eps * ac)) { // AB parallel to clip line
            out.append(triangle);
            return;
        }
        geometry::Vec2f B_ = A + (clip-A[i])/ab * (B-A);
        geometry::Vec2f C_ = A + (clip-A[i])/ac * (C-A);
        if (mirrored) {
            out.emplace(out.end(), B, B_, C);
            out.emplace(out.end(), C, B_, C_);
        }
        else {
            out.emplace(out.end(), B_, B, C);
            out.emplace(out.end(), B_, C, C_);
        }
    }
    else {
        float bc = C[i]-B[i];
        if (cmp(bc, eps * ac)) { // BC parallel to clip line
            return;
        }
        A += (clip-A[i])/ac * (C-A);
        B += (clip-B[i])/bc * (C-B); // Note: (B[i] == clip) => B unchanged
        if (mirrored) {
            out.emplace(out.end(), B, A, C);
        }
        else {
            out.emplace(out.end(), A, B, C);
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
    buffer.clear();
    for (const Triangle2f& t : data) {
        clipTriangle_<i, LessOrGreater>(buffer, t, clip);
    }
    std::swap(data, buffer);
}

} // namespace

void ShapedText::fill(core::FloatArray& data,
                      const geometry::Vec2f& origin,
                      float r, float g, float b,
                      float clipLeft, float clipRight,
                      float clipTop, float clipBottom) const
{
    fill(data, origin, r, g, b, 0, glyphs().length(), clipLeft, clipRight, clipTop, clipBottom);
}

void ShapedText::fill(core::FloatArray& data,
                      const geometry::Vec2f& origin,
                      float r, float g, float b,
                      Int start, Int end,
                      float clipLeft, float clipRight,
                      float clipTop, float clipBottom) const
{
    // Get clip rectangle in ShapedText coordinates
    geometry::Rect2f clipRect(clipLeft, clipTop, clipRight, clipBottom);
    clipRect.setPMin(clipRect.pMin() - origin);
    clipRect.setPMax(clipRect.pMax() - origin);

    // Iterate over all glyphs. If the glyph's bbox doesn't intersect clipRect,
    // then the glyph is entirely discarded. If the glyph's bbox is contained
    // in clipRect, then the glyph is entirely kept. Otherwise, we process the
    // glyph's triangles to cut them by the clipRect, and only keep the
    // triangles inside.
    //
    const ShapedGlyphArray& glyphs = impl_->glyphs;
    for (Int i = start; i < end; ++i) {
        const ShapedGlyph& glyph = glyphs[i];
        const geometry::Rect2f& bbox = glyph.boundingBox();
        if (clipRect.intersects(bbox)) {
            if (clipRect.contains(bbox)) {
                glyph.fill(data, origin, r, g, b);
            }
            else {
                core::FloatArray& floatBuffer = impl_->floatBuffer;
                Triangle2fArray& trianglesBuffer1 = impl_->trianglesBuffer1;
                Triangle2fArray& trianglesBuffer2 = impl_->trianglesBuffer2;

                floatBuffer.clear();
                glyph.fill(floatBuffer, origin);
                initTriangles(floatBuffer, trianglesBuffer1);
                if (bbox.xMin() < clipRect.xMin()) {
                    clipTriangles_<0, std::less>(trianglesBuffer1, trianglesBuffer2, clipLeft);
                }
                if (bbox.yMin() < clipRect.yMin()) {
                    clipTriangles_<1, std::less>(trianglesBuffer1, trianglesBuffer2, clipTop);
                }
                if (bbox.xMax() > clipRect.xMax()) {
                    clipTriangles_<0, std::greater>(trianglesBuffer1, trianglesBuffer2, clipRight);
                }
                if (bbox.yMax() > clipRect.yMax()) {
                    clipTriangles_<1, std::greater>(trianglesBuffer1, trianglesBuffer2, clipBottom);
                }
                addTriangles(data, trianglesBuffer1, r, g, b);
            }
        }
    }
}

Int ShapedText::positionFromByte(Int byteIndex) {
    auto first = impl_->positions.cbegin();
    auto last = impl_->positions.cend();
    auto comp = [](const ShapedTextPositionInfo& info, Int byteIndex) {
        return info.byteIndex() < byteIndex;
    };
    auto it = std::lower_bound(first, last, byteIndex, comp);
    if (it == last) {
        return maxPosition();
    }
    else {
        return static_cast<Int>(std::distance(first, it));
    }
}

Int ShapedText::positionFromPoint(
    const geometry::Vec2f& point,
    TextBoundaryMarkers boundaryMarkers) {

    std::pair<Int, Int> pair = positionPairFromPoint(point, boundaryMarkers);

    // Determine whether the cursor is closer to the position before or after
    float x = point[0];
    float beforeAdvance = impl_->horizontalAdvance(pair.first);
    float afterAdvance = impl_->horizontalAdvance(pair.second);
    if (x < 0.5 * (beforeAdvance + afterAdvance)) {
        return pair.first;
    }
    else {
        return pair.second;
    }

    // TODO: if direction is rtl, we should revert all `if (x < ...)` comparisons
}

std::pair<Int, Int> ShapedText::positionPairFromPoint(
    const geometry::Vec2f& point,
    TextBoundaryMarkers boundaryMarkers)
{
    // Find smallest text position after the given mouse position
    float x = point[0];
    auto first = impl_->positions.cbegin();
    auto last = impl_->positions.cend();
    auto comp = [](const ShapedTextPositionInfo& info, float x) {
        return info.advance()[0] < x;
    };
    auto it = std::lower_bound(first, last, x, comp);

    // Deduce pair of text positions around the given mouse position
    Int beforePosition = minPosition();
    Int afterPosition = maxPosition();
    if (it == first) {
        afterPosition = beforePosition; // before = after = min
    }
    else if (it == last) {
        beforePosition = afterPosition; // before = after = max
    }
    else {
        afterPosition = static_cast<Int>(std::distance(first, it));
        beforePosition = afterPosition - 1;
    }

    // Extend positions to the given boundary markers
    beforePosition = previousOrEqualBoundary(beforePosition, boundaryMarkers);
    afterPosition = nextOrEqualBoundary(afterPosition, boundaryMarkers);

    return std::make_pair(beforePosition, afterPosition);
}

Int ShapedText::nextBoundary(
    Int position,
    TextBoundaryMarkers boundaryMarkers,
    bool clamp) {

    return nextOrEqualBoundary(position + 1, boundaryMarkers, clamp);
}

Int ShapedText::nextOrEqualBoundary(
    Int position,
    TextBoundaryMarkers boundaryMarkers,
    bool clamp) {

    Int minPosition = 0;
    Int maxPosition = numPositions() - 1;
    if (position < minPosition) {
        position = minPosition;
    }

    while (position <= maxPosition
           && !impl_->positions[position].boundaryMarkers().hasAll(boundaryMarkers)) {
        ++position;
    }

    if (position > maxPosition) {
        return clamp ? maxPosition : -1;
    }
    else {
        return position;
    }
}

Int ShapedText::previousBoundary(
    Int position,
    TextBoundaryMarkers boundaryMarkers,
    bool clamp) {

    return previousOrEqualBoundary(position - 1, boundaryMarkers, clamp);
}

Int ShapedText::previousOrEqualBoundary(
    Int position,
    TextBoundaryMarkers boundaryMarkers,
    bool clamp) {

    Int minPosition = 0;
    Int maxPosition = numPositions() - 1;
    if (position > maxPosition) {
        position = maxPosition;
    }

    while (position >= minPosition
           && !impl_->positions[position].boundaryMarkers().hasAll(boundaryMarkers)) {
        --position;
    }

    if (position < minPosition) {
        return clamp ? minPosition : -1;
    }
    else {
        return position;
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
