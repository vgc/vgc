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

#include <vgc/geometry/windingrule.h>

namespace vgc::geometry {

bool isWindingNumberSatisfyingRule(Int windingNumber, WindingRule rule) {
    switch (rule) {
    case geometry::WindingRule::Odd:
        return (windingNumber / 2 * 2) != windingNumber;
    case geometry::WindingRule::NonZero:
        return windingNumber != 0;
    case geometry::WindingRule::Positive:
        return windingNumber > 0;
    case geometry::WindingRule::Negative:
        return windingNumber < 0;
    }
    return false;
}

} // namespace vgc::geometry
