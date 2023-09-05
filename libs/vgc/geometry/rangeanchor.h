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

#ifndef VGC_GEOMETRY_RANGEANCHOR_H
#define VGC_GEOMETRY_RANGEANCHOR_H

#include <vgc/core/arithmetic.h>
#include <vgc/core/enum.h>
#include <vgc/geometry/api.h>

namespace vgc::geometry {

/// \enum vgc::geometry::RangeAnchor
/// \brief Specificies anchor points on a range.
///
/// \sa `toRangeAnchor()`, `reverse()`,
///     `RangeAlign`, `RectAnchor`.
///
// clang-format off
enum class RangeAnchor : UInt8 {
    Min    = 0x1,
    Center = 0x2,
    Max    = 0x3
};
// clang-format on

VGC_GEOMETRY_API
VGC_DECLARE_ENUM(RangeAnchor)

/// Returns the reversed `RangeAnchor`, that is, with `Min` and `Max` switched.
///
inline RangeAnchor reverse(RangeAnchor anchor) {
    UInt8 i = static_cast<UInt8>(anchor);
    return static_cast<RangeAnchor>(4 - i);
}

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_RANGEANCHOR_H
