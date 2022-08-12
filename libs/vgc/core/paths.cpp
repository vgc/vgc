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

#include <vgc/core/paths.h>

#include <cstdlib> // getenv

namespace vgc::core {

namespace {

std::string basePath_;

} // end namespace

void setBasePath(const std::string& path) {
    basePath_ = path;
}

std::string basePath() {
    if (basePath_.length() == 0) {
        char* path = std::getenv("VGCBASEPATH");
        if (path) {
            basePath_ = path;
        }
        else {
            basePath_ = ".";
        }
    }
    return basePath_;
}

std::string pythonPath() {
    // XXX use generic code to concatenate filepath, e.g.:
    //       core/fileutils.h::concatenateFilePath()
    //     or abstract this further with a core::Dir class similar to QDir.
    return basePath() + "/python";
}

std::string resourcesPath() {
    // XXX use generic code to concatenate filepath, e.g.:
    //       core/fileutils.h::concatenateFilePath()
    //     or abstract this further with a core::Dir class similar to QDir.
    return basePath() + "/resources";
}

std::string resourcePath(const std::string& name) {
    // XXX use generic code to concatenate filepath, e.g.:
    //       core/fileutils.h::concatenateFilePath()
    //     or abstract this further with a core::Dir class similar to QDir.
    return resourcesPath() + "/" + name;
}

} // namespace vgc::core
