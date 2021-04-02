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

#include <vgc/core/array.h>
#include <vgc/core/floatarray.h>
#include <vgc/core/innercore.h>
#include <vgc/geometry/curves2d.h>
#include <vgc/graphics/api.h>

namespace vgc {
namespace graphics {

namespace internal {

class FontLibraryImpl;
class FontFaceImpl;
class FontGlyphImpl;
class ShapedTextImpl;

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

} // namespace internal

VGC_DECLARE_OBJECT(FontLibrary);
VGC_DECLARE_OBJECT(FontFace);
VGC_DECLARE_OBJECT(FontGlyph);

/// \class vgc::graphics::FontLibrary
/// \brief Manages a set of available fonts.
///
/// A font library is an object used to manage a set of loaded fonts. You can
/// add new fonts to the library, remove fonts from the library, and query
/// which fonts are available in the library.
///
class VGC_GRAPHICS_API FontLibrary : public core::Object {
private:
    VGC_OBJECT(FontLibrary)
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

/// \class vgc::graphics::ShapedGlyph
/// \brief Represents one glyph of a shaped text.
///
/// A ShapedGlyph stores a reference to a FontGlyph as well as information
/// where to draw the glyph. It is typically used as part of a ShapedText
/// object.
///
/// See ShapedText for more details.
///
/// Note that for performance and thread-safety reasons, a ShapedGlyph stores a
/// raw FontGlyph pointer, not a FontGlyphPtr. This means that it doesn't keep
/// alive the referenced FontGlyph: it is the responsibility of client code to
/// ensure that the referenced FontGlyph outlive the ShapedGlyph itself. For
/// example, each ShapedText object does keep alive its underlying FontFace
/// (ans thus all its glyphs too), so it is safe to use the glyphs in
/// ShapedText::glyphs() as long as the ShapedText is alive.
///
class VGC_GRAPHICS_API ShapedGlyph {
public:
    /// Creates a ShapedGlyph.
    ///
    ShapedGlyph(FontGlyph* fontGlyph,
                const core::Vec2d& offset,
                const core::Vec2d& advance,
                const core::Vec2d& position) :
        fontGlyph_(fontGlyph),
        offset_(offset),
        advance_(advance),
        position_(position) {}

    /// Returns the FontGlyph that this ShapedGlyph references.
    ///
    /// Note that a ShapedText never contain ShapedGlyph elements whose
    /// fontGlyph() is NULL.
    ///
    /// - In case of a missing glyph in the face, the ".notdef" glyph [1] is
    ///   returned. It is typically represented as a rectangle (sometimes with
    ///   an "X" or "?" inside). More info:
    ///
    ///   https://docs.microsoft.com/en-us/typography/opentype/spec/recom#glyph-0-the-notdef-glyph
    ///
    /// - In case of other shaping problems (such as invalid unicode input),
    ///   the `U+FFFD � REPLACEMENT CHARACTER` might be used, or the codepoints
    ///   causing problems are simply skipped.
    ///
    FontGlyph* fontGlyph() const {
        return fontGlyph_;
    }

    /// Returns how much the glyph should be moved before drawing
    /// it. This should not affect how much the line advances.
    ///
    core::Vec2d offset() const {
        return offset_;
    }    

    // Returns how much the line advances after drawing this ShapedGlyph. The
    // X-coordinate corresponds to the advance when setting text in horizontal
    // direction, and the Y-coordinate corresponds to the advance when setting
    // text in vertical direction.
    //
    core::Vec2d advance() const {
        return advance_;
    }

    /// Returns where to draw the fontGlyph() relative to the origin of the
    /// ShapedText this ShapedGlyph belongs to.
    ///
    /// This is equal to the sum of this ShapedGlyph's offset and the advances
    /// of all the previous ShapedGlyph of the ShapedText.
    ///
    core::Vec2d position() const {
        return position_;
    }

    /// Fills this ShapedGlyph, taking into account its relative position() and
    /// the given origin:
    ///
    /// ```
    ///                      ██
    ///                      ██
    ///                      ██
    ///       position()     ██
    ///   ────────────────>  █████
    /// ─┼─────────────────┼────────────  baseline
    /// origin
    /// ```
    ///
    /// The output triangles are appended to the given output parameter `data`
    /// in the following format:
    ///
    /// ```
    /// [x1, y1, r1, g1, b1,     // First vertex of first triangle
    ///  x2, y2, r2, g2, b2,     // Second vertex of first triangle
    ///  x3, y3, r3, g3, b3,     // Third vertex of first triangle
    ///
    ///  x4, y4, r4, g4, b4,     // First vertex of second triangle
    ///  x5, y5, r5, g5, b5,     // Second vertex of second triangle
    ///  x6, y6, r6, g6, b6,     // Third vertex of second triangle
    ///
    ///  ...]
    /// ```
    ///
    /// Note that fontGlyph()->outline() is a Curves2d where the Y-axis points
    /// up, as per OpenType conventions. This function takes care of reverting
    /// the Y coordinate to follow the VGC convention of having the Y-axis
    /// pointing down, and takes care of translating the glyph to its correct
    /// position.
    ///
    void fill(core::FloatArray& data,
              const core::Vec2d& origin,
              float r, float g, float b) const;

private:
    FontGlyph* fontGlyph_;
    core::Vec2d offset_;
    core::Vec2d advance_;
    core::Vec2d position_;
};

using ShapedGlyphArray = core::Array<ShapedGlyph>;

/// \class vgc::graphics::ShapedText
/// \brief Performs text shaping and stores the resulting shaped text.
///
/// The ShapedText class performs text shaping and stores the resulting shaped
/// text. In other words, it takes as input a given text string and a given
/// FontFace, and stores as output a sequence of FontGlyph elements together
/// with additional information to know where each glyph should be drawn.
///
/// ```cpp
/// vgc::graphics::FontFace* face = someFace();
/// vgc::graphics::ShapedText text(face, "Hello");
/// for (const vgc::graphics::ShapedGlyph& glyph : text.glyphs()) {
///     ...
/// }
/// ```
///
class VGC_GRAPHICS_API ShapedText {
public:
    /// Creates a new ShapedText.
    ///
    ShapedText(FontFace* face, const std::string& text);

    /// Creates a copy of the given ShapedText.
    ///
    ShapedText(const ShapedText& other);

    /// Move-constructs the given ShapedText.
    ///
    ShapedText(ShapedText&& other);

    /// Assigns a copy of the given ShapedText.
    ///
    ShapedText& operator=(const ShapedText& other);

    /// Move-assigns the other ShapedText.
    ///
    ShapedText& operator=(ShapedText&& other);

    /// Destroys this ShapedText.
    ///
    ~ShapedText();

    /// Returns the FontFace of this ShapedText.
    ///
    const FontFace* fontFace() const;

    /// Returns the input text string of this ShapedText.
    ///
    const std::string& text() const;

    /// Modifies the input text string of this ShapedText. This automatically
    /// recomputes the glyphs.
    ///
    void setText(const std::string& text);

    /// Returns the ShapedGlyph elements composing this ShapedText.
    ///
    /// The ShapedGlyph elements are guaranteed to be valid as long as either
    /// this ShapedText are its FontFace is alive.
    ///
    const ShapedGlyphArray& glyphs() const;

    // Returns how much the line advances after drawing this ShapedText. The
    // X-coordinate corresponds to the advance when setting text in horizontal
    // direction, and the Y-coordinate corresponds to the advance when setting
    // text in vertical direction.
    //
    // This is equal to the sum of `glyph->advance()` for all the ShapedGlyph
    // elements in glyphs().
    //
    core::Vec2d advance() const;

    /// Fills this ShapedText at the given origin:
    ///
    /// ```
    ///    ██  ██  █████  ██     ██      ████
    ///    ██  ██  ██     ██     ██     ██  ██
    ///    ██████  █████  ██     ██     ██  ██
    ///    ██  ██  ██     ██     ██     ██  ██
    ///    ██  ██  █████  █████  █████   ████
    /// ─┼──────────────────────────────────────  baseline
    /// origin
    /// ```
    ///
    /// The output triangles are appended to the given output parameter `data`
    /// in the following format:
    ///
    /// ```
    /// [x1, y1, r1, g1, b1,     // First vertex of first triangle
    ///  x2, y2, r2, g2, b2,     // Second vertex of first triangle
    ///  x3, y3, r3, g3, b3,     // Third vertex of first triangle
    ///
    ///  x4, y4, r4, g4, b4,     // First vertex of second triangle
    ///  x5, y5, r5, g5, b5,     // Second vertex of second triangle
    ///  x6, y6, r6, g6, b6,     // Third vertex of second triangle
    ///
    ///  ...]
    /// ```
    ///
    /// Note that this function follows the VGC convention of having the Y-axis
    /// pointing down, so in the example ASCII art above, if origin = (0, 0),
    /// then all the Y-coordinates of the triangle vertices will be negative.
    ///
    void fill(core::FloatArray& data,
              const core::Vec2d& origin,
              float r, float g, float b) const;

private:
    internal::ShapedTextImpl* impl_;
};

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
class VGC_GRAPHICS_API FontFace : public core::Object {
private:
    VGC_OBJECT(FontFace)
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

    /// Shapes the given UTF-8 encoded text, and returns the shaped text.
    ///
    /// The FontGlyph elements referenced in the returned ShapedTextitems are
    /// guaranteed to be valid as long as this FontFace is alive.
    ///
    /// Note that if you need to perform consecutive text shaping computations,
    /// it is more efficient to first create a ShapedText (which acts as a
    /// buffer), then call shapedText.setText() several times.
    ///
    ShapedText shape(const std::string& text);

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
    VGC_OBJECT(FontGlyph)
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
