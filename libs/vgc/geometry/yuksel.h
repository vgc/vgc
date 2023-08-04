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

#ifndef VGC_GEOMETRY_YUKSEL_H
#define VGC_GEOMETRY_YUKSEL_H

#include <vgc/core/span.h>
#include <vgc/geometry/api.h>
#include <vgc/geometry/bezier.h>
#include <vgc/geometry/curve.h>
#include <vgc/geometry/vec2d.h>

namespace vgc::geometry {

// Implements Bézier variant of Yuksel Splines:
// http://www.cemyuksel.com/research/interpolating_curves/

template<typename T, typename Scalar>
class YukselBezierSegment {
private:
    using Quadratic = QuadraticBezier<T, Scalar>;

public:
    // Initialized with null control points.
    YukselBezierSegment() noexcept = default;

    VGC_WARNING_PUSH
    VGC_WARNING_MSVC_DISABLE(26495) // member variable uninitialized
                                    /// Uninitialized construction.
                                    ///
    YukselBezierSegment(core::NoInit) noexcept
        : quadratics_{core::noInit, core::noInit} {
    }
    VGC_WARNING_POP

    YukselBezierSegment(
        core::ConstSpan<T, 4> knots,
        const T& b0,
        Scalar u0,
        const T& b1,
        Scalar u1) noexcept
        : quadratics_{Quadratic(knots[0], b0, knots[2]), Quadratic(knots[1], b1, knots[3])}
        , parameterBounds_{core::clamp(u0, 0, 1), core::clamp(u1, 0, 1)} {
    }

    T eval(Scalar u) const {
        Scalar u0 = (1 - u) * parameterBounds_[0] + u;
        Scalar u1 = u * parameterBounds_[1];
        T p0 = quadratics_[0].eval(u0);
        T p1 = quadratics_[1].eval(u1);
        constexpr Scalar pi_2 = core::pi / 2;
        Scalar a = u * pi_2;
        Scalar cosa = std::cos(a);
        Scalar sina = std::sin(a);
        return (cosa * cosa) * p0 + (sina * sina) * p1;
    }

    T eval(Scalar u, T& velocity) const {
        constexpr Scalar pi_2 = core::pi / 2;
        Scalar ti = parameterBounds_[0];
        Scalar tj = parameterBounds_[1];

        // u [0, 1] -> v [ti, 1]
        Scalar dv_du = 1 - ti;
        Scalar v = u * dv_du + ti;
        // u [0, 1] -> w [0, tj]
        Scalar dw_du = tj;
        Scalar w = u * dw_du;

        T dp0_dv = core::noInit;
        T p0 = quadratics_[0].eval(v, dp0_dv);
        T dp0_du = dp0_dv * dv_du;
        T dp1_dw = core::noInit;
        T p1 = quadratics_[1].eval(w, dp1_dw);
        T dp1_du = dp1_dw * dw_du;

        Scalar a = u * pi_2;
        Scalar cosa = std::cos(a);
        Scalar sina = std::sin(a);
        Scalar cosa2 = cosa * cosa;
        Scalar sina2 = sina * sina;

        T p = cosa2 * p0 + sina2 * p1;
        T dp_dtheta_term1 = 2.0 * cosa * sina * (p1 - p0);
        T dp_du_term1 = dp_dtheta_term1 * pi_2;
        T dp_du = dp_du_term1 + cosa2 * dp0_du + sina2 * dp1_du;

        velocity = dp_du;
        return p;
    }

    T eval(Scalar u, T& tangent, double& speed) const {
        T position = core::noInit;
        T velocity = core::noInit;

        if (u == 0) {
            T acceleration = core::noInit;
            position = computeEndPointDerivatives(0, velocity, acceleration);
            if (velocity == T()) {
                if (acceleration == T()) {
                    tangent = (quadratics_[0].controlPoints()[2]
                               - quadratics_[0].controlPoints()[0])
                                  .normalized();
                    speed = 0;
                    return position;
                }
                tangent = acceleration.normalized();
                speed = 0;
                return position;
            }
        }
        else if (u == 1) {
            T acceleration = core::noInit;
            position = computeEndPointDerivatives(1, velocity, acceleration);
            if (velocity == T()) {
                if (acceleration == T()) {
                    tangent = (quadratics_[1].controlPoints()[2]
                               - quadratics_[1].controlPoints()[0])
                                  .normalized();
                    speed = 0;
                    return position;
                }
                tangent = -1.0 * acceleration.normalized();
                speed = 0;
                return position;
            }
        }
        else {
            position = eval(u, velocity);
        }

        double l = velocity.length();
        tangent = velocity / l;
        speed = l;
        return position;
    }

    const std::array<QuadraticBezier<T, Scalar>, 2>& quadratics() const {
        return quadratics_;
    }

    const std::array<Scalar, 2>& parameterBounds() const {
        return parameterBounds_;
    }

    Vec2d computeEndPointDerivatives(
        Int endpointIndex,
        Vec2d& velocity,
        Vec2d& acceleration) const {

        if (endpointIndex == 0) {
            Scalar v = parameterBounds_[0];
            Scalar dv_du = (1 - v);
            Vec2d position = quadratics_[0].eval(v, velocity);
            velocity *= dv_du;
            acceleration = quadratics_[0].evalSecondDerivative(v) * dv_du;
            return position;
        }
        else {
            Scalar v = parameterBounds_[1];
            Scalar dv_du = v;
            Vec2d position = quadratics_[1].eval(v, velocity);
            velocity *= dv_du;
            acceleration = quadratics_[1].evalSecondDerivative(v) * dv_du;
            return position;
        }
    }

    T startDerivative() const {
        Scalar ti = parameterBounds_[0];
        return quadratics_[0].evalDerivative(ti) * (1 - ti);
    }

    T endDerivative() const {
        Scalar ti = parameterBounds_[1];
        return quadratics_[1].evalDerivative(ti) * ti;
    }

private:
    std::array<QuadraticBezier<T, Scalar>, 2> quadratics_;
    std::array<Scalar, 2> parameterBounds_;
};

using YukselBezierSegment2d = YukselBezierSegment<Vec2d, double>;
using YukselBezierSegment1d = YukselBezierSegment<double, double>;

namespace detail {

struct YukselKnotData {
    double chordLength;
};

} // namespace detail

class VGC_GEOMETRY_API YukselSplineStroke2d : public AbstractStroke2d {
public:
    YukselSplineStroke2d(bool isClosed)
        : AbstractStroke2d(isClosed) {
    }

    YukselSplineStroke2d(bool isClosed, double constantWidth)
        : AbstractStroke2d(isClosed)
        , widths_(1, constantWidth)
        , isWidthConstant_(true) {
    }

    template<typename TRangePositions, typename TRangeWidths>
    YukselSplineStroke2d(
        bool isClosed,
        bool isWidthConstant,
        TRangePositions&& positions,
        TRangeWidths&& widths)

        : AbstractStroke2d(isClosed)
        , positions_(std::forward<TRangePositions>(positions))
        , widths_(std::forward<TRangeWidths>(widths))
        , isWidthConstant_(isWidthConstant) {

        computeCache_();
    }

    const core::Array<Vec2d>& positions() const {
        return positions_;
    }

    core::Array<Vec2d>&& movePositions() {
        return std::move(positions_);
    }

    template<typename TRange>
    void setPositions(TRange&& positions) {
        positions_ = std::forward<TRange>(positions);
        computeCache_();
    }

    const core::Array<double>& widths() const {
        return widths_;
    }

    // TODO: make data class and startEdit() endEdit()
    core::Array<double>&& moveWidths() {
        return std::move(widths_);
    }

    template<typename TRange>
    void setWidths(TRange&& widths) {
        widths_ = std::forward<TRange>(widths);
    }

    void setConstantWidth(double width) {
        isWidthConstant_ = true;
        widths_.resize(1);
        widths_[0] = width;
    }

    bool isWidthConstant() const {
        return isWidthConstant_;
    }

protected:
    Int numKnots_() const override;

    bool isZeroLengthSegment_(Int segmentIndex) const override;

    Vec2d evalNonZeroCenterline(Int segmentIndex, double u) const override;

    Vec2d evalNonZeroCenterline(Int segmentIndex, double u, Vec2d& dp) const override;

    StrokeSampleEx2d evalNonZero(Int segmentIndex, double u) const override;

    void sampleNonZeroSegment(
        StrokeSampleEx2dArray& out,
        Int segmentIndex,
        const CurveSamplingParameters& params,
        detail::AdaptiveStrokeSampler& sampler) const override;

    StrokeSampleEx2d zeroLengthStrokeSample() const override;

    std::array<Vec2d, 2> computeOffsetLineTangentsAtSegmentEndpoint_(
        Int segmentIndex,
        Int endpointIndex) const override;

    YukselBezierSegment2d segmentEvaluator(Int segmentIndex) const;
    YukselBezierSegment2d
    segmentEvaluator(Int segmentIndex, CubicBezier2d& halfwidths) const;

    double constantWidth() const {
        return widths_[0];
    }

private:
    core::Array<Vec2d> positions_;
    core::Array<double> widths_;

    core::Array<detail::YukselKnotData> knotsData_;

    bool isWidthConstant_ = false;

    void computeCache_();
};

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_YUKSEL_H
