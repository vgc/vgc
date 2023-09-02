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

#ifndef VGC_GEOMETRY_RECTANCHOR_H
#define VGC_GEOMETRY_RECTANCHOR_H

#include <vgc/core/arithmetic.h>
#include <vgc/core/enum.h>
#include <vgc/geometry/api.h>
#include <vgc/geometry/rangeanchor.h>

namespace vgc::geometry {

/// \enum vgc::geometry::RectAnchor
/// \brief Specificies anchor points on a rectangle.
///
/// \sa `RectAlign`, `RangeAnchor`,
///     `toRectAnchor(RangeAnchor, RangeAnchor)`, `toRectAnchor(RectAlign)`,
///     `reverse()`,
///     `horizontalAnchor()`, `verticalAnchor()`.
///
// clang-format off
enum class RectAnchor : UInt8 {
    TopLeft     = 0x11,
    Top         = 0x12,
    TopRight    = 0x13,

    Left        = 0x21,
    Center      = 0x22,
    Right       = 0x23,

    BottomLeft  = 0x31,
    Bottom      = 0x32,
    BottomRight = 0x33
};
// clang-format on

VGC_GEOMETRY_API
VGC_DECLARE_ENUM(RectAnchor)

/// Returns the horizontal `RangeAnchor` component of the given `RectAnchor`.
///
inline RangeAnchor horizontalAnchor(RectAnchor anchor) {
    UInt8 i = static_cast<UInt8>(anchor);
    return static_cast<RangeAnchor>(i & 0x0f);
}

/// Returns the vertical `RangeAnchor` component of the given `RectAnchor`.
///
inline RangeAnchor verticalAnchor(RectAnchor anchor) {
    UInt8 i = static_cast<UInt8>(anchor);
    return static_cast<RangeAnchor>((i & 0xf0) >> 4);
}

/// Combines the two given `RangeAnchor` into one `RectAnchor`.
///
inline RectAnchor toRectAnchor(RangeAnchor horizontalAnchor, RangeAnchor verticalAnchor) {
    UInt8 i = static_cast<UInt8>(horizontalAnchor);
    UInt8 j = static_cast<UInt8>(verticalAnchor);
    return static_cast<RectAnchor>(i + (j << 4));
}

/// Returns the reversed `RectAnchor`, that is, with `Top` and `Bottom`
/// switched, and `Left` and `Right` switched.
///
inline RectAnchor reverse(RectAnchor anchor) {
    RangeAnchor hAnchor = reverse(horizontalAnchor(anchor));
    RangeAnchor vAnchor = reverse(verticalAnchor(anchor));
    return toRectAnchor(hAnchor, vAnchor);
}

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_RECTANCHOR_H
