// Copyright 2018 The VGC Developers
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

#ifndef VGC_CORE_LIMITS_H
#define VGC_CORE_LIMITS_H

/// \file vgc/core/limits.h
/// \brief Provides limit values for numeric types, such as maxInt(), etc.
///
/// This header provides the same functionality as found in the C++ standard
/// header <limits>, but with a more concise and user-friendly API.
///

#include <limits>
#include <vgc/core/api.h>

namespace vgc {
namespace core {

/// Returns the maximum value that an 'int' can hold.
///
VGC_CORE_API
inline constexpr int maxInt() {
    return std::numeric_limits<int>::max();
}

} // namespace core
} // namespace vgc

#endif // VGC_CORE_LIMITS_H
