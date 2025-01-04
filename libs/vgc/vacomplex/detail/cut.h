// Copyright 2023 The VGC Developers
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

#ifndef VGC_VACOMPLEX_DETAIL_CUT_H
#define VGC_VACOMPLEX_DETAIL_CUT_H

#include <vgc/core/array.h>
#include <vgc/geometry/curve.h>
#include <vgc/vacomplex/cell.h>

namespace vgc::vacomplex {

/// \class vgc::vacomplex::CutEdgeResult
/// \brief Information about the result of a cutEdge() operation.
///
class VGC_VACOMPLEX_API CutEdgeResult {
public:
    /// Construct an empty `CutEdgeResult`.
    ///
    CutEdgeResult() {
    }

    /// Constructs a `CutEdgeResult` storing the given new `vertices` and `edges`.
    ///
    CutEdgeResult(core::Array<KeyVertex*> vertices, core::Array<KeyEdge*> edges)
        : vertices_(std::move(vertices))
        , edges_(std::move(edges)) {
    }

    /// Returns the new vertices that the cut produced, in the same order as the
    /// sequence of `CurveParameter` given to `cutEdge()`.
    ///
    const core::Array<KeyVertex*>& vertices() const {
        return vertices_;
    }

    /// Returns the new edges that the cut produced, ordered as a path in the
    /// same direction as the original edge.
    ///
    /// This order is not the same as `vertices()` unless the sequence of
    /// `CurveParameter` given to `cutEdge()` was already sorted in increasing
    /// order.
    ///
    const core::Array<KeyEdge*>& edges() const {
        return edges_;
    }

    /// Returns the first new vertex that the cut produced.
    ///
    /// This is equivalent to `vertices().first()`.
    ///
    /// This method is useful in the common case where the `cutEdge()`
    /// operation was called with a single `CurveParameter` (e.g., cutting an
    /// open edge into two open edges), in which case it returns the unique new
    /// vertex corresponding to the cut.
    ///
    /// Throws `IndexError` if the cut produced no vertex, that is, if no
    /// `CurveParameter` was given to `cutEdge()`.
    ///
    KeyVertex* vertex() const {
        return vertices().first();
    }

private:
    core::Array<KeyVertex*> vertices_;
    core::Array<KeyEdge*> edges_;
};

class VGC_VACOMPLEX_API CutFaceResult {
public:
    constexpr CutFaceResult() noexcept = default;

    CutFaceResult(KeyFace* face1, KeyEdge* edge, KeyFace* face2) noexcept
        : edge_(edge)
        , face1_(face1)
        , face2_(face2) {
    }

    KeyEdge* edge() const {
        return edge_;
    }

    void setEdge(KeyEdge* edge) {
        edge_ = edge;
    }

    KeyFace* face1() const {
        return face1_;
    }

    void setFace1(KeyFace* face1) {
        face1_ = face1;
    }

    KeyFace* face2() const {
        return face2_;
    }

    void setFace2(KeyFace* face2) {
        face2_ = face2;
    }

private:
    KeyEdge* edge_ = nullptr;
    KeyFace* face1_ = nullptr;
    KeyFace* face2_ = nullptr;
};

enum class OneCycleCutPolicy {
    Auto,
    Disk,
    Mobius, // non-planar non-orientable.
    Torus,  // non-planar orientable (e.g.: once-punctured torus).
};

VGC_VACOMPLEX_API
VGC_DECLARE_ENUM(OneCycleCutPolicy)

enum class TwoCycleCutPolicy {
    Auto,
    ReverseNone,
    ReverseStart,
    ReverseEnd,
    ReverseBoth
};

VGC_VACOMPLEX_API
VGC_DECLARE_ENUM(TwoCycleCutPolicy)

} // namespace vgc::vacomplex

#endif // VGC_VACOMPLEX_DETAIL_CUT_H
