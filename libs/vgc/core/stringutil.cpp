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

#include <vgc/core/stringutil.h>

#include <sstream>
#include <iomanip>

namespace vgc {
namespace core {

std::string toString(unsigned char x)
{
    return std::to_string(static_cast<int>(x));
}

std::string toString(int x)
{
    return std::to_string(x);
}

std::string toString(double x)
{
    // Convert to string with fixed precision, no scientific notation.
    // Example: 1988.42 -> "1988.420000000000"
    std::stringstream ss;
    ss << std::fixed << std::setprecision(12) << x;
    std::string res = ss.str();

    // Remove trailing zeros
    int commaIndex = -1;
    int n = res.size();
    for (int i = 0; i < n; ++i) {
        if (res[i] == '.') {
            commaIndex = i;
            break;
        }
    }
    if (commaIndex > -1) {
        for (int i = n-1; i > commaIndex; --i) {
            if (res[i] == '0') {
                n = i;
            }
            else {
                break;
            }
        }
    }
    if (commaIndex == n-1) {
        n -= 1;
    }
    res.resize(n);

    return res;

    // Note: the above is presumably quite slow, but seems the only way to get
    // the desired behavior in less than 10 lines of code. In the future, we'd
    // probably want to implement our own double to string algorithm to make it
    // faster and provide user preferences.

    // XXX check that the above is always using '.' as decimal point even when
    // the locale is for instance fr_FR where ',' is commonly used.
}

} // namespace core
} // namespace vgc
