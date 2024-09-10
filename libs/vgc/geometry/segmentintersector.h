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

#ifndef VGC_GEOMETRY_SEGMENTINTERSECTOR_H
#define VGC_GEOMETRY_SEGMENTINTERSECTOR_H

#include <vgc/core/array.h>
#include <vgc/core/ranges.h>
#include <vgc/core/span.h>
#include <vgc/geometry/api.h>
#include <vgc/geometry/segment.h>
#include <vgc/geometry/vec.h>

namespace vgc::geometry {

/// \class SegmentIntersector
/// \brief Computes all intersections between a set of line segments.
///
template<typename Scalar>
class VGC_GEOMETRY_API SegmentIntersector {
public:
    static constexpr Int dimension = 2;
    using ScalarType = Scalar;

    using Segment2 = Segment<2, ScalarType>;
    using Vec2 = Vec<2, Scalar>;

    using PositionIndex = Int;
    using SegmentIndex = Int;
    using SegmentIndexPair = std::pair<SegmentIndex, SegmentIndex>;
    using PointIntersectionIndex = Int;

    // When two or more segments intersect at a point, then for each involved
    // segment we store its corresponding intersection parameter.
    //
    struct PointIntersectionContribution {
        Int pointIntersectionIndex;
        Int segmentIndex;
        Scalar param;
    };

    struct PointIntersection {
        Vec2 position;
        core::Array<PointIntersectionContribution> contributions;
        // TODO Use SmallArray<2, PointIntersectionContribution>, since most
        // intersections are expected to only involve 2 segments. Or if we
        // really want to avoid any memory allocation via pre-reserved
        // arrays, we could use a custom allocator or pairs of indices
        // into another array.
    };

    /// Creates a `SegmentIntersector`.
    ///
    SegmentIntersector() {
    }

    /// Re-initializes this `SegmentIntersector` to its initial state, but
    /// keeping reserved memory for future use.
    ///
    /// It is typically faster to clear an existing `SegmentIntersector` and
    /// re-use it, rather than instanciating a new `SegmentIntersector`, since
    /// the former would minimize the number of dynamic memory allocations.
    ///
    void clear() {
        positions_.clear();
        segments_.clear();
        pointIntersections_.clear();
        pointIntersectionContributions_.clear();
    }

    /// Adds a segment.
    ///
    void addSegment(const Vec2& a, const Vec2& b) {
        Int segmentIndex = positions_.length();
        positions_.append(a);
        positions_.append(b);
        segments_.append(segmentIndex);
    }

    /// Adds a polyline.
    ///
    template<typename Range, VGC_REQUIRES(core::isInputRange<Range>)>
    void addPolyline(const Range& range) {
        addPolyline(range.begin(), range.end());
    }

    template<typename Range, typename UnaryOp, VGC_REQUIRES(core::isInputRange<Range>)>
    void addPolyline(const Range& range, UnaryOp op) {

        // Do nothing if the range does not contain at least two elements.
        if (core::isEmpty(range)) {
            return;
        }
        Vec2 firstPosition = op(*range.begin());
        auto otherPositions = core::drop(range, 1);
        if (core::isEmpty(otherPositions)) {
            return;
        }

        // Reserve memory if it is possible to know in advance
        // the number of elements in the range.
        if constexpr (core::isForwardRange<Range>) {
            auto numPositions = range.end() - range.begin();
            auto numSegments = numPositions - 1;
            positions_.reserve(positions_.size() + numPositions);
            positions_.reserve(segments_.size() + numSegments);
        }

        // Add all the positions and segments
        positions_.append(firstPosition);
        for (const auto& value : otherPositions) {
            positions_.append(op(value));
            segments_.append(positions_.length() - 2);
        }
    }

    /// Computes the intersections based on the provided input segments.
    ///
    void computeIntersections() {
    }

private:
    // Input

    core::Array<Vec2> positions_;         // stores segment positions
    core::Array<PositionIndex> segments_; // stores start index of segments

    // Example:
    //
    // addSegment((1, 2), (3, 4))
    // addPolyline([(5, 6), (7, 8), (9, 10)])
    //
    // => positions_ = [(1, 2), (3, 4), (5, 6), (7, 8), (9, 10)]
    //    segments_ = [0, 2, 3]

    Int numSegments_() const {
        return segments_.length();
    }

    Segment2 getSegment_(SegmentIndex i) {
#ifdef VGC_DEBUG_BUILD
        PositionIndex j = segments_[i];
        return {positions_[j], positions_[j + 1]};
#else
        PositionIndex j = segments_.getUnchecked(i);
        return {positions_.getUnchecked(j), positions_.getUnchecked(j + 1)};
#endif
    }

    // Output

    core::Array<PointIntersection> pointIntersections_;
    core::Array<PointIntersectionContribution> pointIntersectionContributions_;
};

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_SEGMENTINTERSECTOR_H
