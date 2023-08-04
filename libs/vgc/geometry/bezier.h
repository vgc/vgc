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
#include <vgc/geometry/vec2d.h>

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

template<typename T, typename Scalar, size_t degree_>
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

    void compute(core::Span<const T, degree + 1> controlPoints, const Scalar u) {
        //const Scalar oneMinusU = Scalar(1) - u;
        for (size_t i = 0; i < degree; ++i) {
            //values_[i] = oneMinusU * controlPoints[i] + u * controlPoints[i + 1];
            values_[i] = controlPoints[i] + u * (controlPoints[i + 1] - controlPoints[i]);
        }
        if constexpr (degree >= 2) {
            computeRec<2>(u /*, oneMinusU*/);
        }
    }

    void computeMiddle(core::Span<const T, degree + 1> controlPoints) {
        for (size_t i = 0; i < degree; ++i) {
            values_[i] = 0.5 * (controlPoints[i] + controlPoints[i + 1]);
        }
        if constexpr (degree >= 2) {
            computeMiddleRec<2>();
        }
    }

    const T& value() {
        if constexpr (degree >= 1) {
            return values_[size - 1];
        }
        else {
            static T zero = {};
            return zero;
        }
    }

    T derivative() {
        if constexpr (degree >= 2) {
            constexpr size_t i = levelOffset_<degree - 1>;
            constexpr Scalar n = static_cast<Scalar>(degree);
            return n * (values_[i + 1] - values_[i]);
        }
        else {
            return T();
        }
    }

    T secondDerivative() {
        if constexpr (degree >= 3) {
            constexpr size_t i = levelOffset_<degree - 2>;
            constexpr Scalar n = static_cast<Scalar>(degree);
            return n * (n - 1) * (values_[i + 2] - 2 * values_[i + 1] + values_[i]);
        }
        else {
            return T();
        }
    }

    template<
        size_t level,
        size_t d_ = degree,
        VGC_REQUIRES(level >= 1 && (level <= degree + 1) && d_ >= 1)>
    const T& firstValueOfLevel() {
        return values_[levelOffset_<level>];
    }

    template<
        size_t level,
        size_t d_ = degree,
        VGC_REQUIRES(level >= 1 && (level <= degree + 1) && d_ >= 1)>
    const T& lastValueOfLevel() {
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
    void computeRec(const Scalar u) {
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

    std::array<T, size> values_;
};

/// Returns the control points of the Bezier that evaluates to the derivative
/// of the Bezier defined by the given `controlPoints`.
///
template<Int n, typename T, typename Scalar, VGC_REQUIRES(n >= 2)>
void bezierDerivativeBezier(
    core::Span<const T, n> controlPoints,
    core::Span<T, n - 1>& derControlPoints) {

    for (Int i = 0; i < n - 1; ++i) {
        derControlPoints[i] = (n - 1) * (controlPoints[i + 1] - controlPoints[i]);
    }
}

template<Int n, typename T, typename Scalar, VGC_REQUIRES(n >= 2)>
T bezierCasteljau(core::Span<const T, n> controlPoints, Scalar u) {
    DeCasteljauTree<T, Scalar, static_cast<size_t>(n - 1)> tree = {};
    tree.compute(controlPoints, u);
    return tree.value();
}

template<Int n, typename T, typename Scalar, VGC_REQUIRES(n >= 2)>
T bezierCasteljau(
    core::Span<const T, n> controlPoints,
    Scalar u,
    core::TypeIdentity<T>& der) {

    DeCasteljauTree<T, Scalar, static_cast<size_t>(n - 1)> tree = {};
    tree.compute(controlPoints, u);
    der = tree.derivative();
    return tree.value();
}

template<Int n, typename T, typename Scalar, VGC_REQUIRES(n >= 2)>
T bezierDerivativeCasteljau(core::Span<const T, n> controlPoints, Scalar u) {
    core::Span<T, n - 1>& derivativeControlPoints(core::noInit);
    bezierDerivativeBezier(controlPoints, derivativeControlPoints);
    DeCasteljauTree<T, Scalar, static_cast<size_t>(n - 2)> tree = {};
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
template<typename Scalar, typename T>
T quadraticBezier(const T& p0, const T& p1, const T& p2, Scalar u) {

    // clang-format off
    Scalar v  = 1 - u;
    Scalar u2 = u * u;
    Scalar v2 = v * v;

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
template<typename Scalar, typename T>
T quadraticBezierDerivative(const T& p0, const T& p1, const T& p2, Scalar u) {
    Scalar v = 1 - u;
    return 2 * (v * (p1 - p0) + u * (p2 - p1));
}

/// Returns the (non-normalized) second derivative at coordinate `u` of the
/// quadratic Bézier curve defined by the three control points `p0`, `p1`, and
/// `p2`.
///
/// \sa quadraticBezier()
///
template<typename T>
T quadraticBezierSecondDerivative(const T& p0, const T& p1, const T& p2) {
    return 2 * (p0 - 2 * p1 + p2);
}

template<typename T, typename Scalar>
T quadraticBezierCasteljau(core::Span<const T, 3> controlPoints, Scalar u) {
    return bezierCasteljau(controlPoints, u);
}

template<typename T, typename Scalar>
T quadraticBezierCasteljau(
    core::Span<const T, 3> controlPoints,
    Scalar u,
    core::TypeIdentity<T>& der) {

    return bezierCasteljau(controlPoints, u, der);
}

template<typename T, typename Scalar>
class QuadraticBezier {
public:
    // Initialized with null control points.
    QuadraticBezier() noexcept = default;

    VGC_WARNING_PUSH
    VGC_WARNING_MSVC_DISABLE(26495) // member variable uninitialized
    /// Uninitialized construction.
    ///
    QuadraticBezier(core::NoInit) noexcept
        : controlPoints_{core::noInit, core::noInit, core::noInit} {
    }
    VGC_WARNING_POP

    QuadraticBezier(core::ConstSpan<T, 3> controlPoints) noexcept
        : QuadraticBezier(controlPoints[0], controlPoints[1], controlPoints[2]) {
    }

    QuadraticBezier(const T& cp0, const T& cp1, const T& cp2) noexcept
        : controlPoints_{cp0, cp1, cp2} {
    }

    const std::array<T, 3>& controlPoints() const {
        return controlPoints_;
    }

    T eval(Scalar u) const {
        return quadraticBezierCasteljau<T, Scalar>(controlPoints_, u);
    }

    T eval(Scalar u, T& derivative) const {
        return quadraticBezierCasteljau<T, Scalar>(controlPoints_, u, derivative);
    }

    T evalDerivative(Scalar u) const {
        return quadraticBezierDerivative(
            controlPoints_[0], controlPoints_[1], controlPoints_[2], u);
    }

    T evalSecondDerivative(Scalar /*u*/) const {
        return quadraticBezierSecondDerivative(
            controlPoints_[0], controlPoints_[1], controlPoints_[2]);
    }

private:
    std::array<T, 3> controlPoints_;
};

using QuadraticBezier2d = QuadraticBezier<Vec2d, double>;
using QuadraticBezier1d = QuadraticBezier<double, double>;

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
template<typename Scalar, typename T>
T cubicBezier(const T& p0, const T& p1, const T& p2, const T& p3, Scalar u) {

    // clang-format off
    Scalar v  = 1 - u;
    Scalar u2 = u * u;
    Scalar v2 = v * v;
    Scalar u3 = u * u2;
    Scalar v3 = v * v2;

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
template<typename Scalar, typename T>
T cubicBezierDerivative(const T& p0, const T& p1, const T& p2, const T& p3, Scalar u) {

    // clang-format off
    Scalar v  = 1 - u;
    Scalar u2 = u * u;
    Scalar v2 = v * v;

    return //
          3 * v2      * (p1 - p0)
        + 6 * v  * u  * (p2 - p1)
        + 3      * u2 * (p3 - p2);
    // clang-format on
}

/// Overload of `cubicBezierDerivative` expecting a pointer to a contiguous
/// sequence of 4 control points.
///
template<typename Scalar, typename T>
T cubicBezierDerivative(const T* fourPoints, Scalar u) {

    // expected to be inlined
    return cubicBezierDerivative<Scalar, T>(
        fourPoints[0], fourPoints[1], fourPoints[2], fourPoints[3], u);
}

/// Returns the (non-normalized) second derivative at coordinate `u` of the
/// cubic Bézier curve defined by the three control points `p0`, `p1`, `p2`,
/// and `p3`.
///
/// \sa cubicBezier()
///
template<typename Scalar, typename T>
T cubicBezierSecondDerivative(
    const T& p0,
    const T& p1,
    const T& p2,
    const T& p3,
    Scalar u) {

    Scalar v = 1 - u;
    return 6 * (v * (p2 - 2 * p1 + p0) + u * (p3 - 2 * p2 + p1));
}

template<typename T, typename Scalar>
T cubicBezierCasteljau(core::Span<const T, 4> controlPoints, Scalar u) {
    return bezierCasteljau(controlPoints, u);
}

/// Variant of `cubicBezier` expecting a pointer to a contiguous sequence
/// of 4 control points and using Casteljau's algorithm.
///
template<typename T, typename Scalar>
T cubicBezierCasteljau(
    core::Span<const T, 4> controlPoints,
    Scalar u,
    core::TypeIdentity<T>& der) {

    return bezierCasteljau(controlPoints, u, der);
}

template<typename T, typename Scalar>
class CubicBezier {
public:
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

    CubicBezier(core::ConstSpan<T, 4> controlPoints) noexcept
        : CubicBezier(
            controlPoints[0],
            controlPoints[1],
            controlPoints[2],
            controlPoints[3]) {
    }

    CubicBezier(const T& cp0, const T& cp1, const T& cp2, const T& cp3) noexcept
        : controlPoints_{cp0, cp1, cp2, cp3} {
    }

    const std::array<T, 4>& controlPoints() const {
        return controlPoints_;
    }

    const T& controlPoint0() {
        return controlPoints_[0];
    }

    void setControlPoint0(const T& cp) {
        controlPoints_[0] = cp;
    }

    const T& controlPoint1() {
        return controlPoints_[1];
    }

    void setControlPoint1(const T& cp) {
        controlPoints_[1] = cp;
    }

    const T& controlPoint2() {
        return controlPoints_[2];
    }

    void setControlPoint2(const T& cp) {
        controlPoints_[2] = cp;
    }

    const T& controlPoint3() {
        return controlPoints_[3];
    }

    void setControlPoint3(const T& cp) {
        controlPoints_[3] = cp;
    }

    T eval(Scalar u) const {
        return cubicBezierCasteljau<T, Scalar>(controlPoints_, u);
    }

    T eval(Scalar u, T& derivative) const {
        return cubicBezierCasteljau<T, Scalar>(controlPoints_, u, derivative);
    }

    T evalDerivative(Scalar u) const {
        return cubicBezierDerivative(
            controlPoints_[0],
            controlPoints_[1],
            controlPoints_[2],
            controlPoints_[3],
            u);
    }

    T evalSecondDerivative(Scalar u) const {
        return cubicBezierSecondDerivative(
            controlPoints_[0],
            controlPoints_[1],
            controlPoints_[2],
            controlPoints_[3],
            u);
    }

private:
    std::array<T, 4> controlPoints_;
};

using CubicBezier2d = CubicBezier<Vec2d, double>;
using CubicBezier1d = CubicBezier<double, double>;

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_BEZIER_H