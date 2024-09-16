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
#include <vgc/geometry/interpolatingstroke.h>
#include <vgc/geometry/stroke.h>
#include <vgc/geometry/traits.h>

namespace vgc::geometry {

// Implements Bézier variant of Yuksel Splines:
// http://www.cemyuksel.com/research/interpolating_curves/

template<typename Point, typename T = scalarType<Point>>
class YukselBezierSegment {
private:
    using Quadratic = QuadraticBezier<Point, T>;

public:
    using ScalarType = T;
    static constexpr Int dimension = geometry::dimension<Point>;

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
        core::ConstSpan<Point, 4> knots,
        const Point& b0,
        T u0,
        const Point& b1,
        T u1) noexcept

        : quadratics_{Quadratic(knots[0], b0, knots[2]), Quadratic(knots[1], b1, knots[3])}
        , parameterBounds_{core::clamp(u0, 0, 1), core::clamp(u1, 0, 1)} {
    }

    Point eval(T u) const {
        T u0 = (1 - u) * parameterBounds_[0] + u;
        T u1 = u * parameterBounds_[1];
        Point p0 = quadratics_[0].eval(u0);
        Point p1 = quadratics_[1].eval(u1);
        constexpr T pi_2 = core::pi / 2;
        T a = u * pi_2;
        T cosa = std::cos(a);
        T sina = std::sin(a);
        return (cosa * cosa) * p0 + (sina * sina) * p1;
    }

    Point eval(T u, Point& velocity) const {
        constexpr T pi_2 = core::pi / 2;
        T ti = parameterBounds_[0];
        T tj = parameterBounds_[1];

        // u [0, 1] -> v [ti, 1]
        T dv_du = 1 - ti;
        T v = u * dv_du + ti;
        // u [0, 1] -> w [0, tj]
        T dw_du = tj;
        T w = u * dw_du;

        Point dp0_dv = core::noInit;
        Point p0 = quadratics_[0].eval(v, dp0_dv);
        Point dp0_du = dp0_dv * dv_du;
        Point dp1_dw = core::noInit;
        Point p1 = quadratics_[1].eval(w, dp1_dw);
        Point dp1_du = dp1_dw * dw_du;

        T a = u * pi_2;
        T cosa = std::cos(a);
        T sina = std::sin(a);
        T cosa2 = cosa * cosa;
        T sina2 = sina * sina;

        Point p = cosa2 * p0 + sina2 * p1;
        Point dp_dtheta_term1 = 2.0 * cosa * sina * (p1 - p0);
        Point dp_du_term1 = dp_dtheta_term1 * pi_2;
        Point dp_du = dp_du_term1 + cosa2 * dp0_du + sina2 * dp1_du;

        velocity = dp_du;
        return p;
    }

    Point eval(T u, Point& tangent, T& speed, Point& acceleration) const {
        Point position = core::noInit;
        Point velocity = core::noInit;

        if (u == 0) {
            position = computeEndPointDerivatives(0, velocity, acceleration);
            if (velocity == Point()) {
                if (acceleration == Point()) {
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
            position = computeEndPointDerivatives(1, velocity, acceleration);
            if (velocity == Point()) {
                if (acceleration == Point()) {
                    tangent = (quadratics_[1].controlPoints()[2]
                               - quadratics_[1].controlPoints()[0])
                                  .normalized();
                    speed = 0;
                    return position;
                }
                tangent = -(acceleration.normalized());
                speed = 0;
                return position;
            }
        }
        else {
            position = eval(u, velocity);
        }

        T l = velocity.length();
        if (l > 0) {
            tangent = velocity / l;
        }
        else {
            tangent = Point();
            tangent[0] = 1;
        }
        speed = l;
        return position;
    }

    Point eval(T u, Point& tangent, T& speed) const {
        Point dummy;
        return eval(u, tangent, speed, dummy);
    }

    const std::array<QuadraticBezier<Point, T>, 2>& quadratics() const {
        return quadratics_;
    }

    const std::array<T, 2>& parameterBounds() const {
        return parameterBounds_;
    }

    Point computeEndPointDerivatives(
        Int endpointIndex,
        Point& velocity,
        Point& acceleration) const {

        if (endpointIndex == 0) {
            T v = parameterBounds_[0];
            T dv_du = (1 - v);
            Point position = quadratics_[0].eval(v, velocity);
            velocity *= dv_du;
            acceleration = quadratics_[0].evalSecondDerivative(v) * dv_du;
            return position;
        }
        else {
            T v = parameterBounds_[1];
            T dv_du = v;
            Point position = quadratics_[1].eval(v, velocity);
            velocity *= dv_du;
            acceleration = quadratics_[1].evalSecondDerivative(v) * dv_du;
            return position;
        }
    }

    Point startDerivative() const {
        T ti = parameterBounds_[0];
        return quadratics_[0].evalDerivative(ti) * (1 - ti);
    }

    Point endDerivative() const {
        T ti = parameterBounds_[1];
        return quadratics_[1].evalDerivative(ti) * ti;
    }

private:
    std::array<Quadratic, 2> quadratics_;
    std::array<T, 2> parameterBounds_;
};

using YukselBezierSegment1d = YukselBezierSegment<double>;
using YukselBezierSegment2d = YukselBezierSegment<Vec2d>;

class VGC_GEOMETRY_API YukselSplineStroke2d : public AbstractInterpolatingStroke2d {
public:
    YukselSplineStroke2d(bool isClosed)
        : AbstractInterpolatingStroke2d(isClosed) {
    }

    YukselSplineStroke2d(bool isClosed, double constantWidth)
        : AbstractInterpolatingStroke2d(isClosed, constantWidth) {
    }

    template<typename TRangePositions, typename TRangeWidths>
    YukselSplineStroke2d(
        bool isClosed,
        TRangePositions&& positions,
        TRangeWidths&& widths)

        : AbstractInterpolatingStroke2d(
              isClosed,
              std::forward<TRangePositions>(positions),
              std::forward<TRangeWidths>(widths)) {
    }

protected:
    Vec2d evalNonZeroCenterline(Int segmentIndex, double u) const override;

    Vec2d evalNonZeroCenterline(Int segmentIndex, double u, Vec2d& dp) const override;

    StrokeSample2d evalNonZero(Int segmentIndex, double u, double& speed) const override;

    void sampleNonZeroSegment(
        StrokeSample2dArray& out,
        Int segmentIndex,
        const CurveSamplingParameters& params,
        AdaptiveStrokeSampler& sampler) const override;

    StrokeSample2d zeroLengthStrokeSample() const override;

    YukselBezierSegment2d segmentEvaluator(Int segmentIndex) const;
    YukselBezierSegment2d
    segmentEvaluator(Int segmentIndex, CubicBezier2d& halfwidths) const;

protected:
    const StrokeModelInfo& modelInfo_() const override;

    std::unique_ptr<AbstractStroke2d> cloneEmpty_() const override;
    std::unique_ptr<AbstractStroke2d> clone_() const override;
    bool copyAssign_(const AbstractStroke2d* other) override;
    bool moveAssign_(AbstractStroke2d* other) override;

    StrokeBoundaryInfo computeBoundaryInfo_() const override;

protected:
    void updateCache_(
        const core::Array<SegmentComputeData>& baseComputeDataArray) const override;
};

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_YUKSEL_H
