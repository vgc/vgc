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

#include <queue>

#include <vgc/core/array.h>
#include <vgc/core/assert.h>
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
        // Input
        polylines_.clear();
        segments_.clear();
        isReversed_.clear();

        // Intermediate data
        // Note: the eventQueue is already empty.

        // Output
        pointIntersections_.clear();
        pointIntersectionContributions_.clear();
    }

    /// Adds a segment.
    ///
    void addSegment(const Vec2& a, const Vec2& b) {
        addSegment_(a, b);
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
        Vec2 startPosition = op(*range.begin());
        auto endPositions = core::drop(range, 1);
        if (core::isEmpty(endPositions)) {
            return;
        }

        // Reserve memory if the number of segments can be known in advance.
        if constexpr (core::isForwardRange<Range>) {
            auto numSegmentsInPolyline = endPositions.end() - endPositions.begin();
            auto numSegments = segments_.size() + numSegmentsInPolyline;
            segments_.reserve(numSegments);
            isReversed_.reserve(numSegments);
        }

        // Add the segments
        SegmentIndex polylineBegin = segments_.length();
        for (const auto& endPosition_ : endPositions) {
            Vec2 endPosition = op(endPosition_);
            addSegment_(startPosition, endPosition);
            startPosition = endPosition;
        }
        SegmentIndex polylineEnd = segments_.length();
        polylines_.emplaceLast(polylineBegin, polylineEnd);
    }

    /// Computes the intersections based on the provided input segments.
    ///
    void computeIntersections() {

        // Insert all segment left and right points in the event queue. If both
        // endpoints of the segment have the same x-coord, the "left" point is
        // considered to be the one with the smaller y-coord.
        //
        VGC_ASSERT(eventQueue_.empty());
        Int numSegments = segments_.length();
        for (Int i = 0; i < numSegments; ++i) {
            const Segment2& segment = segments_.getUnchecked(i);
            eventQueue_.push(Event{EventType::LeftPoint, segment.a(), i});
            eventQueue_.push(Event{EventType::RightPoint, segment.b(), i});
        }

        // Initialize the list of segments intersecting the sweep line.
        //
        // Invariants:
        // - Any segment in sweepSegments_ is also in the eventQueue_
        //   as either RightPoint or Intersection (or both).
        // - Any segment that is in the eventQueue as LeftPoint is not (and
        //   has never been) in sweepSegments_.
        //
        sweepSegments_.clear();

        // Process all events.
        //
        while (!eventQueue_.empty()) {

            // Get the next event and all events sharing the same position
            Event firstEvent = eventQueue_.top();
            eventQueue_.pop();
            Vec2 position = firstEvent.position;
            sweepEvents_.clear();
            sweepEvents_.append(firstEvent);
            while (!eventQueue_.empty() && eventQueue_.top().position == position) {
                sweepEvents_.append(eventQueue_.top());
                eventQueue_.pop();
            }

            // Classify events into LeftPoint, RightPoint, Intersection
            auto it1 = sweepEvents_.begin();
            auto it4 = sweepEvents_.end();
            auto it2 = sweepEvents_.find(
                [](const Event& event) { return event.type > EventType::LeftPoint; });
            auto it3 = sweepEvents_.find(
                [](const Event& event) { return event.type > EventType::RightPoint; });
            // auto compEventType = [](const Event& event, EventType type) {
            //     return event.type < type;
            // };
            // auto it2 = std::upper_bound(it1, it4, EventType::LeftPoint, compEventType);
            // auto it3 = std::upper_bound(it2, it4, EventType::RightPoint, compEventType);
            core::ConstSpan<Event> leftEvents(it1, it2);
            core::ConstSpan<Event> rightEvents(it2, it3);
            core::ConstSpan<Event> interEvents(it3, it4);

            // TODO: if sweepEvents_ has more than one event, report as intersection.

            // Invariant: all segments in rightEvents and interEvents are adjacent
            // in sweepSegments_.

            // Find where in sweepSegments are the segments in rightEvents and interEvents.
            //
            // Geometric invariant: they are all consecutive in sweepSegments.
            // But since this might be subject to numerical errors, we do not
            // rely on this.
            //
            auto sweepSegmentsIt = sweepSegments_.begin();
            for (const Event& event : rightEvents) {
                sweepSegmentsIt = removeFromSweepSegments_(event.segmentIndex);
            }
            for (const Event& event : interEvents) {
                sweepSegmentsIt = removeFromSweepSegments_(event.segmentIndex);
            }

            // Add the interEvents and rightEvents to sweepSegments.
            // TODO: add them sorted by angle.
            for (const Event& event : interEvents) {
                sweepSegmentsIt =
                    sweepSegments_.insert(sweepSegmentsIt, event.segmentIndex);
            }
            for (const Event& event : leftEvents) {
                sweepSegmentsIt =
                    sweepSegments_.insert(sweepSegmentsIt, event.segmentIndex);
            }

            // TODO: compute intersections between newly added segments and their
            // neighbors in sweepSegments, and add them as Intersection event if
            // any.

            VGC_DEBUG_TMP_EXPR(sweepSegments_);
        }
    }

private:
    // TODO: I think this could be done in constant time by using a (flat)
    // linked list for sweepSegments_, and store in each event a handle to the
    // corresponding sweepSegments_ node.
    //
    core::Array<SegmentIndex>::iterator removeFromSweepSegments_(SegmentIndex i) {
        auto it = sweepSegments_.find(i);
        VGC_ASSERT(it != sweepSegments_.end());
        return sweepSegments_.erase(it);
    }

private:
    // Input

    core::Array<SegmentIndexPair> polylines_;
    core::Array<Segment2> segments_;
    core::Array<bool> isReversed_;

    // Add segment ensuring a <= b order.
    void addSegment_(const Vec2& a, const Vec2& b) {
        if (b < a) {
            segments_.emplaceLast(b, a);
            isReversed_.append(true);
        }
        else {
            segments_.emplaceLast(a, b);
            isReversed_.append(false);
        }
    }

    // Intermediate data

    enum class EventType : UInt8 {
        LeftPoint = 0,
        RightPoint = 1,
        Intersection = 2,
    };

    struct Event {
        EventType type;
        Vec2 position;
        SegmentIndex segmentIndex;

        friend bool operator<(const Event& e1, const Event& e2) {
            return e1.position < e2.position
                   || (e1.position == e2.position
                       && core::toUnderlying(e1.type) < core::toUnderlying(e2.type));
        }

        friend bool operator>(const Event& e1, const Event& e2) {
            return e2 < e1;
        }
    };

    std::priority_queue<Event, std::vector<Event>, std::greater<Event>> eventQueue_;

    // The segments that intersect the sweep line, ordered by increasing
    // y-coords of their intersection with the sweep line.
    //
    core::Array<SegmentIndex> sweepSegments_;

    // The list of all events that correspond to the same position while
    // processing the next event.
    //
    core::Array<Event> sweepEvents_;

    // Output

    core::Array<PointIntersection> pointIntersections_;
    core::Array<PointIntersectionContribution> pointIntersectionContributions_;
};

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_SEGMENTINTERSECTOR_H
