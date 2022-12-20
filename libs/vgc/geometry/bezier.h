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
            computeRec<2>(u/*, oneMinusU*/);
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

    template<VGC_REQUIRES(degree >= 1)>
    const T& value() {
        return values_[size - 1];
    }

    template<VGC_REQUIRES(degree >= 2)>
    T derivative() {
        constexpr size_t i = levelOffset_<degree - 1>;
        return Scalar(degree) * (values_[i + 1] - values_[i]);
    }

    template<
        size_t level,
        VGC_REQUIRES(level >= 1 && (level <= degree + 1) && degree >= 1)>
    const T& firstValueOfLevel() {
        return values_[levelOffset_<level>];
    }

    template<
        size_t level,
        VGC_REQUIRES(level >= 1 && (level <= degree + 1) && degree >= 1)>
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
    void computeRec(const Scalar u/*, const Scalar oneMinusU*/) {
        constexpr size_t a = levelOffset_<level - 1>;
        constexpr size_t b = levelOffset_<level>;
        for (size_t i = 0; i < levelSize_<level>; ++i) {
            // for middle values, instead of doing `(1 - u)*v` we could reuse `u*v` and do `1 - u*v`.
            // it is even simpler for u = 0.5
            //values_[b + i] = oneMinusU * values_[a + i] + u * values_[a + i + 1];
            values_[b + i] = values_[a + i] + u * (values_[a + i + 1] - values_[a + i]);
        }
        if constexpr (level < maxLevel) {
            computeRec<level + 1>(u/*, oneMinusU*/);
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

// clang-format off

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
/// \sa cubicBezierDer(), cubicBezierPosAndDer()
///
template <typename Scalar, typename T>
T cubicBezier(
    const T& p0, const T& p1, const T& p2, const T& p3,
    Scalar u) {

    Scalar v = 1 - u;
    Scalar u2 = u * u;
    Scalar v2 = v * v;
    Scalar u3 = u2 * u;
    Scalar v3 = v2 * v;

    return       v3 * p0
        + 3 * v2 * u * p1
        + 3 * v * u2 * p2
        + u3 * p3;
}

/// Returns the (non-normalized) derivative at coordinate \p u of the cubic
/// Bézier curve defined by the four control points \p p0, \p p1, \p p2, and \p
/// p3.
///
/// \sa cubicBezier(), cubicBezierPosAndDer()
///
template <typename Scalar, typename T>
T cubicBezierDer(
    const T& p0, const T& p1, const T& p2, const T& p3,
    Scalar u) {

    Scalar v = 1 - u;
    Scalar u2 = u * u;
    Scalar v2 = v * v;

    return   3 * v2 * (p1 - p0)
        + 6 * v * u * (p2 - p1)
        + 3 * u2 * (p3 - p2);
}

/// Overload of `cubicBezierDer` expecting a pointer to a contiguous sequence
/// of 4 control points.
///
template <typename Scalar, typename T>
T cubicBezierDer(
    const T* fourPoints,
    Scalar u) {

    // expected to be inlined
    return cubicBezierDer<Scalar, T>(
        fourPoints[0], fourPoints[1], fourPoints[2], fourPoints[3],
        u);
}

/// Returns both the position \p pos and derivative \p der at coordinate \p u
/// of the cubic Bézier curve defined by the four control points \p p0, \p p1,
/// \p p2, and \p p3.
///
/// This function is only very marginally faster than calling cubicBezier() and
/// cubicBezierDer() separately. Therefore, for readability and consistency, we
/// recommend to reserve its usage for cases where performance is critical.
///
/// \sa cubicBezier(), cubicBezierDer()
///
template <typename Scalar, typename T>
void cubicBezierPosAndDer(
    const T& p0, const T& p1, const T& p2, const T& p3,
    Scalar u,
    T& pos,
    T& der) {

    Scalar v = 1 - u;
    Scalar u2 = u * u;
    Scalar v2 = v * v;
    Scalar u3 = u2 * u;
    Scalar v3 = v2 * v;

    pos = v3 * p0
        + 3 * v2 * u * p1
        + 3 * v * u2 * p2
        + u3 * p3;

    der = 3 * v2 * (p1 - p0)
        + 6 * v * u * (p2 - p1)
        + 3 * u2 * (p3 - p2);
}

// clang-format on

template<size_t n, typename T, typename Scalar, VGC_REQUIRES(n >= 2)>
T bezierDerivativeBezier(
    core::Span<const T, n> controlPoints,
    core::Span<T, n - 1>& derControlPoints) {

    for (Int i = 0; i < n - 1; ++i) {
        derControlPoints[i] = (n - 1) * (controlPoints[i + 1] - controlPoints[i]);
    }
}

template<size_t n, typename T, typename Scalar, VGC_REQUIRES(n >= 2)>
T bezierPosCasteljau(core::Span<const T, n> controlPoints, Scalar u) {
    DeCasteljauTree<T, Scalar, n - 1> tree = {};
    tree.compute(controlPoints, u);
    return tree.value();
}

template<size_t n, typename T, typename Scalar, VGC_REQUIRES(n >= 2)>
void bezierPosAndDerCasteljau(
    core::Span<const T, n> controlPoints,
    Scalar u,
    core::TypeIdentity<T>& pos,
    core::TypeIdentity<T>& der) {

    DeCasteljauTree<T, Scalar, n - 1> tree = {};
    tree.compute(controlPoints, u);
    pos = tree.value();
    der = tree.derivative();
}

template<typename T, typename Scalar>
T cubicBezierPosCasteljau(core::Span<const T, 4> controlPoints, Scalar u) {
    return bezierPosCasteljau(controlPoints, u);
}

template<typename T, typename Scalar>
T quadraticBezierPosCasteljau(core::Span<const T, 3> controlPoints, Scalar u) {
    return bezierPosCasteljau(controlPoints, u);
}

/// Variant of `cubicBezierPosAndDer` expecting a pointer to a contiguous sequence
/// of 4 control points and using Casteljau's algorithm.
///
template<typename T, typename Scalar>
void cubicBezierPosAndDerCasteljau(
    core::Span<const T, 4> controlPoints,
    Scalar u,
    core::TypeIdentity<T>& pos,
    core::TypeIdentity<T>& der) {

    bezierPosAndDerCasteljau(controlPoints, u, pos, der);
}

template<typename T, typename Scalar>
void quadraticBezierPosAndDerCasteljau(
    core::Span<const T, 3> controlPoints,
    Scalar u,
    core::TypeIdentity<T>& pos,
    core::TypeIdentity<T>& der) {

    bezierPosAndDerCasteljau(controlPoints, u, pos, der);
}

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_BEZIER_H