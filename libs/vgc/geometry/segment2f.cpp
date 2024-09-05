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

// This file was automatically generated, please do not edit directly.
// Instead, edit tools/segment2x.cpp then run tools/generate.py.

#include <vgc/geometry/segment2f.h>

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

// swapS == true means "the two segments were swapped for the computation"
// swap1 == true means "the segment that was originally first had its endpoints swapped"
// swap2 == true means "the segment that was originally second had its endpoints swapped"
template<bool swapS, bool swap1, bool swap2>
VGC_FORCE_INLINE Segment2fIntersection pointInter(const Vec2f& p, float t1, float t2) {
    if constexpr (swapS) {
        return Segment2fIntersection(p, param<swap1>(t2), param<swap2>(t1));
    }
    else {
        return Segment2fIntersection(p, param<swap1>(t1), param<swap2>(t2));
    }
}

// Note: we cannot guarantee both s1 <= t1 and s2 <= t2.
// For now it's possible that we return both s1 > t1 and s2 > t2.
// But should we guarantee s1 <= t1?
// Or perhaps guarantee s1 <= t1 OR s2 <= t2?
template<bool swapS, bool swap1, bool swap2>
VGC_FORCE_INLINE Segment2fIntersection
segInter(const Vec2f& p, const Vec2f& q, float s1, float t1, float s2, float t2) {
    if constexpr (swapS) {
        return Segment2fIntersection(
            p, q, param<swap1>(s2), param<swap1>(t2), param<swap2>(s1), param<swap2>(t1));
    }
    else {
        return Segment2fIntersection(
            p, q, param<swap1>(s1), param<swap1>(t1), param<swap2>(s2), param<swap2>(t2));
    }
}

// Assumes a1x == b1x == a2x == b2x
// Assumes a1y < b1y and a2y < b2y ("each segment is y-ordered")
// Assumes a1y <= a2y ("segments are y-ordered with each other")
template<bool swapS, bool swap1, bool swap2>
[[maybe_unused]] Segment2fIntersection intersectVerticalYOrdered_v1(
    const Vec2f& a1_,
    const Vec2f& b1_,
    const Vec2f& a2_,
    const Vec2f& b2_) {

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
[[maybe_unused]] Segment2fIntersection intersectVerticalYOrdered_v2(
    const Vec2f& a1_,
    const Vec2f& b1_,
    const Vec2f& a2_,
    const Vec2f& b2_) {

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

// Assumes a1x < b1x and a2x < b2x ("each segment is x-ordered")
// Assumes a1x <= a2x ("segments are x-ordered with each other")
// Assumes all four points are collinear.
// Assumes that the intersection is not null.
//
// TODO: Use Y-coord for better accuracy for almost-vertical segments?
//
template<bool swapS, bool swap1, bool swap2>
[[maybe_unused]] Segment2fIntersection intersectCollinearXOrdered_(
    const Vec2f& a1_,
    const Vec2f& b1_,
    const Vec2f& a2_,
    const Vec2f& b2_) {

    float a1 = a1_.x();
    float b1 = b1_.x();
    float a2 = a2_.x();
    float b2 = b2_.x();

    // The following case is already handled by the caller:
    //
    // if (b1 < a2) {
    //     // a1     b1
    //     // x------x  a2     b2
    //     //           x------x
    //     return {};
    // }

    if (b1 == a2) {
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
// for client code, since it arbritrarily introduces small inaccuracies in the
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
Segment2fIntersection intersectVerticalYOrdered(
    const Vec2f& a1,
    const Vec2f& b1,
    const Vec2f& a2,
    const Vec2f& b2) {

    if (a2.y() < a1.y()) {
        return intersectVerticalYOrdered_v2<S2S1, swap1, swap2>(a2, b2, a1, b1);
    }
    else {
        return intersectVerticalYOrdered_v2<S1S2, swap1, swap2>(a1, b1, a2, b2);
    }
}

// Assumes a1x == b1x == a2x == b2x
// Assumes a1y < b1y
template<bool swapS, bool swap1, bool swap2>
Segment2fIntersection
intersectVerticalYOrderedWithPoint(const Vec2f& a1_, const Vec2f& b1_, const Vec2f& a2_) {

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
Segment2fIntersection
intersectVertical(const Vec2f& a1, const Vec2f& b1, const Vec2f& a2, const Vec2f& b2) {

    if (a1.x() != a2.x()) {
        return {};
    }
    else if (a1.y() < b1.y()) {
        if (a2.y() < b2.y()) {
            return intersectVerticalYOrdered<A1B1, A2B2>(a1, b1, a2, b2);
        }
        else if (a2.y() > b2.y()) {
            return intersectVerticalYOrdered<A1B1, B2A2>(a1, b1, b2, a2);
        }
        else {
            return intersectVerticalYOrderedWithPoint<S1S2, A1B1, A2B2>(a1, b1, a2);
        }
    }
    else if (a1.y() > b1.y()) {
        if (a2.y() < b2.y()) {
            return intersectVerticalYOrdered<B1A1, A2B2>(b1, a1, a2, b2);
        }
        else if (a2.y() > b2.y()) {
            return intersectVerticalYOrdered<B1A1, B2A2>(b1, a1, b2, a2);
        }
        else {
            return intersectVerticalYOrderedWithPoint<S1S2, B1A1, A2B2>(b1, a1, a2);
        }
    }
    else {
        if (a2.y() < b2.y()) {
            return intersectVerticalYOrderedWithPoint<S2S1, A1B1, A2B2>(a2, b2, a1);
        }
        else if (a2.y() > b2.y()) {
            return intersectVerticalYOrderedWithPoint<S2S1, A1B1, B2A2>(b2, a2, a1);
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

// Assumes a1x < b1x and a2x < b2x and a1x <= a2x
//
// In particular, this means a1 != b1 and a2 != b2.
//
template<bool swapS, bool swap1, bool swap2>
Segment2fIntersection
intersectXOrdered_(const Vec2f& a1, const Vec2f& b1, const Vec2f& a2, const Vec2f& b2) {

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
    // Mathematically, if:
    // - three of those determinants are zero, or
    // - both a1Side and b1Side are zero, or
    // - both a2Side and b2Side are zero
    //
    // then all four of them are zero. But due to numerical errors, this may
    // not be the case. For robustness and consistency across multiple
    // computations involving shared endpoints, we always compute all four of
    // them, and determine the type of the intersection based on those.
    //
    Vec2f a1b1 = b1 - a1;
    Vec2f a2b2 = b2 - a2;
    Vec2f a1a2 = a2 - a1;
    Vec2f a1b2 = b2 - a1;
    Vec2f a2b1 = b1 - a2;
    float a1Side = -a2b2.det(a1a2); // == a2b2.det(a2a1)
    float b1Side = a2b2.det(a2b1);
    float a2Side = a1b1.det(a1a2);
    float b2Side = a1b1.det(a1b2);

    // Convert to sign since multiplying two non-zero floats may give zero.
    // Also, this leads to better branchless assembly on some compilers.
    //
    // Total of 3^4 = 81 combinations of possible sign values
    //
    Int8 a1Sign = core::sign(a1Side);
    Int8 b1Sign = core::sign(b1Side);
    Int8 a2Sign = core::sign(a2Side);
    Int8 b2Sign = core::sign(b2Side);

    // Whether each segment:
    // - has its endpoints on opposite sides of the other segment's line (cross == -1),
    // - has one or both its endpoints on the other segment's line (cross == 0),
    // - has its endpoints on the same side of the other segment's line (cross == 1)
    //
    // Total of 3^2 = 9 combinations of possible cross values
    //
    Int8 s1Cross = a1Sign * b1Sign;
    Int8 s2Cross = a2Sign * b2Sign;

    if (s1Cross > 0 || s2Cross > 0) {

        //  => The segments do not intersect
        //
        // (32 configurations of sign values)
        // (5 configurations of cross values)
        //
        return {};
    }
    else if (s1Cross < 0 && s2Cross < 0) {

        // => The segments intersect in their interior
        //
        // (4 configurations of sign values)
        // (1 configuration of cross values)
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
        // Mathematically, these two are equivalent, but due to numerical
        // errors, (A) and (B) may not in fact be equal. When this intersection
        // test is part of a larger algorithm (e.g., line-sweep), using the
        // sign predicates (B) rather than the result of the parameter solve
        // (A) improves correctness of the algorithm by having separate
        // computations be consistent with each other.
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
            Vec2f p = core::fastLerp(a1, b1, t1);
            return pointInter<swapS, swap1, swap2>(p, t1, t2);
        }
        else {
            // Mathematically, this shouldn't be possible. However, due to
            // numerical errors, it may or may not be possible. In case it does
            // occur, we consider this to mean that all four points are
            // collinear, despite all signs (a1Sign, etc.) being non-zero.
            // This shares similarity with case [4] below.
            //
            return intersectCollinearXOrdered_<swapS, swap1, swap2>(a1, b1, a2, b2);
        }
    }
    else { // s1Cross <= 0 && s2Cross <= 0 && (s1Cross == 0 || s2Cross == 0)

        // => The segments intersect in a special case configuration
        //
        // (45 configurations of sign values)
        // (3 configuration of cross values)
        //
        Int8 numCollinears = static_cast<Int8>(a2Sign == 0)   //
                             + static_cast<Int8>(b2Sign == 0) //
                             + static_cast<Int8>(a1Sign == 0) //
                             + static_cast<Int8>(b1Sign == 0);

        VGC_ASSERT(numCollinears > 0);

        if (numCollinears == 1) {

            // => Three points are collinear, one is not collinear.
            //
            // (16 configurations of sign values: 4 in each of the if below)
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
            if (a2Sign == 0) {
                //          x b2             x b2          x b2
                //         /      OR        /       OR    /
                // x------x            x---x---x         x------x
                // a1   a2,b1          a1  a2  b1      a1,a2    b1
                //
                // Note: a2 != a1 and a2 != b1 (since a1Sign != 0 and b1Sign != 0),
                //       but possibly a2x == a1x or a2x == a1x due to numerical errors.
                //
                float t1 = (a2.x() - a1.x()) / (b1.x() - a1.x()); // b2-independent
                return pointInter<swapS, swap1, swap2>(a2, t1, 0);
                // t1 is guaranteed to be 0 <= t1 <= 1
                // t1 is guaranteed to be exactly 0 or 1 if a2x equal to a1x or b1x
                // (!) t1 is NOT guaranteed to be 0 < t1 < 1 if a2x not equal to a1x or b1x
            }
            else if (b2Sign == 0) {
                // a1   b2,b1        a1  b2  b1
                // x------x          x---x---x
                //       /     OR       /
                //      x a2           x a2
                //
                // Note: a1x <= a2x and a2x < b2x so a1x < b2x
                //
                // Note: the following is in theory impossible
                // otherwise we'd have s1Cross > 0:
                //
                // a1     b1
                // x------x x b2
                //         /
                //        x a2
                //
                // But perhaps it is still possible to have b2x very slighly
                // larger than b1x due to numerical errors? Since we haven't
                // proven it that numerical errors causing this are impossible,
                // for now we clamp t1 to 1.
                //
                float t1 = (b2.x() - a1.x()) / (b1.x() - a1.x()); // a2-independent
                t1 = std::min<float>(t1, 1);
                return pointInter<swapS, swap1, swap2>(b2, t1, 1);
                // t1 is guaranteed to be 0 < t1 <= 1
                // t1 is guaranteed to be exactly 1 if b2x equal b1x
                // (!) t1 is NOT guaranteed to be t1 < 1 if b2x not equal to b1x
            }
            else if (a1Sign == 0) {
                // This means a1 on a2b2 line, and since a1x <= a2x and a2x < b2x,
                // the only cases are:
                //
                //     x b2
                //    /         OR
                //   x a2              x b2
                //                    /
                // x------x          x------x
                // a1     b1       a1,a2   b1
                //
                // In theory the first case is already rejected by a2Cross > 0,
                // and the second case means numCollinear == 2. However, due
                // to numerical errors, maybe this might happen and we return
                // the only solution compatible with sign predicates, which
                // corresponds to the second case.
                //
                // One may wonder whether a situation like the following could
                // go here due to numerical errors?
                //
                //  x b2 = (a1x + eps', C')
                //  |
                //  |
                //  x------x
                //  |a1    b1
                //  |
                //  x a2 = (a1x + eps, -C)
                //
                // We haven't proven that it's impossible neither found an
                // example, but in any case, this means we would have to do:
                //
                // float t2 = (a1x - a2x) / (b2x - a2x)
                //            -----------   -----------
                //               <= 0         > 0
                //
                // which still results in t2 = 0 after clamping.
                //
                return pointInter<swapS, swap1, swap2>(a1, 0, 0);
            }
            else { // b1Sign == 0
                //                            x b2               x b2
                // a1    b1,b2       a1      /          a1      /
                // x------x     OR   x------x b1   OR   x------x
                //       /                 /            a1   a2,b1
                //      x a2              x a2
                //
                // Similarly to b2Sign == 0, the case below should have
                // already been rejected by s2Cross > 0, and the first and
                // third case above is in theory impossible as it means
                // numCollinear == 2 but might be possible due to numerical
                // errors.
                //
                // a1     b1
                // x------x
                //
                //      x b2
                //     /
                //    x a2
                //
                // Note: a2x <= b1x (see fast return: x-ranges must intersect)
                //
                float t2 = (b1.x() - a2.x()) / (b2.x() - a2.x()); // a1-independent
                t2 = std::min<float>(t2, 1);
                return pointInter<swapS, swap1, swap2>(b1, 1, t2);
            }
        }
        else if (numCollinears == 2) {
            if (a1Sign == 0 && b1Sign == 0) {
                // (2 configurations of sign values)
                //
                // [3] Mathematically, this means both a1 and b1 are on a2b2
                // line, so all four points are collinear, so a2Sign and b2Sign
                // should also be equal to zero, but are not due to numerical
                // errors.
                //
                // Although we haven't yet found an example of this case, we
                // believe it might be possible in situations that look like
                // the following, where large size disparities between a1b1 and
                // a2b2 make numerical errors more likely.
                //
                // a1                                  x b2                 b1
                // x--------------------------------------------------------x
                //                          x a2
                //
                return intersectCollinearXOrdered_<swapS, swap1, swap2>(a1, b1, a2, b2);
            }
            else if (a2Sign == 0 && b2Sign == 0) {
                // (2 configurations of sign values)
                // Similar as [3]
                return intersectCollinearXOrdered_<swapS, swap1, swap2>(a1, b1, a2, b2);
            }
            else if (a1Sign == 0 && a2Sign == 0) {
                // (4 configurations of sign values)
                //
                // [4] Mathematically, this means that a1 == a2 and that the
                // four points are not collinear (since numCollinears != 4).
                //
                // But due to numerical errors, this can in fact either mean
                // that a1 is really close to a2, or that all four points are
                // nearly collinear (and a1 can be far from a2).
                //
                // We use the following test to leave the ambiguity, where we
                // intentionally consider that if both d2 and delta are zero,
                // then this means that a1 == a2 but the four points are non
                // collinear, since the former can be evaluated exactly, while
                // the latter is subject to numerical errors.
                //
                float d2 = a1a2.squaredLength(); // 0 if a1 == a2
                float delta = a1b1.det(a2b2);    // 0 if a1b1 parallel to a2b2
                if (d2 <= std::abs(delta)) {
                    return pointInter<swapS, swap1, swap2>(a1, 0, 0);
                }
                else {
                    return intersectCollinearXOrdered_<swapS, swap1, swap2>(
                        a1, b1, a2, b2);
                }
            }
            else if (a1Sign == 0 && b2Sign == 0) {
                // (4 configurations of sign values)
                // Similar as [4]
                float d2 = a1b2.squaredLength();
                float delta = a1b1.det(a2b2);
                if (d2 <= std::abs(delta)) {
                    return pointInter<swapS, swap1, swap2>(a1, 0, 1);
                }
                else {
                    return intersectCollinearXOrdered_<swapS, swap1, swap2>(
                        a1, b1, a2, b2);
                }
            }
            else if (b1Sign == 0 && a2Sign == 0) {
                // (4 configurations of sign values)
                // Similar as [4]
                float d2 = a2b1.squaredLength();
                float delta = a1b1.det(a2b2);
                if (d2 <= std::abs(delta)) {
                    return pointInter<swapS, swap1, swap2>(b1, 1, 0);
                }
                else {
                    return intersectCollinearXOrdered_<swapS, swap1, swap2>(
                        a1, b1, a2, b2);
                }
            }
            else { // (b1Sign == 0 && b2Sign == 0)
                // (4 configurations of sign values)
                // Similar as [4]
                float d2 = (b2 - b1).squaredLength();
                float delta = a1b1.det(a2b2);
                if (d2 <= std::abs(delta)) {
                    return pointInter<swapS, swap1, swap2>(b1, 1, 1);
                }
                else {
                    return intersectCollinearXOrdered_<swapS, swap1, swap2>(
                        a1, b1, a2, b2);
                }
            }
        }
        else { // numCollinears >= 3
            // (9 configurations of sign values)
            return intersectCollinearXOrdered_<swapS, swap1, swap2>(a1, b1, a2, b2);
        }
    }
}

// Assumes a1x < b1x and a2x < b2x
template<bool swap1, bool swap2>
Segment2fIntersection
intersectXOrdered(const Vec2f& a1, const Vec2f& b1, const Vec2f& a2, const Vec2f& b2) {
    if (a2.x() < a1.x()) {
        return intersectXOrdered_<S2S1, swap1, swap2>(a2, b2, a1, b1);
    }
    else {
        return intersectXOrdered_<S1S2, swap1, swap2>(a1, b1, a2, b2);
    }
}

// Assumes a1x < b1x
// Assumes a1x <= a2x == b2x <= b1x
// Assumes a2y == b2y
template<bool swapS, bool swap1, bool swap2>
Segment2fIntersection
intersectXOrderedWithPoint(const Vec2f& a1, const Vec2f& b1, const Vec2f& a2) {
    Vec2f a1b1 = b1 - a1;
    Vec2f a1a2 = a2 - a1;
    float a2Side = a1b1.det(a1a2);
    if (a2Side == 0) {
        float t1 = (a2.x() - a1.x()) / (b1.x() - a1.x());
        return pointInter<swapS, swap1, swap2>(a2, t1, 0);
    }
    else {
        return {};
    }
}

// Assumes a1x < b1x
// Assumes a1x <= a2x == b2x <= b1x
// Assumes a2y < b2y
template<bool swapS, bool swap1, bool swap2>
Segment2fIntersection intersectXOrderedWithVerticalYOrdered(
    const Vec2f& a1,
    const Vec2f& b1,
    const Vec2f& a2,
    const Vec2f& b2) {

    float a2y = a2.y();
    float b2y = b2.y();

    // See intersectXOrdered() for details
    Vec2f a1b1 = b1 - a1;
    Vec2f a1a2 = a2 - a1;
    Vec2f a1b2 = b2 - a1;
    float a2Side = a1b1.det(a1a2);
    float b2Side = a1b1.det(a1b2);
    Int8 a2Sign = core::sign(a2Side);
    Int8 b2Sign = core::sign(b2Side);
    Int8 s2Cross = a2Sign * b2Sign;

    if (s2Cross > 0) {
        return {};
    }

    float t1 = (a2.x() - a1.x()) / (b1.x() - a1.x()); // Guaranteed 0 <= t1 <= 1
    if (s2Cross < 0) {
        Vec2f p = core::fastLerp(a1, b1, t1);
        float t2 = (p.y() - a2y) / (b2y - a2y);
        t2 = core::clamp(t2, 0, 1);
        return pointInter<swapS, swap1, swap2>(p, t1, t2);
    }
    else { // s2Cross == 0
        if (a2Sign == 0) {
            return pointInter<swapS, swap1, swap2>(a2, t1, 0);
        }
        else { // b2Sign == 0
            return pointInter<swapS, swap1, swap2>(b2, t1, 1);
        }
    }
}

// Assumes a1x < b1x and a2x == b2x
template<bool swapS, bool swap1, bool swap2>
Segment2fIntersection intersectXOrderedWithVertical(
    const Vec2f& a1,
    const Vec2f& b1,
    const Vec2f& a2,
    const Vec2f& b2) {

    // X-coordinate of vertical a2b2 segment
    float x2 = a2.x(); // == b2.x()

    // Fast return if vertical segment not in range
    if (b1.x() < x2) {
        // a1     b1 x b2
        // x------x  |
        //           x a2
        return {};
    }
    if (x2 < a1.x()) {
        // b2 x a1     b1
        //    | x------x
        // a2 x
        return {};
    }

    // Other cases
    if (a2.y() < b2.y()) {
        return intersectXOrderedWithVerticalYOrdered<swapS, swap1, swap2>(a1, b1, a2, b2);
    }
    else if (a2.y() > b2.y()) {
        constexpr bool swp1_ = swapS ? B1A1 : swap1;
        constexpr bool swp2_ = swapS ? swap2 : B2A2;
        return intersectXOrderedWithVerticalYOrdered<swapS, swp1_, swp2_>(a1, b1, b2, a2);
    }
    else {
        return intersectXOrderedWithPoint<swapS, swap1, swap2>(a1, b1, a2);
    }
}

} // namespace

// Possible ideas for performance improvements:
// - Use sign function?
// - Make the x-ordering branchless with a jump table?
// - Make code non-templated to be more friendly to branch predictor?
//
Segment2fIntersection segmentIntersect(
    const Vec2f& a1,
    const Vec2f& b1,
    const Vec2f& a2,
    const Vec2f& b2) {

    // Order a1b1 and a2b2 by increasing X coordinates.
    // This is very important for robustness, see note below.
    //
    if (a1.x() < b1.x()) {
        if (a2.x() < b2.x()) {
            return intersectXOrdered<A1B1, A2B2>(a1, b1, a2, b2);
        }
        else if (a2.x() > b2.x()) {
            return intersectXOrdered<A1B1, B2A2>(a1, b1, b2, a2);
        }
        else {
            return intersectXOrderedWithVertical<S1S2, A1B1, A2B2>(a1, b1, a2, b2);
        }
    }
    else if (a1.x() > b1.x()) {
        if (a2.x() < b2.x()) {
            return intersectXOrdered<B1A1, A2B2>(b1, a1, a2, b2);
        }
        else if (a2.x() > b2.x()) {
            return intersectXOrdered<B1A1, B2A2>(b1, a1, b2, a2);
        }
        else {
            return intersectXOrderedWithVertical<S1S2, B1A1, A2B2>(b1, a1, a2, b2);
        }
    }
    else {
        if (a2.x() < b2.x()) {
            return intersectXOrderedWithVertical<S2S1, A1B1, A2B2>(a2, b2, a1, b1);
        }
        else if (a2.x() > b2.x()) {
            return intersectXOrderedWithVertical<S2S1, A1B1, B2A2>(b2, a2, a1, b1);
        }
        else {
            return intersectVertical(a1, b1, a2, b2);
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
