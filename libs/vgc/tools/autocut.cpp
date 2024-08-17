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
    const geometry::AbstractStroke2d* stroke = edge->stroke();
    const geometry::StrokeSampling2d& sampling = edge->strokeSampling();
    const geometry::StrokeSample2dArray& samples = sampling.samples();

    core::Array<geometry::CurveParameter> selfIntersections;
    //geometry::CurveParameter startParam = stroke->startParameter();
    //geometry::CurveParameter endParam = stroke->endParameter();
    Int n = samples.length();
    for (Int i = 0; i < n - 1; ++i) {
        Int jEnd = n - 1;
        if (edge->isClosed() && i == 0) {
            jEnd = n - 2;
        }
        for (Int j = i + 2; j < jEnd; ++j) {

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

                selfIntersections.append(param1);
                selfIntersections.append(param2);

                // TODO: handle special case when params are exactly equal to
                // startParam/endParam for open edges? What if some params are
                // exactly equal? Do we still want to cut twice?
            }
        }
    }

    // Cut the edge
    vacomplex::CutEdgeResult cutEdgeRes =
        vacomplex::ops::cutEdge(edge, selfIntersections);

    // Glue the new vertices two by two
    for (Int i = 0; i < selfIntersections.length() - 1; i += 2) {
        std::array<vacomplex::KeyVertex*, 2> vertices = {
            cutEdgeRes.vertices()[i], //
            cutEdgeRes.vertices()[i + 1]};
        vacomplex::ops::glueKeyVertices(vertices, vertices[0]->position());
    }
}

} // namespace vgc::tools
