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

#ifndef VGC_CORE_IO_H
#define VGC_CORE_IO_H

#include <string>
#include <vgc/core/api.h>

namespace vgc::core {

/// Returns as an `std::string` the content of the file at the given `filePath`.
///
/// Exceptions:
/// - Raises `FileError` if the file cannot be read for any reason.
///
VGC_CORE_API
std::string readFile(std::string_view filePath);

/// Returns whether the file at the given `filePath` exists.
///
VGC_CORE_API
bool fileExists(std::string_view filePath);

} // namespace vgc::core

#endif // VGC_CORE_IO_H
