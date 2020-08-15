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

class VGC_GRAPHICS_API FontShape {
public:
    /// Creates a FontShape.
    ///
    FontShape(FontGlyph* glyph, const core::Vec2d& offset) :
        glyph_(glyph), offset_(offset) {}

    /// Returns the glyph.
    ///
    FontGlyph* glyph() const {
        return glyph_.get();
    }

    /// Return the offset.
    ///
    core::Vec2d offset() const {
        return offset_;
    }

private:
    FontGlyphPtr glyph_;
    core::Vec2d offset_;
};

using FontShapeArray = core::Array<FontShape>;

/// \class vgc::graphics::FontShaper
/// \brief An object for performing font shaping operations.
///
class VGC_GRAPHICS_API FontShaper {
public:
    /// Creates a new FontShaper.
    ///
    FontShaper(FontFace* face);

    /// Creates a copy of the FontShaper
    ///
    FontShaper(const FontShaper& other);

    /// Move-constructs the FontShaper
    ///
    FontShaper(FontShaper&& other);

    /// Assigns a copy of the FontShaper
    ///
    FontShaper& operator=(const FontShaper& other);

    /// Move-assigns the other FontShaper
    ///
    FontShaper& operator=(FontShaper&& other);

    /// Destroys the FontShaper.
    ///
    ~FontShaper();

    /// Shapes the given UTF-8 encoded text.
    ///
    void shape(const std::string& text);

    /// Returns the shapes from the previous call of shape().
    ///
    const FontShapeArray& shapes() const;

private:
    internal::FontShaperImpl* impl_;
};

/// \class vgc::graphics::FontFace
/// \brief A given typeface, in a given style.
///
/// A font face represents the data contains in one TTF or OTF file, for
/// example, "SourceSansPro-Bold.otf". A given font family is typically made of
/// several font faces, for example, the "Source Sans Pro" font family has
/// several faces to represent all its variations: light, regular, bold, light
/// italic, italic, bold italic, etc.
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
