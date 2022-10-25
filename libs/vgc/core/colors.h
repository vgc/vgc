// Copyright 2021 The VGC Developers
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

#ifndef VGC_CORE_COLORS_H
#define VGC_CORE_COLORS_H

#include <vgc/core/api.h>
#include <vgc/core/color.h>

namespace vgc::core {

/// \namespace colors
/// \brief Defines predefined colors.
///
namespace colors {

// clang-format off
const Color transparent = Color(0, 0, 0, 0); ///< Predefined color equals to Color(0, 0, 0, 0)
const Color black       = Color(0, 0, 0); ///< Predefined color equals to Color(0, 0, 0)
const Color white       = Color(1, 1, 1); ///< Predefined color equals to Color(1, 1, 1)
const Color red         = Color(1, 0, 0); ///< Predefined color equals to Color(1, 0, 0)
const Color green       = Color(0, 1, 0); ///< Predefined color equals to Color(0, 1, 0)
const Color blue        = Color(0, 0, 1); ///< Predefined color equals to Color(0, 0, 1)
// clang-format on

} // namespace colors

} // namespace vgc::core

#endif // VGC_CORE_COLORS_H
