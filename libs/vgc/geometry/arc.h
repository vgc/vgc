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

#ifndef VGC_GEOMETRY_ARC_H
#define VGC_GEOMETRY_ARC_H

#include <vgc/core/span.h>
#include <vgc/geometry/api.h>
#include <vgc/geometry/mat.h>
#include <vgc/geometry/mat2d.h>
#include <vgc/geometry/mat2f.h>
#include <vgc/geometry/vec.h>
#include <vgc/geometry/vec2d.h>
#include <vgc/geometry/vec2f.h>

namespace vgc::geometry {

/// \class EllipticalArc2
/// \brief Represents an elliptical arc or a line segment (arc of inifinite radius).
///
template<typename Scalar>
class EllipticalArc2 {
public:
    static constexpr Int dimension = 2;
    using ScalarType = Scalar;
    using Vec2Type = Vec<2, Scalar>;
    using Mat2Type = Mat<2, Scalar>;

    /// Creates an `EllipticalArc2` actually representing the line segment from
    /// `startPosition` to `endPosition`.
    ///
    static EllipticalArc2
    fromLineSegment(const Vec2Type& startPosition, const Vec2Type& endPosition) {
        return EllipticalArc2(startPosition, endPosition);
    }

    /// Creates an elliptical arc with the given `center`, ellipse axes `xAxis`
    /// and `yAxis`, and spanning the angles from `startAngle` to `endAngle`
    /// (in radians).
    ///
    static EllipticalArc2 fromCenterParameters(
        const Vec2Type& center,
        const Vec2Type& xAxis,
        const Vec2Type& yAxis,
        ScalarType startAngle,
        ScalarType endAngle) {

        return EllipticalArc2(center, xAxis, yAxis, startAngle, endAngle);
    }

    /// Creates an elliptical arc given the same parameters as defined by the
    /// "arc command" (A) in SVG path data. However, unlike in SVG, note that
    /// `xAxisRotation` must be given in radian rather than in degree ( =
    /// svgValue / 180.0 * pi).
    ///
    /// See: https://www.w3.org/TR/SVG11/paths.html#PathDataEllipticalArcCommands
    ///
    static EllipticalArc2 fromSvgParameters(
        const Vec2Type& startPosition,
        const Vec2Type& endPosition,
        const Vec2Type& radii,
        ScalarType xAxisRotation,
        bool largeArcFlag,
        bool sweepFlag) {

        EllipticalArc2 res(startPosition, endPosition);
        Vec2Type radii_(std::abs(radii.x()), std::abs(radii.y()));
        if (radii_.x() > 0 && radii_.y() > 0) {
            res.convertSvgToCenterParameters_(
                startPosition,
                endPosition,
                radii_,
                xAxisRotation,
                largeArcFlag,
                sweepFlag);
        }
        return res;
    }

    Vec2Type eval(ScalarType u) const {
        if (isLineSegment_) {
            const Vec2Type& startPosition = center_;
            const Vec2Type& deltaPosition = xAxis_;
            return startPosition + u * deltaPosition;
        }
        else {
            ScalarType angle = startAngle_ + u * deltaAngle_;
            ScalarType cosAngle = std::cos(angle);
            ScalarType sinAngle = std::sin(angle);
            return center_ + cosAngle * xAxis_ + sinAngle * yAxis_;
        }
    }

    Vec2Type eval(ScalarType u, Vec2Type& derivative) const {
        if (isLineSegment_) {
            const Vec2Type& startPosition = center_;
            const Vec2Type& deltaPosition = xAxis_;
            derivative = deltaPosition;
            return startPosition + u * deltaPosition;
        }
        else {
            ScalarType angle = startAngle_ + u * deltaAngle_;
            ScalarType cosAngle = std::cos(angle);
            ScalarType sinAngle = std::sin(angle);
            derivative = deltaAngle_ * (-sinAngle * xAxis_ + cosAngle * yAxis_);
            return center_ + cosAngle * xAxis_ + sinAngle * yAxis_;
        }
    }

    Vec2Type evalDerivative(ScalarType u) const {
        if (isLineSegment_) {
            const Vec2Type& deltaPosition = xAxis_;
            return deltaPosition;
        }
        else {
            ScalarType angle = startAngle_ + u * deltaAngle_;
            ScalarType cosAngle = std::cos(angle);
            ScalarType sinAngle = std::sin(angle);
            return deltaAngle_ * (-sinAngle * xAxis_ + cosAngle * yAxis_);
        }
    }

    Vec2Type evalSecondDerivative(ScalarType u) const {
        if (isLineSegment_) {
            return Vec2Type(0, 0);
        }
        else {
            ScalarType angle = startAngle_ + u * deltaAngle_;
            ScalarType cosAngle = std::cos(angle);
            ScalarType sinAngle = std::sin(angle);
            return -deltaAngle_ * deltaAngle_ * (cosAngle * xAxis_ + sinAngle * yAxis_);
        }
    }

private:
    bool isLineSegment_;
    Vec2Type center_; // used as start position if line segment
    Vec2Type xAxis_;  // used as (end - start) vector if line segment
    Vec2Type yAxis_;
    ScalarType startAngle_;
    ScalarType deltaAngle_;

    // Line segment
    EllipticalArc2(Vec2Type startPosition, Vec2Type endPosition)
        : isLineSegment_(true)
        , center_(startPosition)
        , xAxis_(endPosition - startPosition) {
    }

    // Center parameters
    EllipticalArc2(
        Vec2Type center,
        Vec2Type xAxis,
        Vec2Type yAxis,
        ScalarType startAngle,
        ScalarType endAngle)

        : isLineSegment_(false)
        , center_(center)
        , xAxis_(xAxis)
        , yAxis_(yAxis)
        , startAngle_(startAngle)
        , deltaAngle_(endAngle - startAngle) {
    }

    // See https://www.w3.org/TR/SVG11/implnote.html#ArcImplementationNotes
    //
    void convertSvgToCenterParameters_(
        const Vec2Type& startPosition,
        const Vec2Type& endPosition,
        const Vec2Type& radii,
        ScalarType xAxisRotation,
        bool largeArcFlag,
        bool sweepFlag) {

        isLineSegment_ = false;
        ScalarType rx = radii.x();
        ScalarType ry = radii.y();

        // Correction of out-of-range radii
        ScalarType cosphi = std::cos(xAxisRotation);
        ScalarType sinphi = std::sin(xAxisRotation);
        ScalarType rx2 = rx * rx;
        ScalarType ry2 = ry * ry;
        Mat2Type rot(cosphi, -sinphi, sinphi, cosphi);
        Mat2Type rotInv(cosphi, sinphi, -sinphi, cosphi);
        Vec2Type p_ = rotInv * (0.5 * (startPosition - endPosition));
        ScalarType px_2 = p_[0] * p_[0];
        ScalarType py_2 = p_[1] * p_[1];
        ScalarType d2 = px_2 / rx2 + py_2 / ry2;
        if (d2 > 1) {
            ScalarType d = std::sqrt(d2);
            rx *= d;
            ry *= d;
            rx2 = rx * rx;
            ry2 = ry * ry;
        }

        // Conversion from endpoint to center parameterization.
        xAxis_ = Vec2Type(rx * cosphi, rx * sinphi);
        yAxis_ = Vec2Type(-ry * sinphi, ry * cosphi);
        ScalarType rx2py_2 = rx2 * py_2;
        ScalarType ry2px_2 = ry2 * px_2;
        ScalarType a2 = (rx2 * ry2 - rx2py_2 - ry2px_2) / (rx2py_2 + ry2px_2);
        ScalarType a = std::sqrt(std::abs(a2));
        if (largeArcFlag == sweepFlag) {
            a *= -1;
        }
        Vec2Type c_(a * p_[1] * rx / ry, -a * p_[0] * ry / rx);
        center_ = rot * c_ + 0.5 * (startPosition + endPosition);
        Vec2Type pc = p_ - c_;
        Vec2Type mpc = -p_ - c_;
        Vec2Type rInv(1 / rx, 1 / ry);
        Vec2Type e1(1, 0);
        Vec2Type e2(pc.x() * rInv.x(), pc.y() * rInv.y());
        Vec2Type e3(mpc.x() * rInv.x(), mpc.y() * rInv.y());
        startAngle_ = e1.angle(e2);
        deltaAngle_ = e2.angle(e3);
        if (sweepFlag == false && deltaAngle_ > 0) {
            deltaAngle_ -= 2 * core::narrow_cast<ScalarType>(core::pi);
        }
        else if (sweepFlag == true && deltaAngle_ < 0) {
            deltaAngle_ += 2 * core::narrow_cast<ScalarType>(core::pi);
        }
    }
};

using EllipticalArc2f = EllipticalArc2<float>;
using EllipticalArc2d = EllipticalArc2<double>;

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_ARC_H
