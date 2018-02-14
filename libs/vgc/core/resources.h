// Copyright 2017 The VGC Developers
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

#ifndef VGC_CORE_RESOURCES_H
#define VGC_CORE_RESOURCES_H

#include <string>
#include <vgc/core/api.h>

namespace vgc {
namespace core {

/// Returns the absolute path where the runtime resources live.
///
VGC_CORE_API std::string resourcesPath();

/// Returns the absolute path where the given runtime resources
/// live.
///
/// Example:
/// \code
/// std::string vertPath = core::resourcePath("shaders/shader.v.glsl");
/// std::string fragPath = core::resourcePath("shaders/shader.f.glsl");
/// \endcode
///
VGC_CORE_API std::string resourcePath(const std::string& resourceName);

} // namespace core
} // namespace vgc

#endif // VGC_CORE_RESOURCES_H
