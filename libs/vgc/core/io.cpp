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

#include <vgc/core/io.h>

#include <fstream>
#include <ios>

namespace vgc {
namespace core {

std::string readFile(const std::string& filePath)
{
    // Open file
    // XXX What if failure?
    std::ifstream file(filePath);

    // Get upper limit of number of characters in the returned std::string.
    // Actual number of characters could be smaller due to end-of-line
    // conventions: on Windows, "\r\n" in the file becomes "\n" in the
    // std::string
    file.seekg(0, std::ios::end);
    long long int n = file.tellg();

    std::string res;
    if (n >= 0) {
        // Allocate memory
        res.assign(static_cast<size_t>(n), ' ');

        // Read file content and store in std::string
        file.seekg(0);
        file.read(&res[0], n);
    }
    else {
        // XXX This is a failure case of tellg; raise exception?
    }

    // Return
    return res;
}

} // namespace core
} // namespace vgc
