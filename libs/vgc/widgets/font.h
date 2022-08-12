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

#ifndef VGC_WIDGETS_FONT_H
#define VGC_WIDGETS_FONT_H

#include <string>
#include <vgc/widgets/api.h>

namespace vgc::widgets {

/// Loads all the default fonts built-in the vgc::widgets library.
///
VGC_WIDGETS_API
void addDefaultApplicationFonts();

/// Loads the font from the resource file specified by its \p name, and makes
/// it available to the application. An ID is returned that can be used to
/// remove the font again with QFontDatabase::removeApplicationFont() or to
/// retrieve the list of family names contained in the font.
///
/// This is a convenient wrapper around QFontDatabase::addApplicationFont() to
/// use a relative resource file.
///
/// Example:
/// \code
/// addApplicationFont("widgets/fonts/SourceSansPro-Regular.ttf")
/// \endcode
///
VGC_WIDGETS_API
int addApplicationFont(const std::string& name);

/// Prints info about a given font family.
///
VGC_WIDGETS_API
void printFontFamilyInfo(const std::string& family);

} // namespace vgc::widgets

#endif // VGC_WIDGETS_FONT_H
