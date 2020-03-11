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
    core::Array<FontFaceSharedPtr> faces;

    FontLibraryImpl()
    {
        FT_Error error = FT_Init_FreeType(&library);
        if (error) {
            throw FontError(errorMsg(error));
        }
    }

    ~FontLibraryImpl()
    {
        // Release loaded faces. Note that FT_Done_FreeType(library) would
        // already call FT_Done_Face() for us. However, we need to do it first,
        // otherwise they would be double-freed, since ~FontFaceImpl would also
        // automatically call FT_Done_Face().
        //
        // In addition, this code sets each FontFace as invalid, which is
        // important if Python extends the lifetime of some FontFace. This
        // design is clumsy and should be improved with a better Object
        // ownership mechanism.
        //
        for (const FontFaceSharedPtr& face : faces) {
            face->impl_.reset();
        }
        faces.clear();

        // Release library.
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

} // namespace internal

FontLibrary::FontLibrary(const ConstructorKey&) :
    Object(core::Object::ConstructorKey()),
    impl_(new internal::FontLibraryImpl()) // TODO: use make_unique (C++14)
{

}

// static
FontLibrarySharedPtr FontLibrary::create()
{
    return std::make_shared<FontLibrary>(ConstructorKey());
}

// We need the destructor implementation in the *.cpp file, see:
// https://stackoverflow.com/questions/9954518/stdunique-ptr-with-an-incomplete-type-wont-compile
FontLibrary::~FontLibrary()
{

}

FontFace* FontLibrary::addFace(const std::string& filename)
{
    auto fontFace = std::make_shared<FontFace>(FontFace::ConstructorKey());
    FontFace* res = fontFace.get();
    fontFace->impl_.reset(new internal::FontFaceImpl(impl_->library, filename));
    impl_->faces.append(std::move(fontFace));
    return res;
}

FontFace::FontFace(
        const ConstructorKey&) :
    Object(core::Object::ConstructorKey()),
    impl_()
{

}

FontFace::~FontFace()
{

}

bool FontFace::isAlive()
{
    return impl_.get() != nullptr;
}

} // namespace graphics
} // namespace vgc
