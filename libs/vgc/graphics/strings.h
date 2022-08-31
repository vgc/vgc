// Copyright 2022 The VGC Developers
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

#ifndef VGC_GRAPHICS_STRINGS_H
#define VGC_GRAPHICS_STRINGS_H

#include <vgc/core/stringid.h>
#include <vgc/graphics/api.h>

namespace vgc::graphics::strings {

// Properties
// ----------

// non-inherited
VGC_GRAPHICS_API extern const core::StringId background_color;
VGC_GRAPHICS_API extern const core::StringId margin_top;
VGC_GRAPHICS_API extern const core::StringId margin_right;
VGC_GRAPHICS_API extern const core::StringId margin_bottom;
VGC_GRAPHICS_API extern const core::StringId margin_left;
VGC_GRAPHICS_API extern const core::StringId padding_top;
VGC_GRAPHICS_API extern const core::StringId padding_right;
VGC_GRAPHICS_API extern const core::StringId padding_bottom;
VGC_GRAPHICS_API extern const core::StringId padding_left;
VGC_GRAPHICS_API extern const core::StringId border_width;
VGC_GRAPHICS_API extern const core::StringId border_color;
VGC_GRAPHICS_API extern const core::StringId border_top_left_radius;
VGC_GRAPHICS_API extern const core::StringId border_top_right_radius;
VGC_GRAPHICS_API extern const core::StringId border_bottom_right_radius;
VGC_GRAPHICS_API extern const core::StringId border_bottom_left_radius;

// inherited
VGC_GRAPHICS_API extern const core::StringId pixel_hinting;
VGC_GRAPHICS_API extern const core::StringId font_size;
VGC_GRAPHICS_API extern const core::StringId font_ascent;
VGC_GRAPHICS_API extern const core::StringId font_descent;
VGC_GRAPHICS_API extern const core::StringId text_color;
VGC_GRAPHICS_API extern const core::StringId text_selection_color;
VGC_GRAPHICS_API extern const core::StringId text_selection_background_color;
VGC_GRAPHICS_API extern const core::StringId text_horizontal_align;
VGC_GRAPHICS_API extern const core::StringId text_vertical_align;
VGC_GRAPHICS_API extern const core::StringId caret_color;

// Values
// ------

// generic
VGC_GRAPHICS_API extern const core::StringId auto_;

// text-horizontal-align:
VGC_GRAPHICS_API extern const core::StringId left;
VGC_GRAPHICS_API extern const core::StringId center;
VGC_GRAPHICS_API extern const core::StringId right;

// text-vertical-align:
VGC_GRAPHICS_API extern const core::StringId top;
VGC_GRAPHICS_API extern const core::StringId middle;
VGC_GRAPHICS_API extern const core::StringId bottom;

// pixel-hinting:
VGC_GRAPHICS_API extern const core::StringId normal;
VGC_GRAPHICS_API extern const core::StringId off;

} // namespace vgc::graphics::strings

#endif // VGC_GRAPHICS_STRINGS_H
