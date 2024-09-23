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
#include <vgc/geometry/logcategories.h>
#include <vgc/geometry/segment2.h>
#include <vgc/geometry/vec2.h>

#define VGC_DEBUG_TMP_INTER VGC_DEBUG_TMP
//#define VGC_DEBUG_TMP_INTER(...)

namespace vgc::geometry {

// Note: we use the namespace `segmentintersector2` for defining public classes
// and aliases that are only relevant to SegmentIntersector2<T>. The reason for
// defining them at namespace scope rather than nested in
// SegmentIntersector2<T> is to make them accessible in code that doesn't have
// to know (or cannot know) the template parameter `T`.
//
// For example, it would be impossible to specialize fmt::formatter for
// PointIntersection if the latter was defined as a nested class of
// SegmentIntersector2<T> rather than at namespace scope, because nested
// classes of class templates are non-deductible in partial template
// specializations.
//
// See: https://open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0293r0.pdf
//
namespace segmentintersector2 {

using SegmentIndex = Int;
using PointIntersectionIndex = Int;
using SegmentIndexPair = std::pair<SegmentIndex, SegmentIndex>;

/// When two or more segments intersect at a point, then for each involved
/// segment we store its corresponding intersection parameter.
///
template<typename T>
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
template<typename T>
struct PointIntersection {
    Vec2<T> position;
    core::Array<PointIntersectionInfo<T>> infos;
};

namespace detail {

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

} // namespace detail

} // namespace segmentintersector2

/// \class SegmentIntersector2
/// \brief Computes all intersections between a set of line segments.
///
template<typename T>
class VGC_GEOMETRY_API SegmentIntersector2 {
public:
    using ScalarType = T;
    static constexpr Int dimension = 2;

    using SegmentIndex = segmentintersector2::SegmentIndex;
    using SegmentIndexPair = segmentintersector2::SegmentIndexPair;
    using PointIntersectionIndex = segmentintersector2::PointIntersectionIndex;

    using PointIntersectionInfo = segmentintersector2::PointIntersectionInfo<T>;
    using PointIntersection = segmentintersector2::PointIntersection<T>;

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
    segmentintersector2::detail::InputData<T> input_;
    segmentintersector2::detail::AlgorithmData<T> algorithm_;
    segmentintersector2::detail::OutputData<T> output_;
};

namespace segmentintersector2::detail {

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

    // All segments that are intersecting at the current event.
    //
    core::Array<PointIntersectionInfo<T>> infos;

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

// Extend pSegments.contain() to ensures that it contains all
// segments in the given events. Indeed:
//
// 1. The segments in pEvents.intersection() are supposed to all be
// in res.contain(), but in practice, they almost never are due to
// numerical errors. Indeed, with p = intersection(seg1, seg2),
// then most often orientation(seg1.a(), seg1.b(), p) != 0.
//
// 2. The segments in pEvents.right() are also supposed to all be in
// res.contain(), and in most cases they indeed are, since orientation(seg.a(),
// seg.b(), seg.b()) == 0. However, due to numerical errors, it might be
// possible (i.e., we haven't proven otherwise) that sweepEvents is not
// perfectly "partitioned" according to the geometric predidate "is `position`
// above or below the given segment", as required by the `equal_range` binary
// search algorithm:
//
// https://en.cppreference.com/w/cpp/algorithm/equal_range
// https://en.cppreference.com/w/cpp/algorithm#Requirements
//
// This means that `equal_range` could miss some segments, for example if the
// values of `orientation(seg.a(), seg.b(), p)` along the sweep segments are:
//
//
//                      middle   none of those
//                      index    are evaluated
//                         v  vvvvvvvvvvvvvvvvvvvvv
// [-1, -1, -1, -1, 0, 0, +1, 0, +1, +1, +1, +1, +1]
//                  ^^^^      ^
//              result of     segment in pEvents.right(),
//              equal_range   missed by equal_range
//
// In theory, the segments in pEvents.intersection() are supposed to all be
// in res.contain(), but in practice, they are almost never due to
// numerical errors.
//
// Note: this step makes the algorithm O(n²) if all segments intersect at the
// same point. This should be rare, but we could make it (n+k)log(n) again by
// first sorting pEvents.intersection(), pEvents.right(), and res.contain() by
// segment index and using std::set_difference. But this is going to make the
// algorithm slower in the typical case when there at most 2 intersection
// events, or 2 left/right events. A solution might be to use sorting only if
// we detect pEvents.length() > someThreshold.
//
template<typename T>
void extendContainSegments(
    PartitionedSweepSegments& pSegments,
    core::ConstSpan<Event<T>> events) {

    for (const Event<T>& event : events) {
        if (!pSegments.contain().contains(event.segmentIndex)) {
            auto it2 = pSegments.it2;
            auto it3 = pSegments.it3;
            bool found = false;
            // While we haven't searched all segments [it1, it4)
            while (it2 != pSegments.it1 || it3 != pSegments.it4) {
                // Extend the search [it2, it3) by one segment to the left
                if (it2 != pSegments.it1) {
                    --it2;
                    if (*it2 == event.segmentIndex) {
                        pSegments.it2 = it2;
                        found = true;
                        break;
                    }
                }
                // Extend the search [it2, it3) by one segment to the right
                if (it3 != pSegments.it4) {
                    Int segmentIndex = *it3; // before increment as range is semi-open
                    ++it3;
                    if (segmentIndex == event.segmentIndex) {
                        pSegments.it3 = it3;
                        found = true;
                        break;
                    }
                }
            }
            if (!found) {
                // We believe this to be impossible, even accounting for
                // numerical errors. But since the proof is neither obvious nor
                // local, we don't throw and consider this a recoverable error.
                VGC_ERROR(
                    LogVgcGeometry,
                    "Segment from event {} not found in sweep segments.",
                    event);
            }
        }
    }
}

// Find all sweep segments that contain `position` (either in their interior or
// as an endpoint), via a binary search on sweepSegments using the geometric
// predicate "is `position` above or below the segment".
//
// In theory, all these segments are consecutive in sweepSegments, so the
// output is a partition of sweepSegments into three spans:
// - the segments that are below the position (res.below())
// - the segment that contain the position (res.contain())
// - the segment that are above the position (res.above())
//
// Also, in theory, all segments in pEvents.intersection() and pEvents.right()
// contain `position`, so they should be in res.contain() after the binary
// search.
//
// In practice, due to numerical errors, the binary search alone may not
// guarantee all the above. Therefore, it is followed by an additional step
// that extends res.contain() to at least enforce that it is a superset of
// pEvents.intersection() and pEvents.right(). This allows to prove that even
// with numerical errors, we have the following basic guarantees:
//
// 1. The algorithm terminates
//
// 2. No segment past the sweep line ever stays in sweepSegments.
//
// 3. All found intersections between segments are appropriately reported. This
// relies on the fact that we use an intersection algorithm that guarantees
// intersectionPoint <= segment.b(). Therefore Intersection events are always
// popped from the event queue before (or at the same time) the Right events of
// the intersecting segments.
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
    extendContainSegments(res, pEvents.intersection());
    extendContainSegments(res, pEvents.right());

    VGC_DEBUG_TMP_INTER("Sweep segments below position:      {}", res.below());
    VGC_DEBUG_TMP_INTER("Sweep segments containing position: {}", res.contain());
    VGC_DEBUG_TMP_INTER("Sweep segments above position:      {}", res.above());

    return res;
}

// Returns `1 - t` if the segment is reversed, otherwise returns `t`.
template<typename T>
T maybeReverseParam(InputData<T>& in, SegmentIndex i, core::TypeIdentity<T> t) {
    return in.isReversed[i] ? (1 - t) : t;
}

// Computes the parameter in [0, 1] that corresponds to the given
// position along the given segment
template<typename T>
T computeParam(InputData<T>& in, SegmentIndex i, const Vec2<T>& position) {
    const Segment2<T>& segment = in.segments[i];
    T dx = segment.bx() - segment.ax(); // >= 0
    T dy = segment.by() - segment.ay();
    if (std::abs(dy) > dx) {
        return maybeReverseParam(in, i, (position.y() - segment.ay()) / dy);
    }
    else if (dx > 0) {
        return maybeReverseParam(in, i, (position.x() - segment.ax()) / dx);
    }
    else {
        return 0; // guaranteed not reversed
    }
}

struct CompareSegmentIndexWithSecondInPair {
    bool operator()(SegmentIndex i, const SegmentIndexPair& p) {
        return i < p.second;
    }
    bool operator()(const SegmentIndexPair& p, SegmentIndex i) {
        return p.second < i;
    }
};

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
template<typename T>
void reportIntersections(
    InputData<T>& in,
    AlgorithmData<T>& alg,
    OutputData<T>& out,
    const Vec2<T>& position,
    const PartitionedSweepEvents<T>& pEvents,
    const PartitionedSweepSegments& pSegments) {

    // Retrieve the list of segments that intersect at the event position. For
    // now we initialize the param to 0 as there might no need to compute it
    // due to fast returns when n <= 2.
    //
    Int interIndex = out.pointIntersections.length();
    alg.infos.clear();
    for (SegmentIndex i : pSegments.contain()) {
        alg.infos.append(PointIntersectionInfo<T>{interIndex, i, 0});
    }
    for (const Event<T>& event : pEvents.left()) {
        alg.infos.append(PointIntersectionInfo<T>{interIndex, event.segmentIndex, 0});
    }

    // There is no intersection if there is only one segment at the event
    // position. This means we are at an isolated left endpoint or right
    // endpoint of a segment.
    //
    if (alg.infos.length() <= 1) {
        return;
    }

    // Discard the intersection if it only involve two segments, and they are
    // consecutive segments of the same polyline, intersecting at their
    // expected shared endpoint.
    //
    // Note that if there is another segment intersecting there as well,
    // we do want to report the intersection.
    //
    if (alg.infos.length() == 2) {
        // TODO: what if consecutive segments overlap along a subsegment?
        SegmentIndex i1 = alg.infos.first().segmentIndex;
        SegmentIndex i2 = alg.infos.last().segmentIndex;
        if (i2 < i1) {
            std::swap(i1, i2);
        }
        if (i2 == i1 + 1) {
            auto it = std::upper_bound(
                in.polylines.begin(),
                in.polylines.end(),
                i2,
                CompareSegmentIndexWithSecondInPair{});
            if (it != in.polylines.end()) {
                // => there is a pair (j1, j2) with i2 < j2, and
                //    `it` points to the first of such pair.
                SegmentIndex j1 = it->first;
                if (j1 <= i1) {
                    // This means j1 <= i1 < i2 (= i1+1) < j2.
                    // So this intersection is part of the polyline.
                    return;
                }
            }
        }
    }

    // Compute the parameters and report the intersection
    for (PointIntersectionInfo<T>& info : alg.infos) {
        info.param = computeParam(in, info.segmentIndex, position);
    }
    out.pointIntersections.append({position, alg.infos});
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

    // Report intersections
    reportIntersections(in, alg, out, position, pEvents, pSegments);

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
    VGC_DEBUG_TMP_INTER("Segments: {}", in.segments);
    VGC_DEBUG_TMP_INTER("isReversed: {}", in.isReversed);
    VGC_DEBUG_TMP_INTER("segmentSlopes: {}", in.segmentSlopes);
    initializeEventQueue(in, alg);
    initializeSweepSegments(alg);
    while (!alg.eventQueue.empty()) {
        processNextEvent(in, alg, out);
    }
}

// The following makes specialization of fmt::formatter possible for inner
// types of template class. Indeed, the following wouldn't work:
//
//   template<typename T>
//   struct fmt::formatter<typename Outer<T>::Inner> {...}
//
// See Clang -Wunusable-partial-specialization:
// > class template partial specialization contains
// > a template parameter that cannot be deduced
//
template<typename T, typename SFINAE = void>
struct IsPointIntersectionInfo : std::false_type {};

} // namespace segmentintersector2::detail

template<typename T>
void SegmentIntersector2<T>::addSegment(const Vec2<T>& a, const Vec2<T>& b) {
    segmentintersector2::detail::addSegment(input_, a, b);
}

template<typename T>
template<typename Range, typename UnaryOp, VGC_REQUIRES_DEF(core::isInputRange<Range>)>
void SegmentIntersector2<T>::addPolyline(const Range& range, UnaryOp op) {
    segmentintersector2::detail::addPolyline(input_, range, op);
}

template<typename T>
void SegmentIntersector2<T>::computeIntersections() {
    segmentintersector2::detail::computeIntersections(input_, algorithm_, output_);
}

using SegmentIntersector2f = SegmentIntersector2<float>;
extern template class SegmentIntersector2<float>;

using SegmentIntersector2d = SegmentIntersector2<double>;
extern template class SegmentIntersector2<double>;

} // namespace vgc::geometry

template<typename T>
struct fmt::formatter<vgc::geometry::segmentintersector2::PointIntersectionInfo<T>>
    : fmt::formatter<std::string_view> {

    using PointIntersectionInfo =
        vgc::geometry::segmentintersector2::PointIntersectionInfo<T>;

    template<typename FormatContext>
    auto format(const PointIntersectionInfo& info, FormatContext& ctx) {
        return format_to(
            ctx.out(),
            "{{pointIntersectionIndex={}, segmentIndex={}, param={}}}",
            info.pointIntersectionIndex,
            info.segmentIndex,
            info.param);
    }
};

template<typename T>
struct fmt::formatter<vgc::geometry::segmentintersector2::PointIntersection<T>>
    : fmt::formatter<std::string_view> {

    using PointIntersection = vgc::geometry::segmentintersector2::PointIntersection<T>;

    template<typename FormatContext>
    auto format(const PointIntersection& inter, FormatContext& ctx) {
        return format_to(
            ctx.out(), "{{position={}, infos={}}}", inter.position, inter.infos);
    }
};

template<>
struct fmt::formatter<vgc::geometry::segmentintersector2::detail::EventType>
    : fmt::formatter<std::string_view> {

    using EventType = vgc::geometry::segmentintersector2::detail::EventType;

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

template<typename T>
struct fmt::formatter<vgc::geometry::segmentintersector2::detail::Event<T>>
    : fmt::formatter<std::string_view> {

    using Event = vgc::geometry::segmentintersector2::detail::Event<T>;

    template<typename FormatContext>
    auto format(const Event& event, FormatContext& ctx) {
        return format_to(
            ctx.out(),
            "{{type={}, position={}, segmentIndex={}}}",
            event.type,
            event.position,
            event.segmentIndex);
    }
};

template<>
struct fmt::formatter<
    vgc::geometry::segmentintersector2::detail::PartitionedSweepSegments>
    : fmt::formatter<std::string_view> {

    using PartitionedSweepSegments =
        vgc::geometry::segmentintersector2::detail::PartitionedSweepSegments;

    template<typename FormatContext>
    auto format(const PartitionedSweepSegments& pSegments, FormatContext& ctx) {
        return format_to(
            ctx.out(),
            "({{iterators=[0, {}, {}, {}], below={}, contain={}, above={}}}",
            pSegments.it2 - pSegments.it1,
            pSegments.it3 - pSegments.it1,
            pSegments.it4 - pSegments.it1,
            pSegments.below(),
            pSegments.contain(),
            pSegments.above());
    }
};

#endif // VGC_GEOMETRY_SEGMENTINTERSECTOR2_H
