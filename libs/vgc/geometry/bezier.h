// Copyright 2021 The VGC Developers
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

#ifndef VGC_GEOMETRY_BEZIER_H
#define VGC_GEOMETRY_BEZIER_H

#include <vgc/core/span.h>
#include <vgc/geometry/api.h>
#include <vgc/geometry/traits.h>
#include <vgc/geometry/vec2.h>
#include <vgc/geometry/vec3.h>
#include <vgc/geometry/vec4.h>

namespace vgc::geometry {

namespace detail {

// Sum of ai for i in [1, n] with (ai = a1 + i(d - 1) = ai-1 + d for i > 1).
template<Int a1, Int d, Int n>
inline constexpr Int arithmeticSeries = (n * (a1 + a1 + (n - 1) * d)) / 2;
static_assert(arithmeticSeries<3, 2, 3> == 3 + 5 + 7);

// Sum of n consecutive integers starting from a.
template<Int a, Int n>
inline constexpr Int iotaSeries = arithmeticSeries<a, 1, n>;
static_assert(iotaSeries<2, 3> == 2 + 3 + 4);

template<size_t degree>
inline constexpr size_t deCasteljauTreeSize = static_cast<size_t>(iotaSeries<1, degree>);
static_assert(deCasteljauTreeSize<3> == 3 + 2 + 1);

} // namespace detail

template<typename Point, typename T, size_t degree_>
class DeCasteljauTree {
public:
    static constexpr size_t degree = degree_;
    static constexpr size_t size = detail::deCasteljauTreeSize<degree>;

    // example with degree == 3
    // -----------------------------
    // controlPoints  P0  P1  P2  P3
    // level 1          Q0  Q1  Q2
    // level 2            R0  R1
    // level 3              S0
    //
    // values are stored as [Q0, Q1, Q2, R0, R1, S0]
    //

    void compute(core::Span<const Point, degree + 1> controlPoints, const T u) {
        //const T oneMinusU = T(1) - u;
        for (size_t i = 0; i < degree; ++i) {
            //values_[i] = oneMinusU * controlPoints[i] + u * controlPoints[i + 1];
            values_[i] = controlPoints[i] + u * (controlPoints[i + 1] - controlPoints[i]);
        }
        if constexpr (degree >= 2) {
            computeRec<2>(u /*, oneMinusU*/);
        }
    }

    void computeMiddle(core::Span<const Point, degree + 1> controlPoints) {
        for (size_t i = 0; i < degree; ++i) {
            values_[i] = 0.5 * (controlPoints[i] + controlPoints[i + 1]);
        }
        if constexpr (degree >= 2) {
            computeMiddleRec<2>();
        }
    }

    const Point& value() {
        if constexpr (degree >= 1) {
            return values_[size - 1];
        }
        else {
            static Point zero = {};
            return zero;
        }
    }

    Point derivative() {
        if constexpr (degree >= 2) {
            constexpr size_t i = levelOffset_<degree - 1>;
            constexpr T n = static_cast<T>(degree);
            return n * (values_[i + 1] - values_[i]);
        }
        else {
            return Point();
        }
    }

    Point secondDerivative() {
        if constexpr (degree >= 3) {
            constexpr size_t i = levelOffset_<degree - 2>;
            constexpr T n = static_cast<T>(degree);
            return n * (n - 1) * (values_[i + 2] - 2 * values_[i + 1] + values_[i]);
        }
        else {
            return Point();
        }
    }

    template<
        size_t level,
        size_t d_ = degree,
        VGC_REQUIRES(level >= 1 && (level <= degree + 1) && d_ >= 1)>
    const Point& firstValueOfLevel() {
        return values_[levelOffset_<level>];
    }

    template<
        size_t level,
        size_t d_ = degree,
        VGC_REQUIRES(level >= 1 && (level <= degree + 1) && d_ >= 1)>
    const Point& lastValueOfLevel() {
        return values_[levelOffset_<level> + levelSize_<level> - 1];
    }

private:
    // Tree starts at level 1.
    template<size_t level, VGC_REQUIRES(level >= 1 && (level <= degree + 1))>
    static constexpr size_t levelSize_ = (degree + 1) - level;

    // Tree starts at level 1.
    template<size_t level, VGC_REQUIRES(level >= 1 && (level <= degree + 1))>
    static constexpr size_t levelOffset_ =
        (size - static_cast<size_t>(detail::iotaSeries<1, levelSize_<level>>));

    template<
        size_t level,
        size_t maxLevel = degree,
        VGC_REQUIRES(level >= 2 && level <= degree && maxLevel <= degree)>
    void computeRec(const T u) {
        constexpr size_t a = levelOffset_<level - 1>;
        constexpr size_t b = levelOffset_<level>;
        for (size_t i = 0; i < levelSize_<level>; ++i) {
            // note: only uses 1 multiplication
            values_[b + i] = values_[a + i] + u * (values_[a + i + 1] - values_[a + i]);
        }
        if constexpr (level < maxLevel) {
            computeRec<level + 1>(u);
        }
    }

    template<
        size_t level,
        size_t maxLevel = degree,
        VGC_REQUIRES(level >= 2 && level <= degree && maxLevel <= degree)>
    void computeMiddleRec() {
        constexpr size_t a = levelOffset_<level - 1>;
        constexpr size_t b = levelOffset_<level>;
        for (size_t i = 0; i < levelSize_<level>; ++i) {
            values_[b + i] = 0.5 * (values_[a + i] + values_[a + i + 1]);
        }
        if constexpr (level < maxLevel) {
            computeMiddleRec<level + 1>();
        }
    }

    std::array<Point, size> values_;
};

/// Returns the control points of the Bezier that evaluates to the derivative
/// of the Bezier defined by the given `controlPoints`.
///
template<Int n, typename Point, typename T, VGC_REQUIRES(n >= 2)>
void bezierDerivativeBezier(
    core::Span<const Point, n> controlPoints,
    core::Span<Point, n - 1>& derControlPoints) {

    for (Int i = 0; i < n - 1; ++i) {
        derControlPoints[i] = (n - 1) * (controlPoints[i + 1] - controlPoints[i]);
    }
}

template<Int n, typename Point, typename T, VGC_REQUIRES(n >= 2)>
Point bezierCasteljau(core::Span<const Point, n> controlPoints, T u) {
    DeCasteljauTree<Point, T, static_cast<size_t>(n - 1)> tree = {};
    tree.compute(controlPoints, u);
    return tree.value();
}

template<Int n, typename Point, typename T, VGC_REQUIRES(n >= 2)>
Point bezierCasteljau(
    core::Span<const Point, n> controlPoints,
    T u,
    core::TypeIdentity<Point>& der) {

    DeCasteljauTree<Point, T, static_cast<size_t>(n - 1)> tree = {};
    tree.compute(controlPoints, u);
    der = tree.derivative();
    return tree.value();
}

template<Int n, typename Point, typename T, VGC_REQUIRES(n >= 2)>
Point bezierDerivativeCasteljau(core::Span<const Point, n> controlPoints, T u) {
    core::Span<Point, n - 1>& derivativeControlPoints(core::noInit);
    bezierDerivativeBezier(controlPoints, derivativeControlPoints);
    DeCasteljauTree<Point, T, static_cast<size_t>(n - 2)> tree = {};
    tree.compute(derivativeControlPoints, u);
    return tree.value();
}

/// Returns the position at coordinate `u` of the quadratic Bézier curve
/// defined by the three control points `p0`, `p1`, and `p2`.
///
/// When `u = 0`, then the returned position is equal to `p0`. When `u = 1`, then the
/// returned position is equal to `p2`. In the general case, the curve does not
/// pass though `p1`.
///
/// The coordinate `u` is typically between [0, 1], but this is not required.
/// Values of `u` outside the range [0, 1] extrapolate the control points.
///
/// \sa quadraticBezierDerivative()
///
template<typename Point, typename T>
Point quadraticBezier(const Point& p0, const Point& p1, const Point& p2, T u) {

    // clang-format off
    T v  = 1 - u;
    T u2 = u * u;
    T v2 = v * v;

    return    v2      * p0
        + 2 * v  * u  * p1
        +          u2 * p2;
    // clang-format on
}

/// Returns the (non-normalized) first derivative at coordinate `u` of the
/// quadratic Bézier curve defined by the three control points `p0`, `p1`, and
/// `p2`.
///
/// \sa quadraticBezier()
///
template<typename Point, typename T>
Point quadraticBezierDerivative(const Point& p0, const Point& p1, const Point& p2, T u) {
    T v = 1 - u;
    return 2 * (v * (p1 - p0) + u * (p2 - p1));
}

/// Returns the (non-normalized) second derivative at coordinate `u` of the
/// quadratic Bézier curve defined by the three control points `p0`, `p1`, and
/// `p2`.
///
/// \sa quadraticBezier()
///
template<typename Point>
Point quadraticBezierSecondDerivative(const Point& p0, const Point& p1, const Point& p2) {
    return 2 * ((p2 - p1) - (p1 - p0));
}

template<typename Point, typename T>
Point quadraticBezierCasteljau(core::Span<const Point, 3> controlPoints, T u) {
    return bezierCasteljau(controlPoints, u);
}

template<typename Point, typename T>
Point quadraticBezierCasteljau(
    core::Span<const Point, 3> controlPoints,
    T u,
    core::TypeIdentity<Point>& der) {

    return bezierCasteljau(controlPoints, u, der);
}

/// \class QuadraticBezier
/// \brief A Bézier curve of degree 2.
///
template<typename Point, typename T = scalarType<Point>>
class QuadraticBezier {
public:
    using ScalarType = T;
    static constexpr Int dimension = geometry::dimension<Point>;

    /// Creates a zero-initialized quadratic Bézier.
    ///
    QuadraticBezier() noexcept = default;

    VGC_WARNING_PUSH
    VGC_WARNING_MSVC_DISABLE(26495) // member variable uninitialized
    /// Creates an uninitialized quadratic Bézier.
    ///
    QuadraticBezier(core::NoInit) noexcept
        : controlPoints_{core::noInit, core::noInit, core::noInit} {
    }
    VGC_WARNING_POP

    /// Creates a quadratic Bézier with the given `controlPoints`.
    ///
    QuadraticBezier(core::ConstSpan<Point, 3> controlPoints) noexcept
        : QuadraticBezier(controlPoints[0], controlPoints[1], controlPoints[2]) {
    }

    /// Creates a quadratic Bézier with the three given control points.
    ///
    QuadraticBezier(const Point& cp0, const Point& cp1, const Point& cp2) noexcept
        : controlPoints_{cp0, cp1, cp2} {
    }

    /// Creates a quadratic Bézier reduced to the single point `p`.
    ///
    /// This is equivalent to: `QuadraticBezier(p, p, p)`.
    ///
    static QuadraticBezier point(const Point& p) noexcept {
        return QuadraticBezier(p, p, p);
    }

    /// Creates a quadratic Bézier representing the line segment from point `a`
    /// to point `b`.
    ///
    /// This is equivalent to: `QuadraticBezier(a, 0.5 * (a + b), b)`.
    ///
    static QuadraticBezier lineSegment(const Point& a, const Point& b) noexcept {
        constexpr T half = 0.5f;
        return QuadraticBezier(a, half * (a + b), b);
    }

    /// Returns whether this quadratic Bézier is close to being a linearly
    /// parameterized line segment, within the given relative tolerance.
    ///
    bool isLineSegment(
        T relTolSquared = //
        core::defaultRelativeTolerance<T> * core::defaultRelativeTolerance<T>) {

        geometry::Vec2d a = (p2() - p1()) - (p1() - p0()); // = 0 when p1 = (p0+p2)/2
        geometry::Vec2d b = p2() - p0();
        return a.squaredLength() <= relTolSquared * b.squaredLength();
        // Note: using `<=` rather than `<` is important to
        // handle the case where both a and b are null vectors.
    }

    /// Returns the three control points of this quadratic Bézier.
    ///
    const std::array<Point, 3>& controlPoints() const {
        return controlPoints_;
    }

    /// Returns the first control point of this quadratic Bézier.
    ///
    const Point& p0() const {
        return controlPoints()[0];
    }

    /// Returns the second control point of this quadratic Bézier.
    ///
    const Point& p1() const {
        return controlPoints()[1];
    }

    /// Returns the third control point of this quadratic Bézier.
    ///
    const Point& p2() const {
        return controlPoints()[2];
    }

    /// Returns the evaluation of this Bézier at parameter `u`.
    ///
    Point eval(T u) const {
        return quadraticBezierCasteljau<Point, T>(controlPoints_, u);
    }

    /// Returns the evaluation of this Bézier curve at parameter `u`, and
    /// modifies `derivative` to become the derivative of this Bézier curve at
    /// parameter `u`
    ///
    /// This is faster than calling `eval(u)` and `evalDerivative(u)`
    /// separately.
    ///
    Point eval(T u, Point& derivative) const {
        return quadraticBezierCasteljau<Point, T>(controlPoints_, u, derivative);
    }

    /// Returns the derivative of this Bézier curve at parameter `u`.
    ///
    Point evalDerivative(T u) const {
        return quadraticBezierDerivative(
            controlPoints_[0], controlPoints_[1], controlPoints_[2], u);
    }

    /// Returns the second derivative of this Bézier curve at parameter `u`.
    ///
    /// This is actually a constant that does not depend on `u`. This function
    /// is only provided for consistency with `CubicBezier` to facilitate use
    /// in template code.
    ///
    /// Prefer using `secondDerivative()` when possible.
    ///
    Point evalSecondDerivative(T u) const {
        VGC_UNUSED(u);
        return secondDerivative();
    }

    /// Returns the second derivative of this Bézier curve, which is a
    /// constant.
    ///
    Point secondDerivative() const {
        return quadraticBezierSecondDerivative(p0(), p1(), p2());
    }

private:
    std::array<Point, 3> controlPoints_;
};

using QuadraticBezier1f = QuadraticBezier<float>;
using QuadraticBezier1d = QuadraticBezier<double>;

using QuadraticBezier2f = QuadraticBezier<Vec2f>;
using QuadraticBezier2d = QuadraticBezier<Vec2d>;

using QuadraticBezier3f = QuadraticBezier<Vec3f>;
using QuadraticBezier3d = QuadraticBezier<Vec3d>;

using QuadraticBezier4f = QuadraticBezier<Vec4f>;
using QuadraticBezier4d = QuadraticBezier<Vec4d>;

/// Returns the position at coordinate \p u of the cubic Bézier curve defined
/// by the four control points \p p0, \p p1, \p p2, and \p p3.
///
/// When u = 0, then the returned position is equal to p0. When u = 1, then the
/// returned position is equal to p3. In the general case, the curve does not
/// pass though p1 and p2.
///
/// The coordinate \p u is typically between [0, 1], but this is not required.
/// Values of u outside the range [0, 1] extrapolate the control points.
///
/// \sa cubicBezierDerivative()
///
template<typename Point, typename T>
Point cubicBezier(
    const Point& p0,
    const Point& p1,
    const Point& p2,
    const Point& p3,
    T u) {

    // clang-format off
    T v  = 1 - u;
    T u2 = u * u;
    T v2 = v * v;
    T u3 = u * u2;
    T v3 = v * v2;

    return    v3      * p0
        + 3 * v2 * u  * p1
        + 3 * v  * u2 * p2
        +          u3 * p3;
    // clang-format on
}

/// Returns the (non-normalized) first derivative at coordinate `u` of the
/// cubic Bézier curve defined by the three control points `p0`, `p1`, `p2`,
/// and `p3`.
///
/// \sa cubicBezier()
///
template<typename Point, typename T>
Point cubicBezierDerivative(
    const Point& p0,
    const Point& p1,
    const Point& p2,
    const Point& p3,
    T u) {

    // clang-format off
    T v  = 1 - u;
    T u2 = u * u;
    T v2 = v * v;

    return //
          3 * v2      * (p1 - p0)
        + 6 * v  * u  * (p2 - p1)
        + 3      * u2 * (p3 - p2);
    // clang-format on
}

/// Overload of `cubicBezierDerivative` expecting a pointer to a contiguous
/// sequence of 4 control points.
///
template<typename Point, typename T>
Point cubicBezierDerivative(const Point* fourPoints, T u) {

    // expected to be inlined
    return cubicBezierDerivative<T, Point>(
        fourPoints[0], fourPoints[1], fourPoints[2], fourPoints[3], u);
}

/// Returns the (non-normalized) second derivative at coordinate `u` of the
/// cubic Bézier curve defined by the three control points `p0`, `p1`, `p2`,
/// and `p3`.
///
/// \sa cubicBezier()
///
template<typename Point, typename T>
Point cubicBezierSecondDerivative(
    const Point& p0,
    const Point& p1,
    const Point& p2,
    const Point& p3,
    T u) {

    T v = 1 - u;
    return 6 * (v * (p2 - 2 * p1 + p0) + u * (p3 - 2 * p2 + p1));
}

template<typename Point, typename T>
Point cubicBezierCasteljau(core::Span<const Point, 4> controlPoints, T u) {
    return bezierCasteljau(controlPoints, u);
}

/// Variant of `cubicBezier` expecting a pointer to a contiguous sequence
/// of 4 control points and using Casteljau's algorithm.
///
template<typename Point, typename T>
Point cubicBezierCasteljau(
    core::Span<const Point, 4> controlPoints,
    T u,
    core::TypeIdentity<Point>& der) {

    return bezierCasteljau(controlPoints, u, der);
}

template<typename Point, typename T = scalarType<Point>>
class CubicBezier {
public:
    using ScalarType = T;
    static constexpr Int dimension = geometry::dimension<Point>;

    // Initialized with null control points.
    CubicBezier() noexcept = default;

    VGC_WARNING_PUSH
    VGC_WARNING_MSVC_DISABLE(26495) // member variable uninitialized
    /// Uninitialized construction.
    ///
    CubicBezier(core::NoInit) noexcept
        : controlPoints_{core::noInit, core::noInit, core::noInit, core::noInit} {
    }
    VGC_WARNING_POP

    CubicBezier(core::ConstSpan<Point, 4> controlPoints) noexcept
        // clang-format off
        // (disagreement between versions)
        : CubicBezier(
            controlPoints[0],
            controlPoints[1],
            controlPoints[2],
            controlPoints[3]) {
        // clang-format on
    }

    CubicBezier(
        const Point& cp0,
        const Point& cp1,
        const Point& cp2,
        const Point& cp3) noexcept
        : controlPoints_{cp0, cp1, cp2, cp3} {
    }

    const std::array<Point, 4>& controlPoints() const {
        return controlPoints_;
    }

    const Point& controlPoint0() {
        return controlPoints_[0];
    }

    void setControlPoint0(const Point& cp) {
        controlPoints_[0] = cp;
    }

    const Point& controlPoint1() {
        return controlPoints_[1];
    }

    void setControlPoint1(const Point& cp) {
        controlPoints_[1] = cp;
    }

    const Point& controlPoint2() {
        return controlPoints_[2];
    }

    void setControlPoint2(const Point& cp) {
        controlPoints_[2] = cp;
    }

    const Point& controlPoint3() {
        return controlPoints_[3];
    }

    void setControlPoint3(const Point& cp) {
        controlPoints_[3] = cp;
    }

    Point eval(T u) const {
        return cubicBezierCasteljau<Point, T>(controlPoints_, u);
    }

    Point eval(T u, Point& derivative) const {
        return cubicBezierCasteljau<Point, T>(controlPoints_, u, derivative);
    }

    Point evalDerivative(T u) const {
        return cubicBezierDerivative(
            controlPoints_[0],
            controlPoints_[1],
            controlPoints_[2],
            controlPoints_[3],
            u);
    }

    Point evalSecondDerivative(T u) const {
        return cubicBezierSecondDerivative(
            controlPoints_[0],
            controlPoints_[1],
            controlPoints_[2],
            controlPoints_[3],
            u);
    }

private:
    std::array<Point, 4> controlPoints_;
};

using CubicBezier1f = CubicBezier<float>;
using CubicBezier1d = CubicBezier<double>;

using CubicBezier2f = CubicBezier<Vec2f>;
using CubicBezier2d = CubicBezier<Vec2d>;

using CubicBezier3f = CubicBezier<Vec3f>;
using CubicBezier3d = CubicBezier<Vec3d>;

using CubicBezier4f = CubicBezier<Vec4f>;
using CubicBezier4d = CubicBezier<Vec4d>;

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_BEZIER_H
