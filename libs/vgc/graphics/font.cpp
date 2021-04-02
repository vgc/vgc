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

#include <vgc/graphics/font.h>

#include <mutex>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#include <hb.h>
#include <hb-ft.h>

#include <vgc/core/array.h>
#include <vgc/core/paths.h>
#include <vgc/graphics/exceptions.h>

namespace vgc {
namespace graphics {

namespace {

const char* errorMsg(FT_Error err)
{
    // https://www.freetype.org/freetype2/docs/reference/ft2-error_enumerations.html
    #undef __FTERRORS_H__
    #define FT_ERROR_START_LIST     switch (err) {
    #define FT_ERRORDEF(e, v, s)        case e: return s;
    #define FT_ERROR_END_LIST       }
    #include FT_ERRORS_H
    return "unknown error";
}

} // namespace

namespace internal {

class FontLibraryImpl {
public:
    FT_Library library;
    FontFace* defaultFace;

    FontLibraryImpl() :
        defaultFace(nullptr)
    {
        FT_Error error = FT_Init_FreeType(&library);
        if (error) {
            throw FontError(errorMsg(error));
        }

        // Call hb_language_get_default(), protected by a mutex, to avoid
        // thread-safety problems later. See:
        //
        //   https://harfbuzz.github.io/harfbuzz-hb-common.html#hb-language-get-default
        //
        //   « Note that the first time this function is called, it calls "setlocale
        //   (LC_CTYPE, nullptr)" to fetch current locale. The underlying setlocale
        //   function is, in many implementations, NOT threadsafe. To avoid
        //   problems, call this function once before multiple threads can call it.
        //   This function is only used from hb_buffer_guess_segment_properties() by
        //   HarfBuzz itself. »
        //
        static std::mutex* mutex = new std::mutex; // leaky singleton (see vgc/core/stringid.cpp)
        std::lock_guard<std::mutex> lock(*mutex);
        hb_language_get_default();
    }

    ~FontLibraryImpl()
    {
        FT_Error error = FT_Done_FreeType(library);
        if (error) {
            // Note: we print a warning rather than throwing, because throwing
            // in destructors is a bad idea.
            core::warning() << errorMsg(error) << std::endl;
        }
    }
};

class FontFaceImpl {
public:
    FT_Face face;
    hb_font_t* hbFont;
    double ppem;

    FontFaceImpl(FT_Library library, const std::string& filename)
    {
        // Select the index of the face in the font file. For now, we only use
        // index = 0, which should work for many fonts, and is the index we must
        // use for files which only have one face.
        //
        // See:
        // https://www.freetype.org/freetype2/docs/reference/ft2-base_interface.html#ft_open_face
        //
        FT_Long index = 0;

        // Load the face.
        //
        FT_Error error = FT_New_Face(library, filename.c_str(), index, &face);
        if (error) {
            throw FontError(core::format(
                "Error loading font file {}: {}", filename, errorMsg(error)));
        }

        // Select a given charmap for character code to glyph index mapping.
        //
        // For now, we always use the UCS-2 charmap, which all fonts we are willing
        // to support should provide. However, this only gives access to characters
        // in the Basic Multilingual Plane (BMP). In the future, we should also
        // determine whether the font provides UCS-4 or UTF-8 charmaps, in which
        // case we should use these.
        //
        // See:
        // https://www.freetype.org/freetype2/docs/reference/ft2-base_interface.html#ft_set_charmap
        // https://en.wikipedia.org/wiki/Universal_Coded_Character_Set
        // https://docs.microsoft.com/en-us/typography/opentype/spec/name
        //
        // TODO: Instead of FT_Set_Charmap, we may want to use "FT_Select_CharMap" instead:
        // https://www.freetype.org/freetype2/docs/reference/ft2-base_interface.html#ft_select_charmap
        //
        bool isCharmapSet = false;
        for(FT_Int i = 0; i < face->num_charmaps; ++i) {
            FT_CharMap c = face->charmaps[i];
            if ((c->platform_id == 0 && c->encoding_id == 3) ||
                (c->platform_id == 3 && c->encoding_id == 1))
            {
                FT_Error error = FT_Set_Charmap(face, c);
                if (error) {
                    throw FontError(core::format(
                        "Error setting charmap for font file {}: {}",
                        filename, errorMsg(error)));
                }
                isCharmapSet = true;
            }
        }
        if (!isCharmapSet) {
            throw FontError(core::format(
                "Error setting charmap for font file {}: {}",
                filename, "USC-2 charmap not found"));
        }

        // Set character size.
        //
        // Currently, the font size is given in pixels, and pixelWidth is
        // assumed to be equal to pixelHeight (same hdpi as vdpi). If font size
        // given in points, or if hdpi and vdpi are different, then we should
        // perform calculation like:
        //
        // pixelHeight = fontSizeInPoints * vdpi / 72; // 72 pt = 1 inch
        // pixelWidth = fontSizeInPoints * hdpi / 72;
        //
        double fontSizeInPixels = 15;
        ppem = std::round(fontSizeInPixels);
        ppem = std::max(1.0, ppem);
        FT_UInt pixelWidth = static_cast<FT_UInt>(ppem);
        FT_UInt pixelHeight = pixelWidth;
        FT_Set_Pixel_Sizes(face, pixelWidth, pixelHeight);

        // TODO: The font size should be passed to the Face constructor and be
        // immutable. In other words:
        //
        //     FontFace = family   (example: "Arial")
        //                + weight (example: "Bold")
        //                + style  (example: "Italic")
        //                + size   (example: "12pt")
        //
        // The reason why the size should be immutable and part of the identity
        // of a FontFace is that it affects both hinting of the vector outline
        // (in the case of vector fonts) and bitmap selection (in the case of
        // bitmap fonts with different bitmaps provided for different fonts),
        // both of which can be cached for performance. In order to represent a
        // "resolution-independent font face" (for exemple, if the text size is
        // animated or the output resolution unknown), then perhaps it should
        // be a special "size" whose value is "scalable", in which case the
        // font outline isn't hinted and is expressed in font units rather than
        // pixels.
        //
        // Note that currently, "font-size" is interpreted as "size of the EM
        // square", which is what CSS does. Such font-size is neither the
        // capital height (height of "A"), nor the x height (height of "x"),
        // nor the line height (distance between descender and ascender,
        // including the line gap or not), but an arbitrary metric set by the
        // font designer. It would be nice to add another font property like:
        //
        // font-size-mode: em | ascent | descent | height | capital-height | x-height
        //
        // So that UI designer could specify for example that they want the
        // capital height to be a specific value in pixels.
        //
        // Such font-size-mode (or a separate vertical-align-mode) may also be
        // used to specify "what" should be vertically centered: should capital
        // letters be vertically centered? Or should the lowercase letter "x"
        // be vertically centered? Or should the "height" (area from ascender
        // to descender) be vertically centered? The latter is the current
        // behavior, and also the behavior of CSS.
        //
        // The following article is an amazing write-up about this subject,
        // including CSS tricks to compute the appropriate font-size such that
        // the capital height has the desired number of pixels, and such that
        // capital letters are properly vertically centered:
        //
        // https://iamvdo.me/en/blog/css-font-metrics-line-height-and-vertical-align
        //
        // In my experience, having capital letters perfectly centered is
        // indeed what looks best, especially in buttons and form fields. For
        // example, this is the choice of Chrome in my current setup: in the
        // address bar, a capital letter is exactly 10px high, with 9px of
        // space above and 9px below, for a total address bar height of 28px:
        //
        // -------- top of address bar
        //
        //
        // 8px
        //
        //
        // -------- top of ascending letters
        // 1px
        // -------- top of capital letters
        // 2px
        //
        // -------- top of non-ascending letters (x height)
        //
        //
        // 8px
        //
        //
        // -------- baseline
        //
        // 4px
        //
        // -------- bottom of descending letters
        //
        // 5px
        //
        // -------- bottom of address bar
        //
        // Finally, note that there is one more thing to take into
        // consideration when dealing with font size: the "ascent" and
        // "descent" metadata values stored in font files do not generally
        // correspond to the top of ascending letters and the bottom of
        // descending letters. There are in fact several of these metadata
        // values (HHead Ascent, Win Ascent, Typo Ascent, ...), which may or
        // may not have the same value, and may or may not be the real distance
        // between the baseline and the top of ascending letters. It is yet
        // unclear which of these is stored by Freetype in face->ascender, but
        // if necessary, it is possible to query directly the OS/2 metrics or
        // horizontal header metrics via:
        //
        //     #include FT_TRUETYPE_TABLES_H
        //
        //     TT_OS2* os2 = (TT_OS2*)FT_Get_Sfnt_Table(face, FT_SFNT_OS2);
        //     if (os2) {
        //         FT_UShort version = os2->version;
        //         FT_Short  typoAscent  = os2->sTypoAscender;
        //         FT_Short  typoDescent = os2->sTypoDescender;
        //         FT_Short  typoLineGap = os2->sTypoLineGap;
        //         FT_UShort winAscent   = os2->usWinAscent;
        //         FT_UShort winDescent  = os2->usWinDescent;
        //         FT_Short  xHeight     = os2->sxHeight;   // only version 2 and higher
        //         FT_Short  capHeight   = os2->sCapHeight; // only version 2 and higher
        //     }
        //
        //     TT_HoriHeader* hhead = (TT_HoriHeader*)FT_Get_Sfnt_Table(face, FT_SFNT_HHEA);
        //     if (hhead) {
        //         FT_Short ascender  = hhead->Ascender;
        //         FT_Short descender = hhead->Descender;
        //         FT_Short lineGap   = hhead->Line_Gap;
        //     }
        //
        // See: https://www.freetype.org/freetype2/docs/reference/ft2-truetype_tables.html
        //
        // One potential option may also be to load for example the "A" glyph,
        // the "x" glyph, the "b" glyph, and the "p" glyph, and directly
        // measure the ascender and descender based on the glyph outline,
        // ignoring font metadata. Unfortunately, this method might not always
        // be reliable, in particular with cursive/handwriting/display/fantasy
        // categories of fonts.

        // Create HarfBuzz font
        hbFont = hb_ft_font_create(face, NULL);
    }

    ~FontFaceImpl()
    {
        hb_font_destroy(hbFont);
        FT_Error error = FT_Done_Face(face);
        if (error) {
            // Note: we print a warning rather than throwing, because throwing
            // in destructors is a bad idea.
            core::warning() << errorMsg(error) << std::endl;
        }
    }
};

namespace {

// Convert from fractional 26.6 to floating point.
//
// Note: we use a template because FreeType and HarfBuzz use types which may or
// may not be the same underlying fundamental types:
//
// - FreeType: `typedef signed long  FT_Pos;`
// - HarfBuzz: `typedef int32_t hb_position_t;`
//
template<class T>
core::Vec2d toVec2d(T x, T y)
{
    return core::Vec2d(x / 64.0, y / 64.0);
}

// Convert from fractional 26.6 to floating point
//
core::Vec2d toVec2d(const FT_Vector* v)
{
    return toVec2d(v->x, v->y);
}

void closeLastCurveIfOpen(geometry::Curves2d& c)
{
    geometry::Curves2dCommandRange commands = c.commands();
    geometry::Curves2dCommandIterator it = commands.end();
    if (it != commands.begin()) {
        --it;
        if (it->type() != geometry::CurveCommandType::Close) {
            c.close();
        }
    }
}

int moveTo(const FT_Vector* to,
           void* user)
{
    geometry::Curves2d* c = static_cast<geometry::Curves2d*>(user);
    closeLastCurveIfOpen(*c);
    c->moveTo(toVec2d(to));
    return 0;
}

int lineTo(const FT_Vector* to,
           void* user)
{
    geometry::Curves2d* c = static_cast<geometry::Curves2d*>(user);
    c->lineTo(toVec2d(to));
    return 0;
}

int conicTo(const FT_Vector* control,
            const FT_Vector* to,
            void* user)
{
    geometry::Curves2d* c = static_cast<geometry::Curves2d*>(user);
    c->quadraticBezierTo(toVec2d(control), toVec2d(to));
    return 0;
}

int cubicTo(const FT_Vector* control1,
            const FT_Vector* control2,
            const FT_Vector* to,
            void* user)
{
    geometry::Curves2d* c = static_cast<geometry::Curves2d*>(user);
    c->cubicBezierTo(toVec2d(control1), toVec2d(control2), toVec2d(to));
    return 0;
}

} // namespace

class FontGlyphImpl {
public:
    Int index;
    std::string name;
    geometry::Curves2d outline;

    FontGlyphImpl(Int index, const char* name, FT_GlyphSlot slot) :
        index(index),
        name(name)
    {
        int shift = 0;
        FT_Pos delta = 0;
        FT_Outline_Funcs f{&moveTo, &lineTo, &conicTo, &cubicTo, shift, delta};
        FT_Outline_Decompose(&slot->outline, &f, static_cast<void*>(&outline));
        closeLastCurveIfOpen(outline);
    }
};

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
            core::Vec2d glyphOffset = toVec2d(pos.x_offset, pos.y_offset);
            core::Vec2d glyphAdvance = toVec2d(pos.x_advance, pos.y_advance);
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

void FontLibraryImplDeleter::operator()(FontLibraryImpl* p)
{
    delete p;
}

void FontFaceImplDeleter::operator()(FontFaceImpl* p)
{
    delete p;
}

void FontGlyphImplDeleter::operator()(FontGlyphImpl* p)
{
    delete p;
}

} // namespace internal

FontLibrary::FontLibrary() :
    Object(),
    impl_(new internal::FontLibraryImpl())
{

}

// static
FontLibraryPtr FontLibrary::create()
{
    return FontLibraryPtr(new FontLibrary());
}

FontFace* FontLibrary::addFace(const std::string& filename)
{
    FontFace* res = new FontFace(this);
    res->impl_.reset(new internal::FontFaceImpl(impl_->library, filename));
    return res;
}

FontFace* FontLibrary::defaultFace() const
{
    return impl_->defaultFace;
}

void FontLibrary::setDefaultFace(FontFace* fontFace)
{
    impl_->defaultFace = fontFace;
}

void FontLibrary::onDestroyed()
{
    impl_.reset();
}

namespace {

FontLibraryPtr createGlobalFontLibrary()
{
    std::string facePath = core::resourcePath("graphics/fonts/SourceSansPro/TTF/SourceSansPro-Regular.ttf");
    graphics::FontLibraryPtr fontLibrary = graphics::FontLibrary::create();
    graphics::FontFace* fontFace = fontLibrary->addFace(facePath); // XXX can this be nullptr?
    fontLibrary->setDefaultFace(fontFace);
    return fontLibrary;
}

} // namespace

FontLibrary* fontLibrary()
{
    static FontLibraryPtr res = createGlobalFontLibrary();
    return res.get();
}

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

void ShapedText::setText(const std::string& text)
{
    impl_->setText(text);
}

const FontFace* ShapedText::fontFace() const
{
    return impl_->facePtr.get();
}

const std::string& ShapedText::text() const
{
    return impl_->text;
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

FontFace::FontFace(FontLibrary* library) :
    Object(),
    impl_()
{
    appendObjectToParent_(library);
}

namespace {

double fontUnitsToPixels(const internal::FontFaceImpl* impl, FT_Short u)
{
    return static_cast<double>(u) * impl->ppem / impl->face->units_per_EM;
}

} // namespace

double FontFace::ppem() const
{
    return impl_->ppem;
}

double FontFace::ascent() const
{
    return fontUnitsToPixels(impl_.get(), impl_->face->ascender);
}

double FontFace::descent() const
{
    return fontUnitsToPixels(impl_.get(), impl_->face->descender);
}

double FontFace::height() const
{
    return fontUnitsToPixels(impl_.get(), impl_->face->height);
}

FontGlyph* FontFace::getGlyphFromCodePoint(Int codePoint)
{
    if (Int index = getGlyphIndexFromCodePoint(codePoint)) {
        return getGlyphFromIndex(index);
    }
    else {
        return nullptr;
    }
}

FontGlyph* FontFace::getGlyphFromIndex(Int glyphIndex)
{
    // Load glyph data
    FT_Face face = impl_->face;
    FT_UInt index = core::int_cast<FT_UInt>(glyphIndex);
    FT_Int32 flags = FT_LOAD_NO_BITMAP;
    FT_Error error = FT_Load_Glyph(face, index, flags);
    if (error) {
        throw FontError(errorMsg(error));
    }

    // Get glyph name
    const FT_UInt bufferMax = 1024;
    char name[bufferMax];
    if (FT_HAS_GLYPH_NAMES(face)) {
        FT_Error error = FT_Get_Glyph_Name(face, index, name, bufferMax);
        if (error) {
            // Note: This code path is believed to be unreachable since we
            // premptively check for FT_HAS_GLYPH_NAMES, and we know the index
            // is valid. We still keep it for added safety, or if Freetype adds
            // more error cases in the future.
            throw FontError(errorMsg(error));
        }
    }

    // Create FontGlyph object and copy data to object
    FontGlyph* glyph = new FontGlyph(this);
    FT_GlyphSlot slot = face->glyph;
    glyph->impl_.reset(new internal::FontGlyphImpl(glyphIndex, name, slot));
    return glyph;
}

Int FontFace::getGlyphIndexFromCodePoint(Int codePoint)
{
    // Note: we assume the charmap is unicode
    FT_Face face = impl_->face;
    FT_ULong charcode = core::int_cast<FT_ULong>(codePoint);
    FT_UInt index = FT_Get_Char_Index(face, charcode);
    return core::int_cast<Int>(index);
}

ShapedText FontFace::shape(const std::string& text)
{
    return ShapedText(this, text);
}

void FontFace::onDestroyed()
{
    impl_.reset();
}

FontGlyph::FontGlyph(FontFace* face) :
    Object(),
    impl_()
{
    appendObjectToParent_(face);
}

Int FontGlyph::index() const
{
    return impl_->index;
}

std::string FontGlyph::name() const
{
    return impl_->name;
}

const geometry::Curves2d& FontGlyph::outline() const
{
    return impl_->outline;
}

void FontGlyph::onDestroyed()
{
    impl_.reset();
}

} // namespace graphics
} // namespace vgc
