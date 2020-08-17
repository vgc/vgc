// Copyright 2020 The VGC Developers
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

#include <vgc/core/innercore.h>
#include <vgc/core/array.h>
#include <vgc/geometry/curves2d.h>
#include <vgc/graphics/api.h>

namespace vgc {
namespace graphics {

namespace internal {

class FontLibraryImpl;
class FontFaceImpl;
class FontGlyphImpl;
class FontShaperImpl;

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

protected:
    /// \reimp
    void onDestroyed() override;

private:
    internal::FontLibraryPimpl impl_;
};

/// \class vgc::graphics::FontItem
/// \brief Represents one item of a shaped text.
///
/// A FontItem stores a reference to a FontGlyph as well as information where
/// to draw the glyph. It is typically used as output of a text shaping
/// operation. See FontShaper for more details on text shaping.
///
/// Note that for performance and thread-safety reasons, a FontItem stores a
/// raw FontGlyph pointer, not a FontGlyphPtr. This means that it doesn't keep
/// alive the referenced FontGlyph: it is the responsibility of client code to
/// ensure that the referenced FontGlyph outlive the FontItem itself. For
/// example, each FontShaper object does keep alive its underlying FontFace
/// (therefore all its glyphs too), therefore it is safe to use the items in
/// FontShaper::items() as long as the FontShaper is alive.
///
class VGC_GRAPHICS_API FontItem {
public:
    /// Creates a FontItem.
    ///
    FontItem(FontGlyph* glyph, const core::Vec2d& offset) :
        glyph_(glyph), offset_(offset) {}

    /// Returns the glyph.
    ///
    /// Note that the shape() methods of FontShaper and FontFace never return
    /// NULL glyphs:
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
    FontGlyph* glyph() const {
        return glyph_;
    }

    /// Returns the offset.
    ///
    core::Vec2d offset() const {
        return offset_;
    }

private:
    FontGlyph* glyph_;
    core::Vec2d offset_;
};

using FontItemArray = core::Array<FontItem>;

/// \class vgc::graphics::FontShaper
/// \brief Performs text shaping operations using a given font face.
///
/// The FontShaper class performs text shaping operations using a given font face.
///
/// ```cpp
/// vgc::graphics::FontFace* face = someFace();
/// vgc::graphics::FontShaper shaper = face->shaper();
/// for (const vgc::graphics::FontItem& item : shaper.shape("Hello,")) {
///     ...
/// }
/// for (const vgc::graphics::FontItem& item : shaper.shape("World!")) {
///     ...
/// }
/// ```
///
/// Like most classes in VGC, keep in mind that this class is NOT thread-safe.
/// In particular, each FontShaper object internally stores a FontItemArray
/// which is used as a buffer to re-use the container's capacity across
/// consecutive calls to the shape() method. This avoids unnecessary memory
/// deallocations and reallocations and thus improves performance. But this
/// obviously means that you cannnot iterate over (or even copy!) the returned
/// shapes() while another thread calls shape() on the same FontShaper
/// instance.
///
/// However, like most classes in VGC, all methods in this class are
/// re-entrant: as long as you use a separate FontShaper instance per thread,
/// it is safe to call the methods of the FontShaper class in parallel.
///
class VGC_GRAPHICS_API FontShaper {
public:
    /// Creates a new FontShaper.
    ///
    FontShaper(FontFace* face);

    /// Creates a copy of the given FontShaper.
    ///
    FontShaper(const FontShaper& other);

    /// Move-constructs the given FontShaper.
    ///
    FontShaper(FontShaper&& other);

    /// Assigns a copy of the given FontShaper.
    ///
    FontShaper& operator=(const FontShaper& other);

    /// Move-assigns the other FontShaper.
    ///
    FontShaper& operator=(FontShaper&& other);

    /// Destroys this FontShaper.
    ///
    ~FontShaper();

    /// Shapes the given UTF-8 encoded text, and returns the shaped text.
    ///
    /// ```cpp
    /// vgc::graphics::FontFace* face = someFace();
    /// vgc::graphics::FontShaper shaper = face->shaper();
    /// for (const vgc::graphics::FontItem& item : shaper.shape("Hello")) {
    ///     ...
    /// }
    /// ```
    ///
    /// The items in the returned FontItemArray are guaranteed to be valid as
    /// long as either the FontShaper or its underlying FontFace is alive.
    ///
    const FontItemArray& shape(const std::string& text);

    /// Returns the result of the last call to shape().
    ///
    /// The items in the returned FontItemArray are guaranteed to be valid as
    /// long as either the FontShaper or its underlying FontFace is alive.
    ///
    const FontItemArray& items() const;

private:
    friend class FontFace; // Allow to move the stored FontItemArray
    internal::FontShaperImpl* impl_;
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

    /// Creates a FontShaper for shaping text with this FontFace.
    ///
    FontShaper shaper();

    /// Shapes the given UTF-8 encoded text, and returns the shaped text.
    ///
    /// ```cpp
    /// for (const vgc::graphics::FontItem& item : face->shape("Hello")) {
    ///     ...
    /// }
    /// ```
    ///
    /// The items in the returned FontItemArray are guaranteed to be valid as
    /// long as the FontFace is alive.
    ///
    /// Note that if you need to perform consecutive text shaping computations,
    /// it is more efficient to first create a FontShaper (which acts as a
    /// buffer), then call shaper.shape() several times:
    ///
    /// ```cpp
    /// vgc::graphics::FontShaper shaper = face->shaper();
    /// for (const vgc::graphics::FontItem& item : shaper.shape("Hello,")) {
    ///     ...
    /// }
    /// for (const vgc::graphics::FontItem& item : shaper.shape("World!")) {
    ///     ...
    /// }
    /// ```
    ///
    FontItemArray shape(const std::string& text);

protected:
    /// \reimp
    void onDestroyed() override;

private:
    internal::FontFacePimpl impl_;
    friend class FontLibrary;
    friend class internal::FontLibraryImpl;
    friend class internal::FontShaperImpl;
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
