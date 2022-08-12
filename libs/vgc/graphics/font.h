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
#include <vgc/core/innercore.h>
#include <vgc/geometry/curves2d.h>
#include <vgc/geometry/mat3f.h>
#include <vgc/geometry/rect2f.h>
#include <vgc/geometry/vec2f.h>
#include <vgc/graphics/api.h>

// Manually forward-declare/typedef FreeType and HarfBuzz classes, to avoid
// including their headers. This is technically not 100% following the public
// API and *might* break in the future, but it seems unlikely.
//
typedef struct FT_LibraryRec_* FT_Library;
typedef struct FT_FaceRec_* FT_Face;
typedef struct FT_Vector_ FT_Vector;
typedef struct hb_font_t hb_font_t;

namespace vgc::graphics {

VGC_DECLARE_OBJECT(FontLibrary);
VGC_DECLARE_OBJECT(Font);
VGC_DECLARE_OBJECT(Glyph);
VGC_DECLARE_OBJECT(SizedFont);
VGC_DECLARE_OBJECT(SizedGlyph);
// TODO: FontFamily

/// How much hinting should be applied to a font.
///
enum class FontHinting {
    None,
    Native,
    AutoLight,
    AutoNormal
};

/// \class vgc::graphics::SizedFontParams
/// \brief The size and hinting parameters defining a `SizedFont`.
///
class VGC_GRAPHICS_API SizedFontParams {
public:
    /// Creates a SizedFontParams with the given size (expressed in pixels per
    /// EM-square) and the given hinting parameters.
    ///
    /// The given `ppemWidth` and `ppemHeight` represent the how many physical
    /// pixels one EM-square of the font should take. Typically, `ppemWidth`
    /// and `ppemHeight` are equal, but they may be different for screens whose
    /// pixel density is different in the horizontal and vertical direction
    /// (that is, non-square pixels).
    ///
    SizedFontParams(Int ppemWidth, Int ppemHeight, FontHinting hinting)
        : ppemWidth_(ppemWidth)
        , ppemHeight_(ppemHeight)
        , hinting_(hinting) {
    }

    /// Creates a SizedFontParams with the given size (expressed in pixels per
    /// EM-square) and the given hinting parameters.
    ///
    /// This is equivalent to `SizedFontParams(ppem, ppem, hinting)`.
    ///
    SizedFontParams(Int ppem, FontHinting hinting)
        : SizedFontParams(ppem, ppem, hinting) {
    }

    /// Creates a SizedFontParams with the given size in points (e.g., `12` for
    /// a 12pt font), screen dpi, and hinting parameters.
    ///
    /// This is equivalent to `SizedFontParams(ppem, hinting)` with `ppem =
    /// pointSize * dpi / 72`.
    ///
    static SizedFontParams fromPoints(Int pointSize, Int dpi, FontHinting hinting) {
        return SizedFontParams(pointSize * dpi / 72, hinting);
    }

    /// Creates a SizedFontParams with the given width in points (e.g., `12`
    /// for a 12pt font), screen horizontal and vertical dpi, and hinting
    /// parameters.
    ///
    /// This is equivalent to `SizedFontParams(ppemWidth, ppemHeight, hinting)` with:
    ///
    /// ```
    /// ppemWidth = fontSizeInPoints * hdpi / 72;
    /// ppemHeight = fontSizeInPoints * vdpi / 72;
    /// ```
    ///
    static SizedFontParams
    fromPoints(Int pointSize, Int hdpi, Int vdpi, FontHinting hinting) {
        return SizedFontParams(pointSize * hdpi / 72, pointSize * vdpi / 72, hinting);
    }

    /// Returns how many physical horizontal pixels does one EM-square of the font takes.
    ///
    Int ppemWidth() const {
        return ppemWidth_;
    }

    /// Returns how many physical vertical pixels does one EM-square of the font takes.
    ///
    Int ppemHeight() const {
        return ppemHeight_;
    }

    /// Returns the hinting parameters of the font.
    ///
    FontHinting hinting() const {
        return hinting_;
    }

    /// Returns whether the two SizedFontParams are equal.
    ///
    bool operator==(const SizedFontParams& other) const {
        return ppemWidth_ == other.ppemWidth_ && ppemHeight_ == other.ppemHeight_
               && hinting_ == other.hinting_;
    }

    /// Returns whether the two SizedFontParams are different.
    ///
    bool operator!=(const SizedFontParams& other) const {
        return !(*this == other);
    }

private:
    Int ppemWidth_;
    Int ppemHeight_;
    FontHinting hinting_;
};

namespace detail {

class FontLibraryImpl;
class FontImpl;
class SizedFontImpl;
class SizedGlyphImpl;
class ShapedTextImpl; // needed to befriend in SizedFont

// See: https://stackoverflow.com/questions/9954518/stdunique-ptr-with-an-incomplete-type-wont-compile
// TODO: vgc/core/pimpl.h for helper macros.
struct FontLibraryImplDeleter {
    void operator()(FontLibraryImpl* p);
};
struct FontImplDeleter {
    void operator()(FontImpl* p);
};
struct SizedFontImplDeleter {
    void operator()(SizedFontImpl* p);
};
struct SizedGlyphImplDeleter {
    void operator()(SizedGlyphImpl* p);
};
using FontLibraryPimpl = std::unique_ptr<FontLibraryImpl, FontLibraryImplDeleter>;
using FontPimpl = std::unique_ptr<FontImpl, FontImplDeleter>;
using SizedFontPimpl = std::unique_ptr<SizedFontImpl, SizedFontImplDeleter>;
using SizedGlyphPimpl = std::unique_ptr<SizedGlyphImpl, SizedGlyphImplDeleter>;

// Define the SizedFontImpl class. We usually only define these in .cpp files,
// but in this case we define it in the header file because ShapedTextImpl in
// text.cpp also needs to access hbFont.
//
class SizedFontImpl {
public:
    SizedFontParams params;
    FT_Face ftFace;
    hb_font_t* hbFont;
    std::mutex glyphsMutex;
    std::unordered_map<Int, SizedGlyph*> glyphsMap;

    SizedFontImpl(Font* font, const SizedFontParams& params);
    ~SizedFontImpl();
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
geometry::Vec2d f266ToVec2d(T x, T y) {
    return geometry::Vec2d(x / 64.0, y / 64.0);
}

template<class T>
geometry::Vec2f f266ToVec2f(T x, T y) {
    return geometry::Vec2f(x / 64.0f, y / 64.0f);
}

} // namespace detail

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

    /// Adds the font from the given filename to this library.
    ///
    /// ```cpp
    /// Font* font = fontLibrary->addFont("fonts/DejaVuSerif.ttf");
    /// ```
    ///
    /// Some font files actually contain several typefaces, so you can
    /// optionally specify the `index` of the desired typeface. This is rare
    /// and usually leaving the default value (`0`) is sufficient.
    ///
    Font* addFont(const std::string& filename, Int index = 0);

    /// Returns the default font. Returns nullptr if no default font
    /// has been defined via setDefaultFont().
    ///
    /// Note: for now, there is only one default font, which is the one
    /// defined via setDefaultFont(). Unfortunately, this is not always the
    /// best approach, since this default font may not contain all the required
    /// Unicode characters (e.g., Arabic, Chinese, etc.) for a given text
    /// string. In the future, the idea is to implement an additional class
    /// called FontQuery, similar to QFont, which basically stores a desired
    /// `font-family`, `font-style`, `font-size`, etc. Like in CSS, the
    /// `font-family` can be as simple as `serif` (which means "get the default
    /// serif font"), or a more precise query with fallbacks, like `Arial,
    /// Helvetica, sans-serif`. The FontLibrary will be responsible for finding
    /// the appropriate SizedFonts based on a given FontQuery and text string:
    /// there might be different SizedFonts for different segments of the text.
    ///
    // Useful references:
    // https://developer.mozilla.org/en-US/docs/Web/CSS/font-family
    // https://tex.stackexchange.com/a/520048/45625
    // https://harfbuzz.github.io/harfbuzz-hb-common.html#hb-script-t
    // https://github.com/harfbuzz/harfbuzz/issues/1288
    // https://stackoverflow.com/questions/23508605/display-mixed-complex-script-in-text-editor-via-harfbuzz-freetype
    //
    Font* defaultFont() const;

    /// Sets the default font.
    ///
    /// \sa defaultFont().
    ///
    void setDefaultFont(Font* font);

protected:
    /// \reimp
    void onDestroyed() override;

private:
    detail::FontLibraryPimpl impl_;
};

/// Returns the global font library.
///
VGC_GRAPHICS_API FontLibrary* fontLibrary();

/// \class vgc::graphics::Font
/// \brief A given typeface in a given style (e.g., Roboto Bold).
///
/// A `Font` represents a given typeface in a given style or variant. For
/// example, "Roboto Bold". This is typically imported from a given font file,
/// e.g., "Roboto-Bold.ttf".
///
/// Note that in order to render glyphs or get the metrics of a given `Font`,
/// you must first create get a `SizedFont` from the `Font`, which adds size
/// information (e.g, '12pt @ 72 dpi') as well as hinting parameters and other
/// options which affect rendering.
///
/// \sa FontLibrary, SizedFont
///
class VGC_GRAPHICS_API Font : public core::Object {
private:
    VGC_OBJECT(Font, core::Object)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

protected:
    /// Creates a new Font. This constructor is an implementation
    /// detail. In order to create a Font, please use the following:
    ///
    /// ```cpp
    /// Font* font = fontLibrary->addFont(filename);
    /// ```
    ///
    Font(FontLibrary* library);

public:
    /// Returns the library this font belongs to.
    ///
    FontLibrary* library() const;

    /// Returns the index of this font. This is typically `0` except in the rare
    /// cases where a font file contained several typefaces.
    ///
    Int index() const;

    /// Returns a `SizedFont` based on this `Font` and the given size properties.
    ///
    SizedFont* getSizedFont(const SizedFontParams& params);

    /// Returns the glyph corresponding to the given Unicode code point, or
    /// nullptr if this font doesn't have a glyph for this code point.
    ///
    /// ```cpp
    /// Glyph* glyph = font->getGlyphFromCodePoint(0x0041); // => 'A'
    /// ```
    ///
    /// This function is equivalent to calling getGlyphIndexFromCodePoint() then
    /// getGlyphFromIndex().
    ///
    /// Note that some glyphs may not be accessible via this function, because
    /// they do not correspond to any code point. If you need to access such
    /// glyphs, you must instead use getGlyphFromIndex() directly.
    ///
    Glyph* getGlyphFromCodePoint(Int codePoint);

    /// Returns the glyph at the given glyph index. This uses an internal
    /// indexing system, which may or may not be equal to the indices used in
    /// the font file, or to Unicode code points.
    ///
    /// Raises a vgc::core::FontError if the given glyphIndex is not a valid
    /// index or another error occurs.
    ///
    Glyph* getGlyphFromIndex(Int glyphIndex);

    /// Returns the glyph index corresponding to the given Unicode code point,
    /// or 0 if this font doesn't have a glyph for this code point.
    ///
    Int getGlyphIndexFromCodePoint(Int codePoint);

protected:
    /// \reimp
    void onDestroyed() override;

private:
    detail::FontPimpl impl_;
    friend class FontLibrary;
    friend class detail::FontLibraryImpl;
    friend class detail::SizedFontImpl;
};

/// \class vgc::graphics::Glyph
/// \brief A given glyph of a given Font.
///
class VGC_GRAPHICS_API Glyph : public core::Object {
private:
    VGC_OBJECT(Glyph, core::Object)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

protected:
    /// Creates a new Glyph. This constructor is an implementation detail.
    /// In order to get a glyph in a given font, please use one of the following:
    ///
    /// ```cpp
    /// Glyph* glyph1 = font->getGlyphFromCodePoint(codePoint);
    /// Glyph* glyph2 = font->getGlyphFromIndex(glyphIndex);
    /// ```
    ///
    Glyph(Font* font, Int index, const std::string& name);

public:
    /// Returns the font this glyph belongs to.
    ///
    Font* font() const;

    /// Returns the index of this glyph. This is an integer that can be
    /// used to retrieve the glyph via `font->getGlyphFromIndex()`.
    ///
    Int index() const {
        return index_;
    }

    /// Returns the name of this glyph, or an empty string if the font doesn't
    /// support glyph names.
    ///
    const std::string& name() const {
        return name_;
    }

private:
    Int index_;
    std::string name_;
    friend class Font;
};

/// \class vgc::graphics::SizedFont
/// \brief A given typeface, in a given style, in a given size.
///
/// A `SizedFont` represents a given typeface, in a given style, in a given size.
/// For example, "Source Sans Pro, bold, 12pt @ 72dpi".
///
/// Note that a given typeface, even with a given style (example:
/// "SourceSansPro-Bold.otf"), may still use different glyphs based on the
/// size. For example, smaller point size (8pt) may have less details than
/// higher point sizes (36pt), and different hinting should be applied based on
/// the size. This is why we use separate SizedFont objects to represent the
/// same typeface at different sizes.
///
/// If you need to perform text shaping (that is, convert an input string into
/// a sequence of glyphs from this SizedFont), see the class ShapedText.
///
class VGC_GRAPHICS_API SizedFont : public core::Object {
private:
    VGC_OBJECT(SizedFont, core::Object)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

protected:
    /// Creates a new SizedFont. This constructor is an implementation
    /// detail. In order to create a SizedFont, please use the following:
    ///
    /// ```cpp
    /// SizedFont* sizedFont = font->getSizedFont(params);
    /// ```
    ///
    SizedFont(Font* font);

public:
    /// Returns the `Font` that this `SizedFont` is a sized version of.
    ///
    Font* font() const;

    /// Returns the `SizedFontParams` of this `SizedFont`.
    ///
    const SizedFontParams& params() const;

    /// Returns the height of ascenders, in pixels. See:
    ///
    /// https://www.freetype.org/freetype2/docs/glyphs/glyphs-3.html
    ///
    float ascent() const;

    /// Returns the height of descenders, in pixels. Note that it is usually a
    /// negative value. See:
    ///
    /// https://www.freetype.org/freetype2/docs/glyphs/glyphs-3.html
    ///
    float descent() const;

    /// Returns the height of this font, in pixels. This is the vertical
    /// distance between two baselines.
    ///
    float height() const;

    /// Returns the `SizedGlyph` corresponding to the given Unicode code point,
    /// or nullptr if this font doesn't have a glyph for this code point.
    ///
    /// ```cpp
    /// SizedGlyph* sizedGlyph = sizedFont->getSizedGlyphFromCodePoint(0x0041); // => 'A'
    /// ```
    ///
    /// This function is equivalent to calling getGlyphIndexFromCodePoint() then
    /// getSizedGlyphFromIndex().
    ///
    /// Note that some glyphs may not be accessible via this function, because
    /// they do not correspond to any code point. If you need to access such
    /// glyphs, you must instead use getGlyphFromIndex() directly.
    ///
    SizedGlyph* getSizedGlyphFromCodePoint(Int codePoint);

    /// Returns the glyph at the given glyph index. This uses an internal
    /// indexing system, which may or may not be equal to the indices used in
    /// the font file, or to Unicode code points.
    ///
    /// Raises a vgc::core::FontError if the given glyphIndex is not a valid
    /// index or another error occurs.
    ///
    SizedGlyph* getSizedGlyphFromIndex(Int glyphIndex);

    /// Returns the glyph index corresponding to the given Unicode code point,
    /// or 0 if this font doesn't have a glyph for this code point.
    ///
    Int getGlyphIndexFromCodePoint(Int codePoint);

protected:
    /// \reimp
    void onDestroyed() override;

private:
    detail::SizedFontPimpl impl_;
    friend class FontLibrary;
    friend class Font;
    friend class detail::FontLibraryImpl;
    friend class detail::ShapedTextImpl;
};

/// \class vgc::graphics::SizedGlyph
/// \brief A given glyph of a given SizedFont.
///
class VGC_GRAPHICS_API SizedGlyph : public core::Object {
private:
    VGC_OBJECT(SizedGlyph, core::Object)
    VGC_PRIVATIZE_OBJECT_TREE_MUTATORS

protected:
    /// Creates a new SizedGlyph. This constructor is an implementation detail.
    /// In order to get a glyph in a given font, please use one of the following:
    ///
    /// ```cpp
    /// SizedGlyph* sizedGlyph1 = sizedFont->getSizedGlyphFromCodePoint(codePoint);
    /// SizedGlyph* sizedGlyph2 = sizedFont->getSizedGlyphFromIndex(glyphIndex);
    /// ```
    ///
    SizedGlyph(SizedFont* font);

public:
    /// Returns the `SizedFont` that this `SizedGlyph` belongs to.
    ///
    SizedFont* sizedFont() const;

    /// Returns the `Glyph` that this `SizedGlyph` is a sized version of.
    ///
    Glyph* glyph() const;

    /// Returns the index of this glyph. This is an integer that can be
    /// used to retrieve the glyph via `sizedFont->getSizedGlyphFromIndex()`.
    ///
    Int index() const;

    /// Returns the name of this glyph, or an empty string if the font doesn't
    /// support glyph names.
    ///
    const std::string& name() const;

    /// Returns the outline of the glyph as a Curves2d.
    ///
    const geometry::Curves2d& outline() const;

    /// Returns the bounding box of the glyph.
    ///
    const geometry::Rect2f& boundingBox() const;

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
    void fill(core::FloatArray& data, const geometry::Mat3f& transform) const;

    /// Same as fill(data, transform) but with only a translation part. This is
    /// equivalent (but faster) to:
    ///
    /// ```cpp
    /// geometry::Mat3f transform = geometry::Mat3f::identity;
    /// transform.translate(translation);
    /// ```
    ///
    void fill(core::FloatArray& data, const geometry::Vec2f& translation) const;

    /// Same as fill(data, transform) but with only a translation part as well as flipping the Y-coordinate. This is
    /// equivalent (but faster) to:
    ///
    /// ```cpp
    /// geometry::Mat3f transform = geometry::Mat3f::identity;
    /// transform.translate(translation);
    /// transform.scale(1, -1);
    /// ```
    ///
    void fillYMirrored(core::FloatArray& data, const geometry::Vec2f& translation) const;

protected:
    /// \reimp
    void onDestroyed() override;

private:
    detail::SizedGlyphPimpl impl_;
    friend class FontLibrary;
    friend class SizedFont;
    friend class detail::FontLibraryImpl;
    friend class detail::SizedFontImpl;
};

} // namespace vgc::graphics

namespace std {

template<>
struct hash<vgc::graphics::SizedFontParams> {
    std::size_t operator()(const vgc::graphics::SizedFontParams& p) const {
        // http://stackoverflow.com/a/1646913/126995
        std::size_t res = 17;
        res = res * 31 + hash<vgc::Int>()(p.ppemWidth());
        res = res * 31 + hash<vgc::Int>()(p.ppemHeight());
        res = res * 31 + hash<vgc::graphics::FontHinting>()(p.hinting());
        return res;
    }
};

} // namespace std

#endif // VGC_GRAPHICS_FONT_H
