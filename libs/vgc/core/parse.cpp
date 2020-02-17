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

#include <vgc/core/parse.h>

#include <cmath>
#include <iomanip>
#include <sstream>

namespace vgc {
namespace core {

double toDoubleApprox(const std::string& s)
{
    std::stringstream in(s);
    return readDoubleApprox(in);

    // TODO: Use a custom StringViewStream class instead of std::stringstream
    // to avoid copying the data.

    // TODO: implement versions taking as input a custom StringView class (or
    // C++17 string_view)

    // TODO: Allow trailing whitespace but disallow trailing garbage.
    // Currently, our toDoubleApprox("1.0garbage") returns 1.0, while Python's
    // float("1.0garbage") raises ValueError.

    // TODO: Write the accurate version "toDouble()" using built-in C++
    // functions or Gay's dtoa.c (Python's modified version is probably even
    // better.). We should still do our own parsing (for precise control of
    // raised exceptions and allowed pattern), but then if valid, pass the
    // string (potentially cleaned up) to the third-party function for the
    // actual math.
}

namespace internal {

double computeDouble(bool isPositive, double a, int b, int n)
{
    if (b+n-1 > 307) {
        throw RangeError(
            std::string("The number ") + (isPositive ? "" : "-") + toString(a) +
            "e" + toString(b) + " is too big to be represented as a double.");
    }

    if (b+n-1 < -307) {
        return isPositive ? 0.0 : -0.0;
    }

    if (b < -250) {
        // Avoid subnormal numbers by keeping a large margin.
        a *= std::pow(10.0, -20);
        b += 20;
    }

    return isPositive ? a * std::pow(10.0, b) : -a * std::pow(10.0, b);

    // TODO use precomputed powers of tens for better performance and higher accuracy.
}

} // namespace internal

} // namespace core
} // namespace vgc
