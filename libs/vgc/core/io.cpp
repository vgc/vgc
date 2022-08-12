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

#include <vgc/core/io.h>

#include <fstream>
#include <ios>

#include <vgc/core/exceptions.h>

namespace vgc::core {

std::string readFile(const std::string& filePath) {
    // Open an input file stream that throws on badbit, and check that the
    // file stream was successfully opened.
    //
    std::ifstream in(filePath);
    in.exceptions(std::ifstream::badbit);
    if (!in) {
        throw FileError("Cannot open file " + filePath);
    }

    try {
        // Get upper bound of number of characters to be extracted. This is
        // expected not to be exact on Windows due to `\r\n` being translated
        // to `\n`.
        //
        // Note: seekg() and/or tellg() may set badbit and throw if an error
        // occurs, for example if the file does not exist or if we don't have
        // the permission to read that file.
        //
        // See:
        // - https://en.cppreference.com/w/cpp/io/basic_istream/seekg
        // - https://en.cppreference.com/w/cpp/io/basic_istream/tellg
        //
        in.seekg(0, std::ios::end);
        size_t n = in.tellg();
        in.seekg(0);

        // Construct a string long enough to hold the entire file.
        //
        std::string res(n, ' ');

        // Attempt to extract n characters from the file to the string.
        //
        // Note: read() will call `setstate(failbit|eofbit)` if the
        // end-of-file condition occurs before n characters could be
        // extracted. This is expected on Windows since each occurence of
        // `\r\n` is only extracted as a single character `\n`.
        //
        // See:
        // - https://en.cppreference.com/w/cpp/io/basic_istream/read
        // - https://en.cppreference.com/w/cpp/io/ios_base/iostate
        //
        in.read(&res[0], n);
        std::ifstream::iostate state = in.rdstate();
        std::ifstream::iostate ok1 = std::ifstream::goodbit;
        std::ifstream::iostate ok2 = std::ifstream::failbit | std::ifstream::eofbit;
        if (state != ok1 && state != ok2) {
            throw FileError("Cannot read file " + filePath);
        }

        // Query the number of successfully extracted characters, and shrink the
        // output string accordingly. Note that we have the following relation:
        //
        //     gcount = n - (number of occurences of `\r\n` in the file)
        //
        std::streamsize gcount = in.gcount();
        res.resize(gcount);

        // Return the string
        return res;
    }
    catch (std::ios_base::failure& fail) {
        throw FileError("Cannot read file " + filePath + ": " + fail.what());
    }
}

} // namespace vgc::core
