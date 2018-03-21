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

namespace vgc {
namespace core {

std::string toString(double x)
{
    // XXX TODO Use something that provides more control on
    // precision, formatting, trailing zeros, etc.
    //
    // Example:
    // std::stringstream ss;
    // ss << std::fixed << std::setprecision(4) << 1988.42;
    // return ss.str();
    //
    // returns "1988.4200".
    //
    // This is basically what we want (fixed number of decimals, no scientific
    // notation), but we want it faster (not using stringstream) and with no
    // trailing zeros (should return 1988.42).
    //

    return std::to_string(x);
}

} // namespace core
} // namespace vgc
