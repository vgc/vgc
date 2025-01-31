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

#include <vgc/geometry/interpolatingstroke.h>

#include <algorithm> // std::copy

#include <vgc/core/algorithms.h>
#include <vgc/core/format.h>
#include <vgc/core/span.h>
#include <vgc/geometry/logcategories.h>
#include <vgc/geometry/mat3.h>
#include <vgc/geometry/vec2.h>

namespace vgc::geometry {

namespace {

Vec2dArray computeChords(core::ConstSpan<Vec2d> knotPositions) {
    const Vec2d* p = knotPositions.data();
    Int n = knotPositions.length();
    Vec2dArray chords(n, core::noInit);
    if (n > 0) {
        for (Int i = 0; i < n - 1; ++i) {
            chords[i] = p[i + 1] - p[i];
        }
        // Last chord is the closure.
        chords[n - 1] = p[n - 1] - p[0];
    }
    return chords;
}

void computeLengths(const Vec2dArray& vectors, core::DoubleArray& outLengths) {
    Int n = vectors.length();
    outLengths.resizeNoInit(n);
    for (Int i = 0; i < n; ++i) {
        outLengths[i] = vectors[i].length();
    }
}

// Assumes segmentIndex is valid.
void computeSegmentKnotAndChordIndices(
    Int numKnots,
    bool isClosed,
    Int segmentIndex,
    std::array<Int, 4>& outKnotIndices,
    std::array<Int, 3>& outChordIndices) {

    // Get indices of points used by the Catmull-Rom interpolation, handle
    // wrapping for closed curves and boundary for open curves.
    outKnotIndices = {segmentIndex - 1, segmentIndex, segmentIndex + 1, segmentIndex + 2};
    outChordIndices = {segmentIndex - 1, segmentIndex, segmentIndex + 1};

    if (isClosed) {
        if (outKnotIndices[0] < 0) {
            outKnotIndices[0] = numKnots - 1;
            outChordIndices[0] = numKnots - 1;
        }
        if (outKnotIndices[2] > numKnots - 1) {
            outKnotIndices[2] = 0;
            outChordIndices[2] = 0;
            outKnotIndices[3] = 1;
        }
        if (outKnotIndices[3] > numKnots - 1) {
            outKnotIndices[3] = 0;
        }
    }
    else {
        Int zeroLengthChordIndex = numKnots - 1;
        if (outKnotIndices[0] < 0) {
            outKnotIndices[0] = 0;
            outChordIndices[0] = zeroLengthChordIndex;
        }
        if (outKnotIndices[2] > numKnots - 1) {
            outKnotIndices[2] = numKnots - 1;
            outChordIndices[2] = zeroLengthChordIndex;
            outKnotIndices[3] = numKnots - 1;
        }
        else if (outKnotIndices[3] > numKnots - 1) {
            outKnotIndices[3] = numKnots - 1;
        }
    }
}

CurveSegmentType
computeSegmentTypeFromChordLengths(const std::array<double, 3>& segmentChordLengths) {
    if (segmentChordLengths[1] == 0) {
        return CurveSegmentType::Corner;
    }
    bool isAfterCorner = (segmentChordLengths[0] == 0);
    bool isBeforeCorner = (segmentChordLengths[2] == 0);
    if (isAfterCorner) {
        if (isBeforeCorner) {
            return CurveSegmentType::BetweenCorners;
        }
        return CurveSegmentType::AfterCorner;
    }
    else if (isBeforeCorner) {
        return CurveSegmentType::BeforeCorner;
    }
    else {
        return CurveSegmentType::Simple;
    }
}

} // namespace

void AbstractInterpolatingStroke2d::updateCache() const {

    if (!isCacheDirty_) {
        return;
    }

    core::Array<SegmentComputeData> computeDataArray;

    Int numKnots = positions_.length();
    Int numSegments = this->numSegments();
    bool isClosed = this->isClosed();

    Vec2dArray chords = computeChords(positions_);
    if (!isClosed) {
        chords.last() = Vec2d();
    }

    bool updateSegmentTypes = false;
    if (chordLengths_.isEmpty()) {
        computeLengths(chords, chordLengths_);
        totalChordalLength_ = core::sum(chordLengths_);
        segmentTypes_.resizeNoInit(numSegments);
        updateSegmentTypes = true;
    }

    computeDataArray.resizeNoInit(numSegments);

    for (Int i = 0; i < numSegments; ++i) {
        SegmentComputeData& computeData = computeDataArray[i];

        std::array<Int, 4> knotIndices;
        std::array<Int, 3> chordIndices;
        computeSegmentKnotAndChordIndices(
            numKnots, isClosed, i, knotIndices, chordIndices);

        computeData.knotIndices = knotIndices;
        computeData.chords = detail::getElementsUnchecked(chords, chordIndices);
        computeData.chordLengths =
            detail::getElementsUnchecked(chordLengths_, chordIndices);

        if (updateSegmentTypes) {
            CurveSegmentType segmentType =
                computeSegmentTypeFromChordLengths(computeData.chordLengths);
            segmentTypes_.getUnchecked(i) = segmentType;
        }
    }

    // flag must be false before calling updateCache_()
    isCacheDirty_ = false;

    updateCache_(computeDataArray);
}

std::unique_ptr<AbstractStroke2d>
AbstractInterpolatingStroke2d::convert_(const AbstractStroke2d* source) const {
    return AbstractStroke2d::convert_(source);
}

bool AbstractInterpolatingStroke2d::convertAssign_(const AbstractStroke2d* other_) {
    auto other = dynamic_cast<const AbstractInterpolatingStroke2d*>(other_);
    if (!other) {
        return false;
    }
    setPositions(other->positions());
    if (other->hasConstantWidth()) {
        setConstantWidth(other->constantWidth());
    }
    else {
        setWidths(other->widths());
    }
    return true;
}

double AbstractInterpolatingStroke2d::approximateLength_() const {
    updateCache();
    return totalChordalLength_;
}

Int AbstractInterpolatingStroke2d::numKnots_() const {
    return positions_.length();
}

bool AbstractInterpolatingStroke2d::isZeroLengthSegment_(Int segmentIndex) const {
    updateCache();
    return chordLengths_[segmentIndex] == 0;
}

std::array<Vec2d, 2> AbstractInterpolatingStroke2d::endPositions_() const {
    if (!positions_.isEmpty()) {
        return {positions_.first(), positions_.last()};
    }
    else {
        return {Vec2d(), Vec2d()};
    }
}

CurveParameter AbstractInterpolatingStroke2d::resolveParameter_(
    const SampledCurveParameter& param) const {

    // Currently does a coarse approximation, as if speed were constant between samples.
    // TODO: later, resolve according to given tolerance/precision.
    return CurveParameter(
        param.segmentIndex(),
        core::fastLerp(param.u1(), param.u2(), param.lerpParameter()));
}

void AbstractInterpolatingStroke2d::translate_(const Vec2d& delta) {
    for (Vec2d& p : positions_) {
        p += delta;
    }
    onPositionsChanged_();
}

void AbstractInterpolatingStroke2d::transform_(const Mat3d& transformation) {
    for (Vec2d& p : positions_) {
        p = transformation.transform(p);
    }
    onPositionsChanged_();
}

void AbstractInterpolatingStroke2d::close_(bool smoothJoin) {
    if (positions_.length() > 1) {
        if (smoothJoin && positions_.last() == positions_.first()) {
            positions_.removeLast();
            if (!hasConstantWidth_) {
                widths_.removeLast();
                onWidthsChanged_();
            }
            onPositionsChanged_();
        }
    }
}

void AbstractInterpolatingStroke2d::open_(bool keepJoinAsBestAsPossible) {
    VGC_UNUSED(keepJoinAsBestAsPossible);
    if (positions_.length() > 0) {
        positions_.append(positions_.first());
        if (!hasConstantWidth_) {
            widths_.append(widths_.first());
            onWidthsChanged_();
        }
        onPositionsChanged_();
    }
}

namespace {

enum class Order : Int8 {
    Less = -1,  // First operand less than the second
    Equal = 0,  // Both operands are equal
    Greater = 1 // First operand greater than the second
};

Order compareCurveParameters(const CurveParameter& p1, const CurveParameter& p2) {
    Int8 sign = static_cast<Int8>(p2 < p1) - static_cast<Int8>(p1 < p2);
    return static_cast<Order>(sign);
}

} // namespace

// Precondition: p1 and p2 are normalized (see AbstractStroke2d::normalizeParameter()).
// This is automatically done by AbstractStroke2d::subStroke() calling this function.
std::unique_ptr<AbstractStroke2d> AbstractInterpolatingStroke2d::subStroke_(
    const CurveParameter& p1,
    const CurveParameter& p2,
    Int numWraps) const {

    Int numKnots = positions_.length();

    if (numKnots <= 1) {
        return clone();
    }

    // Create a new stroke, initialized to an empty open stroke.
    std::unique_ptr<AbstractStroke2d> result = cloneEmpty();
    AbstractInterpolatingStroke2d* newStroke =
        static_cast<AbstractInterpolatingStroke2d*>(result.get());
    newStroke->open(false);

    // Compare the parameter. This (and the rest of the code) assumes that the
    // params were normalized, that is (i, 1) was converted to (i+1, 0), etc.
    Order order = compareCurveParameters(p1, p2);

    // Sanitize numWraps, and compute whether the substroke is reduced to a point.
    // Open stroke:   this is when p1 == p2 or p1 > p2.
    // Closed stroke: this is when p1 == p2 and numWraps == 0.
    //
    bool isReducedToPoint = false;
    if (isClosed()) {
        numWraps = std::max<Int>(numWraps, 0);
        isReducedToPoint = (order == Order::Equal) && (numWraps == 0);
    }
    else {
        numWraps = 0;
        isReducedToPoint = (order != Order::Less);
    }

    // Fast-return if the substroke is reduced to a point.
    StrokeSample2d s1 = eval(p1);
    if (isReducedToPoint) {
        std::array<Vec2d, 1> points = {s1.position()};
        newStroke->setPositions(points);
        newStroke->setConstantWidth(s1.width());
        return result;
    }

    // Alias for the segment index of each input parameter.
    // Note that the segment index is also the index of the segment's start knot.
    //
    // Example:
    //                          p1 = (1, u1)    p2 = (3, u2)
    //                              |               |
    //    knot index =  0       1   v   2       3   v   4     (numKnots = 5)
    //                  o-------o-------o-------o-------o
    // segment index =      0       1       2       3
    //
    Int i1 = p1.segmentIndex();
    Int i2 = p2.segmentIndex();

    // Reserve memory
    Int reservedLength = 2 + numWraps * numKnots;
    if (order == Order::Less) {
        reservedLength += (i2 - i1);
    }
    else if (order == Order::Greater) {
        // Note: Greater implies isClosed(), so numSegments == numKnots.
        reservedLength += numKnots - (i1 - i2);
    }
    Vec2dArray positions;
    core::DoubleArray widths;
    positions.reserve(reservedLength);
    bool hasWidths = !hasConstantWidth();
    if (hasWidths) {
        widths.reserve(reservedLength);
    }

    // Add the first knot of the substroke.
    //
    //        0       1   p1  2       3   p2  4     (numKnots = 5)
    // in:    o-------o---+---o-------o---+---o
    //
    // out:               o s1
    //
    positions.append(s1.position());
    if (hasWidths) {
        widths.append(s1.width());
    }

    // Compute index of knot after p1, not including p1.
    //
    //        0       1   p1  2       3   p2  4
    //        o-------o---+---o-------o---+---o
    //                        ^
    //                      iFirst
    //
    //        0       1 p1 p2 2       3       4
    //        o-------o-+--+--o-------o-------o
    //                        ^
    //                      iFirst
    //
    // Note: thanks to normalization, p1.u() == 1 is impossible here since:
    // - it is only possible for open curves, with p1 being the maximum possible param
    // - therefore `p1 >= p2`
    // - therefore `order != Order::Less`
    // - therefore `isReducedToAPoint` is true (since `iClosed` is false)
    // - therefore we would have already fast-returned.
    //
    Int iFirst = i1 + 1;
    VGC_ASSERT(iFirst >= 0);
    VGC_ASSERT(iFirst <= numKnots); // Equality possible for closed strokes

    // Compute index of knot just before p2, not including p2.
    //
    //        0       1   p1  2       3   p2  4     (numKnots = 5)
    //        o-------o---+---o-------o---+---o
    //                        ^       ^
    //                      iFirst  iLast
    //
    //        0       1   p1  2      3,p2     4
    //        o-------o---+---o-------o-------o
    //                        ^
    //                  iFirst,iLast
    //
    //        0       1 p1 p2 2       3       4
    //        o-------o-+--+--o-------o-------o
    //                ^       ^
    //              iLast   iFirst
    //
    //        0       1    2,p1,p2    3       4
    //        o-------o-------o-------o-------o
    //                ^               ^
    //              iLast           iFirst
    //
    //  -1   0,p2     1       2   p1  3       4
    //        o-------o-------o---+---o-------o
    //   ^                        ^
    // iLast                    iFirst
    //
    //  -1  0,p1,p2   1       2       3       4
    //        o-------o-------o-------o-------o
    //   ^            ^
    // iLast         iFirst
    //
    // Note: iLast == -1 is only possible if p2 = (0, 0), which means:
    // - p2 is the minimim possible param
    // - therefore `p1 >= p2`
    // - therefore `order != Order::Less`
    // - therefore `iClosed` is true, otherwise `isReducedToAPoint` would
    //   be true and we would have already fast-returned
    //
    Int iLast = i2;
    if (p2.u() == 0) {
        iLast -= 1;
    }

    // Sets iPastLast = iLast + 1 to work on a semi-open range [iFirst, iPastLast).
    //
    // Note that the above range is only valid if iFirst <= iPastLast,
    // which is not always the case for closed curves.
    //
    // If order == Less:
    // - p1 < p2
    // - so i1 <= i2
    // - so i1 + 1 <= i2 + 1
    // - so iFirst <= i2 + 1
    // - with iPastLast = i2 + 1 if u2 != 0
    //                  = i2     otherwise
    // - so either iFirst <= iPastLast or u2 == 0
    // - but u2 == 0 implies:
    //   - i1 < i2 (since p1 < p2 and there is no possible u1 < u2)
    //   - so i1 + 1 < i2 + 1
    //   - so iFirst < i2 + 1
    //   - so iFirst <= i2
    //   - so iFirst <= iPastLast
    //
    // If order == Equal:
    // - isClosed is true
    // - we may have iFirst == iPastLast + 1:
    //
    //        0       1    2,p1,p2    3       4
    //        o-------o-------o-------o-------o------  (numKnots == 5)
    //                        ^       ^                (numSegments == 5)
    //                   iPastLast  iFirst
    //
    Int iPastLast = iLast + 1;
    VGC_ASSERT(iPastLast >= 0);
    VGC_ASSERT(iPastLast <= numKnots); // Equality possible for closed strokes
    VGC_ASSERT(iFirst <= iPastLast || order != Order::Less);

    // Lambda that adds all knots in the range [first, pastLast) to the new stroke.
    // Precondition: first <= last.
    auto extend = [&, hasWidths](Int first, Int pastLast) {
        positions.extend(positions_.begin() + first, positions_.begin() + pastLast);
        if (hasWidths) {
            widths.extend(widths_.begin() + first, widths_.begin() + pastLast);
        }
    };

    // Add all knots between the first and last knot of the substroke.
    switch (order) {
    case Order::Less: {
        if (numWraps == 0) {
            // Open stroke or closed stroke with p1 < p2 and numWraps == 0
            extend(iFirst, iPastLast);
        }
        else {
            VGC_ASSERT(isClosed());
            extend(iFirst, numKnots);
            for (Int j = 1; j < numWraps; ++j) {
                extend(0, numKnots);
            }
            extend(0, iPastLast);
        }
        break;
    }
    case Order::Equal: {
        VGC_ASSERT(isClosed());
        VGC_ASSERT(numWraps > 0);
        extend(iFirst, numKnots);
        for (Int j = 1; j < numWraps; ++j) {
            extend(0, numKnots);
        }
        extend(0, iPastLast);
        break;
    }
    case Order::Greater: {
        VGC_ASSERT(isClosed());
        extend(iFirst, numKnots);
        for (Int j = 0; j < numWraps; ++j) {
            extend(0, numKnots);
        }
        extend(0, iPastLast);
        break;
    }
    }

    // Add the last knot
    StrokeSample2d s2 = eval(p2);
    positions.append(s2.position());
    if (hasWidths) {
        widths.append(s2.width());
    }

    newStroke->setPositions(positions);
    if (!hasConstantWidth()) {
        newStroke->setWidths(widths);
    }
    else {
        newStroke->setConstantWidth(constantWidth());
    }

    return result;
}

void AbstractInterpolatingStroke2d::reverse_() {
    std::reverse(positions_.begin(), positions_.end());
    onPositionsChanged_();
    if (!hasConstantWidth_) {
        std::reverse(widths_.begin(), widths_.end());
        onWidthsChanged_();
    }
}

namespace {

template<typename T, typename TRange>
void extend_(core::Array<T>& dest, const TRange& range, bool reverse, bool skipFirst) {
    Int i = skipFirst ? 1 : 0;
    if (!reverse) {
        dest.extend(range.begin() + i, range.end());
    }
    else {
        dest.extend(range.rbegin() + i, range.rend());
    }
}

} // namespace

void AbstractInterpolatingStroke2d::assignFromConcat_(
    const AbstractStroke2d* a_,
    bool directionA,
    const AbstractStroke2d* b_,
    bool directionB,
    bool smoothJoin) {

    auto a = dynamic_cast<const AbstractInterpolatingStroke2d*>(a_);
    auto b = dynamic_cast<const AbstractInterpolatingStroke2d*>(b_);
    if (!a || !b) {
        VGC_WARNING(
            LogVgcGeometry,
            "AbstractInterpolatingStroke2d::assignFromConcat_() expected "
            "source strokes to be of type AbstractInterpolatingStroke2d.");
        return;
    }

    Int nA = a->positions_.length();
    Int nB = b->positions_.length();
    double cwA = a->constantWidth();
    double cwB = b->constantWidth();

    bool newStrokeHasVaryingWidth =
        !a->hasConstantWidth_ || !b->hasConstantWidth_ || (cwA != cwB);

    Vec2dArray newPositions;
    core::DoubleArray newWidths;

    newPositions.reserve(nA + nB);
    if (newStrokeHasVaryingWidth) {
        newWidths.reserve(nA + nB);
    }
    else {
        newWidths.append(cwA);
    }

    if (nA > 0) {
        if (newStrokeHasVaryingWidth) {
            if (a->hasConstantWidth_) {
                newWidths.extend(nA, cwA);
            }
            else {
                extend_(newWidths, a->widths_, !directionA, false);
            }
        }
        extend_(newPositions, a->positions_, !directionA, false);
    }

    if (nB > 0) {
        bool skipFirst = false;
        if (smoothJoin && nA > 0) {
            Vec2d bFirst = directionB ? b->positions_.first() : b->positions_.last();
            if (newPositions.last() == bFirst) {
                skipFirst = true;
            }
        }
        if (newStrokeHasVaryingWidth) {
            if (b->hasConstantWidth_) {
                newWidths.extend(skipFirst ? nB - 1 : nB, cwB);
            }
            else {
                extend_(newWidths, b->widths_, !directionB, skipFirst);
            }
        }
        extend_(newPositions, b->positions_, !directionB, skipFirst);
    }

    hasConstantWidth_ = !newStrokeHasVaryingWidth;
    positions_ = std::move(newPositions);
    widths_ = std::move(newWidths);
    onPositionsChanged_();
    onWidthsChanged_();
}

namespace {

template<typename TPoint, typename PositionGetter, typename WidthGetter>
[[maybe_unused]] Int filterSculptPointsWidthStep(
    core::Span<TPoint> points,
    core::IntArray& indices,
    Int intervalStart,
    bool isClosed,
    double tolerance,
    PositionGetter positionGetter,
    WidthGetter widthGetter) {

    VGC_UNUSED(isClosed);
    VGC_UNUSED(tolerance);

    Int i = intervalStart;
    Int endIndex = indices[i + 1];
    while (indices[i] != endIndex) {
        Int iA = indices[i];
        Int iB = indices[i + 1];
        if (iA + 1 == iB) {
            ++i;
            continue;
        }

        Vec2d a = positionGetter(points[iA], iA);
        Vec2d b = positionGetter(points[iB], iB);
        double wA = widthGetter(points[iA], iA);
        double wB = widthGetter(points[iB], iB);

        Vec2d ab = b - a;
        double abLen = ab.length();

        // Compute which sample between A and B has an offset point
        // furthest from the offset line AB.
        Int maxOffsetDiffPointIndex = -1;
        if (abLen > 0) {
            Vec2d dir = ab / abLen;
            // Catmull-Rom is not a linear interpolation, since we don't
            // compute the ground truth here we thus need a bigger threshold.
            // For now we use X% of the width from linear interp. value.
            for (Int j = iA + 1; j < iB; ++j) {
                Vec2d p = positionGetter(points[j], j);
                Vec2d ap = p - a;
                double t = ap.dot(dir) / abLen;
                double w = core::fastLerp(wA, wB, t);
                double dist = (std::abs)(w - widthGetter(points[j], j));
                double maxOffsetDiff = w * 0.05;
                if (dist > maxOffsetDiff) {
                    maxOffsetDiff = dist;
                    maxOffsetDiffPointIndex = j;
                }
            }
        }
        // If the distance exceeds the tolerance, then recurse.
        // Otherwise, stop the recursion and move one to the next segment.
        if (maxOffsetDiffPointIndex != -1) {
            // Add sample to the list of selected samples
            indices.insert(i + 1, maxOffsetDiffPointIndex);
        }
        else {
            ++i;
        }
    }
    return i;
}

template<typename TPoint, typename PositionGetter, typename WidthGetter>
Int filterPointsStep(
    core::Span<TPoint> points,
    core::IntArray& indices,
    Int intervalStart,
    bool isClosed,
    double tolerance,
    PositionGetter positionGetter,
    WidthGetter widthGetter) {

    Int i = intervalStart;
    Int endIndex = indices[i + 1];
    while (indices[i] != endIndex) {
        Int iA = indices[i];
        Int iB = indices[i + 1];
        if (iA + 1 == iB) {
            ++i;
            continue;
        }

        Vec2d a = positionGetter(points[iA], iA);
        Vec2d b = positionGetter(points[iB], iB);
        Vec2d ab = b - a;
        double abLen = ab.length();

        // Compute which sample between A and B has a position
        // furthest from the line AB.
        double maxDist = tolerance;
        Int maxDistPointIndex = -1;
        if (abLen > 0) {
            for (Int j = iA + 1; j < iB; ++j) {
                Vec2d p = positionGetter(points[j], j);
                Vec2d ap = p - a;
                double dist = (std::abs)(ab.det(ap) / abLen);
                if (dist > maxDist) {
                    maxDist = dist;
                    maxDistPointIndex = j;
                }
            }
        }
        else {
            for (Int j = iA + 1; j < iB; ++j) {
                Vec2d p = positionGetter(points[j], j);
                Vec2d ap = p - a;
                double dist = ap.length();
                if (dist > maxDist) {
                    maxDist = dist;
                    maxDistPointIndex = j;
                }
            }
        }

        // If the furthest point is too far from AB, then recurse.
        // Otherwise, stop the recursion and move one to the next segment.
        if (maxDistPointIndex != -1) {
            // Add sample to the list of selected samples
            indices.insert(i + 1, maxDistPointIndex);
        }
        else {
            i = filterSculptPointsWidthStep(
                points, indices, i, isClosed, tolerance, positionGetter, widthGetter);
            //++i;
        }
    }
    return i;
}

} // namespace

void AbstractInterpolatingStroke2d::assignFromAverage_(
    core::ConstSpan<const AbstractStroke2d*> strokes,
    core::ConstSpan<bool> directions,
    core::ConstSpan<double> uOffsets,
    bool areClosed) {

    if (strokes.length() == 0) {
        setPositions(Vec2dArray{});
        setConstantWidth(0);
        return;
    }

    struct ThickPoint {
        Vec2d pos = {};
        double width = 0;
        double u = 0;

        ThickPoint() noexcept = default;

        ThickPoint(Vec2d pos, double width, double u)
            : pos(pos)
            , width(width)
            , u(u) {
        }

        explicit ThickPoint(const StrokeSample2d& sample, double arclen)
            : pos(sample.position())
            , width(sample.halfwidth(0) + sample.halfwidth(1))
            , u(sample.s() / arclen) {
        }

        ThickPoint lerp(const ThickPoint& other, double t, double newU) {
            return ThickPoint{
                (1. - t) * pos + t * other.pos, (1. - t) * width + t * other.width, newU};
        }
    };

    Int nStroke = strokes.length();
    for (Int iStroke = 0; iStroke < nStroke; ++iStroke) {
        if (strokes[iStroke]->isClosed() != areClosed) {
            VGC_WARNING(
                LogVgcGeometry,
                "AbstractInterpolatingStroke2d::assignFromAverage_() expected "
                "all source strokes to be {} according to the `areClosed` argument.",
                (areClosed ? "closed" : "open"));
            return;
        }
    }

    core::Array<StrokeSample2dArray> sampleArrays;
    sampleArrays.reserve(nStroke);

    // offset/reverse arrays
    for (Int iStroke = 0; iStroke < nStroke; ++iStroke) {

        // get samples
        {
            StrokeSampling2d sampling =
                strokes[iStroke]->computeSampling(CurveSamplingQuality::AdaptiveLow);

            sampleArrays.append(sampling.stealSamples());
        }
        StrokeSample2dArray& samples = sampleArrays.last();

        Int nSample = samples.length();
        if (nSample < 2) {
            continue;
        }

        double arclength = samples.last().s();

        if (!directions[iStroke]) {
            std::reverse(samples.begin(), samples.end());
            for (StrokeSample2d& sample : samples) {
                sample.setS(arclength - sample.s());
            }
        }

        if (areClosed && !uOffsets.isEmpty() && uOffsets[iStroke] > 0) {

            double sOffset = uOffsets[iStroke] * arclength;

            Int iNewStart = 1;

            for (; iNewStart < nSample; ++iNewStart) {
                if (samples[iNewStart].s() >= sOffset) {
                    break;
                }
            }

            const geometry::StrokeSample2d& s1 = samples[iNewStart];

            if (s1.s() != sOffset) {
                const geometry::StrokeSample2d& s0 = samples[iNewStart - 1];
                ThickPoint p0(s0, arclength);
                ThickPoint p1(s1, arclength);
                double uOffset = sOffset / arclength;
                double t = (uOffset - p0.u) / (p1.u - p0.u);
                ThickPoint tpAtOffset = p0.lerp(p1, t, uOffset);
                samples.emplace(
                    iNewStart,
                    tpAtOffset.pos,
                    Vec2d(),
                    Vec2d(),
                    tpAtOffset.width * 0.5,
                    -1,
                    0,
                    sOffset);
            }

            // remove last
            samples.pop();

            // rotate
            for (Int i = 0; i < iNewStart; ++i) {
                samples.getUnchecked(i).offsetS(-sOffset + arclength);
            }
            for (Int i = iNewStart; i < nSample; ++i) {
                samples.getUnchecked(i).offsetS(-sOffset);
            }
            std::rotate(samples.begin(), samples.begin() + iNewStart, samples.end());

            // rebuild last
            ThickPoint tpFirst(samples.first(), arclength);
            samples.emplaceLast(
                tpFirst.pos, Vec2d(), Vec2d(), tpFirst.width * 0.5, -1, 0, arclength);
        }
    }

    core::Array<ThickPoint> newPoints;
    core::Array<ThickPoint> tmp;

    double arclength0 = sampleArrays[0].last().s();
    newPoints.reserve(sampleArrays[0].length());
    if (arclength0 > 0) {
        for (const StrokeSample2d& sample : sampleArrays[0]) {
            newPoints.emplaceLast(sample, arclength0);
        }
    }
    else {
        newPoints.emplaceLast(sampleArrays[0].first(), 1.);
    }

    for (Int iStroke = 1; iStroke < nStroke; ++iStroke) {

        const StrokeSample2dArray& samples = sampleArrays[iStroke];
        double arclength = samples.last().s();

        if (newPoints.length() < 2) {
            ThickPoint tp0 = newPoints.pop();
            Int n = samples.length();
            newPoints.reserve(n);
            for (const StrokeSample2d& sample : samples) {
                ThickPoint tp(sample, arclength);
                tp.pos += tp0.pos;
                tp.width += tp0.width;
                newPoints.append(tp);
            }
        }
        else if (samples.length() < 2) {
            ThickPoint tp0(samples[0], 1.);
            for (ThickPoint& tp : newPoints) {
                tp.pos += tp0.pos;
                tp.width += tp0.width;
            }
        }
        else {
            Int n0 = newPoints.length();
            Int n1 = samples.length();
            // curves share 2 values of u (0 at start and 1 at end).
            Int n = std::max<Int>(0, n0 - 2) + std::max<Int>(0, n1 - 2) + 2;

            // Compute an interpolation between the two curve with the given direction.

            ThickPoint p0a = newPoints.first();
            ThickPoint p1a(samples.first(), arclength);

            ThickPoint p01a = p0a;
            p01a.pos += p1a.pos;
            p01a.width += p1a.width;

            tmp.reserve(n);
            tmp.append(p01a);

            Int i0 = 1;
            Int i1 = 1;

            for (Int i = 1; i < n - 1; ++i) {
                const ThickPoint& p0b = newPoints[i0];
                ThickPoint p1b(samples[i1], arclength);
                bool canIterate0 = i0 < n0 - 1;
                bool canIterate1 = i1 < n1 - 1;
                if (canIterate1 && (p0b.u > p1b.u || !canIterate0)) {
                    double t = (p1b.u - p0a.u) / (p0b.u - p0a.u);
                    ThickPoint tp = p0a.lerp(p0b, t, p1b.u);
                    tp.pos += p1b.pos;
                    tp.width += p1b.width;
                    tmp.append(tp);
                    p1a = p1b;
                    ++i1;
                }
                else if (canIterate0) {
                    double t = (p0b.u - p1a.u) / (p1b.u - p1a.u);
                    ThickPoint tp = p1a.lerp(p1b, t, p0b.u);
                    tp.pos += p0b.pos;
                    tp.width += p0b.width;
                    tmp.append(tp);
                    p0a = p0b;
                    ++i0;
                }
                else {
                    // shouldn't happen if `n` is correct.
                    break;
                }
            }

            if (areClosed) {
                tmp.append(tmp.first());
            }
            else {
                const ThickPoint& tp0 = newPoints.last();
                ThickPoint tp(samples.last(), arclength);
                tp.pos += tp0.pos;
                tp.width += tp0.width;
                tp.u = 1.;
                tmp.append(tp);
            }

            newPoints.swap(tmp);
            tmp.clear();
        }
    }

    for (ThickPoint& tp : newPoints) {
        double d = core::narrow_cast<double>(nStroke);
        tp.pos /= d;
        tp.width /= d;
    }

    double minWidth = core::DoubleMax;
    double maxWidth = 0;
    for (Int i = 0; i < newPoints.length(); ++i) {
        double w = newPoints[i].width;
        if (w < minWidth) {
            minWidth = w;
        }
        if (w > maxWidth) {
            maxWidth = w;
        }
    }

    core::Array<Int> indices = {0, newPoints.length() - 1};
    filterPointsStep<ThickPoint>(
        newPoints,
        indices,
        0,
        areClosed,
        minWidth * 0.2,
        [](const ThickPoint& tp, Int) { return tp.pos; },
        [](const ThickPoint& tp, Int) { return tp.width; });

    if (areClosed && indices.length() > 1) {
        indices.removeLast();
    }

    core::Array<Vec2d> newPositions;
    core::Array<double> newWidths;
    for (Int idx : indices) {
        const ThickPoint& tp = newPoints[idx];
        newPositions.append(tp.pos);
        newWidths.append(tp.width);
    }

    setPositions(newPositions);
    setWidths(newWidths);
}

namespace {

void snapLinearInArclength_(
    Vec2dArray& positions,
    core::DoubleArray& arclengths,
    const Vec2d& snapStartPosition,
    const Vec2d& snapEndPosition) {

    Int n = positions.length();
    VGC_ASSERT(n > 0);
    VGC_ASSERT(arclengths.length() == n);

    Vec2d d1 = snapStartPosition - positions.first();
    positions.first() = snapStartPosition;

    Vec2d d2 = snapEndPosition - positions.last();
    positions.last() = snapEndPosition;

    double l = arclengths.last();
    if (d1 == d2 || l <= 0) {
        for (Int i = 1; i < n - 1; ++i) {
            geometry::Vec2d& p = positions.getUnchecked(i);
            p += d1;
        }
    }
    else {
        double invL = 1 / l;
        for (Int i = 1; i < n - 1; ++i) {
            double s = arclengths.getUnchecked(i);
            double t = s * invL;
            geometry::Vec2d& p = positions.getUnchecked(i);
            p += core::fastLerp(d1, d2, t);
        }
    }
}

geometry::Vec2d cubicEaseOut(const geometry::Vec2d& value, double t) {
    double x = 1 - t;
    return value * x * x * x;
}

void snapFalloff_(
    Vec2dArray& positions,
    core::DoubleArray& arclengths,
    const Vec2d& snapStartPosition,
    const Vec2d& snapEndPosition,
    double falloff) {

    Int n = positions.length();
    VGC_ASSERT(n > 0);
    VGC_ASSERT(arclengths.length() == n);

    Vec2d d1 = snapStartPosition - positions.first();
    positions.first() = snapStartPosition;

    Vec2d d2 = snapEndPosition - positions.last();
    positions.last() = snapEndPosition;

    if (falloff <= 0) {
        return;
    }
    double invFalloff = 1 / falloff;
    double l = arclengths.last();

    if (2 * falloff < l) {
        // In this case, the start and end snap falloffs do not overlap and can
        // therefore be performed independently.

        // Start snap
        if (d1 != Vec2d(0, 0)) {
            for (Int i = 1; i <= n - 2; ++i) {
                double s = arclengths.getUnchecked(i);
                double t1 = s * invFalloff;
                if (t1 < 1) {
                    geometry::Vec2d& p = positions.getUnchecked(i);
                    p += cubicEaseOut(d1, t1);
                }
                else {
                    break;
                }
            }
        }

        // End snap
        if (d2 != Vec2d(0, 0)) {
            for (Int i = n - 2; i >= 1; --i) {
                double s = arclengths.getUnchecked(i);
                double t2 = (l - s) * invFalloff;
                if (t2 < 1) {
                    geometry::Vec2d& p = positions.getUnchecked(i);
                    p += cubicEaseOut(d2, t2);
                }
                else {
                    break;
                }
            }
        }
    }
    else {
        // In this case, the start and end snap falloffs overlap, and we
        // perform an arclength-linear interpolation between the two. When
        // falloff tends to infinity, this method becomes equivalent to the
        // LinearInArclength snap type.

        double invL = 1 / l;
        for (Int i = 1; i <= n - 2; ++i) {
            double s = arclengths.getUnchecked(i);
            double t = s * invL;
            double t1 = s * invFalloff;
            double t2 = (l - s) * invFalloff;
            geometry::Vec2d& p = positions.getUnchecked(i);
            geometry::Vec2d d1_ = t1 < 1 ? cubicEaseOut(d1, t1) : Vec2d(0, 0);
            geometry::Vec2d d2_ = t2 < 1 ? cubicEaseOut(d2, t2) : Vec2d(0, 0);
            p += core::fastLerp(d1_, d2_, t);
        }
    }
}

} // namespace

void AbstractInterpolatingStroke2d::snap_(
    const Vec2d& snapStartPosition,
    const Vec2d& snapEndPosition,
    CurveSnapSettings settings) {

    // Nothing to do if the stroke is already snapped.
    //
    if (positions_.length() > 0                    //
        && positions_.first() == snapStartPosition //
        && positions_.last() == snapEndPosition) {

        return;
    }

    if (positions_.length() < 2) {

        // When the stroke currently has zero or one knot, we explicitly set
        // its new knots. In particular, this handles the case when a 1-knot
        // zero-length stroke becomes a 2-knot non-zero-length.
        //
        double width0 = widths_.isEmpty() ? 0 : widths_.first();
        positions_.clear();
        widths_.clear();
        // first knot
        {
            positions_.append(snapStartPosition);
            if (!hasConstantWidth()) {
                widths_.append(width0);
            }
        }
        // second knot (only necessary for non-zero-length strokes)
        if (snapStartPosition != snapEndPosition) {
            positions_.append(snapEndPosition);
            if (!hasConstantWidth()) {
                widths_.append(width0);
            }
        }
    }
    else {
        // Otherwise, if the stroke already has at least two knots, we perform
        // the snapping.

        // Compute arclengths.
        // XXX: should this be cached?
        //
        core::DoubleArray arclengths;
        computeArclengths_(arclengths);

        switch (settings.type()) {
        case CurveSnapType::LinearInArclength: {
            snapLinearInArclength_(
                positions_, arclengths, snapStartPosition, snapEndPosition);
            break;
        }
        case CurveSnapType::Falloff: {
            snapFalloff_(
                positions_,
                arclengths,
                snapStartPosition,
                snapEndPosition,
                settings.falloff());
            break;
        }
        }
    }

    onPositionsChanged_();
}

namespace {

struct SculptPoint {
    SculptPoint() noexcept = default;

    SculptPoint(const Vec2d& pos, double width, double d, double s) noexcept
        : pos(pos)
        , width(width)
        , d(d)
        , s(s) {
    }

    Vec2d pos = {};
    // halfwidths are not supported yet.
    double width = 0;
    // signed distance in arclength from central sculpt point.
    double d = 0;
    // position in arclength on the related edge.
    double s = 0;
};

struct SculptSampling {
    core::Array<SculptPoint> sculptPoints;
    // sampling boundaries in arclength from central sculpt point.
    Vec2d cappedRadii = {};
    // distance between sculpt points that are before the middle sculpt point
    double ds0 = 0;
    // distance between sculpt points that are after the middle sculpt point
    double ds1 = 0;
    double radius = 0;
    // s of the middle sculpt point in the sampled curve.
    double sMiddle = 0;
    // index of the sculpt point closest to sMiddle
    Int closestSculptPointIndex = -1;
    // is sculpt interval closed ?
    bool isClosed = false;
    // is sculpt interval touching the start knot?
    // For closed curves, this is the same as isRadiusOverlappingEnd.
    // For open curves, this means that the "before radius" was capped.
    bool isRadiusOverlappingStart = false;
    // is sculpt interval touching the end knot?
    // For closed curve, this is the same as isRadiusOverlappingEnd.
    // For open curves, this means that the "after radius" was capped.
    bool isRadiusOverlappingEnd = false;
};

// Computes a uniform sampling of the subset of the curve centered around the
// closest curve point of s `sMiddle` and extending on both sides by `radius` in
// arclength (if possible, otherwise capped at the endpoints).
//
// Assumes:
// - radius > 0
// - sMiddle is in [samples.first.s(), samples.last.s()].
//
void computeSculptSampling(
    SculptSampling& outSampling,
    StrokeSample2dArray& samples,
    double sMiddle,
    double radius,
    double maxDs,
    bool isClosed,
    bool allowHavingNoSculptPointAtSMiddleToKeepDsUniform) {

    core::Array<SculptPoint>& sculptPoints = outSampling.sculptPoints;

    Int numSamples = samples.length();
    VGC_ASSERT(numSamples > 0);
    VGC_ASSERT(samples.first().s() == 0);

    // First, we determine how many sculpt points we want (and the
    // corresponding ds), based on the curve length, the location of the middle
    // sculpt point in the curve, the sculpt radius, and maxDS.

    Int numSculptPointsBeforeMsp = 0;
    Int numSculptPointsAfterMsp = 0;
    Vec2d cappedRadii = {};
    double ds0 = 0;
    double ds1 = 0;
    double curveLength = samples.last().s(); // XXX substract samples.first().s()?
    double sMsp = sMiddle;
    if (!isClosed) {
        // Compute ds such that it is no larger than maxDs, and such that radius is
        // a multiple of ds (if "uncapped", that is, if the radius doesn't
        // extend further than one of the endpoints of the curve).
        //
        double n = std::ceil(radius / maxDs);
        double ds = radius / n;
        double sBeforeMsp = sMiddle; // XXX substract samples.first().s()?
        if (radius < sBeforeMsp) {
            // uncapped before
            numSculptPointsBeforeMsp = core::narrow_cast<Int>(n);
            cappedRadii[0] = radius;
            outSampling.isRadiusOverlappingStart = false;
        }
        else {
            // capped before
            numSculptPointsBeforeMsp = core::floor_cast<Int>(sBeforeMsp / ds);
            cappedRadii[0] = sBeforeMsp;
            outSampling.isRadiusOverlappingStart = true;
        }
        double sAfterMsp = curveLength - sMiddle;
        if (radius < sAfterMsp) {
            // uncapped after
            numSculptPointsAfterMsp = core::narrow_cast<Int>(n);
            cappedRadii[1] = radius;
            outSampling.isRadiusOverlappingEnd = false;
        }
        else {
            // capped after
            numSculptPointsAfterMsp = core::floor_cast<Int>(sAfterMsp / ds);
            cappedRadii[1] = sAfterMsp;
            outSampling.isRadiusOverlappingEnd = true;
        }

        if (allowHavingNoSculptPointAtSMiddleToKeepDsUniform) {
            double s = cappedRadii[0] + cappedRadii[1];
            ds = s / (numSculptPointsBeforeMsp + numSculptPointsAfterMsp);
            ds0 = ds;
            ds1 = ds;
            sMsp = (sMiddle - cappedRadii[0]) + ds0 * numSculptPointsBeforeMsp;
        }
        else {
            ds0 = cappedRadii[0] / std::max<Int>(1, numSculptPointsBeforeMsp);
            ds1 = cappedRadii[1] / std::max<Int>(1, numSculptPointsAfterMsp);
        }
    }
    else { // isClosed

        // If the curve is closed, then we need to determine whether the
        // sampling itself is closed (the sculpt interval covers the full
        // curve) or open (the sculpt internal only covers a subset of the
        // curve, potentially including the start knot).
        //
        // Note: having an "almost closed" sampling is error-prone due to
        // floating point imprecisions (possible loss of precision when
        // wrapping s values may cause order inconsistencies between
        // wrapped(sMsp - n * ds) and wrapped(sMsp - n + ds)). Therefore, we
        // use a thresold to "snap the sampling to a closed sampling" when the
        // sampling is nearly closed.
        //
        double curveHalfLength = curveLength * 0.5;
        double epsilon = maxDs / 100;
        if (curveHalfLength < radius + epsilon) {

            // If the sculpt interval encompasses the full curve and the curve
            // is closed then we want to produce a closed sculpt sampling.
            //
            // In order to have the sculpt points all exactly spaced by `ds`
            // and looping around, we have to adjust ds and
            // numSculptSamplesPerSide such that curveLength is a multiple of
            // ds.
            //
            //     increasing s
            //    -------------->
            //      ds ds ds ds       o  middle sculpt point
            //     b--b--o--a--a      b  sculpt point before (numBefore = n     = 5)
            //   ds|           |ds    a  scuplt point after  (numAfter  = n - 1 = 4)
            //     b--b--b--a--a      curveLength = 2 * n * ds =
            //      ds ds ds ds                   = (numBefore + numAfter + 1) * ds
            //
            double n = std::ceil(curveHalfLength / maxDs);
            numSculptPointsBeforeMsp = core::narrow_cast<Int>(n);
            numSculptPointsAfterMsp = std::max<Int>(numSculptPointsBeforeMsp - 1, 0);
            ds0 = curveHalfLength / n;
            ds1 = ds0;
            outSampling.isClosed = true;
            outSampling.isRadiusOverlappingStart = true;
            outSampling.isRadiusOverlappingEnd = true;
            cappedRadii[0] = curveHalfLength;
            cappedRadii[1] = curveHalfLength;
        }
        else {
            // If the curve is closed then we do not cap the radii to the input interval.
            //
            double n = std::ceil(radius / maxDs);
            numSculptPointsBeforeMsp = core::narrow_cast<Int>(n);
            numSculptPointsAfterMsp = core::narrow_cast<Int>(n);
            ds0 = radius / n;
            ds1 = ds0;
            cappedRadii[0] = radius;
            cappedRadii[1] = radius;
            // Find out if interval overlaps the start (=end) point.
            if (sMiddle - radius <= 0 || sMiddle + radius >= curveLength) {
                outSampling.isRadiusOverlappingStart = true;
                outSampling.isRadiusOverlappingEnd = true;
            }
            else {
                outSampling.isRadiusOverlappingStart = false;
                outSampling.isRadiusOverlappingEnd = false;
            }
        }
    }
    Int targetNumSculptPoints = numSculptPointsBeforeMsp + numSculptPointsAfterMsp + 1;

    // Once we know ds and how many sculpt points we want, let's generate them
    // by resampling the samples linearly.

    if (curveLength == 0) {
        sculptPoints.emplaceLast(
            samples[0].position(), 2.0 * samples[0].halfwidth(0), 0, 0);
    }
    else {
        bool isDone = false;
        Int spEndIndex = numSculptPointsAfterMsp + 1;
        Int spIndex = -numSculptPointsBeforeMsp;

        double sculptPointSOffset = 0;
        if (isClosed && sMsp + spIndex * ds0 < 0) {
            sculptPointSOffset = curveLength;
        }
        double nextSculptPointS = sculptPointSOffset + sMsp + spIndex * ds0;
        bool isOpenAndOverlappingStart =
            !isClosed && outSampling.isRadiusOverlappingStart;
        if (nextSculptPointS < 0 || isOpenAndOverlappingStart) {
            // Fix potential floating point error that made it overshoot or
            // undershoot the start of the curve.
            nextSculptPointS = samples.first().s(); // = 0
        }

        core::Array<Vec2d> positions(numSamples, core::noInit);
        core::Array<double> widths(numSamples, core::noInit);
        for (Int i = 0; i < (isClosed ? numSamples - 1 : numSamples); ++i) {
            positions[i] = samples[i].position();
            widths[i] = samples[i].halfwidth(0) * 2.0;
        }

        const Int maxIter = isClosed ? 2 : 1; // If the curve is closed we allow 2 passes.
        for (Int iter = 0; iter < maxIter; ++iter) {
            // Iterate over sample segments
            // Loop invariant: `nextSculptPointS >= sa1->s()` (as long as `sa2->s() >= sa1->s()`)
            //
            const StrokeSample2d* sa1 = &samples[0];
            for (Int iSample2 = 1; iSample2 < numSamples && !isDone; ++iSample2) {
                const StrokeSample2d* sa2 = &samples[iSample2];
                const double d = sa2->s() - sa1->s();
                // Skip the segment if it is degenerate.
                if (d > 0) {
                    double invD = 1.0 / d;

                    while (nextSculptPointS <= sa2->s()) {
                        // Sample a sculpt point at t in segment [sa1:0, sa2:1].
                        double t = (nextSculptPointS - sa1->s()) * invD;

                        //Vec2d p = stroke.evalPosition(t);
                        //double w = stroke.evalHalfwidths(t) * 2.0;

                        double u = 1.0 - t;
                        Vec2d p = u * sa1->position() + t * sa2->position();
                        double w = (u * sa1->halfwidth(0) + t * sa2->halfwidth(0)) * 2.0;

                        double distanceToMiddle;
                        if (isClosed) {
                            // If the curve is closed, s can wrap so we need to compute
                            // the distance based on a multiple of the index, which works
                            // because we always have sMsp = sMiddle for closed curves
                            distanceToMiddle = spIndex * ds0;
                        }
                        else {
                            // If the curve is open, then s doesn't wrap so we can
                            // directly compute the distance as a difference
                            distanceToMiddle = nextSculptPointS - sMiddle;
                        }
                        sculptPoints.emplaceLast(
                            p, w, distanceToMiddle, nextSculptPointS);
                        // prepare next
                        ++spIndex;
                        double sRel = spIndex < 0 ? spIndex * ds0 : spIndex * ds1;
                        nextSculptPointS = sculptPointSOffset + sMsp + sRel;
                        if (spIndex >= spEndIndex - 1) {
                            if (spIndex == spEndIndex) {
                                // All sculpt points have been sampled.
                                isDone = true;
                                break;
                            }
                            else { // spIndex == spEndIndex - 1
                                if (!isClosed || iter == 1) {
                                    bool isOpenAndOverlappingEnd =
                                        !isClosed && outSampling.isRadiusOverlappingEnd;
                                    if (nextSculptPointS > samples.last().s()
                                        || isOpenAndOverlappingEnd) {
                                        // Fix potential floating point error that made it
                                        // overshoot or undershoot the end of the curve.
                                        nextSculptPointS = samples.last().s();
                                    }
                                }
                            }
                        }
                    }
                }
                sa1 = sa2;
            }
            if (!isDone) {
                if (isClosed && iter == 0) {
                    // We loop only if the curve is closed
                    sculptPointSOffset -= curveLength;
                    nextSculptPointS -= curveLength;
                }
            }
            else {
                break;
            }
        }
    }

    VGC_ASSERT(targetNumSculptPoints > 0);
    if (sculptPoints.length() != targetNumSculptPoints) {

        // This may indicate either a bug in this function, or dubious
        // parameters passed to this function (e.g., sMiddle not in
        // [samples.first.s(), samples.last.s()], or incorrect samples[i].s())
        //
        VGC_WARNING(
            LogVgcGeometry,
            "Fewer sculpt points generated ({}) than targeted ({}).",
            sculptPoints.length(),
            targetNumSculptPoints);

        // We really want at least one sculpt point, so we add one if there is
        // none. However, it's not a critical issue not to have exactly
        // targetNumSculptPoints, so we don't try to recover from this.
        //
        if (sculptPoints.isEmpty()) {
            sculptPoints.emplaceLast(
                samples[0].position(), 2.0 * samples[0].halfwidth(0), 0, 0);
        }
    }

    outSampling.closestSculptPointIndex = numSculptPointsBeforeMsp;
    outSampling.cappedRadii = cappedRadii;
    outSampling.ds0 = ds0;
    outSampling.ds1 = ds1;
    outSampling.radius = radius;
    outSampling.sMiddle = sMiddle;
}

} // namespace

} // namespace vgc::geometry

template<>
struct fmt::formatter<vgc::geometry::SculptPoint> : fmt::formatter<double> {
    template<typename FormatContext>
    auto format(const vgc::geometry::SculptPoint& point, FormatContext& context) {
        auto&& out = context.out();
        format_to(out, "{{pos=(");
        fmt::formatter<double>::format(point.pos[0], context);
        format_to(out, ", ");
        fmt::formatter<double>::format(point.pos[1], context);
        format_to(out, "), width=");
        fmt::formatter<double>::format(point.width, context);
        format_to(out, ", d=");
        fmt::formatter<double>::format(point.d, context);
        format_to(out, ", s=");
        fmt::formatter<double>::format(point.s, context);
        return format_to(out, "}}");
    }
};

// Same as default Array formatter but with newlines between elements,
// and allowing same format parameters as double
template<>
struct fmt::formatter<vgc::core::Array<vgc::geometry::SculptPoint>>
    : fmt::formatter<vgc::geometry::SculptPoint> {

    template<typename FormatContext>
    auto format( //
        const vgc::core::Array<vgc::geometry::SculptPoint>& array,
        FormatContext& context) {

        auto&& out = context.out();
        format_to(out, "[\n    ");
        bool first = true;
        for (const auto& point : array) {
            if (first) {
                first = false;
            }
            else {
                format_to(out, ",\n    ");
            }
            fmt::formatter<vgc::geometry::SculptPoint>::format(point, context);
        }
        return format_to(out, "]");
    }
};

namespace vgc::geometry {

namespace {

// cubicEaseInOut(t)
//       ^
//     1 |   .-
//       |_.´
//     0 +------> t
//       0    1
//
double cubicEaseInOut(double t) {
    double t2 = t * t;
    return -2 * t * t2 + 3 * t2;
}

// In order to handle boundary conditions when computing a weighted average, we
// compute the weighted average as if we repeatedly applied a central symmetry
// to all the sculpt points:
//
// Original curve:
//                                                            curve
//                                                             end
//                      curve   MSP    ,------------------------|
//                      start  ,-x----'
//                        |---'
//                  <------------|------------>
//                      radius       radius
//
// Sculpt points:
//
//                              MSP    ,------|
//                             ,-x----'
//                        |---'
//                        <------|------------>
//                        capped     capped
//                        radii[0]   radii[1]
//
// 2D central symmetry of sculpt points at both sides (similar for width):
// (repeated infinitely many times... or at least until length > 2 * radius):
//
//                                                            ,---|···
//                                                    ,------'
//                              MSP    ,------|------'
//                             ,-x----'
//                    ,---|---'
//            ,------'
// ···|------'
//
//    |-------------------------------------->|-------------------------------------->
//             repeating pattern
//
//
// Compute weighted average for any sculpt point p:
//                                                            ,---|···
//                                       p2           ,------'
//                                     ,-x----|------'
//                          p  ,------'
//             p1     ,---|-x-'
//            ,x-----'
// ···|------'
//             <------------|------------>
//                 radius       radius
//              \_______________________/
//              p' = weigthed average of all
//                   points between p1 and p2
//
// Note how this method ensures that by design, the weighted average p' at
// the boundary of the sculpt points is exactly equal to p itself. More
// generally, the closer we get to the boundary, the less modified the
// points are.

class WeightedAverageAlgorithm {
public:
    WeightedAverageAlgorithm(const SculptSampling& sculptSampling)
        : sculptSampling_(sculptSampling)
        , sculptPoints_(sculptSampling.sculptPoints) {

        // Compute how many theoretical sculpt points influence each sculpt
        // point (per side). When radius == cappedRadii, this is supposed to be
        // equal to (sculptPoints_.length() - 1) / 2.
        //
        // Note about the division by ds: can it generate a huge
        // numInfluencingPoints? In theory no, because ds is supposed to be a
        // reasonable fraction of radius (e.g., 1%). However, there is the
        // potential case of sculpting a very small edge with a very large
        // radius: this may force ds to be smaller than we want it to be. TODO:
        // Do we want to handle this case by capping sculptSampling_.radius to
        // no more than, say, 10x the edge length?
        //
        numInfluencingPointsPerSide_ = core::round_cast<Int>(
            sculptSampling_.radius
            / (std::min)(sculptSampling_.ds0, sculptSampling_.ds1));

        if (!sculptSampling_.isClosed) {

            // Number of points (= "period") of the repeating pattern
            repeatN_ = (sculptPoints_.length() - 1) * 2;

            // Offset between one repeating pattern to the nextrepeatDelta_
            const SculptPoint& pFirst = sculptPoints_.first();
            const SculptPoint& pLast = sculptPoints_.last();
            repeatDelta_.pos = (pLast.pos - pFirst.pos) * 2;
            repeatDelta_.width = (pLast.width - pFirst.width) * 2;
        }
    }

    SculptPoint computeAveraged(Int i) {
        if (sculptSampling_.isClosed) {
            return computeAveraged_<true>(i);
        }
        else {
            return computeAveraged_<false>(i);
        }
    }

private:
    const SculptSampling& sculptSampling_;
    const core::Array<SculptPoint>& sculptPoints_;
    Int numInfluencingPointsPerSide_ = 0;
    Int repeatN_ = 0;
    SculptPoint repeatDelta_;

    // Note: We use a templated implementation to avoid making a dynamic
    // "if(closed)" in the middle of the hot path, called O(n²) times.

    template<bool isSculptSamplingClosed>
    SculptPoint computeAveraged_(Int i) {
        SculptPoint res = sculptPoints_[i];
        double wSum = cubicEaseInOut(1.0);
        res.pos *= wSum;
        res.width *= wSum;
        for (Int j = 1; j < numInfluencingPointsPerSide_; ++j) {
            double u = 1.0 - core::narrow_cast<double>(j) / numInfluencingPointsPerSide_;
            double w = cubicEaseInOut(u);
            SculptPoint sp1 = getInfluencePoint_<isSculptSamplingClosed>(i - j);
            SculptPoint sp2 = getInfluencePoint_<isSculptSamplingClosed>(i + j);
            res.pos += w * sp1.pos;
            res.pos += w * sp2.pos;
            res.width += w * sp1.width;
            res.width += w * sp2.width;
            wSum += 2 * w;
        }
        res.pos /= wSum;
        res.width /= wSum;
        return res;
    }

    template<bool isSculptSamplingClosed>
    SculptPoint getInfluencePoint_(Int i) {
        if constexpr (isSculptSamplingClosed) {
            return getInfluencePointClosed_(i);
        }
        else {
            return getInfluencePointOpen_(i);
        }
    }

    SculptPoint getInfluencePointClosed_(Int i) {
        // Note: in the closed case, we have sculptPoints.first() == sculptPoints.last()
        const Int n = sculptPoints_.length() - 1;
        Int j = core::moduloUnchecked(i, n);
        return sculptPoints_.getUnchecked(j);
    }

    // Note: we have getInfluencePointOpen_(i + repeatN) = getInfluencePointOpen_(i) + repeatDelta
    // Note 2: We may want to cache some of the computation here if too slow.
    //
    SculptPoint getInfluencePointOpen_(Int i) {

        const Int n = sculptPoints_.length();
        SculptPoint res = {};
        auto [q, r] = std::div(i, repeatN_); // i = q * repeatN + r
        if (r < 0) {
            q -= 1;
            r += repeatN_;
        }
        Vec2d p = {};
        double w = 0;
        if (r >= n) {
            Int mirroredR = repeatN_ - r;
            const SculptPoint& sp = sculptPoints_[mirroredR];
            p = repeatDelta_.pos - sp.pos + 2 * sculptPoints_[0].pos;
            w = repeatDelta_.width - sp.width + 2 * sculptPoints_[0].width;
        }
        else {
            const SculptPoint& sp = sculptPoints_[r];
            p = sp.pos;
            w = sp.width;
        }
        p += repeatDelta_.pos * core::narrow_cast<double>(q);
        w += repeatDelta_.width * core::narrow_cast<double>(q);
        res.pos = p;
        res.width = w;
        return res;
    }
};

class SculptSmoothAlgorithm {
public:
    bool execute(
        Vec2dArray& outKnotPositions,
        core::DoubleArray& outKnotWidths,
        Vec2d& outSculptCursorPosition,
        const Vec2d& position,
        double strength,
        double radius,
        const AbstractInterpolatingStroke2d* stroke,
        bool isClosed,
        CurveSamplingQuality samplingQuality,
        double maxDs,
        double simplifyTolerance) {

        stroke_ = stroke;
        numKnots_ = stroke->positions().length();
        isClosed_ = isClosed;
        hasWidths_ = stroke->widths().length() == numKnots_;

        outSculptCursorPosition = position;

        // Step 1:
        //
        // Compute sculpt points, which are a uniform sampling of the stroke
        // around the sculpt center. Using a uniform sampling is important in
        // order to be able to compute meaningful weighted averages.

        if (!initStrokeSampling_(samplingQuality, maxDs)) {
            return false;
        }

        if (!initSculptSampling_(position, radius, maxDs)) {
            return false;
        }

        if (totalS_ < maxDs * 0.5) {
            outSculptCursorPosition =
                sculptSampling_.sculptPoints[sculptSampling_.closestSculptPointIndex].pos;
            return false;
        }

        // Step 2:
        //
        // Determine which original knots of the curve are within the range of
        // sculpt points, that is, affected by sculpt operation. These are called
        // the "sculpted knots".

        computeSculptedKnotsInterval_();
        if (numSculptedKnots_ == 0) {
            outSculptCursorPosition =
                sculptSampling_.sculptPoints[sculptSampling_.closestSculptPointIndex].pos;
            return false;
        };

        // Step 3a:
        //
        // Smooth the distances between sculpted knots, in order to prevent
        // pairs of nearby knots that create undesirable corners.

        smoothKnotDistances_(radius, strength);

        // Step 3b:
        //
        // Compute new positions of original knots:
        // (a) First append unmodified knots before the sculpted knots
        // (b) Then append the modified knots, computed based on the
        //     sculpted knots and weighted average of sculpt points
        // (c) Then append unmodified knots after the sculpted knots
        //
        // Note: fewer knots than numSculptedKnots_ may actually be appended in
        // step (b), since we perform an average of knots in case there is more
        // than one knot between two consecutive sculpt points.

        appendUnmodifiedKnotsBefore_(); // (a)
        appendModifiedKnots_(strength); // (b)
        appendUnmodifiedKnotsAfter_();  // (c)

        // Step 4:
        //
        // Apply simplification (Douglas-Peuckert based) to the sculpted knots,
        // in order to remove knots that are not needed anymore due to the curve being smoother.
        //
        // The knot interval that we want to smooth is basically the same as sculptedKnotsInterval_
        // but extended by one more knot:
        //
        //
        // Original knots:             x------x-----xx----x-----x-------x
        // Sculpt points:                       o--o--o--o--o--o--o
        // Sculpted knots:                          xx    x     x
        // Transformed knots:                        x    x     x
        // Simplified interval:               x------x----x-----x-------x
        //                                 simplify                  simplify
        //                                first index               last index
        //
        // Knots surviving simplification:    x-----------x-----x-------x    (= `indices`)
        //

        Int simplifyFirstIndex = numUnmodifiedKnotsBefore_ - 1;
        Int simplifyLastIndex = newKnotPositions_.length() - numUnmodifiedKnotsAfter_;
        simplifyFirstIndex =
            core::clamp(simplifyFirstIndex, 0, newKnotPositions_.length() - 1);
        simplifyLastIndex =
            core::clamp(simplifyLastIndex, 0, newKnotPositions_.length() - 1);

        core::IntArray indices;
        indices.extend({simplifyFirstIndex, simplifyLastIndex});
        if (hasWidths_) {
            filterPointsStep<Vec2d>(
                newKnotPositions_,
                indices,
                0,
                isClosed,
                simplifyTolerance,
                [](const Vec2d& p, Int) { return p; },
                [&](const Vec2d&, Int i) -> double { return newKnotWidths_[i]; });
        }
        else {
            filterPointsStep<Vec2d>(
                newKnotPositions_,
                indices,
                0,
                isClosed,
                simplifyTolerance,
                [](const Vec2d& p, Int) { return p; },
                [](const Vec2d&, Int) -> double { return 1.0; });
        }

        // TODO: add index in filterPointsStep functor parameters to be
        //       able to use newPoints_[index] in the width getter.

        // Step 5:
        //
        // Copy the results post-simplification to the final output points/widths
        // arrays.
        //
        // In the case of a closed curve, the original first knot may not have
        // survived simplification, and therefore we need to find a new
        // suitable first knot and rotate the other knots accordingly.
        //
        // The new first knot (given by `newStartPointIndex_`) is chosen as
        // close as possible to the original first knot.
        //

        outKnotPositions.clear();
        outKnotWidths.clear();
        Int n = simplifyFirstIndex + indices.length()
                + (newKnotPositions_.length() - (simplifyLastIndex + 1));
        outKnotPositions.reserve(n);
        if (hasWidths_) {
            outKnotWidths.reserve(n);
        }

        newStartKnotIndex_ = core::modulo(newStartKnotIndex_, newKnotPositions_.length());
        if (newStartKnotIndex_ == 0) { // Simple case: no knot rotation needed

            // Copy the unmodified knots before
            outKnotPositions.extend(
                newKnotPositions_.begin(),
                newKnotPositions_.begin() + simplifyFirstIndex);
            if (hasWidths_) {
                outKnotWidths.extend(
                    newKnotWidths_.begin(), newKnotWidths_.begin() + simplifyFirstIndex);
            }

            // Copy the modified knots that survived simplification
            for (Int i : indices) {
                outKnotPositions.append(newKnotPositions_[i]);
                if (hasWidths_) {
                    outKnotWidths.append(newKnotWidths_[i]);
                }
            }

            // Copy the unmodified knots after
            outKnotPositions.extend(
                newKnotPositions_.begin() + simplifyLastIndex + 1,
                newKnotPositions_.end());
            if (hasWidths_) {
                outKnotWidths.extend(
                    newKnotWidths_.begin() + simplifyLastIndex + 1, newKnotWidths_.end());
            }
        }
        else { // newStartPointIndex_ > 0: rotation needed

            // Copy the modified knots that survived simplification and
            // are equal or after the new first knot.
            for (Int i : indices) {
                if (i >= newStartKnotIndex_) {
                    outKnotPositions.append(newKnotPositions_[i]);
                    if (hasWidths_) {
                        outKnotWidths.append(newKnotWidths_[i]);
                    }
                }
            }

            // Copy the unmodified knots before
            outKnotPositions.extend(
                newKnotPositions_.begin() + simplifyLastIndex + 1,
                newKnotPositions_.end());
            if (hasWidths_) {
                outKnotWidths.extend(
                    newKnotWidths_.begin() + simplifyLastIndex + 1, newKnotWidths_.end());
            }

            // Copy the unmodified knots after
            outKnotPositions.extend(
                newKnotPositions_.begin(),
                newKnotPositions_.begin() + simplifyFirstIndex);
            if (hasWidths_) {
                outKnotWidths.extend(
                    newKnotWidths_.begin(), newKnotWidths_.begin() + simplifyFirstIndex);
            }

            // Copy the modified knots that survived simplification and
            // are before the new first knot.
            for (Int i : indices) {
                if (i < newStartKnotIndex_) {
                    outKnotPositions.append(newKnotPositions_[i]);
                    if (hasWidths_) {
                        outKnotWidths.append(newKnotWidths_[i]);
                    }
                }
            }
        }

        outSculptCursorPosition = outSculptCursorPosition_;

        return true;
    }

private:
    bool initStrokeSampling_(CurveSamplingQuality quality, double maxDs) {
        VGC_UNUSED(quality);
        VGC_UNUSED(maxDs);
        if (numKnots_ < 2) {
            return false;
        }
        CurveSamplingParameters samplingParams(CurveSamplingQuality::AdaptiveLow);
        //samplingParams.setMaxDs(0.5 * maxDs);
        //samplingParams.setMaxIntraSegmentSamples(2047);
        knotsS_.resizeNoInit(numKnots_);
        knotsS_[0] = 0;
        samples_.clear();
        samples_.reserve(numKnots_);
        bool computeArclength = true;
        for (Int i = 0; i < numKnots_ - 1; ++i) {
            Int numSegments = 1;
            stroke_->sampleRange(
                samples_, samplingParams, i, numSegments, computeArclength);
            knotsS_[i + 1] = samples_.last().s();
            samples_.pop();
        }
        Int numExtraSegments = isClosed_ ? 1 : 0;
        stroke_->sampleRange(
            samples_, samplingParams, numKnots_ - 1, numExtraSegments, computeArclength);
        totalS_ = samples_.last().s();
        return true;
    }

    bool initSculptSampling_(const Vec2d& position, double radius, double maxDs) {
        // Note: we could have a distanceToCurve specialized for our geometry.
        // It could check each control polygon region first to skip sampling the ones
        // that are strictly farther than an other.
        DistanceToCurve d = distanceToCurve(samples_, position);
        if (d.distance() > radius) {
            return false;
        }

        // Compute middle sculpt point info (closest point).
        Int mspSegmentIndex = d.segmentIndex();
        double mspSegmentParameter = d.segmentParameter();
        StrokeSample2d mspSample = samples_[mspSegmentIndex];
        if (mspSegmentParameter > 0 && mspSegmentIndex + 1 < samples_.length()) {
            const StrokeSample2d& s2 = samples_[mspSegmentIndex + 1];
            mspSample = lerp(mspSample, s2, mspSegmentParameter);
        }

        computeSculptSampling(
            sculptSampling_, samples_, mspSample.s(), radius, maxDs, isClosed_, true);

        core::Array<SculptPoint>& sculptPoints = sculptSampling_.sculptPoints;

        if (sculptSampling_.isClosed) {

            // Duplicate first point as last point (including s and d values).
            //
            // With the following example values:
            //
            // totalS = 100
            // ds = 10
            // sMiddle = 85
            // radius = 80 (capped to 50)
            //
            // The sculpt points s-values now look like:
            //
            //                           wrap
            //                           <-->
            // [35, 45, 55, 65, 75, 85, 95, 5, 15, 25, 35]
            //
            // While the knot s-values may look like:
            //
            // [0, 38, 63, 92]
            //
            sculptPoints.emplaceLast(sculptPoints.first());
        }

        // Note: for a closed curve with non-closed sculpt sampling, we may have
        // sN < s0.
        //
        // Example:
        //
        // totalS = 100
        // ds = 10
        // sMsp = 85
        // radius = 40
        //
        // The sculpt points s-values now look like:
        //
        //                       wrap
        //                       <-->
        // [45, 55, 65, 75, 85, 95, 5, 15, 25]
        //

        return true;
    }

    void computeSculptedKnotsInterval_() {

        double s0 = sculptSampling_.sculptPoints.first().s;
        double sN = sculptSampling_.sculptPoints.last().s;

        // Search index of first knot at or after s0, that is, the first
        // sculpted knot.
        //
        // We want the invariant: s0 <= knotS_[i0] (if i0 < numKnots_)
        //
        // In case of open curves, if the radius overlaps the start knot then
        // we get i0 = 0, since we have both s0 = 0 (guaranteed by
        // computeSculptSampling()) and knotsS_[0] = 0 (guaranteed by
        // initStrokeSampling_()).
        //
        Int i0 = 0;
        while (i0 < numKnots_ && knotsS_[i0] < s0) { // Important: `<` not `<=`
            ++i0;
        }
        sculptedKnotsStart_ = i0;

        if (sculptSampling_.isClosed) {
            numSculptedKnots_ = numKnots_;
        }
        else {

            // Search index of first knot strictly after sN, that is, the first
            // non-sculpted knot.
            //
            // We want the invariant: knotS_[iN] <= sN (if iN < numKnots_)
            //
            // For closed curves, note that if we are here then we cannot have
            // s0 == sN, since we would have sculptSampling_.isClosed, which is
            // already handled above.
            //
            // In case of open curves, if the radius overlaps the end knot then
            // we get iN = numKnots_, since we have sN = knotsS_.last(), and
            // knotsS_[i] <= knotsS_.last() for all i.
            //
            Int iN = isClosed_ ? 0 : i0;
            while (iN < numKnots_ && knotsS_[iN] <= sN) { // Important: `<=` not `<`
                ++iN;
            }

            // Deduce count from i0 and iN.
            if (!isClosed_) {
                numSculptedKnots_ = iN - i0;
            }
            else if (i0 == iN) {
                if (sculptSampling_.isRadiusOverlappingStart) {
                    numSculptedKnots_ = numKnots_;
                }
                else {
                    numSculptedKnots_ = 0;
                }
            }
            else if (i0 < iN) {
                numSculptedKnots_ = iN - i0;
            }
            else { // i0 > iN
                numSculptedKnots_ = (iN + numKnots_) - i0;
            }
        }
        sculptedKnotsEnd_ = sculptedKnotsStart_ + numSculptedKnots_;
    }

    // Wrap s from [0, totalS) to [s0, s0 + totalS)
    double getIncreasingS_(double s, double s0) {
        if (s < s0) {
            return s + totalS_;
        }
        else {
            return s;
        }
    }

    // Wrap s from [s0, s0 + totalS) to [0, totalS)
    double getOriginalS_(double s) {
        double rem = std::fmod(s, totalS_);
        if (rem < 0) {
            rem += totalS_;
        }
        return rem;
    }

    // Get the increasing-s value of the given knot and the
    // offset between its original-s value and the returned value.
    struct KnotIncreasingSAndOffset {
        double s;
        double offset;
    };
    KnotIncreasingSAndOffset getKnotIncreasingSAndOffset_(Int i) {
        auto [quot, rem] = std::div(i, numKnots_);
        if (rem < 0) {
            quot -= 1;
            rem += numKnots_;
        }
        KnotIncreasingSAndOffset res;
        res.offset = quot * totalS_;
        res.s = knotsS_[rem] + res.offset;
        return res;
    }

    // Get the increasing-s value of the given knot, and also
    // write the offset used as an output parameter
    double getKnotIncreasingS_(Int i) {
        KnotIncreasingSAndOffset res = getKnotIncreasingSAndOffset_(i);
        return res.s;
    }

    // Smooth s-value based on values of neighboring s-values.
    struct ComputeSmoothedKnotSParams {
        double radius;
        double strength;
        double s0;
        double sN;
        double sMiddle;
    };
    double computeSmoothedKnotS_(
        const ComputeSmoothedKnotSParams& params,
        double s,
        double sBefore,
        double sAfter) {

        double d = s - params.sMiddle;
        double u = 1 - std::abs(d / params.radius);
        u = cubicEaseInOut(core::clamp(u, 0, 1));
        double targetS = 0.25 * (sBefore + 2 * s + sAfter);
        double newS = s + (targetS - s) * params.strength * u;
        newS = core::clamp(newS, params.s0, params.sN);
        return newS;
    }

    void smoothKnotDistances_(double radius, double strength) {

        ComputeSmoothedKnotSParams params;
        params.radius = radius;
        params.strength = strength;
        params.s0 = sculptSampling_.sculptPoints.first().s;
        params.sN = sculptSampling_.sculptPoints.last().s;
        params.sMiddle = sculptSampling_.sMiddle;

        core::DoubleArray newKnotsS = knotsS_;

        if (!isClosed_) {

            // Prevent modifying the s-value of the first and last knot
            Int start = (std::max)(sculptedKnotsStart_, Int{1});
            Int end = (std::min)(sculptedKnotsEnd_, numKnots_ - 1);

            // Smooth s-values based on neighboring s-values.
            for (Int i = start; i < end; ++i) {
                double s = knotsS_[i];
                double sBefore = knotsS_[i - 1];
                double sAfter = knotsS_[i + 1];
                double newS = computeSmoothedKnotS_(params, s, sBefore, sAfter);
                newKnotsS[i] = newS;
            }
        }
        else { // isClosed

            // Update values of sN and sMiddle so that they are in our virtual
            // increasing-s space instead of in the original-s space.
            if (sculptSampling_.isClosed) {
                // Note: before executing the line below, we have sN = s0.
                // Therefore, getIncreasingS_(sN, s0) would return s0.
                params.sN = params.s0 + totalS_;
            }
            else {
                params.sN = getIncreasingS_(params.sN, params.s0);
            }
            params.sMiddle = getIncreasingS_(params.sMiddle, params.s0);

            // Smooth increasing-s values based on neighboring increasing-s
            // values, then write back in the original-s space
            for (Int i = sculptedKnotsStart_; i < sculptedKnotsEnd_; ++i) {
                auto [s, offset] = getKnotIncreasingSAndOffset_(i);
                double sBefore = getKnotIncreasingS_(i - 1);
                double sAfter = getKnotIncreasingS_(i + 1);
                double newS = computeSmoothedKnotS_(params, s, sBefore, sAfter);
                newKnotsS.getWrapped(i) = newS - offset;
            }
        }

        swap(newKnotsS, knotsS_);
    }

    void appendUnmodifiedKnotsBefore_() {

        bool isOverlappingStart = sculptedKnotsEnd_ > numKnots_;

        const auto& positions = stroke_->positions();
        const auto& widths = stroke_->widths();

        if (!isOverlappingStart) {
            // Append knots from index 0 (included) to first sculpted knot (excluded).
            Int n = sculptedKnotsStart_;
            newKnotPositions_.extend(positions.begin(), positions.begin() + n);
            if (hasWidths_) {
                newKnotWidths_.extend(widths.begin(), widths.begin() + n);
            }
        }
        else {
            // Append all unmodified knots (before and after)
            Int n = numKnots_ - numSculptedKnots_;
            for (Int i = 0; i < n; ++i) {
                Int j = sculptedKnotsEnd_ + i;
                j = core::modulo(j, numKnots_);
                newKnotPositions_.emplaceLast(positions[j]);
                if (hasWidths_) {
                    newKnotWidths_.emplaceLast(widths[j]);
                }
            }
        }

        numUnmodifiedKnotsBefore_ = newKnotPositions_.length();
    }

    void appendUnmodifiedKnotsAfter_() {

        Int oldNewKnotsLength = newKnotPositions_.length();
        bool isOverlappingStart = sculptedKnotsEnd_ > numKnots_;

        if (!isOverlappingStart) {
            const auto& positions = stroke_->positions();
            const auto& widths = stroke_->widths();
            // Append knots from last modified knot (excluded) to last knot (included).
            Int n = sculptedKnotsEnd_;
            newKnotPositions_.extend(positions.begin() + n, positions.end());
            if (hasWidths_) {
                newKnotWidths_.extend(widths.begin() + n, widths.end());
            }
        }

        numUnmodifiedKnotsAfter_ = newKnotPositions_.length() - oldNewKnotsLength;
    }

    void appendModifiedKnots_(double strength) {

        // Prevent widths from exploding (due to the Catmull-Rom interpolation
        // of knots outputing sculpt points with widths bigger than the knots)
        // by capping the widths based on the input widths.
        //
        const auto& widths = stroke_->widths();
        double minModifiedKnotWidth = core::DoubleInfinity;
        double maxModifiedKnotWidth = 0;
        Int extendedStart = sculptedKnotsStart_ - 1;
        Int extendedEnd = sculptedKnotsEnd_ + 1;
        if (!isClosed_) {
            extendedStart = core::clamp(extendedStart, 0, widths.length());
            extendedEnd = core::clamp(extendedEnd, 0, widths.length());
        }
        for (Int i = extendedStart; i < extendedEnd; ++i) {
            double w = widths.getWrapped(i);
            minModifiedKnotWidth = (std::min)(w, minModifiedKnotWidth);
            maxModifiedKnotWidth = (std::max)(w, maxModifiedKnotWidth);
        }

        // Initialize weighted average algorithm
        //
        WeightedAverageAlgorithm weightedAverage(sculptSampling_);

        SculptPoint wasp1 = {}; // weighted-averaged sculpt point
        Int iWasp1 = -1;        // remember which index was last computed to reuse it
        Int knotIndex = sculptedKnotsStart_;

        // For each pair of consecutive sculpt points:
        // 1. Find all original knots in between (if any)
        // 2. Average their arclength s
        // 3. Replace all these knots by a single knot, whose position/width
        //    is a linear interpolation between the two transformed consecutive
        //    sculpt points.
        //
        core::Array<SculptPoint>& sculptPoints = sculptSampling_.sculptPoints;
        bool hasSculptSamplingWrapped = false;
        Int totalKnotsFound = 0;
        for (Int i = 1; i < sculptPoints.length(); ++i) {

            // Get two consecutive sculpt points (= "sculpt segment").
            //
            const SculptPoint& sp1 = sculptPoints[i - 1];
            const SculptPoint& sp2 = sculptPoints[i];
            double s1 = sp1.s;
            double s2 = sp2.s;

            // Find all knots in [s1, s2] and compute the mean of their s-values.
            // Also add totalS to s1 and/or s2 in case the sculpt segment includes the start knot.
            //
            double sMean = 0;
            Int numKnotsFound = findKnotsInSculptSegment_(
                knotIndex, s1, s2, sMean, hasSculptSamplingWrapped);
            if (numKnotsFound == 0) {
                continue;
            }
            totalKnotsFound += numKnotsFound;

            // Compute the new positions and widths of sculpt points, possibly
            // reusing already-computed wasp1 from previous segment.
            //
            if (iWasp1 != i - 1) {
                wasp1 = weightedAverage.computeAveraged(i - 1);
            }
            SculptPoint wasp2 = weightedAverage.computeAveraged(i);

            // Compute the position of a new knot at s = sMean that replaces
            // all the knots found in [s1, s2].
            //
            double t = (sMean - s1) / (s2 - s1);
            double u = 1.0 - t;
            Vec2d dp = u * (wasp1.pos - sp1.pos) + t * (wasp2.pos - sp2.pos);
            Vec2d p = u * sp1.pos + t * sp2.pos;
            Vec2d np = p + strength * dp;
            newKnotPositions_.append(np);
            if (hasWidths_) {
                double dw = u * (wasp1.width - sp1.width) + t * (wasp2.width - sp2.width);
                double w = u * sp1.width + t * sp2.width;
                double nw = w + strength * dw;
                newKnotWidths_.append(
                    core::clamp(nw, minModifiedKnotWidth, maxModifiedKnotWidth));
            }

            // Reuse wasp2 as wasp1 of next segment
            wasp1 = wasp2;
            iWasp1 = i;
        }
        if (totalKnotsFound != numSculptedKnots_) {
            VGC_WARNING(
                LogVgcGeometry,
                "Number of knots found ({}) is different than excepted ({}) "
                "during smoothing.",
                totalKnotsFound,
                numSculptedKnots_);
        }

        Int iMsp = sculptSampling_.closestSculptPointIndex;
        Vec2d scp = sculptPoints[iMsp].pos;
        Vec2d wascp = weightedAverage.computeAveraged(iMsp).pos;
        outSculptCursorPosition_ = scp + (wascp - scp) * strength;
        // XXX TODO: Fix cursor not displayed exactly at the rendered curve.
        // This is caused by the Catmull-Rom interpolation of the filtered
        // smoothed knots not being the same curve as the smoothed sculpt points.
    }

    // Find all knots within [s1, s2], and compute the mean of their
    // arclength s-values.
    //
    // For closed curves, in order to be able to compute a meaningful sMean and
    // handle the case where s2 < s1, we virtually extend the s-value such that
    // the s-values of knots and the s-values of sculpt points look as-if they
    // were always increasing. This is done by adding totalS to the stored
    // valued whenever we passed the "wrapping point", either of the sculpt
    // points or the knot index.
    //
    Int findKnotsInSculptSegment_(
        Int& knotIndex,
        double& s1,
        double& s2,
        double& sMean,
        bool& hasSculptSamplingWrapped) {

        // Compute sum of s-values for all knots in the sculpt segment
        Int numKnotsFound = 0;
        double sSum = 0;
        if (!isClosed_) {
            while (knotIndex < sculptedKnotsEnd_) {
                double sKnot = knotsS_[knotIndex];
                if (sKnot <= s2) {
                    sSum += sKnot;
                    ++knotIndex;
                    ++numKnotsFound;
                }
                else {
                    break;
                }
            }
        }
        else { // isClosed
            if (hasSculptSamplingWrapped) {
                s1 += totalS_;
                s2 += totalS_;
            }
            else if (s2 < s1) {
                hasSculptSamplingWrapped = true;
                s2 += totalS_;
            }
            while (knotIndex < sculptedKnotsEnd_) {
                Int wrappedKnotIndex = knotIndex;
                double sOffset = 0;
                bool hasKnotIndexWrapped = (knotIndex >= numKnots_);
                if (hasKnotIndexWrapped) {
                    wrappedKnotIndex = knotIndex - numKnots_;
                    sOffset = totalS_;
                }
                if (wrappedKnotIndex == 0) {
                    // We are currently processing the knot that was originally
                    // at index 0. This knot is about to be appended in the
                    // array of new knots, so we remember this new index so
                    // that we can later rotate it back to index 0.
                    newStartKnotIndex_ = newKnotPositions_.length();
                }
                double sKnot = knotsS_[wrappedKnotIndex] + sOffset;
                if (sKnot <= s2) {
                    sSum += sKnot;
                    ++knotIndex;
                    ++numKnotsFound;
                }
                else {
                    break;
                }
            }
        }

        // Compute mean
        if (numKnotsFound > 0) {
            sMean = sSum / core::narrow_cast<double>(numKnotsFound);
        }

        return numKnotsFound;
    }

    // Input
    Int numKnots_ = 0;
    const AbstractInterpolatingStroke2d* stroke_ = nullptr;
    bool isClosed_ = false;
    bool hasWidths_ = false;

    // Computed sampling
    StrokeSample2dArray samples_;
    core::Array<double> knotsS_;
    double totalS_ = 0;

    // Computed sculpt sampling
    SculptSampling sculptSampling_;

    // Sculpted Knot interval. Note that start and end are not necessarily
    // valid indices and may require wrapping
    Int sculptedKnotsStart_ = 0; // first knot after s0
    Int sculptedKnotsEnd_ = 0;   // first knot after sN (end = start + num)
    Int numSculptedKnots_ = 0;   // number of original knots in the sculpt range

    // Number of unmodified knots appended to the array of new knot
    Int numUnmodifiedKnotsBefore_ = 0; // appended before the sculpted knots
    Int numUnmodifiedKnotsAfter_ = 0;  // appended after the sculpted knots

    // Which knot among the new knots should be chosen as the knot of index 0,
    // if the original knot that was at index 0 is not preserved during the
    // averaging of simplification step.
    Int newStartKnotIndex_ = 0;

    // Output
    Vec2dArray newKnotPositions_;
    core::DoubleArray newKnotWidths_;
    Vec2d outSculptCursorPosition_;
};

} // namespace

Vec2d AbstractInterpolatingStroke2d::sculptGrab_(
    const Vec2d& startPosition,
    const Vec2d& endPosition,
    double radius,
    double strength,
    double tolerance,
    bool isClosed) {

    VGC_UNUSED(strength);

    // Let's consider tolerance will be ~= pixelSize for now.
    //
    // sampleStep is screen-space-dependent.
    //   -> doesn't look like a good parameter..

    Int numPoints = positions_.length();
    if (numPoints == 0) {
        return endPosition;
    }

    const double maxDs = (tolerance * 2.0);

    // Note: We sample with widths even though we only need widths for samples in radius.
    // We could benefit from a two step sampling (sample centerline points, then sample
    // cross sections on an sub-interval).
    StrokeSample2dArray samples;
    CurveSamplingParameters samplingParams(CurveSamplingQuality::AdaptiveLow);
    //samplingParams.setMaxDs(0.5 * maxDs);
    //samplingParams.setMaxIntraSegmentSamples(2047);
    core::Array<double> pointsS(numPoints, core::noInit);
    samples.emplaceLast();
    for (Int i = 0; i < numPoints; ++i) {
        pointsS[i] = samples.last().s();
        samples.pop();
        sampleRange(
            samples,
            CurveSamplingQuality::AdaptiveLow,
            i,
            Int{(!isClosed && i == numPoints - 1) ? 0 : 1},
            true);
    }

    // Note: we could have a distanceToCurve specialized for our geometry.
    // It could check each control polygon region first to skip sampling the ones
    // that are strictly farther than an other.
    DistanceToCurve d = distanceToCurve(samples, startPosition);
    if (d.distance() > radius) {
        return endPosition;
    }

    // Compute middle sculpt point info (closest point).
    Int mspSegmentIndex = d.segmentIndex();
    double mspSegmentParameter = d.segmentParameter();
    StrokeSample2d mspSample = samples[mspSegmentIndex];
    if (mspSegmentParameter > 0 && mspSegmentIndex + 1 < samples.length()) {
        const StrokeSample2d& s2 = samples[mspSegmentIndex + 1];
        mspSample = lerp(mspSample, s2, mspSegmentParameter);
    }
    double sMiddle = mspSample.s();

    SculptSampling sculptSampling = {};
    computeSculptSampling(
        sculptSampling, samples, sMiddle, radius, maxDs, isClosed, false);

    core::Array<SculptPoint>& sculptPoints = sculptSampling.sculptPoints;

    Vec2d delta = endPosition - startPosition;

    if (!isClosed) {
        Vec2d uMins = Vec2d(1.0, 1.0) - sculptSampling.cappedRadii / radius;
        Vec2d wMins(cubicEaseInOut(uMins[0]), cubicEaseInOut(uMins[1]));
        for (Int i = 0; i < sculptPoints.length(); ++i) {
            SculptPoint& sp = sculptPoints[i];
            double u = {};
            double wMin = {};
            if (sp.d < 0) {
                u = 1.0 - (-sp.d / radius);
                wMin = wMins[0];
            }
            else if (sp.d > 0) {
                u = 1.0 - (sp.d / radius);
                wMin = wMins[1];
            }
            else {
                // middle sculpt point
                u = 1.0;
                wMin = 0.0;
            }
            double w = cubicEaseInOut(u);
            double t = (w - wMin) / (1 - wMin);
            sp.pos += delta * t;
        }
    }
    else {
        // In this case capped radii are expected to be equal.
        double cappedRadius = sculptSampling.cappedRadii[0];
        double uMin = 1.0 - cappedRadius / radius;
        double wMin = cubicEaseInOut(uMin);
        for (Int i = 0; i < sculptPoints.length(); ++i) {
            SculptPoint& sp = sculptPoints[i];
            double u = {};
            if (sp.d < 0) {
                u = 1.0 - (-sp.d / cappedRadius);
            }
            else if (sp.d > 0) {
                u = 1.0 - (sp.d / cappedRadius);
            }
            else {
                // middle sculpt point
                u = 1.0;
            }
            double w = cubicEaseInOut(u);
            w *= (1.0 - wMin);
            w += wMin;
            sp.pos += delta * w;
        }
    }

    bool hasWidths = !widths_.isEmpty();

    core::IntArray indices = {};

    constexpr bool isFilteringEnabled = true;
    if (isFilteringEnabled) {
        if (!isClosed) {
            // When the sampling is capped at an edge endpoint we want to be able
            // to remove the uniformely sampled sculpt point next to the endpoint
            // since it is closer than ds.
            if (sculptSampling.cappedRadii[0] < radius) {
                double width = hasWidths ? widths_.first() : samples[0].halfwidth(0) * 2;
                sculptPoints.emplaceFirst(
                    positions_.first(),
                    width,
                    -sculptSampling.cappedRadii[0],
                    pointsS.first());
            }
            if (sculptSampling.cappedRadii[1] < radius) {
                double width = hasWidths ? widths_.last() : samples[0].halfwidth(0) * 2;
                sculptPoints.emplaceLast(
                    positions_.last(),
                    width,
                    sculptSampling.cappedRadii[1],
                    pointsS.last());
            }
        }
        indices.extend({0, sculptPoints.length() - 1});
        filterPointsStep<SculptPoint>(
            sculptPoints,
            indices,
            0,
            isClosed,
            tolerance * 0.5,
            [](const SculptPoint& p, Int) { return p.pos; },
            [](const SculptPoint& p, Int) { return p.width; });
    }
    else {
        indices.reserve(sculptPoints.length());
        for (Int i = 0; i < sculptPoints.length(); ++i) {
            indices.append(i);
        }
    }

    double s0 = sculptSampling.sculptPoints.first().s;
    double sN = sculptSampling.sculptPoints.last().s;
    Int numPatchPoints = indices.length();

    // Insert sculpt points in input points
    if (sculptSampling.isClosed) {
        positions_.resize(numPatchPoints);
        for (Int i = 0; i < numPatchPoints; ++i) {
            const SculptPoint& sp = sculptPoints[indices[i]];
            positions_[i] = sp.pos;
        }
        if (hasWidths) {
            widths_.resize(numPatchPoints);
            for (Int i = 0; i < numPatchPoints; ++i) {
                const SculptPoint& sp = sculptPoints[indices[i]];
                widths_[i] = sp.width;
            }
        }
    }
    else if (sculptSampling.isRadiusOverlappingStart && sN <= s0) {
        // original points to keep are in the middle of the original array
        //
        //  original points:  x----x--x----x-----x----x
        //  sculpt points:      x x x n)        (0 x x
        //  keepIndex:                     x            (first > sn)
        //  keepCount:                     1            (count until next >= sn)
        //
        Int keepIndex = 0;
        for (; keepIndex < numPoints; ++keepIndex) {
            if (pointsS[keepIndex] > sN) {
                break;
            }
        }
        Int keepEndIndex = keepIndex;
        for (; keepEndIndex < numPoints; ++keepEndIndex) {
            if (pointsS[keepEndIndex] >= s0) {
                break;
            }
        }
        Int keepCount = keepEndIndex - keepIndex;

        positions_.erase(positions_.begin(), positions_.begin() + keepIndex);
        positions_.resize(keepCount + numPatchPoints);
        for (Int i = 0; i < numPatchPoints; ++i) {
            const SculptPoint& sp = sculptPoints[indices[i]];
            positions_[keepCount + i] = sp.pos;
        }
        if (hasWidths) {
            widths_.erase(widths_.begin(), widths_.begin() + keepIndex);
            widths_.resize(keepCount + numPatchPoints);
            for (Int i = 0; i < numPatchPoints; ++i) {
                const SculptPoint& sp = sculptPoints[indices[i]];
                widths_[keepCount + i] = sp.width;
            }
        }
    }
    else {
        VGC_ASSERT(s0 <= sN);
        // original points to keep are at the beginning and end of the original array
        //
        //  original points:  x----x--x----x-----x----x
        //  sculpt points:        (0 x x x n)
        //  insertIndex:           x                    (first >= sn)
        //  insertEndIndex:                      x      (next > sn)
        //
        Int insertIndex = 0;
        for (; insertIndex < numPoints; ++insertIndex) {
            if (pointsS[insertIndex] >= s0) {
                break;
            }
        }
        Int insertEndIndex = insertIndex;
        for (; insertEndIndex < numPoints; ++insertEndIndex) {
            if (pointsS[insertEndIndex] > sN) {
                break;
            }
        }

        positions_.erase(
            positions_.begin() + insertIndex, positions_.begin() + insertEndIndex);
        positions_.insert(insertIndex, numPatchPoints, {});
        for (Int i = 0; i < numPatchPoints; ++i) {
            const SculptPoint& sp = sculptPoints[indices[i]];
            positions_[insertIndex + i] = sp.pos;
        }
        if (hasWidths) {
            widths_.erase(
                widths_.begin() + insertIndex, widths_.begin() + insertEndIndex);
            widths_.insert(insertIndex, numPatchPoints, {});
            for (Int i = 0; i < numPatchPoints; ++i) {
                const SculptPoint& sp = sculptPoints[indices[i]];
                widths_[insertIndex + i] = sp.width;
            }
        }
    }

    chordLengths_.clear();
    segmentTypes_.clear();
    onPositionsChanged_();
    onWidthsChanged_();

    return sculptPoints[sculptSampling.closestSculptPointIndex].pos;

    // Depending on the sculpt kernel we may have to duplicate the points
    // at the sculpt boundary to "extrude" properly.

    // problem: cannot reuse distanceToCurve.. samples don't have their segment index :(

    // In arclength mode, step is not supported so we have to do this only once.
    // In spatial mode, step is supported and we may have to do this at every step.
}

Vec2d AbstractInterpolatingStroke2d::sculptWidth_(
    const Vec2d& position,
    double delta,
    double radius,
    double tolerance,
    bool isClosed) {

    VGC_UNUSED(tolerance);

    Int numKnots = positions_.length();
    if (numKnots == 0) {
        return position;
    }

    // Sanitize widths_.
    if (widths_.length() != numKnots) {
        if (widths_.isEmpty()) {
            widths_.resize(numKnots, 1.0);
        }
        else {
            widths_.resize(1);
            widths_.resize(numKnots, widths_[0]);
        }
    }

    // It seems `curvature * width` is what we want to not
    // let increase too much.
    //

    // Let's consider tolerance is ~= pixelSize for now.
    //const double maxDs = (tolerance * 2.0);

    // Note: We sample with widths even though we only need widths for samples in radius.
    // We could benefit from a two step sampling (sample centerline points, then sample
    // cross sections on an sub-interval).
    StrokeSample2dArray samples;
    CurveSamplingParameters samplingParams(CurveSamplingQuality::AdaptiveLow);

    core::Array<Int> knotToSampleIndex(numKnots, core::noInit);
    knotToSampleIndex[0] = 0;
    for (Int i = 0; i < numKnots - 1; ++i) {
        sampleRange(samples, samplingParams, i, 1, true);
        knotToSampleIndex[i + 1] = samples.length() - 1;
        samples.pop();
    }
    sampleRange(samples, samplingParams, numKnots - 1, Int{isClosed ? 1 : 0}, true);
    const double curveLength = samples.last().s();

    // Note: we could have a distanceToCurve specialized for our geometry.
    // It could check each control polygon region first to skip sampling the ones
    // that are strictly farther than an other.
    DistanceToCurve dtc = distanceToCurve(samples, position);
    if (dtc.distance() > radius) {
        return position;
    }

    // Compute closest point info.
    Int closestSegmentIndex = dtc.segmentIndex();
    double closestSegmentParameter = dtc.segmentParameter();
    StrokeSample2d closestSample = samples[closestSegmentIndex];
    if (closestSegmentParameter > 0 && closestSegmentIndex + 1 < samples.length()) {
        const StrokeSample2d& s2 = samples[closestSegmentIndex + 1];
        closestSample = lerp(closestSample, s2, closestSegmentParameter);
    }
    double sMiddle = closestSample.s();

    // First pass: update widths of original knots.
    for (Int i = 0; i < numKnots; ++i) {
        StrokeSample2d& sample = samples[knotToSampleIndex[i]];
        double s = sample.s();
        double d = std::abs(s - sMiddle);
        if (isClosed) {
            double d2 = (s + curveLength) - sMiddle;
            double d3 = sMiddle - (s - curveLength);
            if (d2 < d) {
                d = d2;
            }
            if (d3 < d) {
                d = d3;
            }
        }
        if (d < radius) {
            double w = widths_[i];
            double wt = 1.0 - cubicEaseInOut(d / radius);
            w = std::max<double>(0, w + 2. * delta * wt);
            widths_[i] = w;
        }
    }

    // Second pass: add knots if there isn't enough already.
    // Add each only if there is no knot in a range a*r around it.
    double minD = 0.2 * radius;
    std::array<double, 3> targetsD = {0.25 * radius, 0.75 * radius, radius};
    core::Array<double> targetsS;
    if (!isClosed) {
        double dLeft = sMiddle;
        double dRight = curveLength - dLeft;
        for (double targetD : targetsD) {
            if (dLeft > targetD + minD) {
                targetsS.prepend(sMiddle - targetD);
            }
        }
        if (dLeft > minD && dRight > minD) {
            targetsS.append(sMiddle);
        }
        for (double targetD : targetsD) {
            if (dRight > targetD + minD) {
                targetsS.append(sMiddle + targetD);
            }
        }
    }
    else {
        double dMax = 0.5 * curveLength;
        for (double targetD : targetsD) {
            if (targetD <= dMax) {
                if (targetD + minD < dMax) {
                    double s0 = sMiddle - targetD;
                    if (s0 < 0) {
                        s0 += curveLength;
                    }
                    targetsS.append(s0);
                    double s1 = sMiddle + targetD;
                    if (s1 >= curveLength) {
                        s1 -= curveLength;
                    }
                    targetsS.append(s1);
                }
                else {
                    double s = sMiddle - dMax;
                    if (s < 0) {
                        s += curveLength;
                    }
                    targetsS.append(s);
                }
            }
        }
        if (dMax > minD) {
            targetsS.append(0);
        }
        std::sort(targetsS.begin(), targetsS.end());
    }
    // Loop is reversed to simplify the closed case.
    double s1 = curveLength;
    Int j1 = samples.length() - 1;
    Int iKnot = numKnots - 2;
    Int iTarget = targetsS.length() - 1;
    if (isClosed) {
        iKnot = numKnots - 1;
    }
    Vec2dArray tmpPositions;
    core::Array<double> tmpWidths;
    for (; iKnot >= 0 && iTarget >= 0; --iKnot) {
        Int j0 = knotToSampleIndex[iKnot];
        const StrokeSample2d& sample = samples[j0];
        double s0 = sample.s();
        tmpPositions.clear();
        tmpWidths.clear();
        while (iTarget >= 0) {
            double targetS = targetsS[iTarget];
            if (targetS < s0) {
                break;
            }
            if ((targetS >= s0 + minD) && (targetS <= s1 - minD)) {
                // new knot -> find the sampled segment it belongs too.
                for (Int j = j0 + 1; j <= j1; ++j) {
                    const StrokeSample2d& sample1 = samples[j];
                    if (targetS < sample1.s()) {
                        // compute and add new knot
                        const StrokeSample2d& sample0 = samples[j - 1];
                        // (targetS >= s0 + minD) => sample1.s() != sample0.s()
                        double t = (targetS - sample0.s()) / (sample1.s() - sample0.s());
                        Vec2d p =
                            core::fastLerp(sample0.position(), sample1.position(), t);
                        Vec2d hws =
                            core::fastLerp(sample0.halfwidths(), sample1.halfwidths(), t);
                        double w = hws[0] * 2;
                        double d = (std::min)(
                            std::abs(targetS - sMiddle),
                            std::abs(targetS + curveLength - sMiddle));
                        double wt = 1.0 - cubicEaseInOut(d / radius);
                        w = std::max<double>(0, w + 2. * delta * wt);
                        tmpPositions.prepend(p);
                        tmpWidths.prepend(w);
                        break;
                    }
                }
            }
            --iTarget;
        }
        if (!tmpPositions.isEmpty()) {
            positions_.insert(iKnot + 1, tmpPositions);
            widths_.insert(iKnot + 1, tmpWidths);
        }
        s1 = s0;
        j1 = j0;
    }

    //samplingParams.setMaxDs(0.5 * maxDs);
    //samplingParams.setMaxIntraSegmentSamples(2047);

    onPositionsChanged_();
    onWidthsChanged_();

    return closestSample.position();
}

Vec2d AbstractInterpolatingStroke2d::sculptSmooth_(
    const Vec2d& position,
    double radius,
    double strength,
    double tolerance,
    bool isClosed) {

    // Let's consider tolerance will be ~= pixelSize for now.

    const double maxDs = (std::max)(radius / 100, tolerance * 2.0);

    Vec2dArray newPoints;
    core::DoubleArray newWidths;
    Vec2d sculptCursorPosition = position;

    SculptSmoothAlgorithm alg;

    // TODO: optimize that, smooth is too slow.
    // TODO: fix that, smooth breaks dirtying when endpoints move.. (snapping involved??)
    bool success = alg.execute(
        newPoints,
        newWidths,
        sculptCursorPosition,
        position,
        strength,
        radius,
        this,
        isClosed,
        CurveSamplingQuality::AdaptiveLow,
        maxDs,
        tolerance * 0.5);

    if (success) {
        bool hasWidths = widths_.length() == positions_.length();
        positions_.swap(newPoints);
        if (hasWidths) {
            widths_.swap(newWidths);
        }
        onPositionsChanged_();
        onWidthsChanged_();
    }

    return sculptCursorPosition;
}

void AbstractInterpolatingStroke2d::computeArclengths_(
    core::DoubleArray& arclengths) const {

    Int numPositions = positions_.length();
    arclengths.resize(numPositions);
    if (numPositions == 0) {
        return;
    }

    arclengths[0] = 0;
    StrokeSample2dArray sampling;
    CurveSamplingParameters sParams(CurveSamplingQuality::AdaptiveLow);
    double s = 0;
    for (Int i = 1; i < numPositions; ++i) {
        sampleRange(sampling, sParams, i - 1, 1, true);
        s += sampling.last().s();
        arclengths[i] = s;
        sampling.clear();
    }
}

void AbstractInterpolatingStroke2d::onPositionsChanged_() {
    chordLengths_.clear();
    segmentTypes_.clear();
    isCacheDirty_ = true;
}

void AbstractInterpolatingStroke2d::onWidthsChanged_() {
    isCacheDirty_ = true;
}

void detail::checkSegmentIndexIsValid(Int segmentIndex, Int numSegments) {
    VGC_ASSERT(segmentIndex >= 0);
    VGC_ASSERT(segmentIndex < numSegments);
}

/*

std::shared_ptr<FreehandEdgeGeometry> FreehandEdgeGeometry::createFromPoints(
    core::Span<FreehandEdgePoint> points,
    bool isClosed,
    double tolerance) {

    // TODO: detect constant width
    Vec2dArray positions;
    core::Array<double> widths;
    if (points.length() > 2) {
        core::IntArray indices;
        indices.extend({0, points.length() - 1});
        filterPointsStep<FreehandEdgePoint>(
            points,
            indices,
            0,
            isClosed,
            tolerance,
            [](const FreehandEdgePoint& p, Int) { return p.position(); },
            [](const FreehandEdgePoint& p, Int) { return p.width(); });
        Int n = indices.length();
        positions.reserve(n);
        widths.reserve(n);
        for (Int i = 0; i < n; ++i) {
            const FreehandEdgePoint& point = points[indices[i]];
            positions.append(point.position());
            widths.append(point.width());
        }
    }
    else {
        Int n = points.length();
        positions.reserve(n);
        widths.reserve(n);
        for (const FreehandEdgePoint& point : points) {
            positions.append(point.position());
            widths.append(point.width());
        }
    }
    return std::make_shared<FreehandEdgeGeometry>(
        core::SharedConst<Vec2dArray>(std::move(positions)),
        core::SharedConst<core::DoubleArray>(std::move(widths)),
        isClosed,
        false);
}

*/

} // namespace vgc::geometry
