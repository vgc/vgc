// Copyright 2023 The VGC Developers
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

#include <vgc/core/arithmetic.h>

namespace vgc::core {

float pow10f(Int exp) {
    constexpr Int n = 10;
    constexpr Int m = 2 * n + 1;
    // clang-format off
    static float table[m] = {
      1e-10f, 1e-9f, 1e-8f, 1e-7f, 1e-6f, 1e-5f, 1e-4f, 1e-3f, 1e-2f, 1e-1f,
      1.f, 1e1f, 1e2f, 1e3f, 1e4f, 1e5f, 1e6f, 1e7f, 1e8f, 1e9f, 1e10f };
    // clang-format on
    if (-n <= exp && exp <= n) {
        return table[exp + n];
    }
    else {
        return std::pow(10.0f, static_cast<float>(exp));
    }
}

double pow10d(Int exp) {
    constexpr Int n = 22;
    constexpr Int m = 2 * n + 1;
    // clang-format off
    static double table[m] = {
        1e-22, 1e-21, 1e-20, 1e-19, 1e-18, 1e-17, 1e-16, 1e-15, 1e-14, 1e-13,
        1e-12, 1e-11, 1e-10, 1e-9, 1e-8, 1e-7, 1e-6, 1e-5, 1e-4, 1e-3, 1e-2,
        1e-1, 1.f, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10, 1e11,
        1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22 };
    // clang-format on
    if (-n <= exp && exp <= n) {
        return table[exp + n];
    }
    else {
        return std::pow(10.0, static_cast<double>(exp));
    }
}

} // namespace vgc::core
