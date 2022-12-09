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

// Note: we may want to use generic code to concatenate filepath, e.g.:
//
//       core/fileutils.h::concatenateFilePath()
//
// Or abstract this further with a core::Dir class similar to QDir.
//
// Or simply use std::filesystem::path, but GCC v7 doesn't have it even
// in C++17 mode. We have to wait until we require GCC v8+, see:
//
// https://stackoverflow.com/questions/45867379/why-does-gcc-not-seem-to-have-the-filesystem-standard-library
//

namespace vgc::core {

namespace {

std::string basePath_;
std::string pythonPath_;
std::string resourcesPath_;

void initOtherPaths() {
    pythonPath_ = basePath_ + "/python";
    resourcesPath_ = basePath_ + "/resources";
}

void ensurePathsAreInitialized() {
    if (basePath_.length() == 0) {
        char* path = std::getenv("VGCBASEPATH");
        if (path && *path != '\0') {
            basePath_ = path;
        }
        else {
            basePath_ = ".";
        }
        initOtherPaths();
    }
}

} // end namespace

void setBasePath(std::string_view path) {
    basePath_ = path;
    initOtherPaths();
}

std::string basePath() {
    ensurePathsAreInitialized();
    return basePath_;
}

std::string pythonPath() {
    ensurePathsAreInitialized();
    return pythonPath_;
}

std::string resourcesPath() {
    ensurePathsAreInitialized();
    return resourcesPath_;
}

std::string resourcePath(std::string_view name) {
    ensurePathsAreInitialized();
    std::string res;
    res.reserve(resourcesPath_.length() + name.length() + 1);
    res += resourcesPath_;
    res += '/';
    res += name;
    return res;
}

} // namespace vgc::core
