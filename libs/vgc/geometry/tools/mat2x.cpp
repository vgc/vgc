// Copyright 2022 The VGC Developers
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

// This file is used to generate all the variants of this class.
// You must manually run generate.py after any modification.

// clang-format off

#include "mat2x.h"

#include <limits>

namespace vgc::geometry {

Mat2x Mat2x::inverted(bool* isInvertible, float epsilon_) const {

    Mat2x res;

    const auto& d = data_;
    auto& inv = res.data_;

    inv[0][0] =   d[1][1];
    inv[1][0] = - d[1][0];

    float det = d[0][0]*inv[0][0] + d[0][1]*inv[1][0];

    if (std::abs(det) <= epsilon_) {
        if (isInvertible) {
            *isInvertible = false;
        }
        constexpr float inf = core::infinity<float>;
        res.setElements(inf, inf,
                        inf, inf);
    }
    else {
        if (isInvertible) {
            *isInvertible = true;
        }

        inv[0][1] = - d[0][1];
        inv[1][1] =   d[0][0];

        res *= static_cast<float>(1) / det;
    }
    return res;
}

} // namespace vgc::geometry
