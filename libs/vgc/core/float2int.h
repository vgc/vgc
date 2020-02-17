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

#ifndef VGC_CORE_FLOAT2INT_H
#define VGC_CORE_FLOAT2INT_H

#include <cmath>
#include <vgc/core/algorithm.h>
#include <vgc/core/api.h>
#include <vgc/core/inttypes.h>

/// \file vgc/core/float2int.h
/// \brief Defines conversions from floating-point types to integral types.
///

namespace vgc {
namespace core {

/// Maps a double \p x in the range [0, 1] to an 8-bit unsigned integer in
/// the range [0..255]. More precisely, the returned value is the integer in
/// [0..255] which is closest to 255*x.
///
VGC_CORE_API
inline UInt8 double01ToUint8(double x) {
    double y = std::round(vgc::core::clamp(x, 0.0, 1.0) * 255.0);
    return static_cast<UInt8>(y);
}

} // namespace core
} // namespace vgc

#endif // VGC_CORE_FLOAT2INT_H
