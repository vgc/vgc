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

#ifndef VGC_UI_LENGTHTYPE_H
#define VGC_UI_LENGTHTYPE_H

namespace vgc {
namespace ui {

/// \enum vgc::ui::LengthType
/// \brief Encode whether a length is "auto", and if not, what unit is used.
///
// TODO: support "Percentage" and all the dimension units of Android, they are great:
// https://developer.android.com/guide/topics/resources/more-resources.html#Dimension
//
// Could it also be useful to have max-content, min-content, or fit-content from CSS?
// https://developer.mozilla.org/en-US/docs/Web/CSS/width
//
enum class LengthType {
    Auto,
    Dp
};

} // namespace ui
} // namespace vgc

#endif // VGC_UI_LENGTHTYPE_H
