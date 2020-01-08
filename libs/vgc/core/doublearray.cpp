// Copyright 2020 The VGC Developers
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

#include <vgc/core/doublearray.h>

#include <sstream>
#include <vgc/core/streamutil.h>
#include <vgc/core/stringutil.h>

namespace vgc {
namespace core {

std::string toString(const DoubleArray& a)
{
    return toString(a.stdVector());
}

DoubleArray toDoubleArray(const std::string& s)
{
    // XXX TODO Use custom StringStream
    std::stringstream in(s);

    DoubleArray res;
    skipWhitespaceCharacters(in);
    skipExpectedCharacter(in, '[');
    char separator = -1;
    do {
        res.append(readDoubleApprox(in));
        skipWhitespaceCharacters(in);
        separator = readExpectedCharacter(in, {',', ']'});
    }
    while (separator != ']');
    skipWhitespaceCharacters(in);
    skipExpectedEof(in);

    return res;
}

} // namespace core
} // namespace vgc
