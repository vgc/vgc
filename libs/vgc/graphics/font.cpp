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

    FontLibraryImpl()
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
        // TODO: This should be passed to Face constructor and be immutable.
        // In other words:
        //
        //     FontFace = family   (example: "Arial")
        //                + weight (example: "Bold")
        //                + style  (example: "Italic")
        //                + size   (example: "12pt")
        //
        // Size of the EM square in points
        FT_UInt emWidth_ = 11;
        FT_UInt emHeight_ = 11;
        // Size of the EM square in 26.6 fractional points
        FT_F26Dot6 emWidth = 64 * core::int_cast<FT_F26Dot6>(emWidth_);
        FT_F26Dot6 emHeight = 64 * core::int_cast<FT_F26Dot6>(emHeight_);
        // Screen resolution in dpi
        // TODO: Get this from system?
        //       What if using dual monitors with different DPIs?
        FT_UInt hdpi = 96;
        FT_UInt vdpi = 96;
        ppem = static_cast<double>(emWidth_) * hdpi / 72.0; // 72 pt = 1 inch
        FT_Set_Char_Size(face, emWidth, emHeight, hdpi, vdpi);

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

class FontShaperImpl {
public:
    // Keep the FontLibrary, thus the Fontface, alive.
    // Note that the FontLibrary never destroys its children: a created FontFace
    // stays in memory forever until the library itself is destroyed().
    // XXX thread-safety? What if two threads create two FontShapers at the
    //     same time, wouldn't this cause a race condition when increasing
    //     the reference counts? Shouldn't we make the reference counts atomic,
    //     or else protect the assignment to this facePtr by a mutex?
    FontFacePtr facePtr;

    // Output of shaping
    FontItemArray items;

    // HarbBuzz buffer
    hb_buffer_t* buf;

    FontShaperImpl(FontFace* face) :
        facePtr(face),
        items(),
        buf(hb_buffer_create())
    {

    }

    FontShaperImpl(const FontShaperImpl& other) :
        facePtr(other.facePtr),
        items(other.items),
        buf(hb_buffer_create())
    {

    }

    FontShaperImpl& operator=(const FontShaperImpl& other)
    {
        if (this != &other) {
            facePtr = other.facePtr;
            items = other.items;
        }
        return *this;
    }

    ~FontShaperImpl()
    {
        hb_buffer_destroy(buf);
    }

    void shape(const std::string& text)
    {
        // HarfBuzz input
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

        // Convert to FontItemArray
        items.clear();
        core::Vec2d totalAdvance(0.0, 0.0);
        for (unsigned int i = 0; i < n; ++i) {
            hb_glyph_info_t& info = infos[i];
            hb_glyph_position_t& pos = positions[i];
            FontGlyph* glyph = facePtr->getGlyphFromIndex(info.codepoint);
            core::Vec2d offset = toVec2d(pos.x_offset, pos.y_offset);
            core::Vec2d advance = toVec2d(pos.x_advance, pos.y_advance);
            if (glyph) {
                items.append(FontItem(glyph, totalAdvance + offset));
            }
            totalAdvance += advance;
        }
    }

private:
    friend class FontShaper;
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

void FontLibrary::onDestroyed()
{
    impl_.reset();
}

FontShaper::FontShaper(FontFace* face) :
    impl_()
{
    impl_ = new internal::FontShaperImpl(face);
}

FontShaper::FontShaper(const FontShaper& other)
{
    impl_ = new internal::FontShaperImpl(*(other.impl_));
}

FontShaper::FontShaper(FontShaper&& other)
{
    impl_ = other.impl_;
    other.impl_ = nullptr;
}

FontShaper& FontShaper::operator=(const FontShaper& other)
{
    if (this != &other) {
        *impl_ = *(other.impl_);
    }
    return *this;
}

FontShaper& FontShaper::operator=(FontShaper&& other)
{
    if (this != &other) {
        impl_ = other.impl_;
        other.impl_ = nullptr;
    }
    return *this;
}

FontShaper::~FontShaper()
{
    delete impl_;
}

const FontItemArray& FontShaper::shape(const std::string& text)
{
    impl_->shape(text);
    return items();
}

const FontItemArray& FontShaper::items() const
{
    return impl_->items;
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

FontShaper FontFace::shaper()
{
    return FontShaper(this);
}

FontItemArray FontFace::shape(const std::string& text)
{
    FontShaper s = shaper();
    s.shape(text);
    FontItemArray res(std::move(s.impl_->items));
    return res;
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
