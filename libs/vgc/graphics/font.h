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

#ifndef VGC_GRAPHICS_FONT_H
#define VGC_GRAPHICS_FONT_H

#include <mutex>
#include <unordered_map>

#include <vgc/core/array.h>
#include <vgc/core/floatarray.h>
#include <vgc/core/innercore.h>
#include <vgc/core/mat3f.h>
#include <vgc/core/vec2f.h>
#include <vgc/geometry/curves2d.h>
#include <vgc/graphics/api.h>

// Manually forward-declare/typedef FreeType and HarfBuzz classes, to avoid
// including their headers. This is technically not 100% following the public
// API and *might* break in the future, but it seems unlikely.
//
typedef struct FT_LibraryRec_* FT_Library;
typedef struct FT_FaceRec_* FT_Face;
typedef struct FT_Vector_ FT_Vector;
typedef struct hb_font_t hb_font_t;

namespace vgc {
namespace graphics {

VGC_DECLARE_OBJECT(FontLibrary);
VGC_DECLARE_OBJECT(FontFace);
VGC_DECLARE_OBJECT(FontGlyph);

namespace internal {

class FontLibraryImpl;
class FontFaceImpl;
class FontGlyphImpl;
class ShapedTextImpl; // needed to befriend in FontFace

// See: https://stackoverflow.com/questions/9954518/stdunique-ptr-with-an-incomplete-type-wont-compile
// TODO: vgc/core/pimpl.h for helper macros.
struct FontLibraryImplDeleter {
    void operator()(FontLibraryImpl* p);
};
struct FontFaceImplDeleter {
    void operator()(FontFaceImpl* p);
};
struct FontGlyphImplDeleter {
    void operator()(FontGlyphImpl* p);
};
using FontLibraryPimpl = std::unique_ptr<FontLibraryImpl, FontLibraryImplDeleter>;
using FontFacePimpl = std::unique_ptr<FontFaceImpl, FontFaceImplDeleter>;
using FontGlyphPimpl = std::unique_ptr<FontGlyphImpl, FontGlyphImplDeleter>;

// Define the FontFaceImpl class. We usually only define these in .cpp files,
// but in this case we define it in the header file because ShapedTextImpl in
// text.cpp also needs to access hbFont.
//
class FontFaceImpl {
public:
    FT_Face face;
    hb_font_t* hbFont;
    double ppem;
    std::mutex glyphsMutex;
    std::unordered_map<Int, FontGlyph*> glyphsMap;
    FontFaceImpl(FT_Library library, const std::string& filename);
    ~FontFaceImpl();
};

// Converts from fractional 26.6 to floating point.
//
// Note: we use a template because FreeType and HarfBuzz use types which may or
// may not be the same underlying fundamental types:
//
// - FreeType: `typedef signed long FT_Pos;`
// - HarfBuzz: `typedef int32_t hb_position_t;`
//
template<class T>
core::Vec2d f266ToVec2d(T x, T y)
{
    return core::Vec2d(x / 64.0, y / 64.0);
}

} // namespace internal

/// \class vgc::graphics::FontLibrary
/// \brief Manages a set of available fonts.
///
/// A font library is an object used to manage a set of loaded fonts. You can
/// add new fonts to the library, remove fonts from the library, and query
/// which fonts are available in the library.
///
class VGC_GRAPHICS_API FontLibrary : public core::Object {
private:
    VGC_OBJECT(FontLibrary, core::Object)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

protected:
    /// Creates a new FontLibrary. This constructor is an implementation
    /// detail. In order to create a FontLibrary, please use the following:
    ///
    /// ```cpp
    /// FontLibraryPtr fontLibrary = FontLibrary::create();
    /// ```
    ///
    FontLibrary();

public:
    /// Creates an empty FontLibrary, that is, a font library which doesn't
    /// have any available fonts yet.
    ///
    static FontLibraryPtr create();

    /// Adds the face from the given filename to this library.
    ///
    /// ```cpp
    /// FontFace* fontFace = fontLibrary->addFace("fonts/DejaVuSerif.ttf");
    /// ```
    ///
    FontFace* addFace(const std::string& filename);

    /// Returns the default FontFace. Returns nullptr if no default FontFace
    /// has been defined via setDefaultFace().
    ///
    /// Note: for now, there is only one default FontFace, which is the one
    /// defined via setDefaultFontFace(). Unfortunately, this is not always the
    /// best approach, since this default font may not contain all the required
    /// Unicode characters (e.g., Arabic, Chinese, etc.) for a given text
    /// string. In the future, the idea is to implement an additional class
    /// called FontQuery, similar to QFont, which basically stores a desired
    /// `font-family`, `font-style`, `font-size`, etc. Like in CSS, the
    /// `font-family` can be as simple as `serif` (which means "get the default
    /// serif font"), or a more precise query with fallbacks, like `Arial,
    /// Helvetica, sans-serif`. The FontLibrary will be responsible for finding
    /// the appropriate FontFaces based on a given FontQuery and text string:
    /// there might be different FontFaces for different segments of the text.
    ///
    // Useful references:
    // https://developer.mozilla.org/en-US/docs/Web/CSS/font-family
    // https://tex.stackexchange.com/a/520048/45625
    // https://harfbuzz.github.io/harfbuzz-hb-common.html#hb-script-t
    // https://github.com/harfbuzz/harfbuzz/issues/1288
    // https://stackoverflow.com/questions/23508605/display-mixed-complex-script-in-text-editor-via-harfbuzz-freetype
    //
    FontFace* defaultFace() const;

    /// Sets the default FontFace.
    ///
    /// \sa defaultFace().
    ///
    void setDefaultFace(FontFace* fontFace);

protected:
    /// \reimp
    void onDestroyed() override;

private:
    internal::FontLibraryPimpl impl_;
};

/// Returns the global font library.
///
VGC_GRAPHICS_API FontLibrary* fontLibrary();

/// \class vgc::graphics::FontFace
/// \brief A given typeface, in a given style, in a given size.
///
/// A font face represents a given typeface, in a given style, in a given size.
/// For example, "Source Sans Pro, bold, 12pt @ 72dpi".
///
/// Note that a given typeface, even with a given style (example:
/// "SourceSansPro-Bold.otf"), may still use different glyphs based on the
/// size. For example, smaller point size (8pt) may have less details than
/// higher point sizes (36pt), and different hinting should be applied based on
/// the size. This is why we use separate FontFace objects to represent the
/// same typeface at different sizes.
///
/// If you need to perform text shaping (that is, convert an input string into
/// a sequence of glyphs from this FontFace), see the class ShapedText.
///
class VGC_GRAPHICS_API FontFace : public core::Object {
private:
    VGC_OBJECT(FontFace, core::Object)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

protected:
    /// Creates a new FontFace. This constructor is an implementation
    /// detail. In order to create a FontFace, please use the following:
    ///
    /// ```cpp
    /// FontFace* fontFace = fontLibrary->addFace(filename);
    /// ```
    ///
    FontFace(FontLibrary* library);

public:
    /// Returns the number of pixels per em of this FontFace.
    ///
    double ppem() const;

    /// Returns the height of ascenders, in pixels. See:
    ///
    /// https://www.freetype.org/freetype2/docs/glyphs/glyphs-3.html
    ///
    double ascent() const;

    /// Returns the height of descenders, in pixels. Note that it is usually a
    /// negative value. See:
    ///
    /// https://www.freetype.org/freetype2/docs/glyphs/glyphs-3.html
    ///
    double descent() const;

    /// Returns the height of this face, in pixels. This is the vertical
    /// distance between two baselines.
    ///
    double height() const;

    /// Returns the glyph corresponding to the given Unicode code point, or
    /// nullptr if this face doesn't have a glyph for this code point.
    ///
    /// ```cpp
    /// FontGlyph* fontGlyph = fontFace->getGlyphFromCodePoint(0x0041); // => 'A'
    /// ```
    ///
    /// This function is equivalent to calling getGlyphIndexFromCodePoint() then
    /// getGlyphFromIndex().
    ///
    /// Note that some glyphs may not be accessible via this function, because
    /// they do not correspond to any code point. If you need to access such
    /// glyphs, you must instead use getGlyphFromIndex() directly.
    ///
    FontGlyph* getGlyphFromCodePoint(Int codePoint);

    /// Returns the glyph at the given glyph index. This uses an internal
    /// indexing system, which may or may not be equal to the indices used in
    /// the font file, or to Unicode code points.
    ///
    /// Raises a vgc::core::FontError if the given glyphIndex is not a valid
    /// index or another error occurs.
    ///
    FontGlyph* getGlyphFromIndex(Int glyphIndex);

    /// Returns the glyph index corresponding to the given Unicode code point,
    /// or 0 if this face doesn't have a glyph for this code point.
    ///
    Int getGlyphIndexFromCodePoint(Int codePoint);

protected:
    /// \reimp
    void onDestroyed() override;

private:
    internal::FontFacePimpl impl_;
    friend class FontLibrary;
    friend class internal::FontLibraryImpl;
    friend class internal::ShapedTextImpl;
};

/// \class vgc::graphics::FontGlyph
/// \brief A given glyph of a given FontFace.
///
class VGC_GRAPHICS_API FontGlyph : public core::Object {
private:
    VGC_OBJECT(FontGlyph, core::Object)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

protected:
    /// Creates a new FontGlyph. This constructor is an implementation detail.
    /// In order to get a glyph in a given face, please use one of the following:
    ///
    /// ```cpp
    /// FontGlyph* fontGlyph1 = fontFace->getGlyphFromCodePoint(codePoint);
    /// FontGlyph* fontGlyph2 = fontFace->getGlyphFromIndex(glyphIndex);
    /// ```
    ///
    FontGlyph(FontFace* face);

public:
    /// Returns the index of this glyph. This is an integer that can be
    /// used to retrieve the glyph via `face->getGlyphFromIndex()`.
    ///
    Int index() const;

    /// Returns the name of this glyph, or an empty string if the face doesn't
    /// support glyph names.
    ///
    std::string name() const;

    /// Returns the outline of the glyph as a Curves2d.
    ///
    const geometry::Curves2d& outline() const;

    /// Appends to the given FloatArray `data` a triangulation of this glyph
    /// with the given `transform` applied, in the following format:
    ///
    ///
    /// ```
    /// [x1, y1,     // First vertex of first triangle
    ///  x2, y2,     // Second vertex of first triangle
    ///  x3, y3,     // Third vertex of first triangle
    ///
    ///  x4, y4,     // First vertex of second triangle
    ///  x5, y5,     // Second vertex of second triangle
    ///  x6, y6,     // Third vertex of second triangle
    ///
    ///  ...]
    ///
    void fill(core::FloatArray& data,
              const core::Mat3f& transform) const;

protected:
    /// \reimp
    void onDestroyed() override;

private:
    internal::FontGlyphPimpl impl_;
    friend class FontLibrary;
    friend class FontFace;
    friend class internal::FontLibraryImpl;
    friend class internal::FontFaceImpl;
};

} // namespace graphics
} // namespace vgc

#endif // VGC_GRAPHICS_FONT_H
