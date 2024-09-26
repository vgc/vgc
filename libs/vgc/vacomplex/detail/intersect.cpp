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

#include <vgc/geometry/segmentintersector2.h>

namespace vgc::vacomplex {

namespace detail {

namespace {

bool isStartOrEnd(KeyEdge* edge, const geometry::CurveParameter& param) {
    if (edge->isClosed()) {
        return false;
    }
    else {
        return param == edge->stroke()->startParameter()
               || param == edge->stroke()->endParameter();
    }
}

geometry::SegmentIntersector2d::PolylineIndex
addToIntersector(KeyEdge* edge, geometry::SegmentIntersector2d& intersector) {

    const geometry::AbstractStroke2d* stroke = edge->stroke();
    const geometry::StrokeSample2dArray& samples = edge->strokeSampling().samples();

    bool isClosed = stroke->isClosed();
    bool hasDuplicateEnpoints = true; // only used if isClosed is true
    return intersector.addPolyline(   //
        isClosed,
        hasDuplicateEnpoints,
        samples,
        [](const auto& sample) { return sample.position(); });
}

geometry::CurveParameter getCurveParameter(KeyEdge* edge, Int i, double t) {
    const geometry::AbstractStroke2d* stroke = edge->stroke();
    const geometry::StrokeSample2dArray& samples = edge->strokeSampling().samples();
    const geometry::CurveParameter& p = samples[i].parameter();
    const geometry::CurveParameter& q = samples[i + 1].parameter();
    geometry::SampledCurveParameter sParam(p, q, t);
    return stroke->resolveParameter(sParam);
}

void computeIntersections(
    core::ConstSpan<KeyEdge*> edges,
    Group* group,
    const IntersectSettings& settings,
    IntersectCutInfoMap& cutInfos,
    IntersectGlueInfoArray& glueInfos) {

    using PolylineIndex = geometry::SegmentIntersector2d::PolylineIndex;
    using SegmentIndex = geometry::SegmentIntersector2d::SegmentIndex;

    if (!settings.selfIntersect() && !settings.intersectEdges()) {
        return;
    }

    // Store correspondence between polyline index and edge.
    //
    std::unordered_map<PolylineIndex, KeyEdge*> inputEdges;
    std::unordered_map<PolylineIndex, KeyEdge*> otherEdges;
    auto getInputEdge = [&](PolylineIndex i) -> KeyEdge* {
        auto it = inputEdges.find(i);
        return it != inputEdges.end() ? it->second : nullptr;
    };
    auto getOtherEdge = [&](PolylineIndex i) -> KeyEdge* {
        auto it = otherEdges.find(i);
        return it != otherEdges.end() ? it->second : nullptr;
    };

    // Create a SegmentIntersector2d and add all edges as polylines.
    //
    geometry::SegmentIntersector2d intersector;
    for (KeyEdge* edge : edges) {
        PolylineIndex i = addToIntersector(edge, intersector);
        inputEdges[i] = edge;
    }
    if (settings.intersectEdges()) {
        geometry::Rect2d bbox = geometry::Rect2d::empty;
        for (const auto& edge : edges) {
            bbox.uniteWith(edge->boundingBox());
        }
        for (Node* node : *group) {
            if (Cell* cell = node->toCell()) {
                if (KeyEdge* edge = cell->toKeyEdge()) {
                    if (!edges.contains(edge) && edge->boundingBox().intersects(bbox)) {
                        PolylineIndex i = addToIntersector(edge, intersector);
                        otherEdges[i] = edge;
                    }
                }
            }
        }
    }

    // Compute intersections.
    //
    intersector.computeIntersections();

    // Process intersections.
    //
    for (const auto& inter : intersector.pointIntersections()) {

        // For now, we only handle intersections between two segments
        if (inter.infos.length() != 2) {
            continue;
        }

        // Get segments, polylines, and edges relative to this intersection
        SegmentIndex i1 = inter.infos.first().segmentIndex;
        SegmentIndex i2 = inter.infos.last().segmentIndex;
        PolylineIndex j1 = intersector.polylineIndex(i1);
        PolylineIndex j2 = intersector.polylineIndex(i2);
        KeyEdge* edge1 = getInputEdge(j1);
        KeyEdge* edge2 = getInputEdge(j2);
        if (edge1) {
            if (edge2) {
                // Intersection between two input edges
                if (!settings.selfIntersect()) {
                    continue;
                }
            }
            else {
                // Intersection between one input edge and one other edge
                edge2 = getOtherEdge(j2);
            }
        }
        else if (edge2) {
            // Intersection between one input edge and one other edge
            edge1 = getOtherEdge(j1);
        }
        else {
            // Intersection between two other edges
            continue;
        }
        VGC_ASSERT(edge1);
        VGC_ASSERT(edge2);

        // Make i1 and i2 segment indices relative to the polyline
        i1 -= intersector.segmentIndexRange(j1).first();
        i2 -= intersector.segmentIndexRange(j2).first();

        // Get curve parameters
        double t1 = inter.infos.first().param;
        double t2 = inter.infos.last().param;
        geometry::CurveParameter param1 = getCurveParameter(edge1, i1, t1);
        geometry::CurveParameter param2 = getCurveParameter(edge2, i2, t2);

        // Do not cut at a junction between two snapped edges.
        // TODO: handle T-junctions by only cutting one of them, and gluing to
        // the existing vertex.
        if (isStartOrEnd(edge1, param1) || isStartOrEnd(edge2, param2)) {
            continue;
        }

        CurveParameterArray& cutParams1 = cutInfos[edge1].params;
        Int k1 = cutParams1.length();
        cutParams1.append(param1);

        // Warning: `cutInfos[e2]` can create the entry in the map
        // and invalidate existing iterators, e.g., `cutParams1`.
        CurveParameterArray& cutParams2 = cutInfos[edge2].params;
        Int k2 = cutParams2.length();
        cutParams2.append(param2);

        glueInfos.append(IntersectGlueInfo{edge1, k1, edge2, k2});
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

    // Compute intersection between edges and other edges in the group.
    IntersectResult res;
    IntersectCutInfoMap& cutInfos = res.cutInfos_;
    IntersectGlueInfoArray glueInfos = res.glueInfos_;
    computeIntersections(edges, group, settings, cutInfos, glueInfos);

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
