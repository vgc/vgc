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

#include <vgc/vacomplex/detail/operations.h>

#include <vgc/geometry/segment2.h> // segmentIntersect
#include <vgc/geometry/segmentintersector2.h>

#include <vgc/core/profile.h>

namespace vgc::vacomplex {

namespace detail {

namespace {

struct IntersectionParameters {
    geometry::CurveParameter param1;
    geometry::CurveParameter param2;
};

bool isStartOrEnd(KeyEdge* edge, const geometry::CurveParameter& param) {
    if (edge->isClosed()) {
        return false;
    }
    else {
        return param == edge->stroke()->startParameter()
               || param == edge->stroke()->endParameter();
    }
}

core::Array<IntersectionParameters> computeSelfIntersections(KeyEdge* edge) {

    const geometry::AbstractStroke2d* stroke = edge->stroke();
    const geometry::StrokeSampling2d& sampling = edge->strokeSampling();
    const geometry::StrokeSample2dArray& samples = sampling.samples();

    // TODO: case of closed edge
    // TODO: support segments intersecting along a sub-segment.

    core::Array<IntersectionParameters> res;
    res.reserve(10000);
    {
        VGC_PROFILE_SCOPE("Bentley-Ottmann")

        geometry::SegmentIntersector2d intersector;
        intersector.addPolyline(
            samples, [](const auto& sample) { return sample.position(); });
        intersector.computeIntersections();
        for (const auto& inter : intersector.pointIntersections()) {
            // TODO: convert to curve parameters
            //ops::createKeyVertex(inter.position, edge->parentGroup());
            geometry::CurveParameter p{};
            res.append({p, p});
        }
        VGC_DEBUG_TMP_EXPR(intersector.pointIntersections());
    }
    VGC_DEBUG_TMP_EXPR(res.length());

    res.clear();
    {
        VGC_PROFILE_SCOPE("Naive")

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

                geometry::SegmentIntersection2d intersection =
                    segmentIntersect(a1, b1, a2, b2);

                // XXX: What to do if it intersect along a segment?
                //      Apply pertubations to the whole edge and try again?
                if (intersection.type() == geometry::SegmentIntersectionType::Point) {

                    const geometry::CurveParameter& p1 = samples[i].parameter();
                    const geometry::CurveParameter& q1 = samples[i + 1].parameter();
                    const geometry::CurveParameter& p2 = samples[j].parameter();
                    const geometry::CurveParameter& q2 = samples[j + 1].parameter();

                    geometry::SampledCurveParameter sParam1(p1, q1, intersection.t1());
                    geometry::SampledCurveParameter sParam2(p2, q2, intersection.t2());
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
    }
    VGC_DEBUG_TMP_EXPR(res.length());

    return res;
}

core::Array<IntersectionParameters>
computeEdgeIntersections(KeyEdge* edge1, KeyEdge* edge2) {

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

            geometry::SegmentIntersection2d intersection =
                segmentIntersect(a1, b1, a2, b2);

            // XXX: What to do if it intersect along a segment?
            //      Apply pertubations to the whole edge and try again?
            if (intersection.type() == geometry::SegmentIntersectionType::Point) {

                const geometry::CurveParameter& p1 = samples1[i1].parameter();
                const geometry::CurveParameter& q1 = samples1[i1 + 1].parameter();
                const geometry::CurveParameter& p2 = samples2[i2].parameter();
                const geometry::CurveParameter& q2 = samples2[i2 + 1].parameter();

                geometry::SampledCurveParameter sParam1(p1, q1, intersection.t1());
                geometry::SampledCurveParameter sParam2(p2, q2, intersection.t2());
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

void computeSelfIntersections(
    KeyEdge* edge,
    IntersectCutInfoMap& cutInfos,
    IntersectGlueInfoArray& glueInfos) {

    auto intersections = computeSelfIntersections(edge);

    // Fast-return if no interesction. This is important so that
    // cutInfos[edge1] is only created if there is actually an intersection.
    //
    if (intersections.isEmpty()) {
        return;
    }

    CurveParameterArray& cutParams = cutInfos[edge].params;
    cutParams.reserve(intersections.length() * 2);
    for (const IntersectionParameters& intersection : intersections) {
        Int n = cutParams.length();
        cutParams.append(intersection.param1);
        cutParams.append(intersection.param2);
        glueInfos.append(IntersectGlueInfo{edge, n, edge, n + 1});
    }
}

void computeEdgeIntersections(
    KeyEdge* edge1,
    KeyEdge* edge2,
    IntersectCutInfoMap& cutInfos,
    IntersectGlueInfoArray& glueInfos) {

    if (edge2 == edge1) {
        return;
    }

    auto intersections = computeEdgeIntersections(edge1, edge2);
    if (intersections.isEmpty()) {
        return;
    }

    // Get references to the cut parameters we want to append to.
    //
    // Note that we CANNOT just do:
    //   CurveParameterArray& cutParams1 = cutInfos[edge1].params;
    //   CurveParameterArray& cutParams2 = cutInfos[edge2].params;
    //
    // Because cutInfos[edge2] might insert a new CutInfo, possibly causing a
    // re-hashing and invalidating cutParams1.
    //
    auto cutInfo1 = cutInfos.find(edge1);
    if (cutInfo1 == cutInfos.end()) {
        cutInfo1 = cutInfos.insert(std::make_pair(edge1, IntersectCutInfo{})).first;
    }
    auto cutInfo2 = cutInfos.find(edge2);
    if (cutInfo2 == cutInfos.end()) {
        cutInfo2 = cutInfos.insert(std::make_pair(edge2, IntersectCutInfo{})).first;
        cutInfo1 = cutInfos.find(edge1);
    }
    VGC_ASSERT(cutInfo1 != cutInfos.end());
    VGC_ASSERT(cutInfo2 != cutInfos.end());
    CurveParameterArray& cutParams1 = cutInfo1->second.params;
    CurveParameterArray& cutParams2 = cutInfo2->second.params;

    // Compute and append intersections
    cutParams1.reserve(cutParams1.length() + intersections.length());
    cutParams2.reserve(cutParams2.length() + intersections.length());
    for (const IntersectionParameters& intersection : intersections) {
        Int n1 = cutParams1.length();
        Int n2 = cutParams2.length();
        cutParams1.append(intersection.param1);
        cutParams2.append(intersection.param2);
        glueInfos.append(IntersectGlueInfo{edge1, n1, edge2, n2});
    }
}

void computeEdgeIntersections(
    KeyEdge* edge1,
    Group* group,
    IntersectCutInfoMap& cutInfos,
    IntersectGlueInfoArray& glueInfos) {

    for (Node* node : *group) {
        if (Cell* cell = node->toCell()) {
            if (KeyEdge* edge2 = cell->toKeyEdge()) {
                computeEdgeIntersections(edge1, edge2, cutInfos, glueInfos);
            }
        }
    }
}

void cutEdges(IntersectCutInfoMap& cutInfos) {
    for (auto& [edge, cutInfo] : cutInfos) {
        cutInfo.res = ops::cutEdge(edge, cutInfo.params);
    }
}

void glueVertices(
    core::Array<KeyVertex*>& outputKeyVertices,
    const IntersectCutInfoMap& cutInfos,
    const IntersectGlueInfoArray& glueInfos) {

    for (const IntersectGlueInfo& glueInfo : glueInfos) {
        const IntersectCutInfo& cutInfo1 = cutInfos.at(glueInfo.edge1);
        const IntersectCutInfo& cutInfo2 = cutInfos.at(glueInfo.edge2);
        std::array<KeyVertex*, 2> vertices = {
            cutInfo1.res.vertices()[glueInfo.index1], //
            cutInfo2.res.vertices()[glueInfo.index2]};
        KeyVertex* v = ops::glueKeyVertices(vertices, vertices[0]->position());
        outputKeyVertices.append(v);
    }
}

std::optional<geometry::Vec2d> getInteriorPosition(KeyEdge* edge) {
    const geometry::StrokeSample2dArray& samples = edge->strokeSamples();
    Int n = samples.length();
    if (n < 2) {
        return std::nullopt;
    }
    else if (n == 2) {
        const geometry::StrokeSample2d& s0 = samples[0];
        const geometry::StrokeSample2d& s1 = samples[1];
        return 0.5 * (s0.position() + s1.position());
    }
    else { // n > 2
        Int i = n / 2;
        VGC_ASSERT(i > 0);
        VGC_ASSERT(i < n - 1);
        return samples.getUnchecked(i).position();

        // Proof of the above asserts:
        // (using math notation, that is, '/' means real division)
        //
        // We have:
        // [1]  i = floor(n/2)
        // [2]  For all k, k - 1 <= floor(k) <= k
        // [3]  n > 2
        // [3'] -n < -2
        //
        // Therefore:
        //
        // i =  floor(n/2)  [1]
        //   >= n/2 - 1     [2]
        //   >  2/2 - 1     [3]
        //   =  0
        //
        // i =  floor(n/2)  [1]
        //   <= n/2         [2]
        //   =  n - n/2
        //   <  n - 2/2     [3']
        //   =  n - 1
    }
}

// Cut with `edge` all faces that are in the given `group`.
//
// We do this by computing, for each new `edge` along `edge1`, the set of
// faces that are overlapping `edge`.
//
// To do this, we can arbitrarily pick any position on the edge, and
// compute which faces contain that position. Indeed, this set of faces is
// (typically) invariant of the chosen position on the edge, since `edge1`
// was already cut at all intersections with other edges.
//
// A special case where the above is not true is if a face belongs to the
// same group as `edge1`, but its boundary edges do not (in which case they
// were not cut). We do not yet handle this case properly. One possible
// solution might be, in the previous step (cut edges), to also include in
// the set of intersected edges all the edges that are in the boundary of
// faces in the same group as `edge1`.
//
// Another special case is if AutoCutParams::cutFaces() is true but
// AutoCutParams::cutEdges() is false. I can only see this make sense for
// example to draw a closed "hole" in a face. But it is unclear what these
// settings should do in the other cases. We do not handle it in a special way
// for now, so results may be unexpected with these settings.
//
// For now, similarly to how we cut all edges regardless of whether they
// are obscured by faces or above `edge1`, we cut all faces regardless of
// whether they are obscured by other faces of above `edge1`. In the
// future, we might add a setting to take into account such obscured
// edges/faces, and only cut the top-most elements below `edge1`.
// Unfortunately, this is a bit difficult and ill-defined in cases where
// the group is non-planar.
//
void cutFaces(Group* group, KeyEdge* edge) {

    auto position = getInteriorPosition(edge);
    if (!position) {
        return;
    }

    // Find which faces should be cut. This must be done before any cutting,
    // since cutting modifies the group's children.
    //
    core::Array<KeyFace*> facesToCut;
    for (Node* node : *group) {
        if (Cell* cell = node->toCell()) {
            if (KeyFace* face = cell->toKeyFace()) {
                if (face->boundary().contains(edge)) {
                    continue;
                }
                if (face->interiorContains(*position)) {
                    facesToCut.append(face);
                }
            }
        }
    }

    // Cut the faces
    for (KeyFace* face : facesToCut) {
        ops::cutGlueFace(face, edge);
    }
}

} // namespace

IntersectResult Operations::intersectWithGroup(
    core::ConstSpan<KeyEdge*> edges,
    Group* group,
    const IntersectSettings& settings) {

    // Compute info about intersections
    IntersectResult res;
    IntersectCutInfoMap& cutInfos = res.cutInfos_;
    IntersectGlueInfoArray glueInfos = res.glueInfos_;
    if (settings.selfIntersect()) {
        for (KeyEdge* edge : edges) {
            computeSelfIntersections(edge, cutInfos, glueInfos);
        }
    }

    // TMP
    return res;

    if (settings.intersectEdges()) {
        for (const auto& edge : edges) {
            computeEdgeIntersections(edge, group, cutInfos, glueInfos);
        }
    }

    // Cut edges at given CurveParameters and glue vertices two-by-two
    cutEdges(cutInfos);
    glueVertices(res.outputKeyVertices_, cutInfos, glueInfos);

    // Cut faces and add edges to output
    if (settings.intersectFaces()) {
        for (KeyEdge* edge : edges) {
            const auto it = cutInfos.find(edge);
            if (it == cutInfos.end()) {
                // The edge wasn't cut
                cutFaces(group, edge);
                res.outputKeyEdges_.append(edge);
            }
            else {
                // The edge was cut into multiple new edges
                const IntersectCutInfo& edgeCutInfo = it->second;
                for (KeyEdge* newEdge : edgeCutInfo.res.edges()) {
                    cutFaces(group, newEdge);
                    res.outputKeyEdges_.append(newEdge);
                }
            }
        }
    }

    return res;
}

} // namespace detail

} // namespace vgc::vacomplex
