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

#include <hb-ft.h>
#include <hb.h>

#include <QTextBoundaryFinder>

#include <vgc/geometry/vec2f.h>

namespace vgc::graphics {

namespace {

class Triangle2f {
    std::array<geometry::Vec2f, 3> d_;

public:
    Triangle2f(
        const geometry::Vec2f& a,
        const geometry::Vec2f& b,
        const geometry::Vec2f& c)

        : d_({a, b, c}) {
    }

    Triangle2f(float ax, float ay, float bx, float by, float cx, float cy)
        : d_(
            {geometry::Vec2f(ax, ay), geometry::Vec2f(bx, by), geometry::Vec2f(cx, cy)}) {
    }

    geometry::Vec2f& operator[](Int i) {
        return d_[i];
    }

    const geometry::Vec2f& operator[](Int i) const {
        return d_[i];
    }
};

using Triangle2fArray = core::Array<Triangle2f>;

} // namespace

namespace detail {

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
    SizedFontPtr sizedFont_;
    std::string text_;

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

    ShapedTextImpl(SizedFont* sizedFont, std::string_view text)
        : sizedFont_(sizedFont)
        , text_(text)
        , glyphs()
        , graphemes()
        , positions()
        , advance(0, 0)
        , buf(hb_buffer_create()) {

        update();
    }

    ShapedTextImpl(const ShapedTextImpl& other)
        : sizedFont_(other.sizedFont_)
        , text_(other.text_)
        , glyphs(other.glyphs)
        , graphemes(other.graphemes)
        , positions(other.positions)
        , advance(other.advance)
        , buf(hb_buffer_create()) {
    }

    ShapedTextImpl& operator=(const ShapedTextImpl& other) {
        if (this != &other) {
            sizedFont_ = other.sizedFont_;
            text_ = other.text_;
            glyphs = other.glyphs;
            graphemes = other.graphemes;
            positions = other.positions;
            advance = other.advance;
        }
        return *this;
    }

    ~ShapedTextImpl() {
        hb_buffer_destroy(buf);
    }

    void setSizedFont(SizedFont* sizedFont) {
        sizedFont_ = sizedFont;
        update();
    }

    void setText(std::string_view text) {
        text_ = text;
        update();
    }

    void update() {

        // Prepare input
        const char* data = text_.data();
        int dataLength = core::int_cast<int>(text_.size());
        unsigned int firstChar = 0;
        int numChars = dataLength;

        // Shape
        hb_buffer_reset(buf);
        hb_buffer_set_cluster_level(buf, HB_BUFFER_CLUSTER_LEVEL_MONOTONE_CHARACTERS);
        hb_buffer_add_utf8(buf, data, dataLength, firstChar, numChars);
        hb_buffer_guess_segment_properties(buf);
        hb_shape(sizedFont_->impl_->hbFont, buf, nullptr, 0);

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
            SizedGlyph* glyph = sizedFont_->getSizedGlyphFromIndex(info.codepoint);
            Int bytePosition = core::int_cast<Int>(info.cluster);
            geometry::Vec2f glyphOffset = detail::f266ToVec2f(pos.x_offset, pos.y_offset);
            geometry::Vec2f glyphAdvance =
                detail::f266ToVec2f(pos.x_advance, pos.y_advance);
            geometry::Vec2f glyphPosition = advance + glyphOffset;
            if (glyph) {
                glyphs.append(ShapedGlyph(
                    glyph, glyphOffset, glyphAdvance, glyphPosition, bytePosition));
            }
            advance += glyphAdvance;

            // Note: so far, we have only tested cases where the y coords of
            // offset and advance (and thus position) was 0. The desired output
            // is to have the y axis pointing down. This might require to add
            // minus signs in front of pos.y_offset and pos.y_advance.
        }

        // Create grapheme and position info objects with temporary values
        TextBoundaryMarkersArray markersArray = computeBoundaryMarkers(text_);
        graphemes.clear();
        positions.clear();
        for (Int byteIndex = 0; byteIndex < markersArray.length(); ++byteIndex) {
            TextBoundaryMarkers markers = markersArray[byteIndex];
            if (markers.has(TextBoundaryMarker::Grapheme)) {
                if (byteIndex != markersArray.length() - 1) {
                    graphemes.append(ShapedGrapheme(
                        0, geometry::Vec2f(0, 0), geometry::Vec2f(0, 0), byteIndex));
                }
                positions.append(
                    ShapedTextPositionInfo(0, byteIndex, geometry::Vec2f(0, 0), markers));
            }
        }

        // Compute correspondence with glyph indices
        Int numGraphemes = graphemes.length();
        Int numGlyphs = glyphs.length();
        if (numGraphemes > 0 && numGlyphs > 0) {
            Int graphemeIndex = 0;
            Int glyphIndex = 0;
            Int numBytes = core::int_cast<Int>(text_.size());
            for (Int p = 0; p < numBytes; ++p) {
                while (glyphs[glyphIndex].bytePosition() < p //
                       && glyphIndex + 1 < numGlyphs         //
                       && glyphs[glyphIndex + 1].bytePosition() <= p) {
                    glyphIndex += 1;
                }
                while (graphemes[graphemeIndex].bytePosition() < p //
                       && graphemeIndex + 1 < numGraphemes         //
                       && graphemes[graphemeIndex + 1].bytePosition() <= p) {
                    graphemeIndex += 1;
                    graphemes[graphemeIndex].glyphIndex_ = glyphIndex;
                    positions[graphemeIndex].glyphIndex_ = glyphIndex;
                }
            }
        }
        positions.last().glyphIndex_ = numGlyphs;

        // Compute each grapheme advance by:
        //
        // - additionning the advances of its glyphs, if the grapheme is made
        //   of one or several glyphs (e.g., accents), or
        //
        // - dividing the glyph advance by the number of graphemes, if one glyph
        //   covers several graphemes (e.g., ligatures).
        //
        for (Int graphemeIndex = 0; graphemeIndex < numGraphemes; ++graphemeIndex) {
            Int glyphIndexBegin = graphemes[graphemeIndex].glyphIndex();
            Int glyphIndexEnd = (graphemeIndex + 1 < numGraphemes)
                                    ? graphemes[graphemeIndex + 1].glyphIndex()
                                    : numGlyphs;
            if (glyphIndexBegin == glyphIndexEnd) {
                // one glyph for several graphemes
                Int glyphIndex = glyphIndexBegin;
                Int graphemeIndexBegin = graphemeIndex;
                while (graphemeIndex + 1 < numGraphemes
                       && graphemes[graphemeIndex + 1].glyphIndex() == glyphIndex) {
                    graphemeIndex += 1;
                }
                geometry::Vec2f glyphAdvance = glyphs[glyphIndexBegin].advance();
                Int numGraphemesForGlyph = graphemeIndex - graphemeIndexBegin + 1;
                geometry::Vec2f graphemeAdvance =
                    glyphAdvance / static_cast<float>(numGraphemesForGlyph);
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

        // Compute grapheme positions as the sum of preceding grapheme advances
        geometry::Vec2f graphemePosition(0, 0);
        for (Int graphemeIndex = 0; graphemeIndex < numGraphemes; ++graphemeIndex) {
            graphemes[graphemeIndex].position_ = graphemePosition;
            positions[graphemeIndex].advance_ = graphemePosition;
            graphemePosition += graphemes[graphemeIndex].advance();
        }
        positions.last().advance_ = graphemePosition;
    }

    float horizontalAdvance(Int position) {
        return positions[position].advance()[0];
    }

private:
    friend class graphics::ShapedText;
};

} // namespace detail

// clang-format off

void ShapedGlyph::fill(
    core::FloatArray& data,
    const geometry::Vec2f& origin,
    float r, float g, float b) const {

    Int oldLength = data.length();

    // Get position data
    fill(data, origin);

    // Add colors: [x1, y1, x2, y2] -> [x1, x1, r, g, b, x2, x2, r, g, b]
    Int newLength = data.length();
    Int numVertices = (newLength - oldLength) / 2;
    data.resizeNoInit(oldLength + 5 * numVertices);
    float* out = data.begin() + (oldLength + 5 * numVertices);
    float* in = data.begin() + (oldLength + 2 * numVertices);
    while (out != in) {
        *(--out) = b;
        *(--out) = g;
        *(--out) = r;
        *(--out) = *(--in);
        *(--out) = *(--in);
    }
}

// clang-format on

void ShapedGlyph::fill(core::FloatArray& data, const geometry::Vec2f& origin) const {
    sizedGlyph()->fillYMirrored(data, origin + position());
}

ShapedText::ShapedText(SizedFont* sizedFont, std::string_view text)
    : impl_() {

    impl_ = new detail::ShapedTextImpl(sizedFont, text);
}

ShapedText::ShapedText(const ShapedText& other) {
    impl_ = new detail::ShapedTextImpl(*(other.impl_));
}

ShapedText::ShapedText(ShapedText&& other) {
    impl_ = other.impl_;
    other.impl_ = nullptr;
}

ShapedText& ShapedText::operator=(const ShapedText& other) {
    if (this != &other) {
        *impl_ = *(other.impl_);
    }
    return *this;
}

ShapedText& ShapedText::operator=(ShapedText&& other) {
    if (this != &other) {
        impl_ = other.impl_;
        other.impl_ = nullptr;
    }
    return *this;
}

ShapedText::~ShapedText() {
    delete impl_;
}

SizedFont* ShapedText::sizedFont() const {
    return impl_->sizedFont_.get();
}

void ShapedText::setSizedFont(SizedFont* sizedFont) {
    impl_->setSizedFont(sizedFont);
}

const std::string& ShapedText::text() const {
    return impl_->text_;
}

void ShapedText::setText(std::string_view text) {
    impl_->setText(text);
}

const ShapedGlyphArray& ShapedText::glyphs() const {
    return impl_->glyphs;
}

const ShapedGraphemeArray& ShapedText::graphemes() const {
    return impl_->graphemes;
}

ShapedTextPositionInfo ShapedText::positionInfo(Int position) const {
    if (position < 0 || position >= impl_->positions.length()) {
        return ShapedTextPositionInfo(
            -1, -1, geometry::Vec2f(), TextBoundaryMarker::None);
    }
    else {
        return impl_->positions.getUnchecked(position);
    }
}

Int ShapedText::numPositions() const {
    return impl_->positions.length();
}

geometry::Vec2f ShapedText::advance() const {
    return impl_->advance;
}

geometry::Vec2f ShapedText::advance(Int position) const {
    if (position < 0 || position >= impl_->positions.length()) {
        return geometry::Vec2f();
    }
    else {
        return impl_->positions.getUnchecked(position).advance();
    }
}

// clang-format off

void ShapedText::fill(
    core::FloatArray& data,
    const geometry::Vec2f& origin,
    float r, float g, float b) const {

    for (const ShapedGlyph& glyph : glyphs()) {
        glyph.fill(data, origin, r, g, b);
    }
}

void ShapedText::fill(
    core::FloatArray& data,
    const geometry::Vec2f& origin,
    float r, float g, float b,
    Int start, Int end) const {

    const ShapedGlyphArray& glyphs = impl_->glyphs;
    for (Int i = start; i < end; ++i) {
        glyphs[i].fill(data, origin, r, g, b);
    }
}

namespace {

void initTriangles(const core::FloatArray& data, Triangle2fArray& out) {
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
    float r, float g, float b) {

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
void clipTriangle_(
    Triangle2fArray& out,
    const Triangle2f& triangle,
    float clip) {

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
    float ac = C[i] - A[i];
    if (cmp(clip, B[i])) {
        float ab = B[i] - A[i];
        if (cmp(ab, eps * ac)) { // AB parallel to clip line
            out.append(triangle);
            return;
        }
        geometry::Vec2f B_ = A + (clip - A[i]) / ab * (B - A);
        geometry::Vec2f C_ = A + (clip - A[i]) / ac * (C - A);
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
        float bc = C[i] - B[i];
        if (cmp(bc, eps * ac)) { // BC parallel to clip line
            return;
        }
        A += (clip - A[i]) / ac * (C - A);
        B += (clip - B[i]) / bc * (C - B); // Note: (B[i] == clip) => B unchanged
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
void clipTriangles_(
    Triangle2fArray& data,
    Triangle2fArray& buffer,
    float clip) {

    buffer.clear();
    for (const Triangle2f& t : data) {
        clipTriangle_<i, LessOrGreater>(buffer, t, clip);
    }
    std::swap(data, buffer);
}

} // namespace

void ShapedText::fill(
    core::FloatArray& data,
    const geometry::Vec2f& origin,
    float r, float g, float b,
    float clipLeft, float clipRight,
    float clipTop, float clipBottom) const {

    fill(data, origin, r, g, b,
         0, glyphs().length(),
         clipLeft, clipRight, clipTop, clipBottom);
}

void ShapedText::fill(core::FloatArray& data,
                      const geometry::Vec2f& origin,
                      float r, float g, float b,
                      Int start, Int end,
                      float clipLeft, float clipRight,
                      float clipTop, float clipBottom) const {

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
                    clipTriangles_<0, std::less>(
                        trianglesBuffer1, trianglesBuffer2, clipLeft);
                }
                if (bbox.yMin() < clipRect.yMin()) {
                    clipTriangles_<1, std::less>(
                        trianglesBuffer1, trianglesBuffer2, clipTop);
                }
                if (bbox.xMax() > clipRect.xMax()) {
                    clipTriangles_<0, std::greater>(
                        trianglesBuffer1, trianglesBuffer2, clipRight);
                }
                if (bbox.yMax() > clipRect.yMax()) {
                    clipTriangles_<1, std::greater>(
                        trianglesBuffer1, trianglesBuffer2, clipBottom);
                }
                addTriangles(data, trianglesBuffer1, r, g, b);
            }
        }
    }
}

// clang-format on

Int ShapedText::positionFromByte(Int byteIndex) const {
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
    TextBoundaryMarkers boundaryMarkers) const {

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
    TextBoundaryMarkers boundaryMarkers) const {

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
    bool clamp) const {

    return nextOrEqualBoundary(position + 1, boundaryMarkers, clamp);
}

Int ShapedText::nextOrEqualBoundary(
    Int position,
    TextBoundaryMarkers boundaryMarkers,
    bool clamp) const {

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
    bool clamp) const {

    return previousOrEqualBoundary(position - 1, boundaryMarkers, clamp);
}

Int ShapedText::previousOrEqualBoundary(
    Int position,
    TextBoundaryMarkers boundaryMarkers,
    bool clamp) const {

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
#define VGC_EXPECT_EQ(a, b)                                                              \
    if (!((a) == (b))) {                                                                 \
        std::string error = core::format(                                                \
            "Failed to satisfy condition `" #a " == " #b                                 \
            "`. Actual values are {} and {}.",                                           \
            (a),                                                                         \
            (b));                                                                        \
        throw core::LogicError(error);                                                   \
    }                                                                                    \
    else                                                                                 \
        (void)0

namespace {

// Determines whether this byte is a continuation byte of a valid UTF-8 encoded
// stream. These have the form 10xxxxxx.
//
bool isUtf8ContinuationByte(char c) {
    unsigned char c_ = static_cast<unsigned char>(c);
    return (c_ >> 6) == 2;
}

// Computes the mapping between a UTF-16 array of QChar and the same text
// represented as a UTF-8 array of char.
//
core::Array<Int>
computeU16ToU8Map(const QChar* u16Chars, int u16Length, std::string_view u8Chars) {

    // Compute number of UTF-8 positions and UTF-16 positions
    size_t u8Length = u8Chars.size();
    Int u16NumPositions = core::int_cast<Int>(u16Length) + 1;

    // Reserve array size
    core::Array<Int> u16ToU8;
    u16ToU8.reserve(u16NumPositions);

    // Compute UTF-8/UTF-16 index mapping
    size_t u8_index = 0;
    for (int i = 0; i < u16Length; ++i) {
        QChar c = u16Chars[i];
        u16ToU8.append(core::int_cast<Int>(u8_index));
        if (c.isHighSurrogate()) {
            u16ToU8.append(core::int_cast<Int>(u8_index));
            ++i;
        }
        u8_index += 1;
        while (u8_index < u8Length && isUtf8ContinuationByte(u8Chars[u8_index])) {
            u8_index += 1;
        }
    }
    u16ToU8.append(core::int_cast<Int>(u8Length));
    VGC_EXPECT_EQ(u16ToU8.length(), u16NumPositions);

    return u16ToU8;
}

// Returns whether the given character satisfies the WSegSpace spec:
//
// https://www.unicode.org/reports/tr29/#WSegSpace
// https://www.compart.com/en/unicode/category/Zs
//
//   WSegSpace      General_Category = Zs
//                  and not Linebreak = Glue
//
// Note that "Linebreak = Glue" most likely refers to:
//
// http://www.unicode.org/reports/tr14/tr14-39.html#GLI
//
// that is, it removes the following two characters from the Zs Category:
// - U+00A0    NO-BREAK SPACE (NBSP)
// - U+202F    NARROW NO-BREAK SPACE (NNBSP)
//
// (the third Glue character, U+180E, isn't part of Zs)
//
bool isWSegSpace(char c) {
    return c == ' ';
}

} // namespace

TextBoundaryMarkersArray computeBoundaryMarkers(std::string_view text) {

    // Note: even though the interface of QTextBoundaryFinder is
    // iterator-based, Qt actually computes all the boundaries of the given
    // type in one pass on construction of the QTextBoundaryFinder, and stores
    // them in an array of QCharAttributes, see
    // qtbase/src/corelib/text/qunicodetools_p.h:
    //
    //    struct QCharAttributes
    //    {
    //        uchar graphemeBoundary : 1;
    //        uchar wordBreak        : 1;
    //        uchar sentenceBoundary : 1;
    //        uchar lineBreak        : 1;
    //        uchar whiteSpace       : 1;
    //        uchar wordStart        : 1;
    //        uchar wordEnd          : 1;
    //        uchar mandatoryBreak   : 1;
    //    };
    //
    // By providing a buffer to Qt, the attributes are stored there and avoid
    // dynamic allocations. So even though we create multiple
    // QTextBoundaryFinder (one for each boundary type), it is actually not
    // *too* wasteful in memory and performance. Ideally though, it would be
    // nice if Qt would publicly expose the API in QtUnicodeTools, which allows
    // to directly pass "options" so that all the QCharAttributes are compute
    //

    const char* u8Chars = text.data();
    int u8Length = static_cast<int>(text.size());
    Int u8Length_ = static_cast<Int>(u8Length);

    // Convert from UTF-8 to UTF-16
    QString qstring = QString::fromUtf8(u8Chars, u8Length);
    const QChar* u16Chars = qstring.data();
    int u16Length = qstring.length();

    // Compute mapping between UTF-8 and UTF-16
    core::Array<Int> u16ToU8 = computeU16ToU8Map(u16Chars, u16Length, text);

    // Allocate buffers and initialize return value
    int bufferSize = u16Length + 1;
    unsigned char* buffer = new unsigned char[bufferSize];
    TextBoundaryMarkersArray markers;
    markers.resize(u8Length + 1);

    // Declare temp variables
    QTextBoundaryFinder::BoundaryType boundaryType;
    QTextBoundaryFinder it;
    int u16Position;

    // Compute grapheme boundaries
    boundaryType = QTextBoundaryFinder::Grapheme;
    it = QTextBoundaryFinder(boundaryType, u16Chars, u16Length, buffer, bufferSize);
    u16Position = it.position();
    do {
        Int u8Position = u16ToU8[u16Position];
        markers[u8Position].set(TextBoundaryMarker::Grapheme);
        u16Position = it.toNextBoundary();
    } while (u16Position != -1);

    // Compute word boundaries
    boundaryType = QTextBoundaryFinder::Word;
    it = QTextBoundaryFinder(boundaryType, u16Chars, u16Length, buffer, bufferSize);
    u16Position = it.position();
    do {
        Int u8Position = u16ToU8[u16Position];
        markers[u8Position].set(TextBoundaryMarker::Word);
        if (it.boundaryReasons().testFlag(QTextBoundaryFinder::StartOfItem)) {
            markers[u8Position].set(TextBoundaryMarker::SignificantWordStart);
        }
        if (it.boundaryReasons().testFlag(QTextBoundaryFinder::EndOfItem)) {
            markers[u8Position].set(TextBoundaryMarker::SignificantWordEnd);
        }
        u16Position = it.toNextBoundary();
    } while (u16Position != -1);

    // Remove word boundaries between consecutive whitespaces. Indeed, Qt 5.15
    // considers that there are word boundaries between consecutive
    // whitespaces, but we believe it's better not to have them, and in fact a
    // new rule to removing these was added in Unicode 11.0 (2018).
    //
    for (Int i = 1; i < u8Length_; ++i) {
        if (isWSegSpace(text[i - 1]) && isWSegSpace(text[i])) {
            markers[i].unset(TextBoundaryMarker::Word);
            markers[i].unset(TextBoundaryMarker::SignificantWordStart);
            markers[i].unset(TextBoundaryMarker::SignificantWordEnd);
        }
    }

    // Compute sentence boundaries
    boundaryType = QTextBoundaryFinder::Sentence;
    it = QTextBoundaryFinder(boundaryType, u16Chars, u16Length, buffer, bufferSize);
    u16Position = it.position();
    do {
        Int u8Position = u16ToU8[u16Position];
        markers[u8Position].set(TextBoundaryMarker::Sentence);
        u16Position = it.toNextBoundary();
    } while (u16Position != -1);

    // Compute line boundaries
    boundaryType = QTextBoundaryFinder::Line;
    it = QTextBoundaryFinder(boundaryType, u16Chars, u16Length, buffer, bufferSize);
    u16Position = it.position();
    do {
        Int u8Position = u16ToU8[u16Position];
        markers[u8Position].set(TextBoundaryMarker::LineBreakOpportunity);
        if (it.boundaryReasons().testFlag(QTextBoundaryFinder::MandatoryBreak)) {
            markers[u8Position].set(TextBoundaryMarker::MandatoryLineBreak);
        }
        if (it.boundaryReasons().testFlag(QTextBoundaryFinder::SoftHyphen)) {
            markers[u8Position].set(TextBoundaryMarker::SoftHyphen);
        }
        u16Position = it.toNextBoundary();
    } while (u16Position != -1);

    return markers;
}

} // namespace vgc::graphics
