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

#include <vgc/graphics/font.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

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

        // TODO: FT_Set_Char_Size?
    }

    ~FontFaceImpl()
    {
        FT_Error error = FT_Done_Face(face);
        if (error) {
            // Note: we print a warning rather than throwing, because throwing
            // in destructors is a bad idea.
            core::warning() << errorMsg(error) << std::endl;
        }
    }
};

namespace {

core::Vec2d toVec2d(const FT_Vector* v)
{
    // TODO: what if x is expressed as fixed precision 16.16 or 26.6?
    // https://www.freetype.org/freetype2/docs/reference/ft2-basic_types.html#ft_pos
    return core::Vec2d(v->x, v->y);
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

FontFace::FontFace(FontLibrary* library) :
    Object(),
    impl_()
{
    appendObjectToParent_(library);
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
    FT_Int32 flags = FT_LOAD_NO_SCALE;
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
