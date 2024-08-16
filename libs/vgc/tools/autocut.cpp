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

namespace {

// TODO
struct IntersectionInfo {
    geometry::CurveParameter param;
    Int index;

    bool operator<(const IntersectionInfo& other) const {
        return param < other.param;
    }
};

} // namespace

void autoCut(vacomplex::KeyEdge* edge, const AutoCutParams& params) {
    const geometry::AbstractStroke2d* stroke = edge->stroke();
    const geometry::StrokeSampling2d& sampling = edge->strokeSampling();
    const geometry::StrokeSample2dArray& samples = sampling.samples();

    //Int intersectionIndex = 0;
    //core::Array<IntersectionInfo> selfIntersections;

    core::Array<geometry::CurveParameter> selfIntersections;
    geometry::CurveParameter startParam = stroke->startParameter();
    geometry::CurveParameter endParam = stroke->endParameter();
    Int n = samples.length();
    for (Int i = 0; i < n - 1; ++i) {
        for (Int j = i + 2; j < n - 1; ++j) {
            const geometry::Vec2d& a1 = samples[i].position();
            const geometry::Vec2d& b1 = samples[i + 1].position();
            const geometry::Vec2d& a2 = samples[j].position();
            const geometry::Vec2d& b2 = samples[j + 1].position();
            if (auto intersection = fastSegmentIntersection(a1, b1, a2, b2)) {
                const geometry::CurveParameter& p1 = samples[i].parameter();
                const geometry::CurveParameter& q1 = samples[i + 1].parameter();
                const geometry::CurveParameter& p2 = samples[j].parameter();
                const geometry::CurveParameter& q2 = samples[j + 1].parameter();

                geometry::SampledCurveParameter sParam1(p1, q1, intersection->t1());
                geometry::SampledCurveParameter sParam2(p2, q2, intersection->t2());
                geometry::CurveParameter param1 = stroke->resolveParameter(sParam1);
                geometry::CurveParameter param2 = stroke->resolveParameter(sParam2);

                if (param1 != startParam) {
                    selfIntersections.append(param1);
                }
                if (param2 != endParam) {
                    selfIntersections.append(param2);
                }
            }
        }
    }

    vacomplex::ops::cutEdge(edge, selfIntersections);
}

} // namespace vgc::tools
