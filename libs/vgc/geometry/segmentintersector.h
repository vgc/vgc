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
#include <vgc/geometry/vec2d.h>

namespace vgc::geometry {

/// \class SegmentIntersector
/// \brief Computes all intersections between a set of line segments.
///
class VGC_GEOMETRY_API SegmentIntersector {
public:
    /// Creates a `SegmentIntersector`.
    ///
    SegmentIntersector() {
    }

    /// Adds a segment.
    ///
    void addSegment(const Vec2d& a, const Vec2d& b) {
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
        Vec2d firstPosition = op(*range.begin());
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

    /// Computes the intersections.
    ///
    void intersect();

private:
    core::Array<Vec2d> positions_; // stores segment positions
    core::Array<Int> segments_;    // stores start index of segments

    // Example:
    //
    // addSegment((1, 2), (3, 4))
    // addPolyline([(5, 6), (7, 8), (9, 10)])
    //
    // => positions_ = [(1, 2), (3, 4), (5, 6), (7, 8), (9, 10)]
    //    segments_ = [0, 2, 3]

    struct Segment_ {
        Vec2d a;
        Vec2d b;
    };

    inline Int numSegments_() const {
        return segments_.length();
    }

    inline Segment_ segment_(Int i) {
#ifdef VGC_DEBUG_BUILD
        Int j = segments_[i];
        return {positions_[j], positions_[j + 1]};
#else
        Int j = segments_.getUnchecked(i);
        return {positions_.getUnchecked(j), positions_.getUnchecked(j + 1)};
#endif
    }
};

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_SEGMENTINTERSECTOR_H
