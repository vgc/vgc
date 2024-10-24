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

namespace vgc::geometry {

// Note: we use the namespace `segmentintersector2` for defining public classes
// and aliases that are only relevant to SegmentIntersector2<T>. The reason for
// defining them at namespace scope rather than nested in
// SegmentIntersector2<T> is to make them accessible in code that doesn't have
// to know (or cannot know) the template parameter `T`.
//
// For example, it would be impossible to specialize fmt::formatter for
// Vertex if the latter was defined as a nested class of
// SegmentIntersector2<T> rather than at namespace scope, because nested
// classes of class templates are non-deducible in partial template
// specializations.
//
// See: https://open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0293r0.pdf
//
namespace segmentintersector2 {

using PolylineIndex = Int;
using SegmentIndex = Int;
using VertexIndex = Int;
using EdgeIndex = Int;

/// For each vertex, we store a list of `VertexSegment` objects, one per input
/// segment that is incident to the vertex or intersects at the vertex. Each
/// `VertexSegment` object stores the linear parametric value that corresponds
/// to the position of the vertex along the segment.
///
template<typename T>
class VertexSegment {
public:
    /// Constructs an `VertexSegment` with zero-initialized
    /// `vertexIndex()`, `segmentIndex()`, and `parameter()`.
    ///
    VertexSegment() {
    }

    /// Contructs an `VertexSegment`.
    ///
    VertexSegment(VertexIndex vertexIndex, SegmentIndex segmentIndex, T parameter)
        : vertexIndex_(vertexIndex)
        , segmentIndex_(segmentIndex)
        , parameter_(parameter) {
    }

    /// Returns the index of the `Vertex` this `VertexSegment` refers to.
    ///
    VertexIndex vertexIndex() const {
        return vertexIndex_;
    }

    /// Modifies `vertexIndex()`.
    ///
    void setVertexIndex(VertexIndex vertexIndex) {
        vertexIndex_ = vertexIndex;
    }

    /// Returns the index of the segment that intersects at the `Vertex`.
    ///
    SegmentIndex segmentIndex() const {
        return segmentIndex_;
    }

    /// Modifies `segmentIndex()`.
    ///
    void setSegmentIndex(SegmentIndex segmentIndex) {
        segmentIndex_ = segmentIndex;
    }

    /// Returns the linear parametric value corresponding to where the `Vertex`
    /// is located along the segment.
    ///
    T parameter() const {
        return parameter_;
    }

    /// Modifies `parameter()`.
    ///
    void setParameter(T parameter) {
        parameter_ = parameter;
    }

private:
    VertexIndex vertexIndex_;
    SegmentIndex segmentIndex_;
    T parameter_;
};

/// A `Vertex` corresponds either to the endpoint of a segment or an intersection
/// point between two segments.
///
// TODO: Minimize memory allocations via SmallArray<2, VertexSegment>?
// Indeed, most vertices are expected to only involve 2 segments or fewer.
// Alternatively, we could use a custom allocator or pairs of indices
// into another array.
//
template<typename T>
class Vertex {
public:
    /// Constructs a `Vertex` with a default constructed `position` and
    /// `segments()`.
    ///
    Vertex() noexcept {
    }

    /// Constructs a `Vertex` with the given `position` and empty
    /// `segments()`.
    ///
    explicit Vertex(const Vec2<T>& position) noexcept
        : position_(position) {
    }

    /// Constructs a `Vertex` with the given `position` and `segments`.
    ///
    Vertex(const Vec2<T>& position, const core::Array<VertexSegment<T>>& segments)
        : position_(position)
        , segments_(segments) {
    }

    /// Returns the 2D position of this vertex.
    ///
    Vec2<T> position() const {
        return position_;
    }

    /// Modifies the 2D position of this vertex.
    ///
    void setPosition(const Vec2<T>& position) {
        position_ = position;
    }

    /// Returns the list of all segments that have this vertex as endpoint or
    /// that intersect another segment at this vertex.
    ///
    const core::Array<VertexSegment<T>>& segments() const {
        return segments_;
    }

    /// Sets the `segments` of this point intersection.
    ///
    void setSegments(core::Array<VertexSegment<T>> segments) {
        segments_ = std::move(segments);
    }

    /// Adds a `VertexSegment` to this point intersection.
    ///
    void addSegment(const VertexSegment<T>& segment) {
        segments_.append(segment);
    }

private:
    Vec2<T> position_;
    core::Array<VertexSegment<T>> segments_;
};

/// When two or more segments overlap along a shared subsegment, then for each
/// involved segment we store the corresponding parametric values of the edge
/// along the segment.
///
template<typename T>
class EdgeSegment {
public:
    /// Constructs an `EdgeSegment` with zero-initialized
    /// `edgeIndex()`, `segmentIndex()`, `parameter1(), and `parameter2()`.
    ///
    EdgeSegment() {
    }

    /// Contructs an `EdgeSegment`
    ///
    EdgeSegment(
        EdgeIndex edgeIndex,
        SegmentIndex segmentIndex,
        T parameter1,
        T parameter2)

        : edgeIndex_(edgeIndex)
        , segmentIndex_(segmentIndex)
        , parameter1_(parameter1)
        , parameter2_(parameter2) {
    }

    /// Returns the index of the `Edge` this `EdgeSegment` refers to.
    ///
    EdgeIndex edgeIndex() const {
        return edgeIndex_;
    }

    /// Modifies `edgeIndex()`.
    ///
    void setEdgeIndex(EdgeIndex edgeIndex) {
        edgeIndex_ = edgeIndex;
    }

    /// Returns the index of the segment that overlap the `Edge`.
    ///
    SegmentIndex segmentIndex() const {
        return segmentIndex_;
    }

    /// Modifies `segmentIndex()`.
    ///
    void setSegmentIndex(SegmentIndex segmentIndex) {
        segmentIndex_ = segmentIndex;
    }

    /// Returns the linear parametric value corresponding to where the `Edge`
    /// starts along the segment.
    ///
    T parameter1() const {
        return parameter1_;
    }

    /// Modifies `parameter1()`.
    ///
    void setParameter1(T parameter1) {
        parameter1_ = parameter1;
    }

    /// Returns the linear parametric value corresponding to where the `Edge`
    /// ends along the segment.
    ///
    T parameter2() const {
        return parameter2_;
    }

    /// Modifies `parameter2()`.
    ///
    void setParameter2(T parameter2) {
        parameter2_ = parameter2;
    }

private:
    VertexIndex edgeIndex_;
    SegmentIndex segmentIndex_;
    T parameter1_;
    T parameter2_;
};

/// After intersections are computed, the input segments are decomposed into
/// interior-disjoint subsegments called `Edge`, that start and end at a
/// `Vertex`.
///
template<typename T>
class Edge {
public:
    /// Constructs an `Edge` with a default constructed `subsegment` and
    /// `segments()`.
    ///
    Edge() noexcept {
    }

    /// Constructs a `Edge` with the given `subsegment` and empty `segments()`.
    ///
    explicit Edge(const Segment2<T>& subsegment) noexcept
        : subsegment_(subsegment) {
    }

    /// Constructs a `Edge` with the given `subsegment` and `segments`.
    ///
    Edge(const Segment2<T>& subsegment, const core::Array<EdgeSegment<T>>& segments)
        : subsegment_(subsegment)
        , segments_(segments) {
    }

    /// Returns the geometry of this edge, that is, a shared subsegment between
    /// overlapping input segments.
    ///
    Segment2<T> subsegment() const {
        return subsegment_;
    }

    /// Modifies the shared subsegment of this point intersection.
    ///
    void setSubsegment(const Segment2<T>& subsegment) {
        subsegment_ = subsegment;
    }

    /// Returns information about the segments that overlap this edge.
    ///
    const core::Array<EdgeSegment<T>>& segments() const {
        return segments_;
    }

    /// Sets the `segments()` of this edge.
    ///
    void setSegments(core::Array<EdgeSegment<T>> segments) {
        segments_ = std::move(segments);
    }

    /// Adds an `EdgeSegment` to this edge.
    ///
    void addSegment(const EdgeSegment<T>& segment) {
        segments_.append(segment);
    }

private:
    Segment2<T> subsegment_;
    core::Array<EdgeSegment<T>> segments_;
};

/// Represent a range of continuous segment indices.
///
template<typename T>
class SegmentIndexRange {
public:
    /// Creates a `SegmentIndexRange`.
    ///
    SegmentIndexRange(SegmentIndex first, SegmentIndex pastTheLast)
        : first_(first)
        , pastTheLast_(pastTheLast) {
    }

    /// Returns the first index (included) of this range of indices.
    ///
    SegmentIndex first() const {
        return first_;
    }

    /// Returns the last index (excluded) of this range of indices.
    ///
    SegmentIndex pastTheLast() const {
        return pastTheLast_;
    }

    /// Returns whether this range of indices is empty.
    ///
    bool isEmpty() const {
        return pastTheLast_ > first_;
    }

    // TODO: provide proxy iterators begin()/end() to make this iterable?

private:
    SegmentIndex first_;
    SegmentIndex pastTheLast_;
};

namespace detail {

enum class EventType : UInt8 {
    Left = 0,
    Right = 1,
    Intersection = 2,
    Degenerate = 3,
};

template<typename T>
struct InputData;

template<typename T>
struct AlgorithmData;

template<typename T>
struct OutputData;

struct PolylineInfo {
    SegmentIndex first;
    SegmentIndex last;
    bool isClosed;
};

} // namespace detail

} // namespace segmentintersector2

/// \class SegmentIntersector2
/// \brief Computes all intersections between a set of line segments.
///
template<typename T>
class SegmentIntersector2 {
public:
    using ScalarType = T;
    static constexpr Int dimension = 2;

    using SegmentIndex = segmentintersector2::SegmentIndex;
    using PolylineIndex = segmentintersector2::PolylineIndex;
    using VertexIndex = segmentintersector2::VertexIndex;
    using EdgeIndex = segmentintersector2::EdgeIndex;

    using SegmentIndexRange = segmentintersector2::SegmentIndexRange<T>;

    using VertexSegment = segmentintersector2::VertexSegment<T>;
    using Vertex = segmentintersector2::Vertex<T>;

    using EdgeSegment = segmentintersector2::EdgeSegment<T>;
    using Edge = segmentintersector2::Edge<T>;

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

    /// Adds a segment to this intersector, and returns its index.
    ///
    SegmentIndex addSegment(const Vec2<T>& a, const Vec2<T>& b);

    /// Adds an open polyline to this intersector, and returns its index.
    ///
    /// The unary operator `proj` should take as argument an element of the
    /// given `range`, and return a point of type `Vec2`.
    ///
    /// With `n` being the length of the range, then:
    ///
    /// - If `n < 2`, this does not add any segment.
    ///
    /// - If `n >= 2`, this adds `n-1` segments.
    ///
    /// All added segments have consecutive indices, which can be
    /// retrieved via `segmentIndexRange(polylineIndex)`.
    ///
    template<
        typename R,
        typename Proj = c20::identity,
        VGC_REQUIRES(core::isCompatibleInputRange<R, Vec2<T>, Proj>)>
    PolylineIndex addPolyline(R&& range, Proj proj = {}) {
        return addPolyline(false, false, range, proj);
    }

    /// Adds a (possibly closed) polyline to this intersector, and returns its index.
    ///
    /// The unary operator `proj` should take as argument an element of the
    /// given `range`, and return a point of type `Vec2`.
    ///
    /// With `n` being the length of the range, then:
    ///
    /// - If `isClosed == false` and `n < 2`, this does not add any segment.
    ///
    /// - If `isClosed == false` and `n >= 2`, this adds `n-1` segments.
    ///
    /// - If `isClosed == true`, `hasDuplicateEndpoints == true`, and `n < 2`,
    ///   this does not add any segment.
    ///
    /// - If `isClosed == true`, `hasDuplicateEndpoints == true`, and `n >= 2`,
    ///   this adds `n-1` segments and expects the first and last points to be equal.
    ///
    /// - If `isClosed == true`, `hasDuplicateEndpoints == false`, and `n < 1`,
    ///   this does not add any segment.
    ///
    /// - If `isClosed == true`, `hasDuplicateEndpoints == false`, and `n >= 1`,
    ///   this adds `n` segments. Note that if the first and last points are
    ///   equal, then the last segment is reduced to a point.
    ///
    /// All added segments have consecutive indices, which can be
    /// retrieved via `segmentIndexRange(polylineIndex)`.
    ///
    template<
        typename R,
        typename Proj = c20::identity,
        VGC_REQUIRES(core::isCompatibleInputRange<R, Vec2<T>, Proj>)>
    PolylineIndex
    addPolyline(bool isClosed, bool hasDuplicateEndpoints, R&& range, Proj proj = {});

    /// Computes the intersections between all added segments and polylines.
    ///
    void computeIntersections();

    /// Returns the computed intersection points.
    ///
    const core::Array<Vertex>& intersectionPoints() const {
        return output_.intersectionPoints;
    };

    /// Returns the computed intersection subsegments.
    ///
    const core::Array<Edge>& intersectionSubsegments() const {
        return output_.intersectionSubsegments;
    };

    /// Returns which polyline contains the given segment.
    ///
    /// Returns -1 if `i` corresponds to a segment that is not part of a
    /// polyline, that is, was added directly via `addSegment()` instead of
    /// via `addPolyline()`.
    ///
    /// Raises `IndexError` if `i` is not a valid segment index.
    ///
    PolylineIndex polylineIndex(SegmentIndex i) {
        return input_.segmentPolylines[i];
    }

    /// Returns the range of segment indices [first, last) that
    /// corresponds to the given polyline index.
    ///
    /// Raises `IndexError` if `i` is not a valid polyline index.
    ///
    SegmentIndexRange segmentIndexRange(PolylineIndex i) {
        const segmentintersector2::detail::PolylineInfo& info = input_.polylines[i];
        return {info.first, info.last};
    }

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
    core::Array<PolylineInfo> polylines;
    core::Array<Segment2<T>> segments;
    core::Array<T> segmentSlopes;
    core::Array<bool> isReversed;
    core::Array<PolylineIndex> segmentPolylines;

    void clear() {
        polylines.clear();
        segments.clear();
        segmentSlopes.clear();
        isReversed.clear();
        segmentPolylines.clear();
    }
};

template<typename T>
struct OutputData {
    core::Array<Vertex<T>> intersectionPoints;
    core::Array<Edge<T>> intersectionSubsegments;

    void clear() {
        intersectionPoints.clear();
        intersectionSubsegments.clear();
    }
};

// When segments overlap along a shared subsegment,
// we call them part of the same "overlap group".
// The union of all the segments in the group is a bigger segment.
//
// Formally, an "overlap group" is a connected component of the graph G where
// each segment is a node, and each pair of overlapping segments is an edge. We
// compute the groups in O(n) by reporting each edge in O(1) during the plane
// sweep, then computing its connected components in O(n) at the end of the
// plane sweep.
//
struct OverlapGroups {
    core::Array<std::pair<SegmentIndex, SegmentIndex>> pairs;
    core::Array<bool> isRemoved;

    void clear() {
        pairs.clear();
    }

    void init(Int numSegments) {
        clear();
        isRemoved.assign(numSegments, false);
    }

    void addOverlappingSegments(SegmentIndex i1, SegmentIndex i2) {
        pairs.append({i1, i2});
    }

    void setRemoved(SegmentIndex i1) {
        isRemoved[i1] = true;
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
    // y-coords of their intersection with the sweep line, and ordered
    // by slope in case of equality (vertical segments last).
    //
    core::Array<SegmentIndex> sweepSegments;

    // All segments that are intersecting at the current event.
    //
    core::Array<VertexSegment<T>> vertexSegments;
    core::Array<EdgeSegment<T>> edgeSegments;

    // The new segments that must be added (or removed and re-added) to
    // sweepSegments_ when handling an event.
    //
    core::Array<SegmentIndex> outgoingSegments;

    // The list of all events that correspond to the same position while
    // processing the next event.
    //
    core::Array<Event<T>> sweepEvents;

    // Handling of segments that overlap along a subsegment.
    //
    OverlapGroups overlapGroups;

    void clear() {
        VGC_ASSERT(eventQueue.empty()); // priority_queue has no clear() method
        sweepSegments.clear();
        outgoingSegments.clear();
        sweepEvents.clear();
        overlapGroups.clear();
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
SegmentIndex addSegment(
    InputData<T>& in,
    const Vec2<T>& a,
    const Vec2<T>& b,
    PolylineIndex polylineIndex) {

    SegmentIndex res = in.segments.length();
    if (b < a) {
        in.segments.emplaceLast(b, a);
        in.segmentSlopes.append(computeSlope(b, a));
        in.isReversed.append(true);
    }
    else {
        in.segments.emplaceLast(a, b);
        in.segmentSlopes.append(computeSlope(a, b));
        in.isReversed.append(false);
    }
    in.segmentPolylines.append(polylineIndex);
    return res;
}

template<typename T, typename R, typename Proj>
PolylineIndex addPolyline(
    InputData<T>& in,
    R&& range,
    Proj proj,
    bool isClosed,
    bool hasDuplicateEndpoints) {

    // Handle special case there the polyline has no segments
    PolylineIndex polylineIndex = in.polylines.length();
    SegmentIndex polylineBegin = in.segments.length();
    auto it = range.begin();
    auto end = range.end();
    bool isEmpty = (it == end);
    if (isEmpty) {
        in.polylines.emplaceLast(PolylineInfo{polylineBegin, polylineBegin, isClosed});
        return polylineIndex;
    }

    // Get the first element of the range.
    //
    // Note:
    // - It's UB to call begin() more than once on input ranges
    // - The input iterator might be non-copyable (i.e., move-only)
    //
    // So we cannot do `firstPosition = range.begin()`, then iterate on the
    // rest of the range with `for (auto& p : range | std::views::drop(1))`,
    // since to underlying `drop_view` would call `range.begin()` again.
    //
    // Note that C++23 introduces std::views::adjacent, but it requires a
    // forward_range.
    //
    Vec2<T> firstPosition = proj(*it);
    Vec2<T> startPosition = firstPosition;
    ++it;

    // Reserve memory if the number of segments can be known in advance.
    if constexpr (core::isForwardRange<R>) {
        auto numSegmentsInPolyline = std::distance(it, end);
        if (isClosed && !hasDuplicateEndpoints) {
            numSegmentsInPolyline += 1;
        }
        auto numSegments = in.segments.size() + numSegmentsInPolyline;
        in.segments.reserve(numSegments);
        in.segmentSlopes.reserve(numSegments);
        in.isReversed.reserve(numSegments);
        in.segmentPolylines.reserve(numSegments);
    }

    // Add the segments
    for (; it != end; ++it) {
        Vec2<T> endPosition = proj(*it);
        addSegment(in, startPosition, endPosition, polylineIndex);
        startPosition = endPosition;
    }
    if (isClosed && !hasDuplicateEndpoints) {
        addSegment(in, startPosition, firstPosition, polylineIndex);
    }

    SegmentIndex polylineEnd = in.segments.length();
    in.polylines.emplaceLast(PolylineInfo{polylineBegin, polylineEnd, isClosed});
    return polylineIndex;
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
        if (segment.isDegenerate()) {
            alg.eventQueue.push(Event<T>{EventType::Degenerate, segment.a(), i});
        }
        else {
            alg.eventQueue.push(Event<T>{EventType::Left, segment.a(), i});
            alg.eventQueue.push(Event<T>{EventType::Right, segment.b(), i});
        }
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

template<typename T>
void initializeOverlapGroups(InputData<T>& in, AlgorithmData<T>& alg) {
    Int numSegments = in.segments.length();
    alg.overlapGroups.init(numSegments);
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
    Event<T> firstEvent = alg.eventQueue.top();
    alg.eventQueue.pop();
    Vec2<T> position = firstEvent.position;
    alg.sweepEvents.clear();
    alg.sweepEvents.append(firstEvent);
    while (!alg.eventQueue.empty() && alg.eventQueue.top().position == position) {
        alg.sweepEvents.append(alg.eventQueue.top());
        alg.eventQueue.pop();
    }

    // Ignore all events from segments that have been removed as
    // part of an overlap group. Conceptually, these are not part
    // of the event queue anymore, but our implementation does not
    // support removing events so we simply ignore them.
    //
    alg.sweepEvents.removeIf([&](const Event<T>& event) {
        return alg.overlapGroups.isRemoved[event.segmentIndex];
    });
    return position;
}

template<typename T>
struct PartitionedSweepEvents {
    using Iterator = typename core::Array<Event<T>>::iterator;

    Iterator it1;
    Iterator it2;
    Iterator it3;
    Iterator it4;
    Iterator it5;

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

    // Degenerate events
    core::ConstSpan<Event<T>> degenerate() const {
        return {it4, it5};
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
    res.it5 = alg.sweepEvents.end();
    res.it2 = std::find_if(res.it1, res.it5, [](const Event<T>& event) {
        return event.type > EventType::Left; // find first event not Left
    });
    res.it3 = std::find_if(res.it2, res.it5, [](const Event<T>& event) {
        return event.type > EventType::Right; // find first event not Left/Right
    });
    res.it4 = std::find_if(res.it3, res.it5, [](const Event<T>& event) {
        return event.type > EventType::Intersection; // not Left/Right/Intersection
    });
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
                // In theory, this should not happen, but due to numerical
                // errors it does happen occasionally.
                // TODO: More analysis to understand why/when it happens.
                //       Could it be happening because of an actual bug?
#ifdef VGC_DEBUG_BUILD
                VGC_WARNING(
                    LogVgcGeometry,
                    "Segment from event {} not found in sweep segments.",
                    event);
#endif
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
    VertexIndex vertexIndex = out.intersectionPoints.length();
    alg.vertexSegments.clear();
    for (SegmentIndex i : pSegments.contain()) {
        if (!alg.overlapGroups.isRemoved[i]) {
            alg.vertexSegments.append(VertexSegment<T>{vertexIndex, i, 0});
        }
    }
    for (const Event<T>& event : pEvents.left()) {
        // Note: Left events cannot be in the removed set
        alg.vertexSegments.append(VertexSegment<T>{vertexIndex, event.segmentIndex, 0});
    }
    for (const Event<T>& event : pEvents.degenerate()) {
        // Note: Degenerate events cannot be in the removed set
        alg.vertexSegments.append(VertexSegment<T>{vertexIndex, event.segmentIndex, 0});
    }

    // There is no intersection if there is only one segment at the event
    // position. This means we are at an isolated left endpoint or right
    // endpoint of a segment.
    //
    if (alg.vertexSegments.length() <= 1) {
        return;
    }

    // Discard the intersection if it only involves two segments, and they are
    // consecutive segments of the same polyline, intersecting at their
    // expected shared endpoint.
    //
    // Note that if there is another segment intersecting there as well,
    // we do want to report the intersection.
    //
    if (alg.vertexSegments.length() == 2) {
        // TODO: what if consecutive segments overlap along a subsegment?
        SegmentIndex i1 = alg.vertexSegments.first().segmentIndex();
        SegmentIndex i2 = alg.vertexSegments.last().segmentIndex();
        PolylineIndex j = in.segmentPolylines[i1];
        if (j >= 0 && in.segmentPolylines[i2] == j) {
            if (i2 < i1) {
                std::swap(i1, i2);
            }
            if (i2 == i1 + 1) {
                return;
            }
            const PolylineInfo& info = in.polylines[j];
            if (info.isClosed && i1 == info.first && i2 == info.last - 1) {
                return;
            }
        }
    }

    // Compute the parameters and report the intersection
    for (VertexSegment<T>& vs : alg.vertexSegments) {
        vs.setParameter(computeParam(in, vs.segmentIndex(), position));
    }
    out.intersectionPoints.append(Vertex<T>(position, alg.vertexSegments));
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

    // Add to outgoingSegments all segments in pSegments.contain()
    // which are not ending at the position.
    //
    // This is guaranteed to have no duplicates since alg.sweepSegments does
    // not have duplicates and pSegments.contain() is a subspan of
    // alg.sweepSegments.
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

    // Add to outgoingSegments all segments in Left events.
    //
    // This is guaranteed to not create duplicate since all the segments in
    // pEvents.left() have never been added to the event queue yet.
    //
    // Note that we do not add to outgoingSegments any Degenerate segment.
    //
    for (const Event<T>& event : pEvents.left()) {
        alg.outgoingSegments.append(event.segmentIndex);
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

    // Find if there are overlapping segment among the outgoing segments. They
    // are those with the same slope, but due to numerical errors, we do not
    // rely on slope but on our robust intersection test for this. But since
    // outgoingSegments are sorted by slopes, we still only have to test
    // consecutive segment pairs.
    //
    for (Int j = 1; j < alg.outgoingSegments.length(); ++j) {
        SegmentIndex i1 = alg.outgoingSegments.getUnchecked(j - 1);
        SegmentIndex i2 = alg.outgoingSegments.getUnchecked(j);
        const Segment2<T>& s1 = in.segments.getUnchecked(i1);
        const Segment2<T>& s2 = in.segments.getUnchecked(i2);
        SegmentIntersection2<T> inter = s1.intersect(s2);
        if (inter.type() == SegmentIntersectionType::Segment) {

            // Report that s1 and s2 overlap. These are post-processed after
            // the plane sweep to add all the relevant intersection to the
            // output structures.
            //
            alg.overlapGroups.addOverlappingSegments(i1, i2);

            // Only keep the segment that extend further to the right of the
            // sweep line (or in case of vertical segments, the segment that
            // extend further to the top along the sweep line).
            //
            if (s2.b() > s1.b()) {
                alg.overlapGroups.setRemoved(i1);
                alg.outgoingSegments.removeAt(j - 1);
            }
            else {
                alg.overlapGroups.setRemoved(i2);
                alg.outgoingSegments.removeAt(j);
            }
            --j;
        }
    }
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
    else if (inter.type() == SegmentIntersectionType::Segment) {

        // In theory, it is impossible to get an intersection of type Segment,
        // since one of the segment (say, s1) pass through the event position
        // p, while the other (s2) does not. More precisely:
        //
        // - If they have different slopes:
        //     They either don't intersect or intersect at a point
        //
        // - If both have the same slope:
        //   - If they are non-vertical, then they are parallel non-intersecting
        //   - If they are vertical, s2 is either entirely below p, in which case
        //     it is not anymore in sweepSegments, or s2 is entirely above p, in
        //     which case it is not yet in p
        //
        // In practice, due to numerical, it might possibly happen?
        // For now we simply ignore this.
        //
#ifdef VGC_DEBUG_BUILD
        VGC_WARNING(
            LogVgcGeometry,
            "Intersection of type Segment found between {} and {} at event position {}.",
            s1,
            s2,
            position);
#endif
    }
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

// During the plane sweep, each time we detect two segments that overlap, we
// add their indices (i1, i2) to overlapGroups.pairs, and only keep one of them
// (the one that extend further to the right) in sweepSegments and eventQueue.
//
// These pairs (i1, i2) form an undirect graph between segments, where each connected
// component of size >= 2 is what we call an "overlap group".
//
// In this function, we compute these overlap group, and process them to report
// all pairwise overlaps between segments, as well as all point intersections
// that were not reported couldn't be reported the plane sweep due to the
// overlapping segments being removed from the data structure.
//
template<typename T>
void postProcessOverlappingSegments(
    InputData<T>& in,
    AlgorithmData<T>& alg,
    OutputData<T>& out) {

    // Fast return if there are no overlapping segments
    if (alg.overlapGroups.pairs.isEmpty()) {
        return;
    }

    // Convert the graph representation from edge pairs to adjacency lists. We
    // store all adjacency lists in a single contiguous array to minimize
    // allocations.
    Int numSegments = in.segments.length();
    core::Array<Int> numEdges(numSegments, 0);
    for (const auto& [i1, i2] : alg.overlapGroups.pairs) {
        ++numEdges[i1];
        ++numEdges[i2];
    }
    core::Array<Int> adjBuffer;
    adjBuffer.resize(alg.overlapGroups.pairs.length() * 2);
    core::Array<core::Span<Int>> adj;
    Int* adjBegin = adjBuffer.data();
    for (SegmentIndex i = 0; i < numSegments; ++i) {
        Int* adjEnd = adjBegin + numEdges[i];
        adj.append(core::Span<Int>(adjBegin, adjEnd));
        adjBegin = adjEnd;
    }
    for (const auto& [i1, i2] : alg.overlapGroups.pairs) {
        Int k1 = --numEdges[i1];
        Int k2 = --numEdges[i2];
        adj[i1][k1] = i2;
        adj[i2][k2] = i1;
    }

    // Compute groups as connected components of the graph.
    // Isolated segments (not part of an overlap group) are assigned group -1.
    core::Array<Int> segmentGroup(numSegments, -1); // SegmentIndex -> GroupIndex
    core::Array<Int> stack;                         // For depth-first visit
    Int numGroups = 0;
    core::Array<Int> groupBuffer; // Contiguously store all segments of a group
    core::Array<core::Span<Int>> groups;
    groupBuffer.reserve(numSegments); // Mandatory for span pointer stability
    for (SegmentIndex i = 0; i < numSegments; ++i) {

        // If segment i is not yet assigned a group but has segment overlaps,
        // then find all segments in the overlap group and assign them a group
        // index.
        //
        if (segmentGroup[i] == -1 && !adj[i].isEmpty()) {
            Int* groupBegin = groupBuffer.data() + groupBuffer.length();
            Int groupIndex = numGroups++;
            stack.append(i);
            while (!stack.isEmpty()) {
                Int j = stack.pop();
                segmentGroup[j] = groupIndex;
                groupBuffer.append(j);
                for (Int k : adj[j]) {
                    if (segmentGroup[k] == -1) {
                        stack.append(k);
                    }
                }
            }
            Int* groupEnd = groupBuffer.data() + groupBuffer.length();
            groups.append(core::Span<Int>(groupBegin, groupEnd));
        }
    }

    // Report segment intersections.
    //
    // TODO: Report missing point intersections.
    //
    // TODO: If two segment intersections are equal, merge them into one
    // Edge, instead of having all Edge only
    // involve two segments. Or perhaps, change the output structure completely
    // towards something more like a planar map decomposition.
    //
    // TODO: take isReversed into account.
    //
    for (const auto& group : groups) {
        Int numSegmentsInGroup = group.length();
        for (SegmentIndex i1 = 0; i1 < numSegmentsInGroup; ++i1) {
            for (SegmentIndex i2 = i1 + 1; i2 < numSegmentsInGroup; ++i2) {
                const Segment2<T>& s1 = in.segments[i1];
                const Segment2<T>& s2 = in.segments[i2];
                SegmentIntersection2<T> inter = s1.intersect(s2);
                if (inter.type() == SegmentIntersectionType::Segment) {
                    EdgeIndex edgeIndex = out.intersectionSubsegments.length();
                    alg.edgeSegments.clear();
                    alg.edgeSegments.append(
                        EdgeSegment<T>(edgeIndex, i1, inter.s1(), inter.t1()));
                    alg.edgeSegments.append(
                        EdgeSegment<T>(edgeIndex, i2, inter.s2(), inter.t2()));
                    out.intersectionSubsegments.append(
                        {inter.segment(), alg.edgeSegments});
                }
            }
        }
    }
}

template<typename T>
void processNextEvent(InputData<T>& in, AlgorithmData<T>& alg, OutputData<T>& out) {

    // Pop the next event from the event queue, as well as all subsequent
    // events sharing the same position. Store them in alg.sweepEvents.
    Vec2<T> position = getNextEvent(alg);
    if (alg.sweepEvents.isEmpty()) {
        // This can happen due to events related to a segment that is removed
        // as being part of an overlap group. We just ignore them and move on
        // to the next event.
        return;
    }

    // Partition the events into Left, Right, and Intersection events.
    PartitionedSweepEvents pEvents = partitionSweepEvents(alg);

    // Partition the sweep segments into those that are below, above, or
    // contain the event position. We guarantee that segments corresponding to
    // Intersection and Right events are within pSegments.contain(), unless the
    // segment is not in sweepSegments at all (which is in theory impossible,
    // but can happen due to numerical errors).
    //
    PartitionedSweepSegments pSegments =
        partitionSweepSegments(in, alg, position, pEvents);

    // Compute which segments are outgoing at the given position.
    //
    // This also computes which segments overlap and only keep
    // one representative per overlap group (the one that extends
    // further to the right). This is why this must be called
    // before `reportIntersections`, as we do not want to report
    // intersections between segments part of the same overlap
    // group during the plane sweep (these intersections are added
    // as a post-process step).
    //
    computeOutgoingSegments(in, alg, position, pEvents, pSegments);

    // Report point-intersections
    reportIntersections(in, alg, out, position, pEvents, pSegments);

    // Remove ingoing segments and add outgoing segments.
    // This invalidates previous iterators and spans stored in pSegments.
    // The new iterator `it` refers to either:
    // - the first inserted segment if any, otherwise
    // - the segment (or sentinel) just after the removed segments
    auto it = alg.sweepSegments.erase(pSegments.it2, pSegments.it3);
    it = alg.sweepSegments.insert(it, alg.outgoingSegments);

    // Add intersections between newly-neighbor segments to the event queue.
    findNewIntersections(in, alg, position, it);
}

template<typename T>
void computeIntersections(InputData<T>& in, AlgorithmData<T>& alg, OutputData<T>& out) {
    initializeEventQueue(in, alg);
    initializeSweepSegments(alg);
    initializeOverlapGroups(in, alg);
    while (!alg.eventQueue.empty()) {
        processNextEvent(in, alg, out);
    }
    postProcessOverlappingSegments(in, alg, out);
}

} // namespace segmentintersector2::detail

template<typename T>
segmentintersector2::SegmentIndex
SegmentIntersector2<T>::addSegment(const Vec2<T>& a, const Vec2<T>& b) {
    PolylineIndex polylineIndex = -1;
    return segmentintersector2::detail::addSegment(input_, a, b, polylineIndex);
}

template<typename T>
template<
    typename R,
    typename Proj,
    VGC_REQUIRES_DEF(core::isCompatibleInputRange<R, Vec2<T>, Proj>)>
segmentintersector2::PolylineIndex SegmentIntersector2<T>::addPolyline(
    bool isClosed,
    bool hasDuplicateEndpoints,
    R&& range,
    Proj proj) {

    return segmentintersector2::detail::addPolyline(
        input_, range, proj, isClosed, hasDuplicateEndpoints);
}

template<typename T>
void SegmentIntersector2<T>::computeIntersections() {
    segmentintersector2::detail::computeIntersections(input_, algorithm_, output_);
}

VGC_GEOMETRY_API_DECLARE_TEMPLATE_CLASS(SegmentIntersector2<float>);
VGC_GEOMETRY_API_DECLARE_TEMPLATE_CLASS(SegmentIntersector2<double>);

using SegmentIntersector2f = SegmentIntersector2<float>;
using SegmentIntersector2d = SegmentIntersector2<double>;

} // namespace vgc::geometry

template<typename T>
struct fmt::formatter<vgc::geometry::segmentintersector2::VertexSegment<T>>
    : fmt::formatter<std::string_view> {

    using VertexSegment = vgc::geometry::segmentintersector2::VertexSegment<T>;

    template<typename FormatContext>
    auto format(const VertexSegment& vs, FormatContext& ctx) {
        return format_to(
            ctx.out(),
            "{{vertexIndex={}, segmentIndex={}, param={}}}",
            vs.vertexIndex(),
            vs.segmentIndex(),
            vs.parameter());
    }
};

template<typename T>
struct fmt::formatter<vgc::geometry::segmentintersector2::Vertex<T>>
    : fmt::formatter<std::string_view> {

    using Vertex = vgc::geometry::segmentintersector2::Vertex<T>;

    template<typename FormatContext>
    auto format(const Vertex& vertex, FormatContext& ctx) {
        return format_to(
            ctx.out(),
            "{{position={}, segments={}}}",
            vertex.position(),
            vertex.segments());
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
        case EventType::Degenerate:
            return format_to(ctx.out(), "Degenerate");
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
