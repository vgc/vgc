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

VGC_UI_API extern const core::StringId BoolSettingEdit;
VGC_UI_API extern const core::StringId Button;
VGC_UI_API extern const core::StringId Dialog;
VGC_UI_API extern const core::StringId Flex;
VGC_UI_API extern const core::StringId Grid;
VGC_UI_API extern const core::StringId IconWidget;
VGC_UI_API extern const core::StringId Label;
VGC_UI_API extern const core::StringId LineEdit;
VGC_UI_API extern const core::StringId Menu;
VGC_UI_API extern const core::StringId MenuBar;
VGC_UI_API extern const core::StringId MenuButton;
VGC_UI_API extern const core::StringId MessageDialog;
VGC_UI_API extern const core::StringId NumberEdit;
VGC_UI_API extern const core::StringId NumberSettingEdit;
VGC_UI_API extern const core::StringId Panel;
VGC_UI_API extern const core::StringId PanelArea;
VGC_UI_API extern const core::StringId Plot2d;
VGC_UI_API extern const core::StringId SettingEdit;
VGC_UI_API extern const core::StringId TabBar;
VGC_UI_API extern const core::StringId TabBody;
VGC_UI_API extern const core::StringId Toggle;
VGC_UI_API extern const core::StringId Widget;

VGC_UI_API extern const core::StringId min_width;
VGC_UI_API extern const core::StringId min_height;
VGC_UI_API extern const core::StringId max_width;
VGC_UI_API extern const core::StringId max_height;
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

VGC_UI_API extern const core::StringId main_alignment;
VGC_UI_API extern const core::StringId cross_alignment;
VGC_UI_API extern const core::StringId start;
VGC_UI_API extern const core::StringId end;
VGC_UI_API extern const core::StringId center;

VGC_UI_API extern const core::StringId main_spacing;
VGC_UI_API extern const core::StringId packed;
VGC_UI_API extern const core::StringId space_between;
VGC_UI_API extern const core::StringId space_around;
VGC_UI_API extern const core::StringId space_evenly;
VGC_UI_API extern const core::StringId force_stretch;

VGC_UI_API extern const core::StringId handle_size;
VGC_UI_API extern const core::StringId handle_hovered_size;
VGC_UI_API extern const core::StringId handle_hovered_color;

VGC_UI_API extern const core::StringId root;
VGC_UI_API extern const core::StringId macos;

VGC_UI_API extern const core::StringId uncheckable;
VGC_UI_API extern const core::StringId checkable;
VGC_UI_API extern const core::StringId bistate;
VGC_UI_API extern const core::StringId tristate;
VGC_UI_API extern const core::StringId unchecked;
VGC_UI_API extern const core::StringId checked;
VGC_UI_API extern const core::StringId indeterminate;

VGC_UI_API extern const core::StringId hovered;
VGC_UI_API extern const core::StringId pressed;
VGC_UI_API extern const core::StringId active;
VGC_UI_API extern const core::StringId enabled;
VGC_UI_API extern const core::StringId disabled;

VGC_UI_API extern const core::StringId on;
VGC_UI_API extern const core::StringId off;

// Button
VGC_UI_API extern const core::StringId icon;
VGC_UI_API extern const core::StringId text;
VGC_UI_API extern const core::StringId shortcut;

// Menu
VGC_UI_API extern const core::StringId button;
VGC_UI_API extern const core::StringId separator;

// Dialog
VGC_UI_API extern const core::StringId body;
VGC_UI_API extern const core::StringId buttons;
VGC_UI_API extern const core::StringId centered;
VGC_UI_API extern const core::StringId content;
VGC_UI_API extern const core::StringId title;

} // namespace vgc::ui::strings

#endif // VGC_UI_STRINGS_H
