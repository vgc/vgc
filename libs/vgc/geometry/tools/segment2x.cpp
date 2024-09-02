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

// This file is used to generate all the variants of this class.
// You must manually run generate.py after any modification.

#include "segment2x.h"

#include <vgc/core/assert.h>

namespace vgc::geometry {

namespace {

// Possible values for the `swapS` template parameter.
// Indicates whether the two segments were swapped.
constexpr bool S1S2 = false;
constexpr bool S2S1 = true;

// Possible values for the `swap1` template parameter.
// Indicates whether the two endpoints of the first segment were swapped.
constexpr bool A1B1 = false;
constexpr bool B1A1 = true;

// Possible values for the `swap2` template parameter.
// Indicates whether the two endpoints of the second segment were swapped.
constexpr bool A2B2 = false;
constexpr bool B2A2 = true;

template<bool swap>
VGC_FORCE_INLINE float param(float t) {
    if constexpr (swap) {
        return 1 - t;
    }
    else {
        return t;
    }
}

template<bool swapS, bool swap1, bool swap2>
VGC_FORCE_INLINE Segment2xIntersection pointInter(const Vec2x& p, float t1, float t2) {
    if constexpr (swapS) {
        return Segment2xIntersection(p, param<swap2>(t2), param<swap1>(t1));
    }
    else {
        return Segment2xIntersection(p, param<swap1>(t1), param<swap2>(t2));
    }
}

// Note: we cannot guarantee both s1 <= t1 and s2 <= t2.
// For now it's possible that we return both s1 > t1 and s2 > t2.
// But should we guarantee s1 <= t1?
// Or perhaps guarantee s1 <= t1 OR s2 <= t2?
template<bool swapS, bool swap1, bool swap2>
VGC_FORCE_INLINE Segment2xIntersection
segInter(const Vec2x& p, const Vec2x& q, float s1, float t1, float s2, float t2) {
    if constexpr (swapS) {
        return Segment2xIntersection(
            p, q, param<swap2>(s2), param<swap2>(t2), param<swap1>(s1), param<swap1>(t1));
    }
    else {
        return Segment2xIntersection(
            p, q, param<swap1>(s1), param<swap1>(t1), param<swap2>(s2), param<swap2>(t2));
    }
}

// Assumes a1x == b1x == a2x == b2x
// Assumes a1y < b1y and a2y < b2y ("each segment is y-ordered")
// Assumes a1y <= a2y ("segments are y-ordered with each other")
template<bool swapS, bool swap1, bool swap2>
[[maybe_unused]] Segment2xIntersection intersectBothVerticalYOrdered_v1(
    const Vec2x& a1_,
    const Vec2x& b1_,
    const Vec2x& a2_,
    const Vec2x& b2_) {

    float a1 = a1_.y();
    float b1 = b1_.y();
    float a2 = a2_.y();
    float b2 = b2_.y();

    if (b1 < a2) {
        // -----------------------> Y axis
        // a1     b1
        // x------x  a2     b2
        //           x------x
        return {};
    }
    else if (b1 == a2) {
        // x------x
        //        x------x
        return pointInter<swapS, swap1, swap2>(b1_, 1, 0);
    }
    else if (a1 < a2) { // a1 < a2 < b1
        float s1 = (a2 - a1) / (b1 - a1);
        if (b2 < b1) {
            // x------x
            //    x--x
            float t1 = (b2 - a1) / (b1 - a1);
            return segInter<swapS, swap1, swap2>(a2_, b2_, s1, t1, 0, 1);

            // Note: we intentionally don't precompute `1 / (b1 - a1)`
            //       as it would be less accurate, see:
            //
            // 3/5.      => 0.59999999999999997779553950749686919152736663818359375
            // 3*(1/5.)) => 0.600000000000000088817841970012523233890533447265625
            // print(3/5.)     => 0.6
            // print(3*(1/5.)) => 0.6000000000000001
        }
        else if (b2 > b1) {
            // x------x
            //    x-----x
            float t2 = (b1 - a2) / (b2 - a2);
            return segInter<swapS, swap1, swap2>(a2_, b1_, s1, 1, 0, t2);
        }
        else {
            // x------x
            //    x---x
            return segInter<swapS, swap1, swap2>(a2_, b2_, s1, 1, 0, 1);
        }
    }
    else {
        VGC_ASSERT(a1 == a2);
        if (b2 < b1) {
            // x------x
            // x----x
            float t1 = (b2 - a1) / (b1 - a1);
            return segInter<swapS, swap1, swap2>(a2_, b2_, 0, t1, 0, 1);
        }
        else if (b2 > b1) {
            // x------x
            // x--------x
            float t2 = (b1 - a2) / (b2 - a2);
            return segInter<swapS, swap1, swap2>(a1_, b1_, 0, 1, 0, t2);
        }
        else {
            // x------x
            // x------x
            return segInter<swapS, swap1, swap2>(a1_, b1_, 0, 1, 0, 1);
        }
    }
}

// Assumes a1x == b1x == a2x == b2x
// Assumes a1y < b1y and a2y < b2y ("each segment is y-ordered")
// Assumes a1y <= a2y ("segments are y-ordered with each other")
//
// This version has less branching than v2 but more floating points operation
// in some cases. It is unclear which is faster, and it might depend on the
// platform (e.g., ARM vs. x86_64).
//
// TODO: benchmarks between v1 and v2
//
template<bool swapS, bool swap1, bool swap2>
[[maybe_unused]] Segment2xIntersection intersectBothVerticalYOrdered_v2(
    const Vec2x& a1_,
    const Vec2x& b1_,
    const Vec2x& a2_,
    const Vec2x& b2_) {

    float a1 = a1_.y();
    float b1 = b1_.y();
    float a2 = a2_.y();
    float b2 = b2_.y();

    if (b1 < a2) {
        // -----------------------> Y axis
        // a1     b1
        // x------x  a2     b2
        //           x------x
        return {};
    }
    else if (b1 == a2) {
        // x------x
        //        x------x
        return pointInter<swapS, swap1, swap2>(b1_, 1, 0);
    }
    else if (b1 < b2) { // a2 < b1 < b2
        // x------x     OR   x------x
        // x--------x          x------x
        float s1 = (a2 - a1) / (b1 - a1); // Guaranteed 0 if a1 == a2
        float t2 = (b1 - a2) / (b2 - a2); // (!) NOT guaranteed 0 < t2 < 1 [1]
        return segInter<swapS, swap1, swap2>(a2_, b1_, s1, 1, 0, t2);
    }
    else {
        // x------x   OR   x------x   OR   x------x   OR   x------x
        // x------x        x----x            x----x          x--x
        float s1 = (a2 - a1) / (b1 - a1); // Guaranteed 0 if a1 == a2
        float t1 = (b2 - a1) / (b1 - a1); // Guaranteed 1 if b1 == b2
        return segInter<swapS, swap1, swap2>(a2_, b2_, s1, t1, 0, 1);
        // See comment on v1 why we don't precompute `1 / (b1 - a1)`
    }
}

// [1] Unfortunately, due to how floating points work, then with:
//       a < x < b
//       t = (x - a) / (b - a)
//     we do not have the guarantee that 0 < t < 1
//
// Example where t == 1:
//
// a = -10
// x = 0.9999999999999999
// b = 1
// t = (x-a)/(b-a)
// print(a < x)     => True
// print(x < b)     => True
// print(t == 1)    => True
//
// Example where t == 0:
// (this is more unlikely since there is MUCH more precision
//  near 0 than near 1, but it is still possible)
//
// a = 0
// x = 1e-200
// b = 1e200
// t = (x-a)/(b-a)
// print(a < x)     => True
// print(x < b)     => True
// print(t == 0)    => True
//
// One option would be to enforce `0 < t < 1` by arbitrarily offsetting the
// result by `1.0 - core::nextbefore(1.0)` when t is equal to 0 or 1 but we
// know that it isn't an exact intersection. Unfortunately, this would make the
// implementation more complex, slower, and it isn't even clear that it is best
// for client code, since it arbritrarily introduces small inaccuraces in the
// returned parameters.
//
// Therefore, we choose not to provide the guarantee `0 < t < 1` when the
// intersection isn't exact.
//
// However, we do provide the converse guarantee, that is, `t == 0 || t == 1`
// when the intersection is exact.

// Assumes a1x == b1x == a2x == b2x
// Assumes a1y < b1y and a2y < b2y
template<bool swap1, bool swap2>
Segment2xIntersection intersectBothVerticalYOrdered(
    const Vec2x& a1,
    const Vec2x& b1,
    const Vec2x& a2,
    const Vec2x& b2) {

    if (a2.y() < a1.y()) {
        return intersectBothVerticalYOrdered_v2<S2S1, swap1, swap2>(a2, b2, a1, b1);
    }
    else {
        return intersectBothVerticalYOrdered_v2<S1S2, swap1, swap2>(a1, b1, a2, b2);
    }
}

// Assumes a1x == b1x == a2x == b2x
// Assumes a1y < b1y
template<bool swapS, bool swap1, bool swap2>
Segment2xIntersection intersectOneVerticalYOrderedWithPoint(
    const Vec2x& a1_,
    const Vec2x& b1_,
    const Vec2x& a2_) {

    float a1 = a1_.y();
    float b1 = b1_.y();
    float a2 = a2_.y();

    if (a2 < a1 || b1 < a2) {
        //    a1    b1         a1    b1
        // a2 x-----x    OR    x-----x  a2
        // x                            x
        return {};
    }
    else {
        // x-----x  OR  x-----x  OR  x-----x
        // x              x                x
        float t1 = (a2 - a1) / (b1 - a1);
        return pointInter<swapS, swap1, swap2>(a2_, t1, 0);
        // t1 is guaranteed to be exactly 0 or 1 if a2 equals a1 or b1
        // (!) t1 is NOT guaranteed to be 0 < t1 < 1 if not equal to a1 or b1
    }
}

// Assumes a1x == b1x and a2x == b2x
Segment2xIntersection intersectBothVertical(
    const Vec2x& a1,
    const Vec2x& b1,
    const Vec2x& a2,
    const Vec2x& b2) {

    if (a1.x() != a2.x()) {
        return {};
    }
    else if (a1.y() < b1.y()) {
        if (a2.y() < b2.y()) {
            return intersectBothVerticalYOrdered<A1B1, A2B2>(a1, b1, a2, b2);
        }
        else if (a2.y() > b2.y()) {
            return intersectBothVerticalYOrdered<A1B1, B2A2>(a1, b1, b2, a2);
        }
        else {
            return intersectOneVerticalYOrderedWithPoint<S1S2, A1B1, A2B2>(a1, b1, a2);
        }
    }
    else if (a1.y() > b1.y()) {
        if (a2.y() < b2.y()) {
            return intersectBothVerticalYOrdered<B1A1, A2B2>(b1, a1, a2, b2);
        }
        else if (a2.y() > b2.y()) {
            return intersectBothVerticalYOrdered<B1A1, B2A2>(b1, a1, b2, a2);
        }
        else {
            return intersectOneVerticalYOrderedWithPoint<S1S2, B1A1, A2B2>(a1, b1, a2);
        }
    }
    else {
        if (a2.y() < b2.y()) {
            return intersectOneVerticalYOrderedWithPoint<S2S1, A1B1, A2B2>(a2, b2, a1);
        }
        else if (a2.y() > b2.y()) {
            return intersectOneVerticalYOrderedWithPoint<S2S1, A1B1, B2A2>(b2, a2, a1);
        }
        else {
            // Both segments are points and have the same X
            if (a1.y() == a2.y()) {
                return pointInter<S1S2, A1B1, A2B2>(a1, 0, 0);
            }
            else {
                return {};
            }
        }
    }
}

// ----------- WIP below ----
// (everything above is ok I think)

// Assumes a1x == b1x and a2x < b2x
Segment2xIntersection
intersectOneVert(const Vec2x& a1, const Vec2x& b1, const Vec2x& a2, const Vec2x& b2) {
    // TODO
}

// Assumes a1x < b1x and a2x < b2x and a1x <= a2x
//
// In particular, this means a1 != b1 and a2 != b2.
//
template<bool swapS, bool swap1, bool swap2>
Segment2xIntersection intersectBothXOrdered_(
    const Vec2x& a1,
    const Vec2x& b1,
    const Vec2x& a2,
    const Vec2x& b2) {

    // Fast return if the x-range of the segments do not intersect.
    //
    // This is expected to be the most likely code path, which takes four
    // comparisons and zero arithmetic operations.
    //
    if (b1.x() < a2.x()) {
        // a1     b1
        // x------x  a2     b2
        //           x------x
        return {};
    }

    // Determines:
    // - which side of the (a1, b1) line each point a2 and b2 are, and
    // - which side of the (a2, b2) line each point a1 and b1 are
    //
    // Mathematically, if two of those determinants are zero, then all four of
    // them are zero. But due to numerical errors, this may not be the case.
    // For robustness and consistency across multiple computations involving
    // shared endpoints, we always compute all four of them, and consider the
    // four points to be collinear as soon as two of them are zero.
    //
    Vec2x a1b1 = b1 - a1;
    Vec2x a2b2 = b2 - a2;
    Vec2x a1a2 = a2 - a1;
    Vec2x a1b2 = b2 - a1;
    Vec2x a2b1 = b1 - a2;
    float a2Side = a1b1.det(a1a2);
    float b2Side = a1b1.det(a1b2);
    float a1Side = -a2b2.det(a1a2);
    float b1Side = a2b2.det(a2b1);

    // Convert to sign since multiplying two non-zero floats may give zero.
    // Also, this leads to better jump-less assembly on some compilers.
    //
    Int8 a2Sign = core::sign(a2Side);
    Int8 b2Sign = core::sign(b2Side);
    Int8 a1Sign = core::sign(a1Side);
    Int8 b1Sign = core::sign(b1Side);

    if (a2Sign * b2Sign < 0 && a1Sign * b1Sign < 0) {

        // => The segments intersect in a non-special case configuration
        //
        // We just have to solve for the intersection parameters.
        //
        // Note that we intentionally do not rely on:
        //
        //   (A)  t1 >= 0 && t1 <= 1 && t2 >= 0 && t2 <= 1
        //
        // to check whether the segments intersect, but instead rely on:
        //
        //   (B)  a2Sign * b2Sign < 0 && a1Sign * b1Sign < 0
        //
        // Mathematically, these two are equivalent, but due to floating point
        // numerical errors, (A) and (B) may not in fact be equal. When this
        // intersection test is part of a larger algorithm (e.g., line-sweep),
        // using the sign predicates (B) rather than the result of the
        // parameter solve (A) improves correctness of the algorithm by having
        // separate computations be consistent with each other.
        //
        // Related: Boissonnat and Preparata 2000,
        //          Robust Plane Sweep for Intersecting Segments
        //
        float delta = a1b1.det(a2b2); // depends on all four points, see [2]
        if (std::abs(delta) > 0) {
            float t1 = a1Side / delta;
            float t2 = -a2Side / delta;
            t1 = core::clamp(t1, 0, 1);
            t2 = core::clamp(t2, 0, 1);
            Vec2x p = core::fastLerp(a1, b1, t1);
            return pointInter<swapS, swap1, swap2>(p, t1, t2);
        }
        else {
            // This is mathematically impossible. However, due to floating point errors,
            // it may or may not be possible. In case it does occur, we consider
            // this to mean that all four points are collinear, despite
            // all signs (a1Sign, etc.) being non-zero.
            //
            return {}; // TODO: all collinear case
        }
    }
    else {
        Int8 numCollinears = static_cast<Int8>(a2Sign == 0)   //
                             + static_cast<Int8>(b2Sign == 0) //
                             + static_cast<Int8>(a1Sign == 0) //
                             + static_cast<Int8>(b1Sign == 0);

        if (numCollinears == 0) {

            // => a2Sign * b2Sign > 0 || a1Sign * b1Sign > 0  (see (B))
            //
            // => At least one of the two segments lies entirely on one of the
            // two halfplanes defined by the other segment, so the segments do
            // not intersect.

            return {};
        }
        else if (numCollinears == 1) {

            // => Three points are collinear, one is not collinear.
            //
            // [2] In this case, we make sure that the result does not rely on
            // the position of the non-collinear point, improving consistency
            // when this intersection test is part of a larger algorithm.
            //
            // TODO: Use Y-coord for better accuracy for almost-vertical segments?
            //
            // Preconditions:
            //   a1x < b1x
            //   a2x < b2x
            //   a1x <= a2x <= b1x
            //
            if (a2Side == 0) {
                //          x b2             x b2          x b2
                //         /      OR        /       OR    /
                // x------x            x---x---x         x------x
                // a1   a2,b1          a1  a2  b1      a1,a2    b1
                //
                float t1 = (a2.x() - a1.x()) / (b1.x() - a1.x()); // b2-independent
                return pointInter<swapS, swap1, swap2>(a2, t1, 0);
                // t1 is guaranteed to be 0 <= t1 <= 1
                // t1 is guaranteed to be exactly 0 or 1 if a2x equal to a1x or b1x
                // (!) t1 is NOT guaranteed to be 0 < t1 < 1 if a2x not equal to a1x or b1x
            }
            else if (b2Side == 0) {
                if (b2.x() > a2.x()) {
                    // a1     b1
                    // x------x x b2
                    //         /
                    //        x a2
                    return {};
                }
                else {
                    // a1   b2,b1        a1  b2  b1
                    // x------x          x---x---x
                    //       /     OR       /
                    //      x a2           x a2
                    //
                    // Note: a1x <= a2x and a2x < b2x so a1x < b2x
                    //
                    float t1 = (b2.x() - a1.x()) / (b1.x() - a1.x()); // a2-independent
                    return pointInter<swapS, swap1, swap2>(b2, t1, 1);
                    // t1 is guaranteed to be 0 < t1 <= 1
                    // t1 is guaranteed to be exactly 1 if b2x equal b1x
                    // (!) t1 is NOT guaranteed to be t1 < 1 if b2x not equal to b1x
                }
            }
            else {
                // TODO
            }
        }
        else { //numCollinears >= 2
            // TODO: code when they are all collinear.
        }
    }
}

// Assumes a1x < b1x and a2x < b2x
template<bool swap1, bool swap2>
Segment2xIntersection intersectBothXOrdered(
    const Vec2x& a1,
    const Vec2x& b1,
    const Vec2x& a2,
    const Vec2x& b2) {

    if (a2.x() < a1.x()) {
        return intersectBothXOrdered_<S2S1, swap1, swap2>(a2, b2, a1, b1);
    }
    else {
        return intersectBothXOrdered_<S1S2, swap1, swap2>(a1, b1, a2, b2);
    }
}

} // namespace

Segment2xIntersection segmentIntersect(
    const Vec2x& a1,
    const Vec2x& b1,
    const Vec2x& a2,
    const Vec2x& b2) {

    // Order a1b1 and a2b2 by increasing X coordinates.
    // This is very important for robustness, see note below.
    // XXX: Can we do this branchless with a jump table / function pointers?
    // Would branch predictor in intersectBothXOrdered_ work better if
    // if wasn't templated, i.e., there would be only one function and
    // therefore only one "if" branch?
    if (a1.x() < b1.x()) {
        if (a2.x() < b2.x()) {
            return intersectBothXOrdered<A1B1, A2B2>(a1, b1, a2, b2);
        }
        else if (a2.x() > b2.x()) {
            return intersectBothXOrdered<A1B1, B2A2>(a1, b1, b2, a2);
        }
        else {
            //return intersectOneXOrderedWithVertical<S1S2, A1B1>(a1, b1, a2, b2);
        }
    }
    else if (a1.x() > b1.x()) {
        if (a2.x() < b2.x()) {
            return intersectBothXOrdered<B1A1, A2B2>(b1, a1, a2, b2);
        }
        else if (a2.x() > b2.x()) {
            return intersectBothXOrdered<B1A1, B2A2>(b1, a1, b2, a2);
        }
        else {
            //return intersectOneXOrderedWithVertical<S1S2, B1A1>(b1, a1, a2, b2);
        }
    }
    else {
        if (a2.x() < b2.x()) {
            // XXX: do we need 2 or 3 template params for this?
            //return intersectOneXOrderedWithVertical<S2S1, A2B2>(a2, b2, a1, b1);
        }
        else if (a2.x() > b2.x()) {
            //reversedSegment2 = true;
            //res = intersectOneVert(a1, b1, b2, a2);
        }
        else {
            return intersectBothVertical(a1, b1, a2, b2);
        }
    }
}

// Why ordering by X-increasing coordinates is important?
//
// This is not just a performance optimization, it has practical consequences
// on the result of the computation.
//
// Example:        c
//                 o
//     a                b
//     o----------------o
//
// How to determine if `c` lies exactly on the line (a, b)?
//
// In code, we do this by computing `det(b-a, c-a)` and check if it equals zero.
//
// But what if we swap a and b?
//
// Without X-ordering, we would compute `det(a-b, c-b)` instead.
//
// In theory, both of these values are exact opposite of each other:
//
// det(a-b, c-b) = det(a-b, (c-a)-(a-b))
//               = det(a-b, c-a) - det(a-b,a-b)
//               = det(a-b, c-a)
//               = - det(b-a, c-a)
//
// Yet with:
//   a = (0, 0)
//   b = (0.1, 10.01)
//   c = (1, 100.1)
//
// We have:
//   det(b-a, c-a) = +0.000000000000000000000000000000000000000000000000000
//   det(a-b, c-b) = +0.000000000000001776356839400250464677810668945312500
//
// We can see that numerical errors introduced in the computation mean that
// using (b, a) instead of (b, a) can change the output of the algorithm.
// Always doing the computation with a < b means that we guarantee that the
// results are consistent across multiple calls with segments sharing the
// same endpoints but simply reversed.

} // namespace vgc::geometry
