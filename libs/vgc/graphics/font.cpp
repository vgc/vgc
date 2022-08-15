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

#include <hb-ft.h>
#include <hb.h>

#include <vgc/core/paths.h>
#include <vgc/graphics/exceptions.h>
#include <vgc/graphics/logcategories.h>

namespace vgc::graphics {

namespace {

// clang-format off

const char* errorMsg(FT_Error err) {
    // https://www.freetype.org/freetype2/docs/reference/ft2-error_enumerations.html
    #undef __FTERRORS_H__
    #define FT_ERROR_START_LIST  switch (err) {
    #define FT_ERRORDEF(e, v, s)     case e: return s;
    #define FT_ERROR_END_LIST    }
    #include FT_ERRORS_H
    return "unknown error";
}

// clang-format on

} // namespace

namespace detail {

class FontLibraryImpl {
public:
    FT_Library library;
    Font* defaultFont;

    FontLibraryImpl()
        : defaultFont(nullptr) {

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
        static std::mutex* mutex = new std::mutex; // trusty leaky singleton
        std::lock_guard<std::mutex> lock(*mutex);
        hb_language_get_default();
    }

    ~FontLibraryImpl() {
        FT_Error error = FT_Done_FreeType(library);
        if (error) {
            // Note: we print a warning rather than throwing, because throwing
            // in destructors is a bad idea.
            VGC_WARNING(LogVgcGraphics, errorMsg(error));
        }
    }
};

namespace {

void ftNewFace_(
    const std::string& filename,
    Int index,
    FT_Library ftLibrary,
    FT_Face* ftFace) {

    // Load the face.
    //
    FT_Error error =
        FT_New_Face(ftLibrary, filename.c_str(), core::int_cast<FT_Long>(index), ftFace);
    if (error) {
        throw FontError(
            core::format("Error loading font file {}: {}", filename, errorMsg(error)));
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
    FT_Face face = *ftFace;
    bool isCharmapSet = false;
    for (FT_Int i = 0; i < face->num_charmaps; ++i) {
        FT_CharMap c = face->charmaps[i];
        if ((c->platform_id == 0 && c->encoding_id == 3)
            || (c->platform_id == 3 && c->encoding_id == 1)) {
            error = FT_Set_Charmap(face, c);
            if (error) {
                throw FontError(core::format(
                    "Error setting charmap for font file {}: {}",
                    filename,
                    errorMsg(error)));
            }
            isCharmapSet = true;
        }
    }
    if (!isCharmapSet) {
        throw FontError(core::format(
            "Error setting charmap for font file {}: {}",
            filename,
            "USC-2 charmap not found"));
    }
}

void ftDoneFace_(FT_Face ftFace) {
    FT_Error error = FT_Done_Face(ftFace);
    if (error) {
        // Note: we print a warning rather than throwing, because throwing
        // in destructors is a bad idea.
        VGC_WARNING(LogVgcGraphics, errorMsg(error));
    }
}

} // namespace

class FontImpl {
public:
    std::string filename;
    Int index;
    FT_Library ftLibrary;
    FT_Face ftFace;
    std::mutex glyphsMutex;
    std::unordered_map<Int, Glyph*> glyphsMap;
    std::mutex sizedFontsMutex;
    std::unordered_map<SizedFontParams, SizedFont*> sizedFontsMap;

    FontImpl(const std::string& filename, Int index, FT_Library library)
        : filename(filename)
        , index(index)
        , ftLibrary(library) {

        ftNewFace_(filename, index, library, &ftFace);
    }

    ~FontImpl() {
        ftDoneFace_(ftFace);
    }
};

SizedFontImpl::SizedFontImpl(Font* font, const SizedFontParams& params_)
    : params(params_) {

    const std::string& filename = font->impl_->filename;
    Int index = font->index();
    FT_Library ftLibrary = font->impl_->ftLibrary;
    ftNewFace_(filename, index, ftLibrary, &ftFace);

    // Set pixel sizes
    Int ppemWidth = params.ppemWidth();
    Int ppemHeight = params.ppemHeight();
    FontHinting hinting = params.hinting();
    if (ppemWidth < 1 || ppemHeight < 1) {
        Int ppemWidth_ = (std::min)(Int(1), ppemWidth);
        Int ppemHeight_ = (std::min)(Int(1), ppemHeight);
        VGC_WARNING(
            LogVgcGraphics,
            "Provided (ppemWidth, ppemHeight) = ({}, {}) must"
            " be at least 1. Using ({}, {}) instead.",
            ppemWidth,
            ppemHeight,
            ppemWidth_,
            ppemHeight_);
        params = SizedFontParams(ppemWidth_, ppemHeight_, hinting);
        ppemWidth = ppemWidth_;
        ppemHeight = ppemHeight_;
    }
    FT_UInt pixelWidth = core::int_cast<FT_UInt>(ppemWidth);
    FT_UInt pixelHeight = core::int_cast<FT_UInt>(ppemHeight);
    FT_Set_Pixel_Sizes(ftFace, pixelWidth, pixelHeight);

    // Note that "font-size" means "size of the EM square", as is traditionally
    // done (CSS, etc.). Such font-size is neither the capital height (height
    // of "A"), nor the x height (height of "x"), nor the line height (distance
    // between descender and ascender, including the line gap or not), but an
    // arbitrary metric set by the font designer. It would be nice to add
    // another font property like:
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

    // Create HarfBuzz font.
    // Note: `hbFont` will use the same ppem as provided by `face`.
    hbFont = hb_ft_font_create(ftFace, NULL);
}

SizedFontImpl::~SizedFontImpl() {
    hb_font_destroy(hbFont);
    ftDoneFace_(ftFace);
}

namespace {

// Converts from fractional 26.6 to floating point.
geometry::Vec2d f266ToVec2d(const FT_Vector* v) {
    return detail::f266ToVec2d(v->x, v->y);
}

void closeLastCurveIfOpen(geometry::Curves2d& c) {
    geometry::Curves2dCommandRange commands = c.commands();
    geometry::Curves2dCommandIterator it = commands.end();
    if (it != commands.begin()) {
        --it;
        if (it->type() != geometry::CurveCommandType::Close) {
            c.close();
        }
    }
}

int moveTo(const FT_Vector* to, void* user) {
    geometry::Curves2d* c = static_cast<geometry::Curves2d*>(user);
    closeLastCurveIfOpen(*c);
    c->moveTo(f266ToVec2d(to));
    return 0;
}

int lineTo(const FT_Vector* to, void* user) {
    geometry::Curves2d* c = static_cast<geometry::Curves2d*>(user);
    c->lineTo(f266ToVec2d(to));
    return 0;
}

int conicTo(const FT_Vector* control, const FT_Vector* to, void* user) {
    geometry::Curves2d* c = static_cast<geometry::Curves2d*>(user);
    c->quadraticBezierTo(f266ToVec2d(control), f266ToVec2d(to));
    return 0;
}

int cubicTo(
    const FT_Vector* control1,
    const FT_Vector* control2,
    const FT_Vector* to,
    void* user) {

    geometry::Curves2d* c = static_cast<geometry::Curves2d*>(user);
    c->cubicBezierTo(f266ToVec2d(control1), f266ToVec2d(control2), f266ToVec2d(to));
    return 0;
}

} // namespace

class SizedGlyphImpl {
public:
    Glyph* glyph;
    geometry::Curves2d outline;
    core::FloatArray triangles;
    geometry::Rect2f boundingBox = geometry::Rect2f::empty;

    SizedGlyphImpl(Glyph* glyph, FT_GlyphSlot slot)
        : glyph(glyph) {

        // Note: hinting might already be baked in the given FT_GlyphSlot.
        // See implementation of SizedFont::getGlyphFromIndex(Int glyphIndex).
        int shift = 0;
        FT_Pos delta = 0;
        FT_Outline_Funcs f{&moveTo, &lineTo, &conicTo, &cubicTo, shift, delta};
        FT_Outline_Decompose(&slot->outline, &f, static_cast<void*>(&outline));
        closeLastCurveIfOpen(outline);
        outline.fill(triangles, geometry::Curves2dSampleParams::semiAdaptive(1.0));
        for (const geometry::Vec2f& point :
             geometry::Vec2fConstSpan(triangles.data(), triangles.length() / 2, 2)) {
            boundingBox.uniteWith(point);
        }
    }
};

void FontLibraryImplDeleter::operator()(FontLibraryImpl* p) {
    delete p;
}

void FontImplDeleter::operator()(FontImpl* p) {
    delete p;
}

void SizedFontImplDeleter::operator()(SizedFontImpl* p) {
    delete p;
}

void SizedGlyphImplDeleter::operator()(SizedGlyphImpl* p) {
    delete p;
}

} // namespace detail

FontLibrary::FontLibrary()
    : Object()
    , impl_(new detail::FontLibraryImpl()) {
}

// static
FontLibraryPtr FontLibrary::create() {
    return FontLibraryPtr(new FontLibrary());
}

Font* FontLibrary::addFont(const std::string& filename, Int index) {
    Font* res = new Font(this);
    res->impl_.reset(new detail::FontImpl(filename, index, impl_->library));
    return res;
}

Font* FontLibrary::defaultFont() const {
    return impl_->defaultFont;
}

void FontLibrary::setDefaultFont(Font* font) {
    impl_->defaultFont = font;
}

void FontLibrary::onDestroyed() {
    impl_.reset();
}

namespace {

FontLibraryPtr createGlobalFontLibrary() {
    std::string fontPath =
        core::resourcePath("graphics/fonts/SourceSansPro/TTF/SourceSansPro-Regular.ttf");
    graphics::FontLibraryPtr fontLibrary = graphics::FontLibrary::create();
    graphics::Font* font = fontLibrary->addFont(fontPath); // XXX can this be nullptr?
    fontLibrary->setDefaultFont(font);
    return fontLibrary;
}

} // namespace

FontLibrary* fontLibrary() {
    static FontLibraryPtr res = createGlobalFontLibrary();
    return res.get();
}

Font::Font(FontLibrary* library)
    : Object()
    , impl_() {

    appendObjectToParent_(library);
}

FontLibrary* Font::library() const {
    return static_cast<FontLibrary*>(parentObject());
}

Int Font::index() const {
    return impl_->index;
}

SizedFont* Font::getSizedFont(const SizedFontParams& params) {

    // Prevent two threads from modifying the glyphsMap at the same time
    const std::lock_guard<std::mutex> lock(impl_->sizedFontsMutex);

    // Get existing SizedGlyph* or insert nullptr in the map
    SizedFont*& sizedFont = impl_->sizedFontsMap[params];

    // If no existing SizedGlyph*, create it
    if (!sizedFont) {
        sizedFont = new SizedFont(this);
        sizedFont->impl_.reset(new detail::SizedFontImpl(this, params));
    }

    return sizedFont;
}

Glyph* Font::getGlyphFromCodePoint(Int codePoint) {
    if (Int index = getGlyphIndexFromCodePoint(codePoint)) {
        return getGlyphFromIndex(index);
    }
    else {
        return nullptr;
    }
}

Glyph* Font::getGlyphFromIndex(Int glyphIndex) {

    // Prevent two threads from modifying the glyphsMap at the same time
    const std::lock_guard<std::mutex> lock(impl_->glyphsMutex);

    // Get existing SizedGlyph* or insert nullptr in the map
    Glyph*& glyph = impl_->glyphsMap[glyphIndex];

    // If no existing SizedGlyph*, create it
    if (!glyph) {
        FT_Face face = impl_->ftFace;
        FT_UInt index = core::int_cast<FT_UInt>(glyphIndex);
        FT_Int32 flags = FT_LOAD_NO_BITMAP | FT_LOAD_NO_SCALE;
        FT_Error error = FT_Load_Glyph(face, index, flags);
        if (error) {
            throw FontError(errorMsg(error));
        }

        // Get glyph name
        const FT_UInt bufferMax = 1024;
        char name[bufferMax];
        if (FT_HAS_GLYPH_NAMES(face)) {
            error = FT_Get_Glyph_Name(face, index, name, bufferMax);
            if (error) {
                throw FontError(errorMsg(error));
            }
        }

        // Create Glyph object
        glyph = new Glyph(this, glyphIndex, name);
    }

    return glyph;
}

Int Font::getGlyphIndexFromCodePoint(Int codePoint) {
    // Note: we assume the charmap is unicode
    FT_Face face = impl_->ftFace;
    FT_ULong charcode = core::int_cast<FT_ULong>(codePoint);
    FT_UInt index = FT_Get_Char_Index(face, charcode);
    return core::int_cast<Int>(index);
}

void Font::onDestroyed() {
    impl_.reset();
}

Glyph::Glyph(Font* font, Int index, const std::string& name)
    : index_(index)
    , name_(name) {

    appendObjectToParent_(font);
}

Font* Glyph::font() const {
    return static_cast<Font*>(parentObject());
}

SizedFont::SizedFont(Font* font)
    : Object()
    , impl_() {

    appendObjectToParent_(font);
}

namespace {

float fontUnitsToVerticalPixels(const detail::SizedFontImpl* impl, FT_Short u) {
    Int ppem = impl->params.ppemHeight();
    return static_cast<float>(u) * ppem / impl->ftFace->units_per_EM;
}

} // namespace

Font* SizedFont::font() const {
    return static_cast<Font*>(parentObject());
}

const SizedFontParams& SizedFont::params() const {
    return impl_->params;
}

float SizedFont::ascent() const {
    return fontUnitsToVerticalPixels(impl_.get(), impl_->ftFace->ascender);
}

float SizedFont::descent() const {
    return fontUnitsToVerticalPixels(impl_.get(), impl_->ftFace->descender);
}

float SizedFont::height() const {
    return fontUnitsToVerticalPixels(impl_.get(), impl_->ftFace->height);
}

SizedGlyph* SizedFont::getSizedGlyphFromCodePoint(Int codePoint) {
    if (Int index = getGlyphIndexFromCodePoint(codePoint)) {
        return getSizedGlyphFromIndex(index);
    }
    else {
        return nullptr;
    }
}

SizedGlyph* SizedFont::getSizedGlyphFromIndex(Int glyphIndex) {

    // Prevent two threads from modifying the glyphsMap at the same time
    const std::lock_guard<std::mutex> lock(impl_->glyphsMutex);

    // Get existing SizedGlyph* or insert nullptr in the map
    SizedGlyph*& sizedGlyph = impl_->glyphsMap[glyphIndex];

    // If no existing SizedGlyph*, create it
    if (!sizedGlyph) {
        // Load glyph
        // See https://freetype.org/freetype2/docs/reference/ft2-base_interface.html#ft_load_xxx
        FT_Face face = impl_->ftFace;
        FT_UInt index = core::int_cast<FT_UInt>(glyphIndex);
        FT_Int32 flags = FT_LOAD_NO_BITMAP;
        switch (impl_->params.hinting()) {
        case FontHinting::None:
            flags |= FT_LOAD_NO_HINTING;
            break;
        case FontHinting::Native:
            flags |= FT_LOAD_NO_AUTOHINT;
            break;
        case FontHinting::AutoLight:
            flags |= FT_LOAD_FORCE_AUTOHINT;
            flags |= FT_LOAD_TARGET_LIGHT;
            break;
        case FontHinting::AutoNormal:
            flags |= FT_LOAD_FORCE_AUTOHINT;
            flags |= FT_LOAD_TARGET_NORMAL;
            break;
        }
        FT_Error error = FT_Load_Glyph(face, index, flags);
        if (error) {
            throw FontError(errorMsg(error));
        }

        // Get unsized Glyph object
        Glyph* glyph = font()->getGlyphFromIndex(glyphIndex);

        // Create SizedGlyph object and copy data to object
        sizedGlyph = new SizedGlyph(this);
        FT_GlyphSlot slot = face->glyph;
        sizedGlyph->impl_.reset(new detail::SizedGlyphImpl(glyph, slot));
    }

    return sizedGlyph;
}

Int SizedFont::getGlyphIndexFromCodePoint(Int codePoint) {
    // Note: we assume the charmap is unicode
    FT_Face face = impl_->ftFace;
    FT_ULong charcode = core::int_cast<FT_ULong>(codePoint);
    FT_UInt index = FT_Get_Char_Index(face, charcode);
    return core::int_cast<Int>(index);
}

void SizedFont::onDestroyed() {
    impl_.reset();
}

SizedGlyph::SizedGlyph(SizedFont* sizedFont)
    : Object()
    , impl_() {

    appendObjectToParent_(sizedFont);
}

SizedFont* SizedGlyph::sizedFont() const {
    return static_cast<SizedFont*>(parentObject());
}

Glyph* SizedGlyph::glyph() const {
    return impl_->glyph;
}

Int SizedGlyph::index() const {
    return impl_->glyph->index();
}

const std::string& SizedGlyph::name() const {
    return impl_->glyph->name();
}

const geometry::Curves2d& SizedGlyph::outline() const {
    return impl_->outline;
}

const geometry::Rect2f& SizedGlyph::boundingBox() const {
    return impl_->boundingBox;
}

void SizedGlyph::fill(core::FloatArray& data, const geometry::Mat3f& transform) const {

    // Note: if the SizedGlyph have hinting enabled, it only makes sense to use
    // a `transform` that has a scale ratio of 1 or -1 in each axis, and use
    // integer values for the translate part. Also, even with hinting disabled,
    // if the SizedGlyph has a small ppem then the cached tesselation may have a
    // small number of triangles not suitable to draw at larger sizes.

    Int numVertices = impl_->triangles.length() / 2;
    Int oldLength = data.length();

    data.resizeNoInit(oldLength + 2 * numVertices);

    const float* in = impl_->triangles.begin();
    float* out = data.begin() + oldLength;
    float* end = data.end();

    while (out != end) {
        float x = *in++;
        float y = *in++;
        geometry::Vec2f v = transform.transformPoint({x, y});
        *out++ = v[0];
        *out++ = v[1];
    }
}

void SizedGlyph::fill(core::FloatArray& data, const geometry::Vec2f& translation) const {

    Int numVertices = impl_->triangles.length() / 2;
    Int oldLength = data.length();

    data.resizeNoInit(oldLength + 2 * numVertices);

    const float* in = impl_->triangles.begin();
    float* out = data.begin() + oldLength;
    float* end = data.end();
    float x0 = translation[0];
    float y0 = translation[1];

    while (out != end) {
        float x = *in++;
        float y = *in++;
        *out++ = x0 + x;
        *out++ = y0 + y;
    }
}

void SizedGlyph::fillYMirrored( //
    core::FloatArray& data,
    const geometry::Vec2f& translation) const {

    Int numVertices = impl_->triangles.length() / 2;
    Int oldLength = data.length();

    data.resizeNoInit(oldLength + 2 * numVertices);

    const float* in = impl_->triangles.begin();
    float* out = data.begin() + oldLength;
    float* end = data.end();
    float x0 = translation[0];
    float y0 = translation[1];

    while (out != end) {
        float x = *in++;
        float y = *in++;
        *out++ = x0 + x;
        *out++ = y0 - y;
    }
}

void SizedGlyph::onDestroyed() {
    impl_.reset();
}

} // namespace vgc::graphics
