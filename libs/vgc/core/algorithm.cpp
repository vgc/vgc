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

#include <vgc/core/algorithm.h>

namespace vgc::core {

std::string
replace(const std::string& s, const std::string& from, const std::string& to) {
    std::string res;
    res.reserve(s.size());

    // Early return if either `s` or `from` are empty strings
    size_t ns = s.size();
    size_t nf = from.size();
    if (ns == 0 || nf == 0) {
        res = s;
        return res;
    }

    size_t i = 0;
    size_t k = 0;
    while (i + k <= ns) {
        // Invariants:
        // 0 <= k <= from.size()
        // 0 <= i+k <= s.size()
        // if k>0, then s[i..i+k-1] == from[0..k-1]

        if (k == nf) {
            // Found. We replace from by to.
            res += to;
            i += k;
            k = 0;
        }
        else if (i + k == ns || s[i + k] != from[k]) {
            // Not found, let's copy s into res.
            // In most loop iteration, we end up here, and k == 0, that
            // is, we copy one more character from s to res.
            size_t imax = (std::min)(i + k, ns - 1);
            while (i <= imax) {
                res += s[i];
                ++i;
            }
            if (i == ns)
                break;
            else
                k = 0;
        }
        else {
            // Partial match found. Let's keep looking.
            ++k;
        }
    }

    return res;
}

} // namespace vgc::core
