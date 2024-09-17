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

#ifndef VGC_GEOMETRY_SEGMENTINTERSECTOR2_H
#define VGC_GEOMETRY_SEGMENTINTERSECTOR2_H

#include <algorithm> // lower_bound, upper_bound, equal_range
#include <queue>     // priority_queue
#include <utility>   // pair
#include <vector>

#include <vgc/core/array.h>
#include <vgc/core/assert.h>
#include <vgc/core/ranges.h>
#include <vgc/core/span.h>
#include <vgc/geometry/api.h>
#include <vgc/geometry/segment2.h>
#include <vgc/geometry/vec2.h>

#define VGC_DEBUG_TMP_INTER VGC_DEBUG_TMP
#define VGC_DEBUG_TMP_INTER(...)

namespace vgc::geometry {

namespace detail::segmentintersector2 {

using SegmentIndex = Int;
using PointIntersectionIndex = Int;
using SegmentIndexPair = std::pair<SegmentIndex, SegmentIndex>;

enum class EventType : UInt8 {
    Left = 0,
    Right = 1,
    Intersection = 2,
};

template<typename T>
struct InputData;

template<typename T>
struct AlgorithmData;

template<typename T>
struct OutputData;

} // namespace detail::segmentintersector2

/// \class SegmentIntersector2
/// \brief Computes all intersections between a set of line segments.
///
template<typename T>
class VGC_GEOMETRY_API SegmentIntersector2 {
public:
    using ScalarType = T;
    static constexpr Int dimension = 2;

    using SegmentIndex = detail::segmentintersector2::SegmentIndex;
    using SegmentIndexPair = detail::segmentintersector2::SegmentIndexPair;
    using PointIntersectionIndex = detail::segmentintersector2::PointIntersectionIndex;

    /// When two or more segments intersect at a point, then for each involved
    /// segment we store its corresponding intersection parameter.
    ///
    struct PointIntersectionInfo {
        Int pointIntersectionIndex;
        Int segmentIndex;
        T param;
    };

    /// Stores the position of an intersection point, together with the
    /// information of which segments are intersecting at that position.
    ///
    // TODO: Minimize memory allocations via SmallArray<2, Info>?
    // Indeed, most intersections are expected to only involve 2 segments.
    // Alternatively, we could use a custom allocator or pairs of indices
    // into another array.
    //
    struct PointIntersection {
        Vec2<T> position;
        core::Array<PointIntersectionInfo> infos;
    };

    /// Creates a `SegmentIntersector2`.
    ///
    SegmentIntersector2() {
    }

    /// Re-initializes this `SegmentIntersector2` to its initial state, but
    /// keeping reserved memory for future use.
    ///
    /// It is typically faster to clear an existing `SegmentIntersector2` and
    /// re-use it, rather than instanciating a new `SegmentIntersector2`, since
    /// the former would minimize the number of dynamic memory allocations.
    ///
    void clear() {
        input_.clear();
        algorithm_.clear();
        output_.clear();
    }

    /// Adds a segment.
    ///
    void addSegment(const Vec2<T>& a, const Vec2<T>& b);

    /// Adds a polyline.
    ///
    template<typename Range, VGC_REQUIRES(core::isInputRange<Range>)>
    void addPolyline(const Range& range) {
        addPolyline(range.begin(), range.end(), core::identity);
    }

    /// Adds a polyline.
    ///
    template<typename Range, typename UnaryOp, VGC_REQUIRES(core::isInputRange<Range>)>
    void addPolyline(const Range& range, UnaryOp op);

    /// Computes the intersections between the input segments and polylines.
    ///
    void computeIntersections();

    /// Returns the computed point intersections.
    ///
    const core::Array<PointIntersection>& pointIntersections() const {
        return output_.pointIntersections;
    };

private:
    detail::segmentintersector2::InputData<T> input_;
    detail::segmentintersector2::AlgorithmData<T> algorithm_;
    detail::segmentintersector2::OutputData<T> output_;
};

namespace detail::segmentintersector2 {

// We implement here a variant of the Bentley-Ottmann algorithm.
//
// References:
//
// - [Wiki] https://en.wikipedia.org/wiki/Bentley%E2%80%93Ottmann_algorithm
// - [BentleyOttmann] Algorithms for Reporting and Counting Geometric Intersections (1979)
// - [BoissonatPreparata] Robust Plane Sweep for Intersecting Segments (2000)
// - [deBerg] Computational Geometry, Algorithms and Applications, Third Edition (2008)
// - [Mount] CMSC-754 Computational Geometry Lecture Notes (2012)
//
// In all the comments, we assume a right-handed coordinate system:
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
// The sweep segments (record of which segments are intersecting the sweep line
// at any given time in the algorithm, sometimes called the "status structure")
// are sorted in ascending y-coord, that is, from bottom to top, with respect
// to the position where they intersect the sweep line.

// Convenient aliases to public types

template<typename T>
using PointIntersection = typename SegmentIntersector2<T>::PointIntersection;

template<typename T>
using PointIntersectionInfo = typename SegmentIntersector2<T>::PointIntersectionInfo;

// Private data types

template<typename T>
struct Event {
    EventType type;
    Vec2<T> position;
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

template<typename T>
struct InputData {
    core::Array<SegmentIndexPair> polylines;
    core::Array<Segment2<T>> segments;
    core::Array<T> segmentSlopes;
    core::Array<bool> isReversed;

    void clear() {
        polylines.clear();
        segments.clear();
        segmentSlopes.clear();
        isReversed.clear();
    }
};

template<typename T>
struct OutputData {
    core::Array<PointIntersection<T>> pointIntersections;

    void clear() {
        pointIntersections.clear();
    }
};

template<typename T>
struct AlgorithmData {

    // The event queue.
    //
    using EventQueue = std::priority_queue< //
        Event<T>,
        std::vector<Event<T>>,
        std::greater<Event<T>>>;
    EventQueue eventQueue;

    // The segments that intersect the sweep line, ordered by increasing
    // y-coords of their intersection with the sweep line.
    //
    core::Array<SegmentIndex> sweepSegments;

    // The new segments that must be added (or removed and re-added) to
    // sweepSegments_ when handling an event.
    //
    core::Array<SegmentIndex> outgoingSegments;

    // The list of all events that correspond to the same position while
    // processing the next event.
    //
    core::Array<Event<T>> sweepEvents;

    void clear() {
        VGC_ASSERT(eventQueue.empty()); // priority_queue has no clear() method
        sweepSegments.clear();
        outgoingSegments.clear();
        sweepEvents.clear();
    }
};

// The slope is used to determine segment ordering in sweep segments when
// several segments are outgoing at an intersection position. It is
// intentionally equal to infinity for vertical segments.
//
template<typename T>
T computeSlope(const Vec2<T>& a, const Vec2<T>& b) {
    if (b.x() == a.x()) {
        return core::infinity<T>;
        // Note: positive infinity since we know b.y() >= a.y()
        // If b.y() == a.y(), the segment is degenerate and
        // the slope will be ignored anyway.
    }
    else {
        return (b.y() - a.y()) / (b.x() - a.x());
    }
}

// Add segment ensuring a <= b order.
template<typename T>
void addSegment(InputData<T>& in, const Vec2<T>& a, const Vec2<T>& b) {
    if (b < a) {
        in.segments.emplaceLast(b, a);
        in.segmentSlopes.append(computeSlope<T>(b, a));
        in.isReversed.append(true);
    }
    else {
        in.segments.emplaceLast(a, b);
        in.segmentSlopes.append(computeSlope<T>(a, b));
        in.isReversed.append(false);
    }

    // [1] We need to fully qualify computeSlope<T> because otherwise the
    // template arg couldn't be deduced, since Vec2f/Vec2d are not actually
    // template instanciations of Vec2. This might be reason enough to abandon
    // the current python-generated Vec2f/Vec2d, and use proper C++ templates
    // instead, despite longer symbol names. Also, we basically cannot have
    // auto-completion for these types here: we can autocomplete on `in.<tab>`,
    // but not on `a.<tab>`.
}

template<
    typename T,
    typename Range,
    typename UnaryOp,
    VGC_REQUIRES(core::isInputRange<Range>)>
void addPolyline(InputData<T>& in, const Range& range, UnaryOp op) {

    // Do nothing if the range does not contain at least two elements.
    if (core::isEmpty(range)) {
        return;
    }
    Vec2<T> startPosition = op(*range.begin());
    auto endPositions = core::drop(range, 1);
    if (core::isEmpty(endPositions)) {
        return;
    }

    // Reserve memory if the number of segments can be known in advance.
    if constexpr (core::isForwardRange<Range>) {
        auto numSegmentsInPolyline = endPositions.end() - endPositions.begin();
        auto numSegments = in.segments.size() + numSegmentsInPolyline;
        in.segments.reserve(numSegments);
        in.isReversed.reserve(numSegments);
    }

    // Add the segments
    SegmentIndex polylineBegin = in.segments.length();
    for (const auto& endPosition_ : endPositions) {
        Vec2<T> endPosition = op(endPosition_);
        addSegment(in, startPosition, endPosition);
        startPosition = endPosition;
    }
    SegmentIndex polylineEnd = in.segments.length();
    in.polylines.emplaceLast(polylineBegin, polylineEnd);
}

template<typename T>
Int8 detSign(const Vec2<T>& a, const Vec2<T>& b) {
    // In some cases, a.det(a) (i.e., a.x() * b.y() -  b.x() * a.y())
    // does not exactly return 0 even when a == b. The implementation
    // below provides more accurate results.
    T s = a.x() * b.y();
    T t = b.x() * a.y();
    return static_cast<Int8>(t < s) - static_cast<Int8>(s < t);
}

template<typename T>
bool isDetPositive(const Vec2<T>& a, const Vec2<T>& b) {
    T s = a.x() * b.y();
    T t = b.x() * a.y();
    return t < s;
}

template<typename T>
bool isDetNegative(const Vec2<T>& a, const Vec2<T>& b) {
    T s = a.x() * b.y();
    T t = b.x() * a.y();
    return s < t;
}

template<typename T>
Int8 orientation(const Vec2<T>& a, const Vec2<T>& b, const Vec2<T>& c) {
    return detSign<T>(b - a, c - a);
}

template<typename T>
bool isOrientationPositive(const Vec2<T>& a, const Vec2<T>& b, const Vec2<T>& c) {
    return isDetPositive<T>(b - a, c - a);
}

template<typename T>
bool isOrientationNegative(const Vec2<T>& a, const Vec2<T>& b, const Vec2<T>& c) {
    return isDetNegative<T>(b - a, c - a);
}

// Insert the left endpoint and right endpoint of all segments into the
// event queue. If both endpoints have the same x-coord (i.e., the
// segment is vertical), the "left" endpoint is considered to be the
// one with the smaller y-coord.
//
// If both endpoints are equal (i.e., the segment is degenerate, reduced to a
// point), the order is irrelevant. Both Left and Right events will
// be retrieved and processed at the same time.
//
template<typename T>
void initializeEventQueue(InputData<T>& in, AlgorithmData<T>& alg) {
    VGC_ASSERT(alg.eventQueue.empty()); // priority_queue has no clear() method
    Int numSegments = in.segments.length();
    for (Int i = 0; i < numSegments; ++i) {
        const Segment2<T>& segment = in.segments.getUnchecked(i);
        alg.eventQueue.push(Event<T>{EventType::Left, segment.a(), i});
        alg.eventQueue.push(Event<T>{EventType::Right, segment.b(), i});
    }
}

// Initialize the list of segments intersecting the sweep line (the
// "sweep segments").
//
// Invariants:
//
// - Any segment in the eventQueue_ as Left event is not (and has
//   never been) in sweepSegments_.
//
// - Any segment in the eventQueue_ as Left event is also in the
//   eventQueue_ as Right event, but is not in the eventQueue_ as
//   Intersection event.
//
// - Any segment in the eventQueue_ as Intersection event is also in the
//   eventQueue_ as Right event, but is not in the eventQueue_ as
//   LefPoint event.
//
// - Any segment in sweepSegments_ is also in the eventQueue_
//   as exactly one Right event, and possibly one or several
//   Intersection events.
//
// - There are no duplicates in sweepSegments_.
//
template<typename T>
void initializeSweepSegments(AlgorithmData<T>& alg) {
    alg.sweepSegments.clear();
}

// Get the next event and all events sharing the same position. We call these
// the "sweep events".
//
// Note that our algorithm uses an std::priority_queue for our event queue,
// which uses a very efficient and cache-friendly vector-based heap, but this
// doesn't support checking whether an event is already in the event queue, or
// removing events from the queue. Therefore, this means that unlike usual
// descriptions of Bentley-Ottmann, we can end up with duplicates in the queue.
//
// [Wiki] and [Mount] use the approach of removing intersection events
// when segments become non-adjacent. See for example [Mount, p25]:
//
// > Observe that our algorithm is very careful about storing intersection
// > events only for adjacent elements in the priority queue. For example,
// > consider two segments s and s' that intersect at a segment p, such that,
// > when the two are initially added to the sweep-line status, they are
// > adjacent. Therefore, the intersection point p is added to event queue (see
// > Fig. 23). As intervening segments are seen between them, they successfully
// > become non-adjacent and then adjacent again. Because our algorithm is
// > careful about deleting intersections between non-adjacent entries in the
// > sweep-line status, the event p is repeated deleted and reinserted. If we
// > had not done this, we would have many duplicate events in the queue.
//
// [deBerg] uses the approach of not removing intersection events, but
// at least checking the content of the queue before insertion to avoid
// inserting twice the same intersection. See example [deBerg, p25, p27]:
//
// > We do not use a heap to implement the event queue, because we have to
// > be able to test whether a given event is already present in Q.
//
// > FindNewEvent(sl, sr, p):
// >   if: sl and sr intersect below the sweep line, or on it and to the
// >       right of the current event point p, and the intersection is not
// >       yet present as an event in Q.
// >   then: Insert the intersection point as an event into Q.
//
// Our approach is to not avoid duplicates and simply handle them when
// processing the event. We believe it does not change the asymptotic
// complexity of the algorithm (hypothesis: the total number of duplicates
// encountered during the algorithm is less than the number of segments), and
// that using a fast heap may in practice make the algorithm faster.
//
template<typename T>
Vec2<T> getNextEvent(AlgorithmData<T>& alg) {
    Event firstEvent = alg.eventQueue.top();
    alg.eventQueue.pop();
    Vec2<T> position = firstEvent.position;
    alg.sweepEvents.clear();
    alg.sweepEvents.append(firstEvent);
    while (!alg.eventQueue.empty() && alg.eventQueue.top().position == position) {
        alg.sweepEvents.append(alg.eventQueue.top());
        alg.eventQueue.pop();
    }

    VGC_DEBUG_TMP_INTER("----------------------------------");
    VGC_DEBUG_TMP_INTER("Sweep segments before event: {}", alg.sweepSegments);
    VGC_DEBUG_TMP_INTER("Handling position : {}", position);

    return position;
}

template<typename T>
struct PartitionedSweepEvents {
    using Iterator = typename core::Array<Event<T>>::iterator;

    Iterator it1;
    Iterator it2;
    Iterator it3;
    Iterator it4;

    // Left events
    core::ConstSpan<Event<T>> left() const {
        return {it1, it2};
    }

    // Right events
    core::ConstSpan<Event<T>> right() const {
        return {it2, it3};
    }

    // Intersection events
    core::ConstSpan<Event<T>> intersection() const {
        return {it3, it4};
    }
};

// Classify events into Left, Right, Intersection.
//
// This implementation works because the event type is part of the priority,
// with Left < Right < Intersection.
//
// Invariants:
// - res.left() does not contain duplicates
// - res.right() does not contain duplicates
//
// However, res.intersection() may contain duplicates, since two segments s1
// and s2 may become neighbors in alg.sweepSegments (adding their intersection
// to the queue), then not neighbor anymore, then neighbor again (re-adding
// their intersection to the queue). See [Mount, p26, Fig23].
//
template<typename T>
PartitionedSweepEvents<T> partitionSweepEvents(AlgorithmData<T>& alg) {
    PartitionedSweepEvents<T> res;
    res.it1 = alg.sweepEvents.begin();
    res.it4 = alg.sweepEvents.end();
    res.it2 = std::find_if(res.it1, res.it4, [](const Event<T>& event) {
        return event.type > EventType::Left; // find first event not Left
    });
    res.it3 = std::find_if(res.it2, res.it4, [](const Event<T>& event) {
        return event.type > EventType::Right; // find first event not Left or Right
    });

    VGC_DEBUG_TMP_INTER("Sweep events: {}", alg.sweepEvents);
    VGC_DEBUG_TMP_INTER("  Left events: {}", res.left());
    VGC_DEBUG_TMP_INTER("  Right events: {}", res.right());
    VGC_DEBUG_TMP_INTER("  Intersection events: {}", res.intersection());

    return res;
}

// Function object that determines whether a position is above or below a given
// segment index.
//
// Typically, we prefer to use a lambda for these things, but in this case we
// cannot because it is used for equal_range which will call both `i < p` and
// `p < i`, and `i` and `p` are of different types, so the function object
// needs to have two call operators.
//
template<typename T>
struct ComparePositionWithSegment {
    core::Array<Segment2<T>>& segments;

    // Returns whether seg[i] < p: "is p above the segment?"
    bool operator()(SegmentIndex i, const Vec2<T>& p) {
        const Segment2<T>& seg = segments.getUnchecked(i);
        return isOrientationPositive<T>(seg.a(), seg.b(), p);
    }

    // Returns whether p < seg[i]: "is p below the segment?"
    bool operator()(const Vec2<T>& p, SegmentIndex i) {
        const Segment2<T>& seg = segments.getUnchecked(i);
        return isOrientationNegative<T>(seg.a(), seg.b(), p);
    }
};

struct PartitionedSweepSegments {
    using Iterator = core::Array<SegmentIndex>::iterator;

    Iterator it1;
    Iterator it2;
    Iterator it3;
    Iterator it4;

    // Segments in the sweep line that are below the event position
    core::ConstSpan<SegmentIndex> below() const {
        return {it1, it2};
    }

    // Segments in the sweep line that contain the event position
    core::ConstSpan<SegmentIndex> contain() const {
        return {it2, it3};
    }

    // Segments in the sweep line that are above the event position
    core::ConstSpan<SegmentIndex> above() const {
        return {it3, it4};
    }
};

// Find all segments in alg.sweepSegments that contain `position`
// either in their interior or as an endpoint. Some of these may
// already be in the events above (e.g., if there is an
// Intersection event), but some may not, e.g., if there is a
// T-junction at the `position`.
//
// This basically partition alg.sweepSegments into three spans:
// - the segments that are below the position
// - the segment that contain the position
// - the segment that are above the position
//
// Note that all segments in interEvents should in theory be found
// in alg.sweepSegments as containing the position, but in practice
// may not due to numerical errors. More precisely, after computing
// the intersection p between two, because of rounding errors, it
// is possible that p is not considered exactly on each of the
// segments.
//
template<typename T>
PartitionedSweepSegments partitionSweepSegments(
    InputData<T>& in,
    AlgorithmData<T>& alg,
    const Vec2<T>& position,
    const PartitionedSweepEvents<T>& pEvents) {

    ComparePositionWithSegment<T> comp{in.segments};
    PartitionedSweepSegments res;
    res.it1 = alg.sweepSegments.begin();
    res.it4 = alg.sweepSegments.end();
    std::tie(res.it2, res.it3) = std::equal_range(res.it1, res.it4, position, comp);

    // In theory, the segments in pEvents.intersection() are supposed to all be
    // in res.contain(), but in practice may not be due to numerical errors.
    //
    // We fix this here by extending the range of segments to make sure that it
    // includes all intersection events. At a side effect, this may add other
    // segments, but this is fine since these segments where also within
    // numerical errors of containing the position.
    //
    // Note that the segments in pEvents.right() are guaranteed to be in
    // res.contain(), since their right endpoint is guaranteed to be exactly
    // equal to position. Unless the segments were not truly "partitioned" as
    // per the definition of equal_range, see:
    //
    // https://en.cppreference.com/w/cpp/algorithm/equal_range
    // https://en.cppreference.com/w/cpp/algorithm#Requirements
    //
    // TODO: check in debug build whether the segments are partitioned via
    // std::is_partitioned()? Note that this would make the algorithm
    // not O((n+k)log(n)) anymore in the worst case.
    //
    // Note: the code below makes the algorithm O(n²) if all segments intersect
    // at the same point. This should be rare, but we could make it n log(n)
    // again by first sorting pEvents.intersection() and res.contain() by
    // segment index and using std::set_difference. But this is going to make
    // the algorithm slower in the typical case when there are just 2
    // intersection events. A solution might be to use sorting only if we
    // detect intersectionEvents.length() > SomeThreshold.
    //
    for (const Event<T>& event : pEvents.intersection()) {
        if (!res.contain().contains(event.segmentIndex)) {
            // TODO: Make this long-term log. It's very unlikely but we might
            // be interested to know about it if it does happen, for example
            // to add it in a test suite.
            VGC_DEBUG_TMP_INTER(
                "Segment from event {} was not partitioned as containing position {}.",
                event,
                position);
            // TODO: extend search from neighbor instead of using linear-time find
            auto it = alg.sweepSegments.find(event.segmentIndex);
            if (it != alg.sweepSegments.end()) {
                if (it >= res.it3) {
                    res.it3 = it + 1;
                }
                else if (it < res.it2) {
                    res.it2 = it;
                }
            }
            else {
                // TODO: Make this long-term warning in release, and throw in debug.
                // That's a serious bug in the algorithm, but hard to prove that
                // it cannot happen, due to numerical errors.
                VGC_DEBUG_TMP_INTER(
                    "Segment from event {} not found in sweep segments.", event);
            }
        }
    }

    VGC_DEBUG_TMP_INTER("Sweep segments below position:      {}", res.below());
    VGC_DEBUG_TMP_INTER("Sweep segments containing position: {}", res.contain());
    VGC_DEBUG_TMP_INTER("Sweep segments above position:      {}", res.above());

    return res;
}

// Find which segments are outgoing at the position. These will
// be used as replacement for the current containSegments.
//
template<typename T>
void computeOutgoingSegments(
    InputData<T>& in,
    AlgorithmData<T>& alg,
    const Vec2<T>& position,
    const PartitionedSweepEvents<T>& pEvents,
    const PartitionedSweepSegments& pSegments) {

    // It is guaranteed to have no duplicates since alg.sweepSegments does
    // not have duplicates (hence its subspan containSegments does not
    // have duplicate either) and all the segments in pEvents.left() have
    // never been added to the event queue yet.
    //
    alg.outgoingSegments.clear();
    for (SegmentIndex i : pSegments.contain()) {
        const Segment2<T>& segment = in.segments.getUnchecked(i);
        if (segment.b() != position) {
            alg.outgoingSegments.append(i);
        }
        else {
            // This means that the segment is a Right event
            // and we will therefore remove it from alg.sweepSegments
        }
    }
    for (const Event<T>& event : pEvents.left()) {
        SegmentIndex i = event.segmentIndex;
        const Segment2<T>& segment = in.segments.getUnchecked(i);
        if (!segment.isDegenerate()) {
            alg.outgoingSegments.append(i);
        }
        else {
            // This means the segment was reduced to a point
            // and was both a Left
            // and Right event. We never add it to alg.sweepSegments.
            //
            // TODO: correcly report intersections between degenerate
            // segments and other segments, including when two degenerate
            // segments are equal, we should report them as intersecting.
        }
    }

    // Sort outgoing segments by increasing slope, which correspond to their
    // y-order on the sweep line after it moves just after the current
    // position.
    //
    core::sort(
        alg.outgoingSegments,
        [&slopes = in.segmentSlopes](SegmentIndex i1, SegmentIndex i2) {
            return slopes.getUnchecked(i1) < slopes.getUnchecked(i2);
        });
}

// Compute the intersection between the two given segments, and add it to the
// event queue if this intersection is to the right of the sweep line, or
// exactly on the sweep line and above the event position.
//
template<typename T>
void findNewIntersection(
    InputData<T>& in,
    AlgorithmData<T>& alg,
    const Vec2<T>& position,
    SegmentIndex i1,
    SegmentIndex i2) {

    const Segment2<T>& s1 = in.segments[i1];
    const Segment2<T>& s2 = in.segments[i2];
    SegmentIntersection2<T> inter = s1.intersect(s2);
    if (inter.type() == SegmentIntersectionType::Point) {
        if (inter.p() > position) {
            alg.eventQueue.push(Event<T>{EventType::Intersection, inter.p(), i1});
            alg.eventQueue.push(Event<T>{EventType::Intersection, inter.p(), i2});
        }
    }
    // TODO: SegmentIntersectionType::Segment
}

// Compute intersection between segments that have just become neighbors in
// sweepSegments (as a result of sweepSegments being modified), and if they
// intersect to the right of the sweep line, insert them in the event queue.
//
template<typename T>
void findNewIntersections(
    InputData<T>& in,
    AlgorithmData<T>& alg,
    const Vec2<T>& position,
    core::Array<SegmentIndex>::iterator it) {

    Int numSegmentsAdded = alg.outgoingSegments.length();
    if (numSegmentsAdded == 0) {
        // No segments were added to sweepSegments.
        // Which means that some segments were removed (EventType::Right).
        // If there are segments on both side of the event position,
        // this means that they newly became neighbors.
        if (it != alg.sweepSegments.begin() && it != alg.sweepSegments.end()) {
            findNewIntersection(in, alg, position, *(it - 1), *it);
        }
    }
    else {
        // Segments were added to sweepSegments (including segments that
        // were already in sweepSegments, but removed and re-added after sorting).
        // `it` points to the first added segment.
        if (it != alg.sweepSegments.begin()) {
            findNewIntersection(in, alg, position, *(it - 1), *it);
        }
        it += numSegmentsAdded;
        if (it != alg.sweepSegments.end()) {
            findNewIntersection(in, alg, position, *(it - 1), *it);
        }
    }
}

template<typename T>
void processNextEvent(InputData<T>& in, AlgorithmData<T>& alg, OutputData<T>& out) {

    // Pop the next event from the event queue, as well as all subsequent
    // events sharing the same position. Store them in alg.sweepEvents.
    Vec2<T> position = getNextEvent(alg);

    // Partition the events into Left, Right, and Intersection events.
    PartitionedSweepEvents pEvents = partitionSweepEvents(alg);

    // Partition the sweep segments into those that are below, above, or
    // contain the event position. We guarantee that segments corresponding
    // to Intersection events are within pSegments.contain().
    PartitionedSweepSegments pSegments =
        partitionSweepSegments(in, alg, position, pEvents);

    // Report intersections. This includes:
    // - Cases where two segments meet at their endpoint:
    //   - 2+ Left events, or
    //   - 2+ Right events, or
    //   - 1+ Left and 1+ Left events
    // - Cases where two segments intersect in their interior
    //   - 2+ Intersection events, and
    //   - 2+ segments in pSegments.contain()
    // - Cases where a segment ends in the interior of another segment (T-junction):
    //   - 1+ Left events or 1+ Right events, and
    //   - 2+ segments in pSegments.contain()
    //
    // Note that the segments in pEvents.right() and pEvents.intersection()
    // are all in pSegments.contain(), but the segments in pEvents.left()
    // are not, since they have not yet been added to the sweep segments.
    //
    if (pSegments.contain().length() + pEvents.left().length() >= 2) {
        // TODO: set proper value for contributions
        core::Array<PointIntersectionInfo<T>> infos;
        out.pointIntersections.append({position, infos});
    }

    // Compute which segments are outgoing at the given position
    computeOutgoingSegments(in, alg, position, pEvents, pSegments);

    // Remove ingoing segments and add outgoing segments.
    // This invalidates previous iterators and spans stored in pSegments.
    // The new iterator `it` refers to either:
    // - the first inserted segment if any, otherwise
    // - the segment (or sentinel) just after the removed segments
    auto it = alg.sweepSegments.erase(pSegments.it2, pSegments.it3);
    it = alg.sweepSegments.insert(it, alg.outgoingSegments);

    // Add intersections between newly-neighbor segments to the event queue.
    findNewIntersections(in, alg, position, it);

    VGC_DEBUG_TMP_INTER("Sweep segments after event: {}", alg.sweepSegments);
}

template<typename T>
void computeIntersections(InputData<T>& in, AlgorithmData<T>& alg, OutputData<T>& out) {
    initializeEventQueue(in, alg);
    initializeSweepSegments(alg);
    while (!alg.eventQueue.empty()) {
        processNextEvent(in, alg, out);
    }
}

} // namespace detail::segmentintersector2

template<typename T>
void SegmentIntersector2<T>::addSegment(const Vec2<T>& a, const Vec2<T>& b) {
    detail::segmentintersector2::addSegment(input_, a, b);
}

template<typename T>
template<typename Range, typename UnaryOp, VGC_REQUIRES_DEF(core::isInputRange<Range>)>
void SegmentIntersector2<T>::addPolyline(const Range& range, UnaryOp op) {
    detail::segmentintersector2::addPolyline(input_, range, op);
}

template<typename T>
void SegmentIntersector2<T>::computeIntersections() {
    detail::segmentintersector2::computeIntersections(input_, algorithm_, output_);
}

using SegmentIntersector2f = SegmentIntersector2<float>;
extern template class SegmentIntersector2<float>;

using SegmentIntersector2d = SegmentIntersector2<double>;
extern template class SegmentIntersector2<double>;

} // namespace vgc::geometry

template<>
struct fmt::formatter<vgc::geometry::detail::segmentintersector2::EventType>
    : fmt::formatter<std::string_view> {

    using EventType = vgc::geometry::detail::segmentintersector2::EventType;

    template<typename FormatContext>
    auto format(const EventType& eventType, FormatContext& ctx) {
        switch (eventType) {
        case EventType::Left:
            return format_to(ctx.out(), "Left");
        case EventType::Right:
            return format_to(ctx.out(), "Right");
        case EventType::Intersection:
            return format_to(ctx.out(), "Intersection");
        }
        return ctx.out();
    }
};

template<typename Scalar>
struct fmt::formatter<vgc::geometry::detail::segmentintersector2::Event<Scalar>>
    : fmt::formatter<std::string_view> {

    using EventType = vgc::geometry::detail::segmentintersector2::EventType;
    using Event = vgc::geometry::detail::segmentintersector2::Event<Scalar>;

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

#endif // VGC_GEOMETRY_SEGMENTINTERSECTOR2_H