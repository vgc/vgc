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

#ifndef VGC_GEOMETRY_RANGEALIGN_H
#define VGC_GEOMETRY_RANGEALIGN_H

#include <vgc/core/arithmetic.h>
#include <vgc/core/enum.h>
#include <vgc/geometry/api.h>
#include <vgc/geometry/rangeanchor.h>

namespace vgc::geometry {

/// \enum vgc::geometry::RangeAlign
/// \brief Specificies how to align a shape inside or outside of a range.
///
/// ```
///   OutMin | Min    Center    Max | OutMax
/// ---------+----------------------+--------->
///         min                    max
/// ```
///
/// \sa `toRangeAlign()`, `reverse()`,
///     `RangeAnchor`, `RectAlign`.
///
// clang-format off
enum class RangeAlign : UInt8 {
    OutMin = 0x0, ///< Outside the range, aligned with the "min" side.
    Min    = 0x1, ///< Inside the range, aligned with the "min" side.
    Center = 0x2, ///< Centered in the middle of the range.
    Max    = 0x3, ///< Inside the range, aligned with the "max" side.
    OutMax = 0x4  ///< Outside the range, aligned with the "max" side.
};
// clang-format on

VGC_GEOMETRY_API
VGC_DECLARE_ENUM(RangeAlign)

/// Returns the reversed `RangeAlign`, that is, with `Min` and `Max` switched.
///
inline RangeAlign reverse(RangeAlign align) {
    UInt8 i = static_cast<UInt8>(align);
    return static_cast<RangeAlign>(4 - i);
}

/// Converts a `RangeAnchor` to its corresponding `RangeAlign`.
///
inline RangeAlign toRangeAlign(RangeAnchor anchor) {
    return static_cast<RangeAlign>(anchor);
}

/// Converts a `RangeAlign` to its corresponsing `RangeAnchor`.
///
/// This is a lossy conversion:
/// - Both `OutMin` and `Min` are converted to `RangeAnchor::Min`.
/// - Both `OutMax` and `Max` are converted to `RangeAnchor::Max`.
///
inline RangeAnchor toRangeAnchor(RangeAlign align) {
    switch (align) {
    case RangeAlign::OutMin:
    case RangeAlign::Min:
        return RangeAnchor::Min;
    case RangeAlign::Center:
        return RangeAnchor::Center;
    case RangeAlign::Max:
    case RangeAlign::OutMax:
        return RangeAnchor::Max;
    }
    return RangeAnchor::Center;
}

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_RANGEALIGN_H
