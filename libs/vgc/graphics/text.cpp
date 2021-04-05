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
    core::Vec2d advance;

    // HarbBuzz buffer
    //
    hb_buffer_t* buf;

    ShapedTextImpl(FontFace* face, const std::string& text) :
        facePtr(face),
        text(),
        glyphs(),
        advance(0, 0),
        buf(hb_buffer_create())
    {
        setText(text);
    }

    ShapedTextImpl(const ShapedTextImpl& other) :
        facePtr(other.facePtr),
        text(other.text),
        glyphs(other.glyphs),
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
        hb_buffer_add_utf8(buf, data, dataLength, firstChar, numChars);
        hb_buffer_guess_segment_properties(buf);
        hb_shape(facePtr->impl_->hbFont, buf, NULL, 0);

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
            core::Vec2d glyphOffset = internal::f266ToVec2d(pos.x_offset, pos.y_offset);
            core::Vec2d glyphAdvance = internal::f266ToVec2d(pos.x_advance, pos.y_advance);
            core::Vec2d glyphPosition = advance + glyphOffset;
            if (glyph) {
                glyphs.append(ShapedGlyph(glyph, glyphOffset, glyphAdvance, glyphPosition));
            }
            advance += glyphAdvance;
        }
    }

private:
    friend class ShapedText;
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

const FontFace* ShapedText::fontFace() const
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

} // namespace graphics
} // namespace vgc
