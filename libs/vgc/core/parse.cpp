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

#include <vgc/core/parse.h>

#include <cmath>
#include <iomanip>
#include <sstream>

namespace vgc::core {

namespace detail {

double computeDouble(bool isPositive, double a, int b, int n) {
    if (b + n - 1 > 307) {
        throw RangeError(
            std::string("The number ") + (isPositive ? "" : "-") + toString(a) + "e"
            + toString(b) + " is too big to be represented as a double.");
    }

    if (b + n - 1 < -307) {
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

} // namespace detail

} // namespace vgc::core
