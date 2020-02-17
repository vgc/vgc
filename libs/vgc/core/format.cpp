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

#include <vgc/core/format.h>

namespace vgc {
namespace core {

std::string secondsToString(double t, TimeUnit unit, int decimals)
{
    std::string u;
    switch (unit) {
    case TimeUnit::Seconds:                u = "s";  break;
    case TimeUnit::Milliseconds: t *= 1e3; u = "ms"; break;
    case TimeUnit::Microseconds: t *= 1e6; u = "µs"; break;
    case TimeUnit::Nanoseconds:  t *= 1e9; u = "ns"; break;
    }    
    return core::format("{:.{}f}{}", t, decimals, u);
}

} // namespace core
} // namespace vgc
