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

#ifndef VGC_CORE_IO_H
#define VGC_CORE_IO_H

#include <string>
#include <vgc/core/api.h>
#include <vgc/core/exceptions.h>

namespace vgc {
namespace core {

/// \class vgc::core::FileError
/// \brief Raised when failed to read a file.
///
/// This exception is raised by vgc::core::readFile() if the input file cannot
/// be read (for example, due to file permissions, or because the file does
/// not exist).
///
class VGC_CORE_API FileError : public RuntimeError {
public:
    /// Constructs a FileError with the given \p reason.
    ///
    FileError(const std::string& reason) : RuntimeError(reason) {}

    /// Destructs the FileError.
    ///
    ~FileError();
};

/// Returns as a std::string the content of the file given by its \p filePath.
///
/// Exceptions:
/// - Raises FileError if the file cannot be read for any reason.
///
VGC_CORE_API
std::string readFile(const std::string& filePath);

} // namespace core
} // namespace vgc

#endif // VGC_CORE_IO_H
