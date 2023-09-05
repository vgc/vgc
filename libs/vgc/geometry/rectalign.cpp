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

#include <vgc/geometry/rectalign.h>

namespace vgc::geometry {

VGC_DEFINE_ENUM(
    RectAlign,
    (OutTopOutLeft, "outtop-outleft"),
    (OutTopLeft, "outtop-left"),
    (OutTop, "outtop"),
    (OutTopRight, "outtop-right"),
    (OutTopOutRight, "outtop-outright"),
    (TopOutLeft, "top-outleft"),
    (TopLeft, "top-left"),
    (Top, "top"),
    (TopRight, "top-right"),
    (TopOutRight, "top-outright"),
    (OutLeft, "outleft"),
    (Left, "left"),
    (Center, "center"),
    (Right, "right"),
    (OutRight, "outright"),
    (BottomOutLeft, "bottom-outleft"),
    (BottomLeft, "bottom-left"),
    (Bottom, "bottom"),
    (BottomRight, "bottom-right"),
    (BottomOutRight, "bottom-outright"),
    (OutBottomOutLeft, "outbottom-outleft"),
    (OutBottomLeft, "outbottom-left"),
    (OutBottom, "outbottom"),
    (OutBottomRight, "outbottom-right"),
    (OutBottomOutRight, "outbottom-outright"))

} // namespace vgc::geometry
