// Copyright 2023 The VGC Developers
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

#ifndef VGC_TOOLS_STRINGS_H
#define VGC_TOOLS_STRINGS_H

#include <vgc/core/stringid.h>
#include <vgc/tools/api.h>

namespace vgc::tools::strings {

VGC_TOOLS_API extern const core::StringId ColorListView;
VGC_TOOLS_API extern const core::StringId ColorListViewItem;
VGC_TOOLS_API extern const core::StringId ColorPalette;
VGC_TOOLS_API extern const core::StringId ColorPaletteSelector;
VGC_TOOLS_API extern const core::StringId ColorPreview;

} // namespace vgc::tools::strings

#endif // VGC_TOOLS_STRINGS_H
