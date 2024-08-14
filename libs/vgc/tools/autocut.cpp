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
    const geometry::StrokeSampling2d& sampling = edge->strokeSampling();
    const geometry::StrokeSample2dArray& samples = sampling.samples();
    //geometry::Polyline2d polyline = edge->strokeSampling().centerline();
    Int n = samples.length();
    for (Int i = 0; i < n - 1; ++i) {
        for (Int j = i + 2; j < n - 1; ++j) {
            const geometry::Vec2d& a1 = samples[i].position();
            const geometry::Vec2d& b1 = samples[i + 1].position();
            const geometry::Vec2d& a2 = samples[j].position();
            const geometry::Vec2d& b2 = samples[j + 1].position();
            if (auto intersection = fastSegmentIntersection(a1, b1, a2, b2)) {
                // TODO:
                // const geometry::CurveParameter& p1 = samples[i].parameter();
                // const geometry::CurveParameter& q1 = samples[i + 1].parameter();
                // const geometry::CurveParameter& p2 = samples[j].parameter();
                // const geometry::CurveParameter& q2 = samples[j + 1].parameter();
                vacomplex::ops::createKeyVertex(intersection->position(), group);
            }
        }
    }
}

} // namespace vgc::tools
