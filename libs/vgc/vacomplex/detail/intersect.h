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

#ifndef VGC_VACOMPLEX_DETAIL_INTERSECT_H
#define VGC_VACOMPLEX_DETAIL_INTERSECT_H

#include <vgc/vacomplex/api.h>

#include <unordered_set>

#include <vgc/core/array.h>
#include <vgc/geometry/curve.h>
#include <vgc/vacomplex/cell.h>
#include <vgc/vacomplex/keyedge.h>

#include <vgc/vacomplex/detail/cut.h>

namespace vgc::vacomplex {

/// \class vgc::vacomplex::IntersectSettings
/// \brief Settings for intersect operations.
///
class VGC_VACOMPLEX_API IntersectSettings {
public:
    /// Returns the tolerance to use for intersection tests.
    ///
    double tolerance() const {
        return tolerance_;
    }

    /// Sets the value for `tolerance()`.
    ///
    void setTolerance(double value) {
        tolerance_ = value;
    }

    /// Whether to compute self-intersections.
    ///
    bool selfIntersect() const {
        return selfIntersect_;
    }

    /// Sets the value for `selfIntersect()`.
    ///
    void setSelfIntersect(bool value) {
        selfIntersect_ = value;
    }

    /// Whether to compute intersections with other edges.
    ///
    bool intersectEdges() const {
        return intersectEdges_;
    }

    /// Sets the value for `intersectEdges()`.
    ///
    void setIntersectEdges(bool value) {
        intersectEdges_ = value;
    }

    /// Whether to compute intersections with other faces.
    ///
    bool intersectFaces() const {
        return intersectFaces_;
    }

    /// Sets the value for `intersectEdges()`.
    ///
    void setIntersectFaces(bool value) {
        intersectFaces_ = value;
    }

private:
    double tolerance_ = 1.e-6;
    bool selfIntersect_ = true;
    bool intersectEdges_ = true;
    bool intersectFaces_ = true;
};

namespace detail {

class Operations;

using CurveParameterArray = core::Array<geometry::CurveParameter>;

// Stores at what params should a given edge be cut, as well as the result of
// the cut operation.
//
struct IntersectCutInfo {
    CurveParameterArray params;
    CutEdgeResult res; // Note: res.vertices not alive anymore at the end
};

// Stores the information that the `index1` cut vertex of `edge1` should be
// glued with the `index2` cut vertex of `edge2`.
//
struct IntersectGlueInfo {
    KeyEdge* edge1;
    Int index1;
    KeyEdge* edge2;
    Int index2;
};

using IntersectCutInfoMap = std::unordered_map<KeyEdge*, IntersectCutInfo>;
using IntersectGlueInfoArray = core::Array<IntersectGlueInfo>;

} // namespace detail

/// \class vgc::vacomplex::IntersectResult
/// \brief Information about the result of an intersect() operation.
///
class VGC_VACOMPLEX_API IntersectResult {
public:
    /// Construct an empty `IntersectResult`.
    ///
    IntersectResult() {
    }

    /// Construct an `IntersectResult` with the given output key vertices and edges.
    ///
    IntersectResult(
        core::Array<KeyVertex*> outputKeyVertices,
        core::Array<KeyEdge*> outputKeyEdges)

        : outputKeyVertices_(std::move(outputKeyVertices))
        , outputKeyEdges_(std::move(outputKeyEdges)) {
    }

    /// Returns all the key vertices that were either given as input, or that
    /// have been created by intersecting input edges.
    ///
    const core::Array<KeyVertex*>& outputKeyVertices() const {
        return outputKeyVertices_;
    }

    /// Returns all the key edges that were either given as input, or that
    /// result from intersecting these input edges.
    ///
    const core::Array<KeyEdge*>& outputKeyEdges() const {
        return outputKeyEdges_;
    }

private:
    friend detail::Operations;

    core::Array<KeyVertex*> outputKeyVertices_;
    core::Array<KeyEdge*> outputKeyEdges_;

    // Info about the cut and glue operations
    detail::IntersectCutInfoMap cutInfos_;
    detail::IntersectGlueInfoArray glueInfos_;
};

} // namespace vgc::vacomplex

#endif // VGC_VACOMPLEX_DETAIL_INTERSECT_H
