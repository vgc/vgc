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
#include <vgc/graphics/api.h>

namespace vgc {
namespace graphics {

namespace internal {

class FontLibraryImpl;
class FontFaceImpl;

} // namespace internal

VGC_CORE_DECLARE_PTRS(FontLibrary);
VGC_CORE_DECLARE_PTRS(FontFace);

/// \class vgc::graphics::FontLibrary
/// \brief Manages a set of available fonts.
///
/// A font library is an object used to manage a set of loaded fonts. You can
/// add new fonts to the library, remove fonts from the library, and query
/// which fonts are available in the library.
///
class VGC_GRAPHICS_API FontLibrary : public core::Object {

    VGC_CORE_OBJECT(FontLibrary)

public:    
    /// Creates a new FontLibrary. This constructor is an implementation
    /// detail. In order to create a FontLibrary, please use the following:
    ///
    /// \code
    /// FontLibrarySharedPtr fontLibrary = FontLibrary::create();
    /// \endcode
    ///
    FontLibrary(const ConstructorKey&);

    /// Destructs the FontLibrary.
    ///
    ~FontLibrary();

    /// Creates an empty FontLibrary, that is, a font library which doesn't
    /// have any available fonts yet.
    ///
    static FontLibrarySharedPtr create();

    /// Adds the face from the given filename to this library.
    ///
    /// ```cpp
    /// FontFace* fontFace = fontLibrary->addFace("fonts/DejaVuSerif.ttf");
    /// ```
    ///
    FontFace* addFace(const std::string& filename);

private:
    std::unique_ptr<internal::FontLibraryImpl> impl_;
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

    VGC_CORE_OBJECT(FontFace)

public:
    /// Creates a new FontFace. This constructor is an implementation
    /// detail. In order to create a FontFace, please use the following:
    ///
    /// \code
    /// FontFace* fontFace = fontLibrary->addFace();
    /// \endcode
    ///
    FontFace(const ConstructorKey&);

    /// Destructs the FontFace.
    ///
    ~FontFace();

    /// Returns whether the FontFace is alive. The reason it might
    /// not be alive is if the library it belongs to has already been
    /// deleted.
    ///
    // TODO: use new Object ownership mechanism to prevent python from
    // extending the lifetime of objects.
    bool isAlive();

private:
    std::unique_ptr<internal::FontFaceImpl> impl_;
    friend class FontLibrary;
    friend class internal::FontLibraryImpl;
};

} // namespace graphics
} // namespace vgc

#endif // VGC_GRAPHICS_FONT_H
