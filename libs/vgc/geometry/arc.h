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
#include <vgc/geometry/mat2.h>
#include <vgc/geometry/vec.h>
#include <vgc/geometry/vec2.h>

namespace vgc::geometry {

/// \class vgc::geometry::EllipticalArc2
/// \brief Represents an elliptical arc or a line segment (arc of inifinite radius).
///
template<typename T>
class EllipticalArc2 {
public:
    using ScalarType = T;
    static constexpr Int dimension = 2;

    /// Creates an `EllipticalArc2` actually representing the line segment from
    /// `startPosition` to `endPosition`.
    ///
    static EllipticalArc2
    fromLineSegment(const Vec2<T>& startPosition, const Vec2<T>& endPosition) {
        return EllipticalArc2(startPosition, endPosition);
    }

    /// Creates an elliptical arc with the given `center`, ellipse axes `xAxis`
    /// and `yAxis`, and spanning the angles from `startAngle` to `endAngle`
    /// (in radians).
    ///
    static EllipticalArc2 fromCenterParameters(
        const Vec2<T>& center,
        const Vec2<T>& xAxis,
        const Vec2<T>& yAxis,
        T startAngle,
        T endAngle) {

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
        const Vec2<T>& startPosition,
        const Vec2<T>& endPosition,
        const Vec2<T>& radii,
        T xAxisRotation,
        bool largeArcFlag,
        bool sweepFlag) {

        EllipticalArc2 res(startPosition, endPosition);
        Vec2<T> radii_(std::abs(radii.x()), std::abs(radii.y()));
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

    Vec2<T> eval(T u) const {
        if (isLineSegment_) {
            const Vec2<T>& startPosition = center_;
            const Vec2<T>& deltaPosition = xAxis_;
            return startPosition + u * deltaPosition;
        }
        else {
            T angle = startAngle_ + u * deltaAngle_;
            T cosAngle = std::cos(angle);
            T sinAngle = std::sin(angle);
            return center_ + cosAngle * xAxis_ + sinAngle * yAxis_;
        }
    }

    Vec2<T> eval(T u, Vec2<T>& derivative) const {
        if (isLineSegment_) {
            const Vec2<T>& startPosition = center_;
            const Vec2<T>& deltaPosition = xAxis_;
            derivative = deltaPosition;
            return startPosition + u * deltaPosition;
        }
        else {
            T angle = startAngle_ + u * deltaAngle_;
            T cosAngle = std::cos(angle);
            T sinAngle = std::sin(angle);
            derivative = deltaAngle_ * (-sinAngle * xAxis_ + cosAngle * yAxis_);
            return center_ + cosAngle * xAxis_ + sinAngle * yAxis_;
        }
    }

    Vec2<T> evalDerivative(T u) const {
        if (isLineSegment_) {
            const Vec2<T>& deltaPosition = xAxis_;
            return deltaPosition;
        }
        else {
            T angle = startAngle_ + u * deltaAngle_;
            T cosAngle = std::cos(angle);
            T sinAngle = std::sin(angle);
            return deltaAngle_ * (-sinAngle * xAxis_ + cosAngle * yAxis_);
        }
    }

    Vec2<T> evalSecondDerivative(T u) const {
        if (isLineSegment_) {
            return Vec2<T>(0, 0);
        }
        else {
            T angle = startAngle_ + u * deltaAngle_;
            T cosAngle = std::cos(angle);
            T sinAngle = std::sin(angle);
            return -deltaAngle_ * deltaAngle_ * (cosAngle * xAxis_ + sinAngle * yAxis_);
        }
    }

private:
    bool isLineSegment_;
    Vec2<T> center_; // used as start position if line segment
    Vec2<T> xAxis_;  // used as (end - start) vector if line segment
    Vec2<T> yAxis_;
    T startAngle_;
    T deltaAngle_;

    // Line segment
    EllipticalArc2(Vec2<T> startPosition, Vec2<T> endPosition)
        : isLineSegment_(true)
        , center_(startPosition)
        , xAxis_(endPosition - startPosition) {
    }

    // Center parameters
    EllipticalArc2(Vec2<T> center, Vec2<T> xAxis, Vec2<T> yAxis, T startAngle, T endAngle)

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
        const Vec2<T>& startPosition,
        const Vec2<T>& endPosition,
        const Vec2<T>& radii,
        T xAxisRotation,
        bool largeArcFlag,
        bool sweepFlag) {

        isLineSegment_ = false;
        T rx = radii.x();
        T ry = radii.y();

        // Correction of out-of-range radii
        T cosphi = std::cos(xAxisRotation);
        T sinphi = std::sin(xAxisRotation);
        T rx2 = rx * rx;
        T ry2 = ry * ry;
        Mat2<T> rot(cosphi, -sinphi, sinphi, cosphi);
        Mat2<T> rotInv(cosphi, sinphi, -sinphi, cosphi);
        Vec2<T> p_ = rotInv * (0.5 * (startPosition - endPosition));
        T px_2 = p_[0] * p_[0];
        T py_2 = p_[1] * p_[1];
        T d2 = px_2 / rx2 + py_2 / ry2;
        if (d2 > 1) {
            T d = std::sqrt(d2);
            rx *= d;
            ry *= d;
            rx2 = rx * rx;
            ry2 = ry * ry;
        }

        // Conversion from endpoint to center parameterization.
        xAxis_ = Vec2<T>(rx * cosphi, rx * sinphi);
        yAxis_ = Vec2<T>(-ry * sinphi, ry * cosphi);
        T rx2py_2 = rx2 * py_2;
        T ry2px_2 = ry2 * px_2;
        T a2 = (rx2 * ry2 - rx2py_2 - ry2px_2) / (rx2py_2 + ry2px_2);
        T a = std::sqrt(std::abs(a2));
        if (largeArcFlag == sweepFlag) {
            a *= -1;
        }
        Vec2<T> c_(a * p_[1] * rx / ry, -a * p_[0] * ry / rx);
        center_ = rot * c_ + 0.5 * (startPosition + endPosition);
        Vec2<T> pc = p_ - c_;
        Vec2<T> mpc = -p_ - c_;
        Vec2<T> rInv(1 / rx, 1 / ry);
        Vec2<T> e1(1, 0);
        Vec2<T> e2(pc.x() * rInv.x(), pc.y() * rInv.y());
        Vec2<T> e3(mpc.x() * rInv.x(), mpc.y() * rInv.y());
        startAngle_ = e1.angle(e2);
        deltaAngle_ = e2.angle(e3);
        if (sweepFlag == false && deltaAngle_ > 0) {
            deltaAngle_ -= 2 * core::narrow_cast<T>(core::pi);
        }
        else if (sweepFlag == true && deltaAngle_ < 0) {
            deltaAngle_ += 2 * core::narrow_cast<T>(core::pi);
        }
    }
};

using EllipticalArc2f = EllipticalArc2<float>;
using EllipticalArc2d = EllipticalArc2<double>;

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_ARC_H
