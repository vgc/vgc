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

#include <vgc/tools/sketchpasses.h>

#include <array>

#include <vgc/core/assert.h>
#include <vgc/geometry/bezier.h>
#include <vgc/tools/logcategories.h>

namespace vgc::tools {

void EmptyPass::doUpdateFrom(const SketchPointBuffer& input, SketchPointBuffer& output) {

    // Remove all previously unstable points.
    //
    Int oldNumStablePoints = output.numStablePoints();
    output.resize(oldNumStablePoints);

    // Add all other points (some of which are now stable, some of which are
    // still unstable).
    //
    output.extend(input.begin() + oldNumStablePoints, input.end());

    // Set the new number of stable points as being the same as the input.
    //
    output.setNumStablePoints(input.numStablePoints());

    // Note: there is no need to compute the output chord lengths in the
    // EmptyPass since they are the same as the input chord length.
}

void TransformPass::doUpdateFrom(
    const SketchPointBuffer& input,
    SketchPointBuffer& output) {

    // Remove all previously unstable points.
    //
    Int oldNumStablePoints = output.numStablePoints();
    output.resize(oldNumStablePoints);

    // Add all other points (some of which are now stable, some of which are
    // still unstable).
    //
    for (auto it = input.begin() + oldNumStablePoints; it != input.end(); ++it) {
        SketchPoint p = *it;
        p.setPosition(transformAffine(p.position()));
        output.append(p);
    }

    // Updates chord lengths.
    //
    output.updateChordLengths();

    // Set the new number of stable points as being the same as the input.
    //
    output.setNumStablePoints(input.numStablePoints());
}

namespace {

// Returns the binomial coefficient C(n, k) for 0 <= k <= n.
//
// The returned array is of size n + 1.
//
// These are computed using the Pascal's triangle:
//
//  C(0, k) =       1
//  C(1, k) =      1 1
//  C(2, k) =     1 2 1
//  C(3, k) =    1 3 3 1
//  C(4, k) =   1 4 6 4 1
//
template<size_t n>
constexpr std::array<Int, n + 1> binomialCoefficients() {
    std::array<Int, n + 1> res{}; // Value-initialization required for constexprness.
    res[0] = 1;
    for (size_t m = 1; m <= n; ++m) {
        // Compute C(m, k) coefficients from C(m-1, k) coefficients
        res[m] = 1;
        for (size_t k = m - 1; k >= 1; --k) {
            res[k] = res[k] + res[k - 1];
        }
    }
    return res;
}

bool clampMin_(double k, SketchPoint& p, const SketchPoint& minLimitor, double ds) {
    double minWidth = minLimitor.width() - k * ds;
    if (p.width() < minWidth) {
        p.setWidth(minWidth);
        return true;
    }
    return false;
}

void clampMax_(double k, SketchPoint& p, const SketchPoint& maxLimitor, double ds) {
    double maxWidth = maxLimitor.width() + k * ds;
    if (p.width() > maxWidth) {
        p.setWidth(maxWidth);
    }
}

// Clamps a point p based on a minLimitor before p.
// Returns whether p was widened according to minLimitor.
//
bool clampMinForward(double k, SketchPoint& p, const SketchPoint& minLimitor) {
    double dsMinLimitor = p.s() - minLimitor.s();
    return clampMin_(k, p, minLimitor, dsMinLimitor);
}

// Clamps a point p based on a minLimitor after p.
// Returns whether p was widened according to minLimitor.
//
bool clampMinBackward(double k, SketchPoint& p, const SketchPoint& minLimitor) {
    double dsMinLimitor = minLimitor.s() - p.s();
    return clampMin_(k, p, minLimitor, dsMinLimitor);
}

// Clamps a point p based on a maxLimitor before p.
//
void clampMaxForward(double k, SketchPoint& p, const SketchPoint& maxLimitor) {
    double dsMaxLimitor = p.s() - maxLimitor.s();
    clampMax_(k, p, maxLimitor, dsMaxLimitor);
}

// Clamps a point p based on a minLimitor and maxLimitor before p.
// Returns whether p was widened according to minLimitor.
//
bool clampMinMaxForward(
    double k,
    SketchPoint& p,
    const SketchPoint& minLimitor,
    const SketchPoint& maxLimitor) {

    if (clampMinForward(k, p, minLimitor)) {
        return true;
    }
    else {
        clampMaxForward(k, p, maxLimitor);
        return false;
    }
}

// Ensures that |dw/ds| <= k (e.g., k = 0.5).
//
// Importantly, we should have at least |dw/ds| <= 1, otherwise in the current
// tesselation model with round caps, it causes the following ugly artifact:
//
// Mouse move #123:   (pos = (100, 0), width = 3)
//
// -------_
//         =
//      +   |
//         =
// -------'
//
// Mouse move #124:   (pos = (101, 0), width = 1)
//
// -------   <- Ugly temporal disontinuity (previously existing geometry disappears)
//        .  <- Ugly geometric discontinuity (prone to cusps, etc.)
//       + |
//        '
// -------
//
// The idea is that what users see should be as close as possible as the
// "integral of disks" interpretation of a brush stroke. With a physical round
// paint brush, if you push the brush more then it creates a bigger disk. If
// you then pull a little without moving lateraly, then it doesn't remove what
// was previously already painted.
//
// Algorithm pseudo-code when applied to a global list of points:
//
// 1. Sort samples by width in a list
// 2. While the list isn't empty:
//    a. Pop sample with largest width
//    b. Modify the width of its two siblings to enforce |dw/ds| <= k
//    c. Update the location of the two siblings in the sorted list to keep it sorted
//
// Unfortunately, the above algorithm have global effect: adding one point with
// very large width might increase the width of all previous points. This is
// undesirable for performance and user-predictibility, as we want to keep the
// "unstable points" part of the sketched curve as small as possible.
// Therefore, in the implementation below, we only allow for a given point to
// affect `windowSize` points before itself.
//
void applyWidthRoughnessLimitor(
    double k,
    Int windowSize,
    const SketchPoint* lastStablePoint, // nullptr if no stable points
    core::Span<SketchPoint> unstablePoints) {

    if (unstablePoints.isEmpty()) {
        return;
    }

    // Apply width-limitor to first unstable point.
    //
    const SketchPoint* minLimitor = lastStablePoint;
    const SketchPoint* maxLimitor = lastStablePoint;
    if (lastStablePoint) {
        clampMinMaxForward(k, unstablePoints[0], *minLimitor, *maxLimitor);
    }

    // Apply width-limitor to subsequent unstable points.
    //
    for (Int i = 1; i < unstablePoints.length(); ++i) {

        //                   window size = 3    (each point influences up to
        //                <------------------|    3 points before itself)
        //
        //          p[windowStart]   p[i-1] p[i]
        // x-----x--------x--------x----x----x
        //   maxLimitor
        //
        Int windowStart = i - windowSize;
        if (windowStart > 0) {
            maxLimitor = &unstablePoints[windowStart - 1];
        }
        else {
            windowStart = 0;
            // maxLimitor == lastStablePoint
        }
        SketchPoint& p = unstablePoints[i];

        // Widen current point p[i] based on p[windowStart - 1]
        // Shorten current point p[i] based on p[i - 1]
        minLimitor = &unstablePoints[i - 1];
        bool widened = maxLimitor ? clampMinMaxForward(k, p, *minLimitor, *maxLimitor)
                                  : clampMinForward(k, p, *minLimitor);

        // Widen previous points within window if necessary.
        // Note: whenever a point is not itself widened, we know that previous
        // points will not be widened either, so we can skip computation.
        if (!widened) {
            for (Int j = i - 1; j >= windowStart; --j) {
                if (!clampMinBackward(k, unstablePoints[j], p)) {
                    break;
                }
            }
        }
    }
}

} // namespace

void SmoothingPass::doUpdateFrom(
    const SketchPointBuffer& input,
    SketchPointBuffer& output) {

    const SketchPointArray& inputPoints = input.data();
    Int numPoints = inputPoints.length();
    Int oldNumStablePoints = output.numStablePoints();
    if (numPoints == oldNumStablePoints) {
        return;
    }

    // Keep our stable points, fill the rest with the input points.
    Int unstableIndexStart = oldNumStablePoints;
    output.resize(oldNumStablePoints);
    output.extend(input.begin() + unstableIndexStart, input.end());

    Int instabilityDelta = 0;

    Int pointsSmoothingLevel = 2;
    if (pointsSmoothingLevel > 0 && numPoints >= 3) {
        // Apply gaussian smoothing.
        Int iStart = unstableIndexStart;
        if (pointsSmoothingLevel == 1) {
            iStart = std::max<Int>(1, iStart);
            for (Int i = iStart; i < numPoints - 1; ++i) {
                output.at(i).setPosition(                                    //
                    (1 / 4.0) * inputPoints.getUnchecked(i - 1).position() + //
                    (2 / 4.0) * inputPoints.getUnchecked(i + 0).position() + //
                    (1 / 4.0) * inputPoints.getUnchecked(i + 1).position());
            }
        }
        else if (pointsSmoothingLevel == 2) {
            if (iStart <= 1) {
                output.at(1).setPosition(                                //
                    (1 / 4.0) * inputPoints.getUnchecked(0).position() + //
                    (2 / 4.0) * inputPoints.getUnchecked(1).position() + //
                    (1 / 4.0) * inputPoints.getUnchecked(2).position());
                iStart = 2;
            }
            for (Int i = iStart; i < numPoints - 2; ++i) {
                output.at(i).setPosition(                                     //
                    (1 / 16.0) * inputPoints.getUnchecked(i - 2).position() + //
                    (4 / 16.0) * inputPoints.getUnchecked(i - 1).position() + //
                    (6 / 16.0) * inputPoints.getUnchecked(i + 0).position() + //
                    (4 / 16.0) * inputPoints.getUnchecked(i + 1).position() + //
                    (1 / 16.0) * inputPoints.getUnchecked(i + 2).position());
            }
            if (numPoints - 2 >= iStart) {
                Int i = numPoints - 2;
                output.at(i).setPosition(                                    //
                    (1 / 4.0) * inputPoints.getUnchecked(i - 1).position() + //
                    (2 / 4.0) * inputPoints.getUnchecked(i + 0).position() + //
                    (1 / 4.0) * inputPoints.getUnchecked(i + 1).position());
            }
        }
    }
    instabilityDelta = std::max<Int>(instabilityDelta, pointsSmoothingLevel);

    // Smooth width.
    //
    // This is different as smoothing points since we don't need to keep
    // the first/last width unchanged.
    //
    constexpr size_t widthSmoothingLevel = 2;
    if constexpr (widthSmoothingLevel != 0) {

        // Get binomial coefficients
        constexpr Int l = widthSmoothingLevel;
        constexpr Int m = 2 * widthSmoothingLevel + 1;
        constexpr std::array<Int, m> coeffs =
            binomialCoefficients<2 * widthSmoothingLevel>();

        // Apply convolution with coefficients
        for (Int i = unstableIndexStart; i < numPoints; ++i) {
            double value = {};
            double sumCoeffs = 0;
            Int j = i - l;
            for (Int k = 0; k < m; ++k, ++j) {
                if (0 <= j && j < numPoints) {
                    sumCoeffs += coeffs[k];
                    value += coeffs[k] * inputPoints[j].width();
                }
            }
            output.at(i).setWidth(value / sumCoeffs);
        }
        instabilityDelta = std::max<Int>(instabilityDelta, widthSmoothingLevel);
    }

    // compute chord lengths
    output.updateChordLengths();

    // Width limitor
    constexpr double widthRoughness = 0.8;
    constexpr Int roughnessLimitorWindowSize = 3;
    const SketchPoint* lastStablePoint =
        oldNumStablePoints == 0 ? nullptr : &output[oldNumStablePoints - 1];
    applyWidthRoughnessLimitor(
        widthRoughness,
        roughnessLimitorWindowSize,
        lastStablePoint,
        output.unstablePoints());
    instabilityDelta += roughnessLimitorWindowSize;

    output.setNumStablePoints(
        std::max<Int>(0, input.numStablePoints() - instabilityDelta));
}

namespace {

// This is a variant of Douglas-Peucker designed to dequantize mouse inputs
// from integer to float coordinates.
//
// To this end, the distance test checks if the current line segment AB passes
// through all pixel squares of the samples in the interval. We call
// `threshold` the minimal distance between AB and the pixel center such that
// AB does not passes through the pixel square.
//
//     ---------------
//    |               |
//    |     pixel     |             threshold = distance(pixelCenter, A'B'),
//    |     center    |         B'  where A'B' is a line parallel to AB and
//    |       x       |       '     touching the pixel square.
//    |               |     '
//    |               |   '
//    |               | '
//     ---------------'
//                  '
//                ' A'
//
// This threshold only depends on the angle AB. If AB is perfectly horizontal
// or vertical, the threshold is equal to 0.5. If AB is at 45°, the threshold
// is equal to sqrt(2)/2 (the half-diagonal).
//
// We then scale this threshold is scaled with the given `thresholdCoefficient`
// and add the given `tolerance`.
//
// If the current segment does not pass the test, then the farthest samples is
// selected for the next iteration.
//
// In some variants of this algorithm, we may also slighly move the position of
// selected samples towards the segment AB (e.g., by 0.75 * threshold), which
// seems to empirically give nicer results in some circumstances.
//
Int douglasPeucker(
    core::Span<SketchPoint> points,
    core::IntArray& indices,
    Int intervalStart,
    double thresholdCoefficient,
    double tolerance = 1e-10) {

    Int i = intervalStart;
    Int endIndex = indices[i + 1];
    while (indices[i] != endIndex) {

        // Get line AB. Fast discard if AB too small.
        Int iA = indices[i];
        Int iB = indices[i + 1];
        geometry::Vec2d a = points[iA].position();
        geometry::Vec2d b = points[iB].position();
        geometry::Vec2d ab = b - a;
        double abLen = ab.length();
        if (abLen < core::epsilon) {
            ++i;
            continue;
        }

        // Compute `threshold`
        constexpr double sqrtOf2 = 1.4142135623730950488017;
        double abMaxNormalizedAbsoluteCoord =
            (std::max)(std::abs(ab.x()), std::abs(ab.y())) / abLen;
        double abAngleWithClosestAxis = std::acos(abMaxNormalizedAbsoluteCoord);
        double threshold =
            std::cos(core::pi / 4 - abAngleWithClosestAxis) * (sqrtOf2 / 2);

        // Apply threshold coefficient and additive tolerance
        double adjustedThreshold = thresholdCoefficient * threshold + tolerance;

        // Compute which sample between A and B is furthest from the line AB
        double maxDist = 0;
        Int farthestPointSide = 0;
        Int farthestPointIndex = -1;
        for (Int j = iA + 1; j < iB; ++j) {
            geometry::Vec2d p = points[j].position();
            geometry::Vec2d ap = p - a;
            double dist = ab.det(ap) / abLen;
            // ┌─── x
            // │    ↑ side 1
            // │ A ───→ B
            // y    ↓ side 0
            Int side = 0;
            if (dist < 0) {
                dist = -dist;
                side = 1;
            }
            if (dist > maxDist) {
                maxDist = dist;
                farthestPointSide = side;
                farthestPointIndex = j;
            }
        }

        // If the furthest point is too far from AB, then recurse.
        // Otherwise, stop the recursion and move one to the next segment.
        if (maxDist > adjustedThreshold) {

            // Add sample to the list of selected samples
            indices.insert(i + 1, farthestPointIndex);

            // Move the position of the selected sample slightly towards AB
            constexpr bool isMoveEnabled = true;
            if (isMoveEnabled) {
                geometry::Vec2d n = ab.orthogonalized() / abLen;
                if (farthestPointSide != 0) {
                    n = -n;
                }
                // TODO: scale delta based on some data to prevent shrinkage?
                double delta = 0.8 * threshold;
                SketchPoint& p = points[farthestPointIndex];
                p.setPosition(p.position() - delta * n);
            }
        }
        else {
            ++i;
        }
    }
    return i;
}

} // namespace

void DouglasPeuckerPass::doUpdateFrom(
    const SketchPointBuffer& input,
    SketchPointBuffer& output) {

    // A copy is required to make a mutable span, which the Douglas-Peuckert
    // algorithm needs (it modifies the points slightly).
    //
    SketchPointArray inputPoints = input.data();

    Int firstIndex = 0;
    Int lastIndex = inputPoints.length() - 1;
    core::IntArray indices = {firstIndex, lastIndex};
    Int intervalStart = 0;

    core::Span<SketchPoint> span = inputPoints;
    double thresholdCoefficient = 1.0;
    douglasPeucker(span, indices, intervalStart, thresholdCoefficient);

    Int numSimplifiedPoints = indices.length();
    output.resize(numSimplifiedPoints);
    for (Int i = 0; i < numSimplifiedPoints; ++i) {
        output.at(i) = inputPoints[indices[i]];
    }

    output.updateChordLengths();

    // For now, for simplicity, we do not provide any stable points guarantees
    // and simply recompute the Douglas-Peuckert algorithm from scratch.
    //
    output.setNumStablePoints(0);
}

namespace {

// Sets the output to be a line segment from first to last input point,
// assuming the first output point is stable as soon as the first input point
// is stable too.
//
void setLineSegmentWithFixedEndpoints(
    const SketchPointBuffer& input,
    SketchPointBuffer& output) {

    if (!input.isEmpty()) {
        output.resize(2);
        if (output.numStablePoints() == 0) {
            output.at(0) = input.first();
        }
        output.at(1) = input.last();
        output.updateChordLengths();
        output.setNumStablePoints(input.numStablePoints() > 0 ? 1 : 0);
    }
}

// Sets the output to be a line segment from first to last input point,
// assuming the first output point is never stable.
//
void setLineSegmentWithFreeEndpoints(
    const SketchPointBuffer& input,
    SketchPointBuffer& output) {

    if (!input.isEmpty()) {
        output.resize(2);
        output.at(0) = input.first();
        output.at(1) = input.last();
        output.updateChordLengths();
        output.setNumStablePoints(0);
    }
}

// When fitting a curve with fixed endpoints, this handles the case where the
// input is "small" (only two points or less), in which case the output should
// simply be a line (or an empty array).
//
// Returns whether the input was indeed small and therefore handled.
//
bool handleSmallInputWithFixedEndpoints(
    const SketchPointBuffer& input,
    SketchPointBuffer& output) {

    if (input.length() <= 2) {
        setLineSegmentWithFixedEndpoints(input, output);
        return true;
    }
    else {
        return false;
    }
}

// When fitting a curve with free endpoints, this handles the case where the
// input is "small" (only two points or less), in which case the output should
// simply be a line (or an empty array).
//
// Returns whether the input was indeed small and therefore handled.
//
bool handleSmallInputWithFreeEndpoints(
    const SketchPointBuffer& input,
    SketchPointBuffer& output) {

    if (input.length() <= 2) {
        setLineSegmentWithFreeEndpoints(input, output);
        return true;
    }
    else {
        return false;
    }
}

} // namespace

void SingleLineSegmentWithFixedEndpointsPass::doUpdateFrom(
    const SketchPointBuffer& input,
    SketchPointBuffer& output) {

    setLineSegmentWithFixedEndpoints(input, output);
}

// This basically implements:
//
// [1] Graphics Gems 5: The Best Least-Squares Line Fit (Alciatore and Miranda 1995)
//
// The chapter above provides a proof that the line minimizing the squared
// orthogonal distances to the input points goes through the centroid, and
// derives a closed form formula for the direction of the line.
//
// I believe this is equivalent to computing the first principal component of
// the data after centering it around the centroid (see PCA / SVD methods).
//
void SingleLineSegmentWithFreeEndpointsPass::doUpdateFrom(
    const SketchPointBuffer& input,
    SketchPointBuffer& output) {

    if (handleSmallInputWithFreeEndpoints(input, output)) {
        return;
    }

    // Compute centroid
    Int numPoints = input.length();
    geometry::Vec2d centroid;
    for (const SketchPoint& p : input) {
        centroid += p.position();
    }
    centroid /= core::narrow_cast<double>(numPoints);

    // Compute a = sum(xi² - yi²) and b = sum(2 xi yi)
    //
    // These can be interpreted as the coefficients of a complex number z = a + ib
    // such that sqrt(z) is parallel to the best fit line. See:
    //
    // [2] https://en.wikipedia.org/wiki/Deming_regression#Orthogonal_regression
    //
    double a = 0;
    double b = 0;
    for (const SketchPoint& p : input) {
        geometry::Vec2d q = p.position() - centroid;
        a += q.x() * q.x() - q.y() * q.y();
        b += q.x() * q.y();
    }
    b *= 2;

    // Compute coefficients of the best fit line as Ax + By + C = 0
    //
    // Note: if b = 0, then the best fit line is perfectly horizontal or
    // vertical, and [1] actually fails to handle/discuss the vertical case.
    //
    // Example 1: input points = (1, 0), (-1, 0)
    // The equation provided in [1] gives:
    // a = 2       b = 0
    // A = b = 0   B = -a - sqrt(a² + b²) = -4    => OK (horizontal line)
    //
    // Example 1: input points = (0, 1), (0, -1)
    // The equation provided in [1] gives:
    // a = -2      b = 0
    // A = b = 0   B = -a - sqrt(a² + b²) = 0     => WRONG (we need A != 0)
    //
    // The vertical case corresponds to the special case "z is a negative real
    // number" when computing the square root of a complex number via the
    // method in https://math.stackexchange.com/a/44500, which is essentially
    // the geometric interpretation of the equations provided in [1].
    //
    double A = 1;
    double B = 0;
    if (b == 0) { // XXX or |b| < eps * |a| ? What's a good eps?
        if (a < 0) {
            // Intuition: sum(xi²) < sum(yi²) => vertical line
            A = 1;
            B = 0;
        }
        else {
            // Intuition: sum(xi²) > sum(yi²) => horizontal line
            A = 0;
            B = 1;
        }
        // Note: if b == 0 AND a == 0, this means there is a circular symmetry:
        // all lines passing through the centroid are equally good/bad.
    }
    else {
        A = b;
        B = -(a + std::sqrt(a * a + b * b));
    }
    // Note: C = - A * centroid.x() + B * centroid.y(), but we do not need it.

    // Initialize the output from the first and last input points.
    // This ensures that we already have the width and timestamps correct.
    //
    output.resize(2);
    SketchPoint& p0 = output.at(0);
    SketchPoint& p1 = output.at(1);
    output.at(0) = input.first();
    output.at(1) = input.last();

    // Find points further away from centroid along the line, and project them
    // on the line to define the two output positions
    //
    // Note: the normal / direction vector is non-null since we know that
    // (A, B) is either (1, 0), (0, 1), or (b, ...) with b != 0
    //
    geometry::Vec2d d(-B, A);
    double vMin = core::DoubleInfinity;
    double vMax = -core::DoubleInfinity;
    for (const SketchPoint& p : input) {
        geometry::Vec2d q = p.position() - centroid;
        double v = q.dot(d);
        if (v < vMin) {
            vMin = v;
        }
        if (v > vMax) {
            vMax = v;
        }
    }
    double l2inv = 1.0 / d.squaredLength();
    geometry::Vec2d pMin = centroid + l2inv * vMin * d;
    geometry::Vec2d pMax = centroid + l2inv * vMax * d;
    if ((p1.position() - p0.position()).dot(d) > 0) {
        p0.setPosition(pMin);
        p1.setPosition(pMax);
    }
    else {
        p0.setPosition(pMax);
        p1.setPosition(pMin);
    }

    output.updateChordLengths();
    output.setNumStablePoints(0);

    // TODO: better width than using the width of first and last point?
}

namespace {

// Input:  n positions P0, ..., Pn-1
//         n params    u0, ..., un-1
//
// Output: Quadratic Bezier control points (B0, B1, B2) that minimizes:
//
//   E = sum || B(ui) - Pi ||²
//
// where:
//
//   B(u) = (1 - u)² B0 + 2(1 - u)u B1 + u² B2
//
//   B0 = P0
//   B1 = (x, y) is the variable that we solve for
//   B2 = Pn-1
//
// How do we solve for B1?
//
// The minimum of E is reached when dE/dx = 0 and dE/dy = 0
//
// dE/dx = sum d/dx(|| B(ui) - Pi ||²)
//       = sum 2 (B(ui) - Pi) ⋅ d/dx(B(ui) - Pi)    (dot product)
//
// d/dx(B(ui) - Pi) = d/dx(B(ui)) - d/dx(Pi)
//                  = d/dx(B(ui))              (since Pi is a constant)
//                  = (1 - ui)² d(B0)/dx + 2(1 - ui)ui d(B1)/dx + ui² d(B2)/dx
//                              ^^^^^^^^               ^^^^^^^^       ^^^^^^^^
//                              = (0, 0)               = (1, 0)       = (0, 0)
//
// So with the notations:
//   a0i = (1 - ui)²
//   a1i = 2(1 - ui)ui
//   a2i = ui²
//
// We have:
//
// d/dx(B(ui) - Pi) = (a1i, 0)
//
// And in the dot product (B(ui) - Pi) ⋅ d/dx(B(ui) - Pi), only the X-component
// is non-null and we get:
//
// dE/dx = sum 2 (B(ui)x - Pix) a1i
//       = sum 2 (a0i B0x + a1i x + a2i B2x - Pix) a1i
//
// Therefore,
//
// dE/dx = 0 <=> sum (a0i B0x + a1i x + a2i B2x - Pix) a1i = 0
//           <=> x * sum (a1i²) = sum (Pix - a0i B0x - a2i B2x) a1i
//
// So we get x = sum (Pix - a0i B0x - a2i B2x) a1i / sum (a1i²)
//
// We get a similar result for y, so in the end:
//
// (x, y) = sum (Pi - a0i B0 - a2i B2) a1i / sum (a1i²)
//
geometry::QuadraticBezier2d quadraticFitWithFixedEndpoints(
    core::ConstSpan<geometry::Vec2d> positions,
    core::ConstSpan<double> params) {

    Int n = positions.length();
    VGC_ASSERT(positions.length() == params.length());
    VGC_ASSERT(n > 0);

    const geometry::Vec2d& B0 = positions.first();
    if (n == 1) {
        return geometry::QuadraticBezier2d(B0, B0, B0);
    }

    const geometry::Vec2d& B2 = positions.last();
    if (n == 2) {
        // Line segment
        return geometry::QuadraticBezier2d(B0, 0.5 * (B0 + B2), B2);
    }

    // Initialize numerator and denominator
    geometry::Vec2d numerator;
    double denominator = 0;

    // Iterate over all points except the first and last and accumulate
    // the terms in the numerator and denominator
    for (Int i = 1; i < n - 1; ++i) {
        geometry::Vec2d p = positions.getUnchecked(i);
        double u = params.getUnchecked(i);
        double v = 1 - u;
        double a0 = v * v;
        double a1 = 2 * v * u;
        double a2 = u * u;
        numerator += (p - a0 * B0 - a2 * B2) * a1;
        denominator += a1 * a1;
    }

    // Compute B1
    if (denominator > 0) {
        geometry::Vec2d B1 = numerator / denominator;
        return geometry::QuadraticBezier2d(B0, B1, B2);
    }
    else {
        // This means that a1 = 0 for all i, so (ui = 0) or (ui = 1) for all i.
        // It's basically bad input, and it's reasonable to fallback to a line segment.
        return geometry::QuadraticBezier2d(B0, 0.5 * (B0 + B2), B2);
    }
}

// For now, we use one Newton-Raphson step, which can be unstable and make it
// worse if we are unlucky.
//
// Indeed, we are trying to find the root of a cubic, and if the current param
// is near a maximum/minimum of the cubic, then the Newton-Raphson step may
// send it very very far
//
//
// ^
// |
// |        If we start here and intersect the tangent with the X axis
// |           v
// |     .  +  .
// |   +          +         We end up here, which is worse than where we started
// |                +                    v
// +-----------------+-------------------------->
//                    +
//
// In the future, we may want to actually solve the cubic to actually find the
// real root. There are closed form solution, which may work well, but it might
// actually be slower (due to evaluation sqrt and cubic root) than actually do
// Newton-Raphson steps but in a more controlled way (ensure first that the
// initial guess is in a "stable" interval, and actually converge with several
// steps instead of only making one step.
//
[[maybe_unused]] void optimizeParameters1(
    const geometry::QuadraticBezier2d& bezier,
    core::ConstSpan<geometry::Vec2d> positions,
    core::Span<double> params) {

    Int n = positions.length();
    VGC_ASSERT(positions.length() == params.length());

    // The second derivative of a quadratic does not actually depend on u, so
    // we compute it outside the loop.
    //
    geometry::Vec2d b2 = bezier.evalSecondDerivative(0);

    // For each parameter u, compute a better parameter by
    // doing one Newton-Raphson iteration:
    //
    //   u = u - f(u) / f'(u)
    //
    // with:
    //
    //   f(u) = 0.5 * d/du || B(u) - P ||²
    //        = (B(u) - P) ⋅ dB/du
    //
    //   f'(u) = dB/du ⋅ dB/du + (B(u) - P) ⋅ d²B/du
    //
    //
    for (Int i = 1; i < n - 1; ++i) {
        geometry::Vec2d p = positions.getUnchecked(i);
        double& u = params.getUnchecked(i);
        geometry::Vec2d b1;
        geometry::Vec2d b0 = bezier.eval(u, b1);
        double numerator = (b0 - p).dot(b1);
        double denominator = b1.dot(b1) + (b0 - p).dot(b2);
        if (std::abs(denominator) > 0) {
            u = u - numerator / denominator;
        }
        // Enforce increasing u-parameters
        double uBefore = params.getUnchecked(i - 1);
        u = core::clamp(u, uBefore, 1.0);
    }
}

// This version ignores the input params, and instead directly find which param
// is the global minimum for each given position. This improves a lot the
// results (but is slower) since it never gets stuck in a local extrema.

[[maybe_unused]] void optimizeParameters2(
    const geometry::QuadraticBezier2d& bezier,
    core::ConstSpan<geometry::Vec2d> positions,
    core::Span<double> params) {

    Int n = positions.length();
    VGC_ASSERT(positions.length() == params.length());

    // Evaluate uniform samples along the Bezier curve
    constexpr Int numUniformSamples = 256;
    constexpr double du = 1.0 / (numUniformSamples - 1);
    std::array<std::pair<double, geometry::Vec2d>, numUniformSamples> uniformSamples;
    for (Int k = 0; k < numUniformSamples; k++) {
        double u = du * k;
        uniformSamples[k] = {u, bezier.eval(u)};
    }

    geometry::Vec2d b2 = bezier.evalSecondDerivative(0);

    for (Int i = 1; i < n - 1; ++i) {
        geometry::Vec2d p = positions.getUnchecked(i);

        // Find closest point among uniform samples and corresponding u
        double minDist = core::DoubleInfinity;
        double u = 0;
        for (auto [u_, q] : uniformSamples) {
            double dist = (q - p).squaredLength();
            if (dist < minDist) {
                minDist = dist;
                u = u_;
            }
        }

        // Perform several Newton-Raphson iterations from there
        const Int numIterations = 10;
        for (Int j = 0; j < numIterations; ++j) {
            geometry::Vec2d b1;
            geometry::Vec2d b0 = bezier.eval(u, b1);
            double numerator = (b0 - p).dot(b1);
            double denominator = b1.dot(b1) + (b0 - p).dot(b2);
            if (std::abs(denominator) > 0) {
                u = u - numerator / denominator;
            }
            else {
                break;
            }
        }

        // Enforce increasing u-parameters
        double uBefore = params.getUnchecked(i - 1);
        u = core::clamp(u, uBefore, 1.0);

        // Set the value in params
        params.getUnchecked(i) = u;
    }
}

// This version accurately detects the case where there are two local
// minima, and accurately computes the most appropriate one.
//
// It does not attemps to keep u-parameters increasing, since the results are
// already really good even when keeping potential switch-backs.
//
// Some explanations of the methods and notations:
//
// B(u) = (1-u)² B0 + 2(1-u)u B1 + u² B2
//      = au² + bu + c
//
// with:
//  a = B0 - 2 B1 + B2
//  b = -2 B0 + 2 B1
//  c = B0
//
// therefore:
//  B'(u) = 2au + b
//  B''(u) = 2a
//
// We want to find the local minima of || B(u) - P || for each input point P.
//
// These satisfy:
//   f(u) = 0
//   f'(u) >= 0
//
// with:
//   f(u) = 0.5 * d/du || B(u) - P ||²
//        = (B(u) - P) B'(u)
//        = 2a²u³ + 3abu² + (b²+2(c-P)a)u + (c-P)b
//
//   f'(u)  = 6a²u² + 6abu + (b²+2(c-P)a)
//   f''(u) = 12a²u + 6ab
//
// (where all vector-vector products are dot products)
//
// So f is a cubic polynomial with a positive u³ term.
// It has either one of two real solutions satisfying:
//
//   f(u) = 0
//   f'(u) >= 0
//
// The local extrema (if D >= 0) of f are:
//
//  f'(u) = 0  =>   u1, u2 = (-6ab ± sqrt(D))/12a²
//                  with D = (6ab)² - 4*6a²*(b²+2(c-P)a)
//
// The inflexion point of f (= its point of rotational symmetry) is:
//
//  f''(u) = 0  =>  u = -ab / 2a²
//
// Note how the inflexion point does not depend on P! In fact, it can be proven
// that it corresponds to the maximum of curvature of B.
//
[[maybe_unused]] void optimizeParameters3(
    const geometry::QuadraticBezier2d& bezier,
    core::ConstSpan<geometry::Vec2d> positions,
    core::Span<double> params) {

    VGC_DEBUG_TMP("--------------------------------------------------");

    Int n = positions.length();
    VGC_ASSERT(positions.length() == params.length());

    const geometry::Vec2d& B0 = bezier.controlPoints()[0];
    const geometry::Vec2d& B1 = bezier.controlPoints()[1];
    const geometry::Vec2d& B2 = bezier.controlPoints()[2];
    geometry::Vec2d a = B0 - 2 * B1 + B2;
    geometry::Vec2d b = 2 * (B1 - B0);
    geometry::Vec2d c = B0;

    double a2 = a.dot(a);
    if (a2 <= 0) {
        // => B0 - 2 B1 + B2 = 0 => B1 = 0.5 * (B0 + B1) => line segment
        //
        // In this case, since B(u) is actually a linear function, the initial
        // parameters should already be pretty good, but we still improve them
        // anyway by computing the projection to the line segment.
        //
        geometry::Vec2d B0B2 = B2 - B0;
        double l2 = B0B2.squaredLength();
        if (l2 <= 0) {
            // The segment is reduced to a point: we just keep the params as is.
            return;
        }
        double l2Inv = 1.0 / l2;
        for (Int i = 1; i < n - 1; ++i) {
            geometry::Vec2d p = positions.getUnchecked(i);
            double u = (p - B0).dot(B0B2) * l2Inv;
            params.getUnchecked(i) = u;
        }
        return;
    }
    double b2 = b.dot(b);
    double ab = a.dot(b);
    double abab = ab * ab; // Note: this is different from a2 * b2

    double a2Inv = 1.0 / a2;
    double uInflexion = -0.5 * ab * a2Inv;
    geometry::Vec2d der2 = 2 * a; // second derivative of B

    // Lambda that evaluates f(u) for point P.
    //
    auto f = [=](double u, const geometry::Vec2d& p) {
        geometry::Vec2d der;
        geometry::Vec2d pos = bezier.eval(u, der);
        return (pos - p).dot(der);
    };

    // Lambda that computes Newton-Raphson iteration starting at u, and returns
    // the final result.
    //
    auto newtonRaphson = [=](double u, const geometry::Vec2d& p) {
        const Int maxIterations = 32;
        const double resolution = 1e-8;
        for (Int j = 0; j < maxIterations; ++j) {
            geometry::Vec2d der;
            geometry::Vec2d pos = bezier.eval(u, der);
            double numerator = (pos - p).dot(der);
            double denominator = der.dot(der) + (pos - p).dot(der2);
            double lastU = u;
            if (std::abs(denominator) > 0) {
                u = u - numerator / denominator;
            }
            else {
                // This is not supposed to happen since we enforce the initial
                // guess to be in stable interval where f'(u) > 0 in the whole
                // interval. If this happens anyway (numerical error?), we try
                // to recover by simply adding a small perturbation to u.
                //
                VGC_WARNING(
                    LogVgcToolsSketch, "Null derivative in Newton-Raphson iteration.");
                u += 0.1;
            }
            if (std::abs(lastU - u) < resolution) {
                break;
            }
            lastU = u;
        }
        return u;
    };

    for (Int i = 1; i < n - 1; ++i) {
        geometry::Vec2d p = positions.getUnchecked(i);
        double D = 36 * abab - 24 * a2 * (b2 + 2 * (c - p).dot(a));

        double uBefore = params.getUnchecked(i);
        double u = 0;
        //params.getUnchecked(i - 1);
        VGC_DEBUG_TMP_EXPR(i);
        VGC_DEBUG_TMP_EXPR(uBefore);
        VGC_DEBUG_TMP_EXPR(uInflexion);
        VGC_DEBUG_TMP_EXPR(D);

        if (D <= 0) {
            // If D == 0:                         If D < 0:
            //
            // f'(uInflexion) = 0                 f'(u) > 0 everywhere
            // f'(u) > 0 everywhere else
            //
            //            |                               |
            //           /                               /
            //      .-o-'                               o  uInflexion
            //     /  uInflexion                      /
            //    |                                  |
            //
            // There is exactly one solution.
            //
            double A = f(uInflexion, p);
            VGC_DEBUG_TMP_EXPR(A);
            if (A == 0) {
                u = uInflexion;
            }
            else if (A > 0) {
                u = newtonRaphson(uInflexion - 1, p); // (-inf, uInflexion) is stable
            }
            else {
                u = newtonRaphson(uInflexion + 1, p); // (uInflexion, inf) is stable
            }
        }
        else {
            //  uExtrema1
            //     v
            //   .--.  uInflexion
            //  '    o
            //        .__.'
            //         ^
            //      uExtrema2
            //
            double offset = (1.0 / 12.0) * std::sqrt(D) * a2Inv;
            double uExtrema1 = uInflexion - offset;
            double uExtrema2 = uInflexion + offset;
            VGC_DEBUG_TMP_EXPR(uExtrema1);
            VGC_DEBUG_TMP_EXPR(uExtrema2);
            if (f(uExtrema2, p) > 0) {               // no solution in (uExtrema1, inf)
                u = newtonRaphson(uExtrema1 - 1, p); // (-inf, uExtrema1) is stable
                VGC_DEBUG_TMP("f(uExtrema2, p) > 0");
            }
            else if (f(uExtrema1, p) < 0) {          // no solution in (-inf, uExtrema2)
                u = newtonRaphson(uExtrema2 + 1, p); // (uExtrema2, inf) is stable
                VGC_DEBUG_TMP("f(uExtrema1, p) < 0");
            }
            else {
                // There is one solution in (-inf, uExtrema1) and one in
                // (uExtrema2, inf).
                //
                // We pick the one that preserves which side of uInflexion we
                // are. This choice is very stable and leads to good results
                // because uInflexion does not depend on P, and input points
                // that are close to uInflexion are typically in the case where
                // there is only one solution anyway (D < 0).

                if (uBefore < uInflexion) {
                    VGC_DEBUG_TMP("Two solutions: choosing first.");
                    u = newtonRaphson(uExtrema1 - 1, p);
                }
                else {
                    VGC_DEBUG_TMP("Two solutions: choosing second.");
                    u = newtonRaphson(uExtrema2 + 1, p);
                }
            }
        }
        VGC_DEBUG_TMP_EXPR(u);

        // Set the value in params
        params.getUnchecked(i) = u;
    }
}

// Using this method at the end of a fit pass make it possible
// to visualize the computed parameters by simply taking the input
// points, and moving them to bezier(params[i]).
//
// You may want to call optimizeParameters() beforehand if you wish
// to visualize the output of optimizeParameters(), or not call it
// if you prefer to visualize the output of the least-squares fit.
//
[[maybe_unused]] void setOutputAsMovedInputPoints(
    const geometry::QuadraticBezier2d& bezier,
    core::ConstSpan<double> params,
    const SketchPointBuffer& input,
    SketchPointBuffer& output) {

    Int n = input.length();
    VGC_ASSERT(params.length() == input.length());

    output.resize(1);
    output.reserve(n);
    if (output.numStablePoints() == 0) {
        output.at(0) = input.first();
    }
    const SketchPointArray& inputData = input.data();
    for (Int i = 1; i < n - 1; ++i) {
        SketchPoint p = inputData.getUnchecked(i);
        double u = params.getUnchecked(i);
        p.setPosition(bezier.eval(u));
        output.append(p);
    }
    output.append(input.last());
}

[[maybe_unused]] void setOutputAsUniformParams(
    const geometry::QuadraticBezier2d& bezier,
    Int numOutputSegments,
    core::ConstSpan<double> params,
    const SketchPointBuffer& input,
    SketchPointBuffer& output) {

    Int n = input.length();
    VGC_ASSERT(params.length() == input.length());
    VGC_ASSERT(numOutputSegments >= 1);
    VGC_ASSERT(params.first() == 0.0);
    VGC_ASSERT(params.last() == 1.0);

    double du = 1.0 / numOutputSegments;

    Int i = 0; // Invariant: 0 <= i < n-1 (so both i and i+1 are valid points)
    output.resize(1);
    output.reserve(n);
    if (output.numStablePoints() == 0) {
        output.at(0) = input.first();
    }
    const SketchPointArray& inputData = input.data();
    for (Int j = 1; j < numOutputSegments; ++j) {

        // The parameter of the output sample
        double u = j * du;

        // Find pair (i, i+1) of input points such that params[i] <= u < params[i+1].
        //
        // Note that since:
        //
        // - 0 < u < 1
        // - params.first() == 0
        // - params.last() == 1
        // - params[k] <= params[k+1] for all k in [0..n-2]
        //
        // It is always possible to find such pair and preserve the invariant
        // i < n-1, since once we reach i = n-2, then the while condition is
        // always false since u < 1 and params[i+1] = params[n-1] = 1 so
        // u < params[i+1].
        //
        while (u >= params.getUnchecked(i + 1)) {
            ++i;
            VGC_ASSERT(i + 1 < n);
        }

        // Output point at bezier.eval(u), with all other params linearly
        // interpolated between input[i] and input[i+1]
        //
        // TODO: For the width/pressure, it would probably be better to take
        // the average of all input points between param `(j - 0.5) * du` and
        // param `(j + 0.5) * du`.
        //
        const SketchPoint p0 = inputData.getUnchecked(i);
        const SketchPoint p1 = inputData.getUnchecked(i + 1);
        double u0 = params.getUnchecked(i);
        double u1 = params.getUnchecked(i + 1);
        VGC_ASSERT(u0 < u1);
        SketchPoint p = core::fastLerp(p0, p1, (u - u0) / (u1 - u0));
        p.setPosition(bezier.eval(u));
        output.append(p);
    }
    output.append(input.last());
}

} // namespace

void SingleQuadraticSegmentWithFixedEndpointsPass::doUpdateFrom(
    const SketchPointBuffer& input,
    SketchPointBuffer& output) {

    if (handleSmallInputWithFixedEndpoints(input, output)) {
        return;
    }

    // Copy positions and initialize u-parameters

    Int n = input.length();
    positions_.resize(0);
    params_.resize(0);
    positions_.reserve(n);
    params_.reserve(n);
    double s0 = input.first().s();
    double totalChordLength = input.last().s() - s0;
    if (totalChordLength <= 0) {
        setLineSegmentWithFixedEndpoints(input, output);
        return;
    }
    double totalChordLengthInv = 1.0 / totalChordLength;
    for (const SketchPoint& p : input) {
        positions_.append(p.position());
        params_.append((p.s() - s0) * totalChordLengthInv);
    }
    // Ensure first and last are exact 0 and 1 (could not be due to numerical errors).
    // We need this as precondition for setOutputAsUniformParams().
    params_.first() = 0;
    params_.last() = 1;

    // Compute initial bezier fit
    geometry::QuadraticBezier2d bezier =
        quadraticFitWithFixedEndpoints(positions_, params_);

    optimizeParameters3(bezier, positions_, params_);

    // Improve fit based on optimized u-parameters
    constexpr Int numIterations = 0;
    for (Int k = 0; k < numIterations; ++k) {
        bezier = quadraticFitWithFixedEndpoints(positions_, params_);
        optimizeParameters3(bezier, positions_, params_);
    }

    // Compute output from fit
    constexpr bool outputAsMovedInputPoint = true;
    if (outputAsMovedInputPoint) {
        setOutputAsMovedInputPoints(bezier, params_, input, output);
    }
    else {
        constexpr Int numOutputSegments = 8;
        setOutputAsUniformParams(bezier, numOutputSegments, params_, input, output);
    }

    output.updateChordLengths();
    output.setNumStablePoints(input.numStablePoints() > 0 ? 1 : 0);
}

} // namespace vgc::tools
