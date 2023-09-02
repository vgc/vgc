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

#ifndef VGC_GEOMETRY_RECTALIGN_H
#define VGC_GEOMETRY_RECTALIGN_H

#include <vgc/core/arithmetic.h>
#include <vgc/core/enum.h>
#include <vgc/geometry/api.h>
#include <vgc/geometry/rangealign.h>
#include <vgc/geometry/rectanchor.h>

namespace vgc::geometry {

/// \enum vgc::geometry::RectAlign
/// \brief Specificies how to align a shape inside or outside of a rectangle.
///
/// ```
///    OutTopOutLeft   OutTopLeft       OutTop       OutTopRight   OutTopOutRight
///                  +-------------------------------------------+
///       TopOutLeft | TopLeft           Top            TopRight | TopOutRight
///                  |                                           |
///          OutLeft | Left             Center             Right | OutRight
///                  |                                           |
///    BottomOutLeft | BottomLeft       Bottom       BottomRight | BottomOutRight
///                  +-------------------------------------------+
/// OutBottomOutLeft   OutBottomLeft   OutBottom  OutBottomRight   OutBottomOutRight
/// ```
///
/// \sa `RectAnchor`, `RangeAlign`,
///     `toRectAlign(RangeAlign, RangeAlign)`, `toRectAlign(RectAnchor)`,
///     `reverse()`,
///     `horizontalAlign()`, `verticalAlign()`.
///
// clang-format off
enum class RectAlign {
    OutTopOutLeft     = 0x00,
    OutTopLeft        = 0x01,
    OutTop            = 0x02,
    OutTopRight       = 0x03,
    OutTopOutRight    = 0x04,

    TopOutLeft        = 0x10,
    TopLeft           = 0x11,
    Top               = 0x12,
    TopRight          = 0x13,
    TopOutRight       = 0x14,

    OutLeft           = 0x20,
    Left              = 0x21,
    Center            = 0x22,
    Right             = 0x23,
    OutRight          = 0x24,

    BottomOutLeft     = 0x30,
    BottomLeft        = 0x31,
    Bottom            = 0x32,
    BottomRight       = 0x33,
    BottomOutRight    = 0x34,

    OutBottomOutLeft  = 0x40,
    OutBottomLeft     = 0x41,
    OutBottom         = 0x42,
    OutBottomRight    = 0x43,
    OutBottomOutRight = 0x44
};
// clang-format on

VGC_GEOMETRY_API
VGC_DECLARE_ENUM(RectAlign)

/// Returns the horizontal `RangeAlign` component of the given `RectAlign`.
///
inline RangeAlign horizontalAlign(RectAlign align) {
    UInt8 i = static_cast<UInt8>(align);
    return static_cast<RangeAlign>(i & 0x0f);
}

/// Returns the vertical `RangeAlign` component of the given `RectAlign`.
///
inline RangeAlign verticalAlign(RectAlign align) {
    UInt8 i = static_cast<UInt8>(align);
    return static_cast<RangeAlign>((i & 0xf0) >> 4);
}

/// Combines the two given `RangeAlign` into one `RectAlign`.
///
inline RectAlign toRectAlign(RangeAlign horizontalAlign, RangeAlign verticalAlign) {
    UInt8 i = static_cast<UInt8>(horizontalAlign);
    UInt8 j = static_cast<UInt8>(verticalAlign);
    return static_cast<RectAlign>(i + (j << 4));
}

/// Returns the reversed `RectAlign`, that is, with `Top` and `Bottom`
/// switched, and `Left` and `Right` switched.
///
inline RectAlign reverse(RectAlign align) {
    RangeAlign hAlign = reverse(horizontalAlign(align));
    RangeAlign vAlign = reverse(verticalAlign(align));
    return toRectAlign(hAlign, vAlign);
}

/// Converts a `RectAnchor` to its corresponding `RectAlign`.
///
inline RectAlign toRectAlign(RectAnchor anchor) {
    return static_cast<RectAlign>(anchor);
}

/// Converts a `RectAlign` to its corresponsing `RectAnchor`.
///
/// This is a lossy conversion:
/// - Both `OutTop` and `Top` are converted to `RectAnchor::Top`.
/// - Both `OutBottom` and `Bottom` are converted to `RectAnchor::Bottom`.
/// - Both `OutLeft` and `Left` are converted to `RectAnchor::Left`.
/// - Both `OutRight` and `Right` are converted to `RectAnchor::Right`.
/// - And so on, for all horizontal/vertical combinations.
///   For example, `OutTopOutLeft` is converted to `TopLeft`.
///
inline RectAnchor toRectAnchor(RectAlign align) {
    RangeAlign hAlign = horizontalAlign(align);
    RangeAlign vAlign = horizontalAlign(align);
    RangeAnchor hAnchor = toRangeAnchor(hAlign);
    RangeAnchor vAnchor = toRangeAnchor(vAlign);
    return toRectAnchor(hAnchor, vAnchor);
}

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_RECTALIGN_H
