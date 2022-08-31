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

#include <vgc/graphics/strings.h>

namespace vgc::graphics::strings {

// Properties
// ----------

// non-inherited
const core::StringId background_color("background-color");

// Margins & Padding currently only accepts <length> and in the future we may accept <percentage>.
//
// In HTML CSS, percentage are relative to the containing block.
// See https://developer.mozilla.org/en-US/docs/Web/CSS/Containing_block.
//
// In our case, we probably want to do something different and consider this:
// - a percentage for a margin would be relative to size + result margin.
// - a percentage for a padding would be relative to size (size includes padding).
const core::StringId margin_top("margin-top");
const core::StringId margin_right("margin-right");
const core::StringId margin_bottom("margin-bottom");
const core::StringId margin_left("margin-left");
const core::StringId padding_top("padding-top");
const core::StringId padding_right("padding-right");
const core::StringId padding_bottom("padding-bottom");
const core::StringId padding_left("padding-left");

const core::StringId border_width("border-width");
const core::StringId border_color("border-color");
const core::StringId border_top_left_radius("border-top-left-radius");
const core::StringId border_top_right_radius("border-top-right-radius");
const core::StringId border_bottom_right_radius("border-bottom-right-radius");
const core::StringId border_bottom_left_radius("border-bottom-left-radius");

// inherited
const core::StringId pixel_hinting("pixel-hinting");
const core::StringId font_size("font-size");
const core::StringId font_ascent("font-ascent");
const core::StringId font_descent("font-descent");
const core::StringId text_color("text-color");
const core::StringId text_selection_color("text-selection-color");
const core::StringId text_selection_background_color("text-selection-background-color");
const core::StringId text_horizontal_align("text-horizontal-align");
const core::StringId text_vertical_align("text-vertical-align");
const core::StringId caret_color("caret-color");

// Values
// ------

// generic
const core::StringId auto_("auto");

// text-horizontal-align:
const core::StringId left("left");
const core::StringId center("center");
const core::StringId right("right");

// text-vertical-align:
const core::StringId top("top");
const core::StringId middle("middle");
const core::StringId bottom("bottom");

// pixel-hinting:
const core::StringId normal("normal");
const core::StringId off("off");

} // namespace vgc::graphics::strings
