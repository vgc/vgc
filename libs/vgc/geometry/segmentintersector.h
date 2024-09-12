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

#include <algorithm> // lower_bound, upper_bound, equal_range
#include <queue>     // priority_queue
#include <utility>   // pair
#include <vector>

#include <vgc/core/array.h>
#include <vgc/core/assert.h>
#include <vgc/core/ranges.h>
#include <vgc/core/span.h>
#include <vgc/geometry/api.h>
#include <vgc/geometry/segment.h>
#include <vgc/geometry/vec.h>

namespace vgc::geometry::detail::segmentintersector {

using SegmentIndex = Int;

enum class EventType : UInt8 {
    LeftPoint = 0,
    RightPoint = 1,
    Intersection = 2,
};

template<typename Scalar>
struct Event {
    EventType type;
    Vec<2, Scalar> position;
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

} // namespace vgc::geometry::detail::segmentintersector

template<>
struct fmt::formatter<vgc::geometry::detail::segmentintersector::EventType>
    : fmt::formatter<std::string_view> {

    using EventType = vgc::geometry::detail::segmentintersector::EventType;

    template<typename FormatContext>
    auto format(const EventType& eventType, FormatContext& ctx) {
        switch (eventType) {
        case EventType::LeftPoint:
            return format_to(ctx.out(), "LeftPoint");
        case EventType::RightPoint:
            return format_to(ctx.out(), "RightPoint");
        case EventType::Intersection:
            return format_to(ctx.out(), "Intersection");
        }
    }
};

template<typename Scalar>
struct fmt::formatter<vgc::geometry::detail::segmentintersector::Event<Scalar>>
    : fmt::formatter<std::string_view> {

    using EventType = vgc::geometry::detail::segmentintersector::EventType;
    using Event = vgc::geometry::detail::segmentintersector::Event<Scalar>;

    template<typename FormatContext>
    auto format(const Event& e, FormatContext& ctx) {
        return format_to(
            ctx.out(),
            "{{type={}, position={}, segmentIndex={}",
            e.type,
            e.position,
            e.segmentIndex);
    }
};

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

    using SegmentIndex = detail::segmentintersector::SegmentIndex;
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
        segmentSlopes_.clear();
        isReversed_.clear();

        // Intermediate data
        // Note: the eventQueue is already empty.

        // Output
        pointIntersections_.clear();
        pointIntersectionContributions_.clear();
    }

private:
    // The slope is used to determine segment order in sweepSegments_ when
    // several segments are outgoing at an intersection position. It is
    // intentionally equal to infinity for vertical segments.
    //
    static Scalar computeSlope_(const Vec2& a, const Vec2& b) {
        if (b.x() == a.x()) {
            return core::infinity<Scalar>;
            // Note: positive infinity since we know b.y() >= a.y()
            // If b.y() == a.y(), the segment is degenerate and
            // the slope will be ignored anyway.
        }
        else {
            return (b.y() - a.y()) / (b.x() - a.x());
        }
    }

    // Add segment ensuring a <= b order.
    void addSegment_(const Vec2& a, const Vec2& b) {
        if (b < a) {
            segments_.emplaceLast(b, a);
            segmentSlopes_.append(computeSlope_(b, a));
            isReversed_.append(true);
        }
        else {
            segments_.emplaceLast(a, b);
            segmentSlopes_.append(computeSlope_(a, b));
            isReversed_.append(false);
        }
    }

public:
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

        // This implements a variant of the Bentley-Ottmann algorithm, see:
        //
        // https://en.wikipedia.org/wiki/Bentley%E2%80%93Ottmann_algorithm
        //
        // In the comments below, we assume a right-handed coordinate system:
        //
        //           above / top
        //
        //             ^ y
        //    left     |           right
        //             |
        //             +-----> x
        //
        //           below / bottom
        //
        // The sweep line is vertical and moves from left to right.
        //
        // The sweep segments (record of which segments are intersecting the
        // sweep line at any given time in the algorithm) are sorted in
        // ascending y-coord, that is, from bottom to top, with respect to the
        // position where they intersect the sweep line.

        // Insert the left endpoint and right endpoint of all segments into the
        // event queue. If both endpoints have the same x-coord (i.e., the
        // segment is vertical), the "left" endpoint is considered to be the
        // one with the smaller y-coord. If both endpoints are equal (i.e., the
        // segment is degenerate, reduced to a point), the order is irrelevant.
        //
        VGC_ASSERT(eventQueue_.empty());
        Int numSegments = segments_.length();
        for (Int i = 0; i < numSegments; ++i) {
            const Segment2& segment = segments_.getUnchecked(i);
            eventQueue_.push(Event{EventType::LeftPoint, segment.a(), i});
            eventQueue_.push(Event{EventType::RightPoint, segment.b(), i});
        }

        // Initialize the list of segments intersecting the sweep line (the
        // "sweep segments").
        //
        // Invariants:
        //
        // - Any segment in the eventQueue_ as LeftPoint event is not (and has
        //   never been) in sweepSegments_.
        //
        // - Any segment in the eventQueue_ as LeftPoint event is also in the
        //   eventQueue_ as RightPoint event, but is not in the eventQueue_ as
        //   Intersection event.
        //
        // - Any segment in the eventQueue_ as Intersection event is also in the
        //   eventQueue_ as RightPoint event, but is not in the eventQueue_ as
        //   LefPoint event.
        //
        // - Any segment in sweepSegments_ is also in the eventQueue_
        //   as exactly one RightPoint event, and possibly one or several
        //   Intersection events.
        //
        // - There are no duplicates in sweepSegments_.
        //
        sweepSegments_.clear();

        // Process all events.
        //
        while (!eventQueue_.empty()) {

            VGC_DEBUG_TMP("----------------------------------");

            VGC_DEBUG_TMP("Sweep segments before event: {}", sweepSegments_);

            // Get the next event and all events sharing the same position. We
            // call these the "sweep events".
            //
            // Note that in some variants of Bentley-Ottmann, the event queue
            // never contains two events for the same position. In our case, we
            // do keep duplicates in the queue, since std::priority_queue does
            // not support checking for the content of the queue. We could use
            // a separate unordered_set for this, but it isn't necessary, as
            // handling duplicate events is not a problem.
            //
            // Also, in some variants, Intersection events are removed from the
            // queue upon handling of a LeftPoint event:
            //
            //      |
            // x----------------- s1
            //      |
            //      |
            //      x-- s3 added as new segment =>
            //      |   - remove from queue potential intersection between s1 and s2.
            //      |   - add to queue potential intersection between s1 and s3.
            //      |   - add to queue potential intersection between s2 and s3.
            //      |
            //   x--------------- s2
            //      |
            //      |
            //    sweep line
            //
            // We do not do that either. One reason is

            //
            Event firstEvent = eventQueue_.top();
            eventQueue_.pop();
            Vec2 position = firstEvent.position;
            sweepEvents_.clear();
            sweepEvents_.append(firstEvent);
            while (!eventQueue_.empty() && eventQueue_.top().position == position) {
                sweepEvents_.append(eventQueue_.top());
                eventQueue_.pop();
            }

            VGC_DEBUG_TMP("Handling position : {}", position);

            // Classify events into LeftPoint, RightPoint, Intersection
            // The following code works because the event type is part of the priority,
            // with LeftPoint < RightPoint < Intersection.
            auto seIt1 = sweepEvents_.begin();
            auto seIt4 = sweepEvents_.end();
            auto seIt2 = sweepEvents_.find(
                [](const Event& event) { return event.type > EventType::LeftPoint; });
            auto seIt3 = sweepEvents_.find(
                [](const Event& event) { return event.type > EventType::RightPoint; });
            core::ConstSpan<Event> leftEvents(seIt1, seIt2);  // segments to add
            core::ConstSpan<Event> rightEvents(seIt2, seIt3); // segments to remove
            core::ConstSpan<Event> interEvents(seIt3, seIt4); // segments to keep

            // Invariants:
            // - leftEvents does not contain duplicates
            // - rightEvents does not contain duplicates
            //
            // However, interEvents may contain duplicate, since two segments
            // s1 and s2 may become neighbors in sweepSegments_ (adding their
            // intersection to the queue), then not neighbor anymore, then
            // neighbor again (re-adding their intersection to the queue)

            VGC_DEBUG_TMP("Sweep events: {}", sweepEvents_);
            VGC_DEBUG_TMP("  LeftPoint events: {}", leftEvents);
            VGC_DEBUG_TMP("  RightPoint events: {}", rightEvents);
            VGC_DEBUG_TMP("  Intersection events: {}", interEvents);

            // Find all segments in sweepSegments_ that contain `position`
            // either in their interior or as an endpoint. Some of these may
            // already be in the events above (e.g., if there is an
            // Intersection event), but some may not, e.g., if there is a
            // T-junction at the `position`.
            //
            // This basically partition sweepSegments_ into three spans:
            // - the segments that are below the position
            // - the segment that contain the position
            // - the segment that are above the position
            //
            // Note that all segments in interEvents should in theory be found
            // in sweepSegments_ as containing the position, but in practice
            // may not due to numerical errors. More precisely, after computing
            // the intersection p between two, because of rounding errors, it
            // is possible that p is not considered exactly on each of the
            // segments.
            //
            struct SsComp {
                // Cannot use a lambda since equal_range will call both
                // i < p and p < i, and i and p are of different types.

                core::Array<Segment2>& segments;

                // Returns whether seg[i] < p: "is p above the segment?"
                bool operator()(SegmentIndex i, Vec2 p) {
                    const Segment2& seg = segments.getUnchecked(i);
                    return isOrientationPositive_(seg.a(), seg.b(), p);
                }

                // Returns whether p < seg[i]: "is p below the segment?"
                bool operator()(Vec2 p, SegmentIndex i) {
                    const Segment2& seg = segments.getUnchecked(i);
                    return isOrientationNegative_(seg.a(), seg.b(), p);
                }
            };
            SsComp ssComp{segments_};

            auto ssIt1 = sweepSegments_.begin();
            auto ssIt4 = sweepSegments_.end();
            auto [ssIt2, ssIt3] = std::equal_range(ssIt1, ssIt4, position, ssComp);

            core::ConstSpan<SegmentIndex> belowSegments(ssIt1, ssIt2);
            core::ConstSpan<SegmentIndex> containSegments(ssIt2, ssIt3);
            core::ConstSpan<SegmentIndex> aboveSegments(ssIt3, ssIt4);

            VGC_DEBUG_TMP("Sweep segments below position:      {}", belowSegments);
            VGC_DEBUG_TMP("Sweep segments containing position: {}", containSegments);
            VGC_DEBUG_TMP("Sweep segments above position:      {}", aboveSegments);

            // The segments in interEvents are supposed to all be in
            // containSegments, but may not due to numerical errors. We fix
            // this here.
            //
            // Note that the segments in rightEvents are guaranteed to be in
            // containSegments, since their right endpoint is guaranteed to be
            // exactly equal to position. Unless the segments were not truly
            // "partitioned" as per the definition of equal_range, see:
            //
            // https://en.cppreference.com/w/cpp/algorithm/equal_range
            // https://en.cppreference.com/w/cpp/algorithm#Requirements
            //
            bool containSegmentsChanged = false;
            for (const Event& event : interEvents) {
                if (!containSegments.contains(event.segmentIndex)) {
                    VGC_DEBUG_TMP("{} not in containSegments", event);
                    // TODO: extend search from neighbor instead of using linear-time find
                    auto ssIt = sweepSegments_.find(event.segmentIndex);
                    if (ssIt != sweepSegments_.end()) {
                        if (ssIt >= ssIt3) {
                            ssIt3 = ssIt + 1;
                            containSegments = core::ConstSpan<SegmentIndex>(ssIt2, ssIt3);
                            aboveSegments = core::ConstSpan<SegmentIndex>(ssIt3, ssIt4);
                            containSegmentsChanged = true;
                        }
                        else if (ssIt < ssIt2) {
                            ssIt2 = ssIt;
                            belowSegments = core::ConstSpan<SegmentIndex>(ssIt1, ssIt2);
                            containSegments = core::ConstSpan<SegmentIndex>(ssIt2, ssIt3);
                            containSegmentsChanged = true;
                        }
                    }
                }
            }
            if (containSegmentsChanged) {
                VGC_DEBUG_TMP("Sweep segments below position:      {}", belowSegments);
                VGC_DEBUG_TMP("Sweep segments containing position: {}", containSegments);
                VGC_DEBUG_TMP("Sweep segments above position:      {}", aboveSegments);
            }

            // TODO: if sweepEvents_ has more than one event, report as intersection.

            // Find which segments are outgoing at the position. These will
            // be used as replacement for the current containSegments.
            //
            // It is guaranteed to have no duplicates since sweepSegments_ does
            // not have duplicates (hence its subspan containSegments does not
            // have duplicate either) and all the segments in leftEvents have
            // never been added to the event queue yet.
            //
            outgoingSegments_.clear();
            for (SegmentIndex i : containSegments) {
                const Segment2& segment = segments_.getUnchecked(i);
                if (segment.b() != position) {
                    outgoingSegments_.append(i);
                }
                else {
                    // This means that the segment is a RightPoint event
                    // and we will therefore remove it from sweepSegments_
                }
            }
            for (const Event& event : leftEvents) {
                SegmentIndex i = event.segmentIndex;
                const Segment2& segment = segments_.getUnchecked(i);
                if (!segment.isDegenerate()) {
                    outgoingSegments_.append(i);
                }
                else {
                    // This means the segment was reduced to a point
                    // and was both a LeftPoint
                    // and RightPoint event. We never add it to sweepSegments_.
                    //
                    // TODO: correcly report intersections between degenerate
                    // segments and other segments, including when two degenerate
                    // segments are equal, we should report them as intersecting.
                }
            }

            // Sort outgoing segments by increasing angle
            const auto& slopes = segmentSlopes_; // needed for lambda capture
            core::sort(
                outgoingSegments_,                            //
                [&slopes](SegmentIndex i1, SegmentIndex i2) { //
                    return slopes.getUnchecked(i1) < slopes.getUnchecked(i2);
                });

            // Remove ingoing segments and add outgoing segments.
            // This invalidates previous spans.
            auto ssIt = sweepSegments_.erase(ssIt2, ssIt3);
            sweepSegments_.insert(ssIt, outgoingSegments_);

            // TODO: compute intersections between newly added segments and their
            // neighbors in sweepSegments, and add them as Intersection event if
            // any.

            VGC_DEBUG_TMP("Sweep segments after event: {}", sweepSegments_);
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

    static Int8 detSign_(const Vec2& a, const Vec2& b) {
        // In some cases, a.det(a) (i.e., a.x() * b.y() -  b.x() * a.y())
        // does not exactly return 0 even when a == b. The implementation
        // below provides more accurate results.
        Scalar s = a.x() * b.y();
        Scalar t = b.x() * a.y();
        return static_cast<Int8>(t < s) - static_cast<Int8>(s < t);
    }

    static bool isDetPositive_(const Vec2& a, const Vec2& b) {
        Scalar s = a.x() * b.y();
        Scalar t = b.x() * a.y();
        return t < s;
    }

    static bool isDetNegative_(const Vec2& a, const Vec2& b) {
        Scalar s = a.x() * b.y();
        Scalar t = b.x() * a.y();
        return s < t;
    }

    static Int8 orientation_(const Vec2& a, const Vec2& b, const Vec2& c) {
        return detSign_(b - a, c - a);
    }

    static bool isOrientationPositive_(const Vec2& a, const Vec2& b, const Vec2& c) {
        return isDetPositive_(b - a, c - a);
    }

    static bool isOrientationNegative_(const Vec2& a, const Vec2& b, const Vec2& c) {
        return isDetNegative_(b - a, c - a);
    }

private:
    // Input.
    //
    core::Array<SegmentIndexPair> polylines_;
    core::Array<Segment2> segments_;
    core::Array<Scalar> segmentSlopes_;
    core::Array<bool> isReversed_;

    // The event queue.
    //
    using Event = detail::segmentintersector::Event<Scalar>;
    using EventType = detail::segmentintersector::EventType;
    using EventQueue = std::priority_queue< //
        Event,
        std::vector<Event>,
        std::greater<Event>>;
    EventQueue eventQueue_;

    // The segments that intersect the sweep line, ordered by increasing
    // y-coords of their intersection with the sweep line.
    //
    core::Array<SegmentIndex> sweepSegments_;

    // The new segments that must be added (or removed and re-added) to
    // sweepSegments_ when handling an event.
    //
    core::Array<SegmentIndex> outgoingSegments_;

    // The list of all events that correspond to the same position while
    // processing the next event.
    //
    core::Array<Event> sweepEvents_;

    // Output.
    //
    core::Array<PointIntersection> pointIntersections_;
    core::Array<PointIntersectionContribution> pointIntersectionContributions_;
};

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_SEGMENTINTERSECTOR_H
