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

#ifndef VGC_UI_STRINGS_H
#define VGC_UI_STRINGS_H

#include <vgc/core/stringid.h>
#include <vgc/ui/api.h>

namespace vgc::ui::strings {

VGC_UI_API extern const core::StringId Button;
VGC_UI_API extern const core::StringId ColorListView;
VGC_UI_API extern const core::StringId ColorListViewItem;
VGC_UI_API extern const core::StringId ColorPalette;
VGC_UI_API extern const core::StringId ColorPaletteSelector;
VGC_UI_API extern const core::StringId Flex;
VGC_UI_API extern const core::StringId Grid;
VGC_UI_API extern const core::StringId Label;
VGC_UI_API extern const core::StringId LineEdit;
VGC_UI_API extern const core::StringId Menu;
VGC_UI_API extern const core::StringId MenuBar;
VGC_UI_API extern const core::StringId Plot2d;
VGC_UI_API extern const core::StringId Widget;

VGC_UI_API extern const core::StringId preferred_width;
VGC_UI_API extern const core::StringId preferred_height;
VGC_UI_API extern const core::StringId column_gap;
VGC_UI_API extern const core::StringId row_gap;
VGC_UI_API extern const core::StringId grid_auto_columns;
VGC_UI_API extern const core::StringId grid_auto_rows;
VGC_UI_API extern const core::StringId horizontal_stretch;
VGC_UI_API extern const core::StringId horizontal_shrink;
VGC_UI_API extern const core::StringId vertical_stretch;
VGC_UI_API extern const core::StringId vertical_shrink;

VGC_UI_API extern const core::StringId hovered;
VGC_UI_API extern const core::StringId pressed;

} // namespace vgc::ui::strings

#endif // VGC_UI_STRINGS_H
