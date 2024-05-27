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

namespace vgc::tools {

Int EmptyPass::doUpdateFrom(const SketchPointBuffer& input) {

    const SketchPointArray& inputPoints = input.data();

    // Remove all previously unstable points.
    //
    Int oldNumStablePoints = this->numStablePoints();
    resizePoints(oldNumStablePoints);

    // Add all other points (some of which are now stable, some of which are
    // still unstable).
    //
    extendPoints(inputPoints.begin() + oldNumStablePoints, inputPoints.end());

    // Set the new number of stable points as being the same as the input.
    //
    Int newNumStablePoints = input.numStablePoints();
    return newNumStablePoints;
}

Int TransformPass::doUpdateFrom(const SketchPointBuffer& input) {

    const SketchPointArray& inputPoints = input.data();

    // Remove all previously unstable points.
    //
    Int oldNumStablePoints = this->numStablePoints();
    resizePoints(oldNumStablePoints);

    // Add all other points (some of which are now stable, some of which are
    // still unstable).
    //
    for (auto it = inputPoints.begin() + oldNumStablePoints; //
         it != inputPoints.end();
         ++it) {

        SketchPoint p = *it;
        p.setPosition(transformAffine(p.position()));
        appendPoint(p);
    }

    // Set the new number of stable points as being the same as the input.
    //
    Int newNumStablePoints = input.numStablePoints();
    return newNumStablePoints;
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

Int SmoothingPass::doUpdateFrom(const SketchPointBuffer& input) {

    const SketchPointArray& inputPoints = input.data();
    Int numPoints = inputPoints.length();
    Int numStablePoints = this->numStablePoints();
    if (numPoints == numStablePoints) {
        return numPoints;
    }

    // Keep our stable points, fill the rest with the input points.
    Int unstableIndexStart = numStablePoints;
    resizePoints(numStablePoints);
    extendPoints(inputPoints.begin() + unstableIndexStart, inputPoints.end());

    Int instabilityDelta = 0;

    Int pointsSmoothingLevel = 2;
    if (pointsSmoothingLevel > 0 && numPoints >= 3) {
        // Apply gaussian smoothing.
        Int iStart = unstableIndexStart;
        if (pointsSmoothingLevel == 1) {
            iStart = std::max<Int>(1, iStart);
            for (Int i = iStart; i < numPoints - 1; ++i) {
                getPointRef(i).setPosition(                                  //
                    (1 / 4.0) * inputPoints.getUnchecked(i - 1).position() + //
                    (2 / 4.0) * inputPoints.getUnchecked(i + 0).position() + //
                    (1 / 4.0) * inputPoints.getUnchecked(i + 1).position());
            }
        }
        else if (pointsSmoothingLevel == 2) {
            if (iStart <= 1) {
                getPointRef(1).setPosition(                              //
                    (1 / 4.0) * inputPoints.getUnchecked(0).position() + //
                    (2 / 4.0) * inputPoints.getUnchecked(1).position() + //
                    (1 / 4.0) * inputPoints.getUnchecked(2).position());
                iStart = 2;
            }
            for (Int i = iStart; i < numPoints - 2; ++i) {
                getPointRef(i).setPosition(                                   //
                    (1 / 16.0) * inputPoints.getUnchecked(i - 2).position() + //
                    (4 / 16.0) * inputPoints.getUnchecked(i - 1).position() + //
                    (6 / 16.0) * inputPoints.getUnchecked(i + 0).position() + //
                    (4 / 16.0) * inputPoints.getUnchecked(i + 1).position() + //
                    (1 / 16.0) * inputPoints.getUnchecked(i + 2).position());
            }
            if (numPoints - 2 >= iStart) {
                Int i = numPoints - 2;
                getPointRef(i).setPosition(                                  //
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
            getPointRef(i).setWidth(value / sumCoeffs);
        }
        instabilityDelta = std::max<Int>(instabilityDelta, widthSmoothingLevel);
    }

    // compute chordal lengths
    updateCumulativeChordalDistances();

    // Width limitor
    constexpr double widthRoughness = 0.8;
    constexpr Int roughnessLimitorWindowSize = 3;
    const SketchPoint* lastStablePoint =
        numStablePoints == 0 ? nullptr : &getPoint(numStablePoints - 1);
    applyWidthRoughnessLimitor(
        widthRoughness, roughnessLimitorWindowSize, lastStablePoint, unstablePoints());
    instabilityDelta += roughnessLimitorWindowSize;

    return std::max<Int>(0, input.numStablePoints() - instabilityDelta);
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

Int DouglasPeuckerPass::doUpdateFrom(const SketchPointBuffer& input) {

    // A copy required to make a mutable span, which the Douglas-Peuckert
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
    resizePoints(numSimplifiedPoints);
    for (Int i = 0; i < numSimplifiedPoints; ++i) {
        getPointRef(i) = inputPoints[indices[i]];
    }

    // For now, for simplicity, we do not provide any stable points guarantees
    // and simply recompute the Douglas-Peuckert algorithm from scratch{
    //
    Int numStablePoints = 0;
    return numStablePoints;
}

Int SingleLineSegmentWithFixedEndpointsPass::doUpdateFrom(
    const SketchPointBuffer& input) {

    const SketchPointArray& inputPoints = input.data();
    if (!inputPoints.isEmpty()) {
        resizePoints(2);
        SketchPoint& p0 = getPointRef(0);
        SketchPoint& p1 = getPointRef(1);
        p0 = inputPoints.first();
        p1 = inputPoints.last();
    }

    // No stable points
    return 0;
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
Int SingleLineSegmentWithFreeEndpointsPass::doUpdateFrom(const SketchPointBuffer& input) {

    const SketchPointArray& inputPoints = input.data();
    Int numPoints = inputPoints.length();
    if (numPoints == 0) {
        return 0;
    }

    resizePoints(2);
    SketchPoint& p0 = getPointRef(0);
    SketchPoint& p1 = getPointRef(1);
    p0 = inputPoints.first();
    p1 = inputPoints.last();
    if (numPoints <= 2) {
        return 0;
    }

    // Compute centroid
    geometry::Vec2d centroid;
    for (const SketchPoint& p : inputPoints) {
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
    for (const SketchPoint& p : inputPoints) {
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

    // Find points further away from centroid along the line, and project them
    // on the line to define out two output points
    //
    // Note: the normal / direction vector is non-null since we know that
    // (A, B) is either (1, 0), (0, 1), or (b, ...) with b != 0
    //
    geometry::Vec2d d(-B, A);
    double vMin = core::DoubleInfinity;
    double vMax = -core::DoubleInfinity;
    for (const SketchPoint& p : inputPoints) {
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
    double p0pMin2 = (p0.position() - pMin).squaredLength();
    double p0pMax2 = (p0.position() - pMax).squaredLength();
    if (p0pMin2 < p0pMax2) {
        // p0 closer to pMin than pMax
        p0.setPosition(pMin);
        p1.setPosition(pMax);
    }
    else {
        p0.setPosition(pMax);
        p1.setPosition(pMin);
    }

    return 0;

    // TODO: better width than using the width of first and last point?
}

} // namespace vgc::tools
