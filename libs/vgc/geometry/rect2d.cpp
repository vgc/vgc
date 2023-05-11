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

// This file was automatically generated, please do not edit directly.
// Instead, edit tools/rect2x.cpp then run tools/generate.py.

// clang-format off

#include <vgc/geometry/rect2d.h>

namespace vgc::geometry {

bool Rect2d::intersectsSegmentWithExternalEndpoints_(
    const Vec2d& p0,
    const Vec2d& p1,
    int p0c,
    int p0r,
    int p1c,
    int p1r) const {

    if (p0c == p1c) {
        if (p0c != 0) {
            // p0 and p1 are both on the same side of rect in x.
            //
            //        ┆     ┆
            //   p0  q1────q2     p0
            //    │   │     │     /
            //   p1  q4────q3    /
            //        ┆     ┆  p1
            //
            return false;
        }
        else if (p0r + p1r == 3) {
            // p0 and p1 are on both sides of the rect in y and
            // inside the rect bounds in x.
            //
            //       ┆  p0   ┆
            //      q1───┼──q2
            //       │   │   │
            //      q4───┼──q3
            //       ┆  p1   ┆
            //
            return true;
        }
    }
    if (p0r == p1r) {
        if (p0r != 0) {
            // p0 and p1 are both on the same side of rect in y.
            //
            //     p0──────p1
            //  ┄┄┄┄┄┄┄q1────q2┄┄┄┄┄┄┄
            //          │     │
            //  ┄┄┄┄┄┄┄q4────q3┄┄┄┄┄┄┄
            //            p0──────p1
            //
            return false;
        }
        else if (p0c + p1c == 3) {
            // p0 and p1 are on both sides of the rect in x and
            // inside the rect bounds in y.
            //
            //  ┄┄┄┄┄┄┄q1────q2┄┄┄┄┄┄┄
            //     p0───┼─────┼───p1
            //          │     │
            //  ┄┄┄┄┄┄┄q4────q3┄┄┄┄┄┄┄
            //
            return true;
        }
    }
    if (p0 == p1) {
        // p0 and p1 are equal and outside of the rect
        return false;
    }
    // Here, the remaining cases are limited to (symmetries excluded):
    //
    //  p0 ┆    ┆       p0 ┆    ┆          ┆ p0 ┆
    //  ┄┄┄a────c┄┄┄    ┄┄┄a────c┄┄┄    ┄┄┄a────c┄┄┄
    //     │    │ p1       │    │          │    │ p1
    //  ┄┄┄b────d┄┄┄    ┄┄┄b────d┄┄┄    ┄┄┄b────d┄┄┄
    //     ┆    ┆          ┆    ┆ p1       ┆    ┆
    //
    // We can see that in every case, p0p1 intersects the rect if and only if
    // any corner is on p0p1 or corners are not all on the same side of p0p1.
    //
    Vec2d p0p1 = p1 - p0;
    auto computeAngleOrientation = [](const Vec2d& ab, const Vec2d& ac) {
        double det = ab.det(ac);
        if (det == 0) {
            return 1;
        }
        return (det > 0) ? 4 : 2;
    };
    int o1 = computeAngleOrientation(p0p1, corner(0) - p0);
    int o2 = computeAngleOrientation(p0p1, corner(1) - p0);
    int o3 = computeAngleOrientation(p0p1, corner(2) - p0);
    int o4 = computeAngleOrientation(p0p1, corner(3) - p0);

    int ox = o1 & o2 & o3 & o4;
    if (ox & 1) {
        // a corner is on p0p1.
        return true;
    }
    else if (ox == 6) {
        // some corners are on different sides of p0p1.
        return true;
    }

    return false;
}

} // namespace vgc::geometry
