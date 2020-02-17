// Copyright 2020 The VGC Developers
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

#ifndef VGC_CORE_INT2FLOAT_H
#define VGC_CORE_INT2FLOAT_H

#include <cmath>
#include <vgc/core/algorithm.h>
#include <vgc/core/api.h>
#include <vgc/core/inttypes.h>

/// \file vgc/core/int2float.h
/// \brief Defines conversions from integral types to floating-point types.
///

namespace vgc {
namespace core {

/// Maps an integer \p x in the range [0..255] to a double in the range [0, 1].
/// If the integer is not initially in the range [0..255], then it is first
/// clamped to this range.
///
VGC_CORE_API
inline double uint8ToDouble01(Int x) {
    return vgc::core::clamp(x, Int(0), Int(255)) / 255.0;
}

} // namespace core
} // namespace vgc

#endif // VGC_CORE_INT2FLOAT_H
