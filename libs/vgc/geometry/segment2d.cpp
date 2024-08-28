// Copyright 2024 The VGC Developers
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
// Instead, edit tools/segment2x.cpp then run tools/generate.py.

// clang-format off

#include <vgc/geometry/segment2d.h>

namespace vgc::geometry {

Segment2dIntersection segmentIntersect(
    const Vec2d& a1,
    const Vec2d& b1,
    const Vec2d& a2,
    const Vec2d& b2) {

    Vec2d d1 = (b1 - a1);
    Vec2d d2 = (b2 - a2);
    double delta = d1.det(d2);
    if (std::abs(delta) > 0) {

        // Handle the special cases where one endpoint is equal to another.
        //
        // This is important so that the returned t-parameter is exactly 0 or 1
        // in these cases, which client code may rely on.
        //
        // Otherwise, numerical errors can occur in the rest of the computation,
        // resulting in values such as `0.00000002` or `0.99999997`.
        //
        if (a1 == a2) {
            return Segment2dIntersection(a1, 0, 0);
        }
        else if (b1 == b2) {
            return Segment2dIntersection(b1, 1, 1);
        }
        else if (a1 == b2) {
            return Segment2dIntersection(a1, 0, 1);
        }
        else if (b1 == a2) {
            return Segment2dIntersection(b1, 1, 0);
        }

        // Solve 2x2 system using Cramer's rule.
        Vec2d a1a2 = a2 - a1;
        double invDelta = 1 / delta;
        double t1 = a1a2.det(d2) * invDelta;
        double t2 = a1a2.det(d1) * invDelta;
        if (t1 >= 0. && t1 <= 1. && t2 >= 0. && t2 <= 1.) {
            Vec2d p = core::fastLerp(a1, b1, t1);
            return Segment2dIntersection(p, t1, t2);
        }
        else {
            return Segment2dIntersection();
        }
    }
    else {
        // Collinear => either empty intersection of segment intersection
        // For now we return a dummy segment intersection so that we can
        // at least detect this.
        // TODO: actually compute the segment or empty intersection.
        return Segment2dIntersection(a1, a1, 0, 0, 0, 0);
    }
}

} // namespace vgc::geometry
