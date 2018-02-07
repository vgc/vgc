// Copyright 2017 The VGC Developers
// See the COPYRIGHT file at the top-level directory of this distribution
// and at https://github.com/vgc.io/vgc/blob/master/COPYRIGHT
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

#ifndef VGC_CORE_MATH_H
#define VGC_CORE_MATH_H

#include <cmath>
#include <vgc/core/api.h>

namespace vgc {
namespace core {

/// Computes the largest integer value not greater than \p x then casts the
/// result to an int. This function is equivalent to:
///
/// \code
/// static_cast<int>(std::floor(x))
/// \endcode
///
template <typename T>
int ifloor(T x) {
    return static_cast<int>(std::floor(x));
}

} // namespace core
} // namespace vgc

#endif // VGC_CORE_MATH_H
