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

#ifndef VGC_WORKSPACE_COLORS_H
#define VGC_WORKSPACE_COLORS_H

#include <vgc/core/color.h>

namespace vgc::workspace {

/// \namespace colors
/// \brief Defines predefined colors.
///
namespace colors {

// TODO: make these user-customizable
const core::Color selection = core::Color(0.0f, 0.7f, 1.0f, 1.0f);
const core::Color outline = core::Color(1.0f, 0.3f, 0.3f, 1.0f);

} // namespace colors

} // namespace vgc::workspace

#endif // VGC_WORKSPACE_COLORS_H
