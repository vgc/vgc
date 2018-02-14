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

#include <vgc/core/resources.h>

namespace vgc {
namespace core {

std::string resourcesPath()
{
    // This macro is defined by the build system.
    // See vgc/core/CMakeFiles.txt
    return VGC_CORE_RESOURCES_PATH_;
}

std::string resourcePath(const std::string& resourceName)
{
    // XXX use generic code to concatenate filepath, e.g.:
    //       core/fileutils.h::concatenateFilePath()
    //     or abstract this further with a core::Dir class similar to QDir.
    return resourcesPath() + "/" + resourceName;
}

} // namespace core
} // namespace vgc
