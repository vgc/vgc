// Copyright 2024 The VGC Developers
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

#ifndef VGC_CANVAS_DISPLAYMODE_H
#define VGC_CANVAS_DISPLAYMODE_H

#include <vgc/canvas/api.h>
#include <vgc/core/enum.h>

namespace vgc::canvas {

/// \enum vgc::canvas::DisplayMode
/// \brief Specifies display mode.
///
// TODO: Draft, Preview, Pixel, Print..
//
enum class DisplayMode {
    Normal,
    OutlineOverlay,
    OutlineOnly
};

VGC_CANVAS_API
VGC_DECLARE_ENUM(DisplayMode)

} // namespace vgc::canvas

#endif // VGC_CANVAS_DISPLAYMODE_H
