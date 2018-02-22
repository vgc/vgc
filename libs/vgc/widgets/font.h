// Copyright 2017 The VGC Developers
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

namespace vgc {
namespace widgets {

/// Loads the font from the resource file specified by its \p name, and makes
/// it available to the application.
///
/// This is a convenient wrapper around QFontDatabase::addApplicationFont().
///
/// Example:
/// \code
/// addApplicationFont("widgets/fonts/SourceSansPro-Regular.ttf")
/// \endcode
///
VGC_WIDGETS_API
void addApplicationFont(const std::string& name);

} // namespace widgets
} // namespace vgc

#endif // VGC_WIDGETS_FONT_H
