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

#include <vgc/tools/autocut.h>

#include <vgc/core/array.h>
#include <vgc/geometry/intersect.h>
#include <vgc/vacomplex/cell.h>
#include <vgc/vacomplex/keyedge.h>
#include <vgc/vacomplex/operations.h>

namespace vgc::tools {

void autoCut(vacomplex::KeyEdge* edge, const AutoCutParams& params) {
    vacomplex::Group* group = edge->parentGroup();
    geometry::Polyline2d polyline = edge->strokeSampling().centerline();
    Int n = polyline.length();
    for (Int i = 0; i < n - 1; ++i) {
        for (Int j = i + 2; j < n - 1; ++j) {
            const geometry::Vec2d& a1 = polyline[i];
            const geometry::Vec2d& b1 = polyline[i + 1];
            const geometry::Vec2d& a2 = polyline[j];
            const geometry::Vec2d& b2 = polyline[j + 1];
            if (auto intersection = geometry::fastSegmentIntersection(a1, b1, a2, b2)) {
                vacomplex::ops::createKeyVertex(intersection->position(), group);
            }
        }
    }
}

} // namespace vgc::tools
