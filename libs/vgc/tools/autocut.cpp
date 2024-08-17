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

struct IntersectionParameters {
    geometry::CurveParameter param1;
    geometry::CurveParameter param2;
};

bool isStartOrEnd(vacomplex::KeyEdge* edge, const geometry::CurveParameter& param) {
    if (edge->isClosed()) {
        return false;
    }
    else {
        return param == edge->stroke()->startParameter()
               || param == edge->stroke()->endParameter();
    }
}

core::Array<IntersectionParameters> computeSelfIntersections(vacomplex::KeyEdge* edge) {

    const geometry::AbstractStroke2d* stroke = edge->stroke();
    const geometry::StrokeSampling2d& sampling = edge->strokeSampling();
    const geometry::StrokeSample2dArray& samples = sampling.samples();

    core::Array<IntersectionParameters> res;

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

                if (!(isStartOrEnd(edge, param1) || isStartOrEnd(edge, param2))) {
                    res.append(IntersectionParameters{param1, param2});
                }
                // TODO: handle special case when params are exactly equal to
                // startParam/endParam for open edges? What if some params are
                // exactly equal? Do we still want to cut twice?
            }
        }
    }
    return res;
}

core::Array<IntersectionParameters>
computeIntersections(vacomplex::KeyEdge* edge1, vacomplex::KeyEdge* edge2) {

    if (!edge1->boundingBox().intersects(edge2->boundingBox())) {
        return {};
    }

    const geometry::AbstractStroke2d* stroke1 = edge1->stroke();
    const geometry::StrokeSampling2d& sampling1 = edge1->strokeSampling();
    const geometry::StrokeSample2dArray& samples1 = sampling1.samples();

    const geometry::AbstractStroke2d* stroke2 = edge2->stroke();
    const geometry::StrokeSampling2d& sampling2 = edge2->strokeSampling();
    const geometry::StrokeSample2dArray& samples2 = sampling2.samples();

    core::Array<IntersectionParameters> res;

    Int n1 = samples1.length();
    Int n2 = samples2.length();
    for (Int i1 = 0; i1 < n1 - 1; ++i1) {
        for (Int i2 = 0; i2 < n2 - 1; ++i2) {

            const geometry::Vec2d& a1 = samples1[i1].position();
            const geometry::Vec2d& b1 = samples1[i1 + 1].position();
            const geometry::Vec2d& a2 = samples2[i2].position();
            const geometry::Vec2d& b2 = samples2[i2 + 1].position();
            if (auto intersection = fastSegmentIntersection(a1, b1, a2, b2)) {

                const geometry::CurveParameter& p1 = samples1[i1].parameter();
                const geometry::CurveParameter& q1 = samples1[i1 + 1].parameter();
                const geometry::CurveParameter& p2 = samples2[i2].parameter();
                const geometry::CurveParameter& q2 = samples2[i2 + 1].parameter();

                geometry::SampledCurveParameter sParam1(p1, q1, intersection->t1());
                geometry::SampledCurveParameter sParam2(p2, q2, intersection->t2());
                geometry::CurveParameter param1 = stroke1->resolveParameter(sParam1);
                geometry::CurveParameter param2 = stroke2->resolveParameter(sParam2);

                if (!(isStartOrEnd(edge1, param1) || isStartOrEnd(edge2, param2))) {
                    res.append(IntersectionParameters{param1, param2});
                }
                // TODO: handle special case when params are exactly equal to
                // startParam/endParam for open edges? What if some params are
                // exactly equal? Do we still want to cut twice?
            }
        }
    }
    return res;
}

using CurveParameterArray = core::Array<geometry::CurveParameter>;

// Stores at what params should a given edge be cut, as well as the result of
// the cut operation.
//
struct CutInfo {
    CurveParameterArray params;
    vacomplex::CutEdgeResult res;
};

// Stores the information that the `index1` cut vertex of `edge1` should be
// glued with the `index2` cut vertex of `edge2`.
//
struct GlueInfo {
    vacomplex::KeyEdge* edge1;
    Int index1;
    vacomplex::KeyEdge* edge2;
    Int index2;
};

} // namespace

void autoCut(vacomplex::KeyEdge* edge1, const AutoCutParams& params) {

    std::unordered_map<vacomplex::KeyEdge*, CutInfo> cutInfos;
    core::Array<GlueInfo> glueInfos;

    // Compute self-intersections.
    //
    if (params.cutItself()) {
        auto intersections = computeSelfIntersections(edge1);
        CurveParameterArray& cutParams = cutInfos[edge1].params;
        cutParams.reserve(intersections.length() * 2);
        for (const IntersectionParameters& intersection : intersections) {
            Int n = cutParams.length();
            cutParams.append(intersection.param1);
            cutParams.append(intersection.param2);
            glueInfos.append(GlueInfo{edge1, n, edge1, n + 1});
        }
    }

    // Compute intersections with other edges.
    //
    if (params.cutEdges()) {
        for (vacomplex::Node* node : *edge1->parentGroup()) {
            if (vacomplex::Cell* cell = node->toCell()) {
                if (vacomplex::KeyEdge* edge2 = cell->toKeyEdge()) {
                    if (edge2 != edge1) {
                        auto intersections = computeIntersections(edge1, edge2);
                        if (intersections.isEmpty()) {
                            continue;
                        }
                        CurveParameterArray& cutParams1 = cutInfos[edge1].params;
                        CurveParameterArray& cutParams2 = cutInfos[edge2].params;
                        cutParams1.reserve(cutParams1.length() + intersections.length());
                        cutParams2.reserve(cutParams2.length() + intersections.length());
                        for (const IntersectionParameters& intersection : intersections) {
                            Int n1 = cutParams1.length();
                            Int n2 = cutParams2.length();
                            cutParams1.append(intersection.param1);
                            cutParams2.append(intersection.param2);
                            glueInfos.append(GlueInfo{edge1, n1, edge2, n2});
                        }
                    }
                }
            }
        }
    }

    // Cut the edges.
    //
    for (auto& [edge, cutInfo] : cutInfos) {
        cutInfo.res = vacomplex::ops::cutEdge(edge, cutInfo.params);
    }

    // Glue the new vertices.
    //
    for (const GlueInfo& glueInfo : glueInfos) {
        const CutInfo& cutInfo1 = cutInfos[glueInfo.edge1];
        const CutInfo& cutInfo2 = cutInfos[glueInfo.edge2];
        std::array<vacomplex::KeyVertex*, 2> vertices = {
            cutInfo1.res.vertices()[glueInfo.index1], //
            cutInfo2.res.vertices()[glueInfo.index2]};
        vacomplex::ops::glueKeyVertices(vertices, vertices[0]->position());
    }
}

} // namespace vgc::tools
