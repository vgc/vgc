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
