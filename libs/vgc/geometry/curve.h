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

#ifndef VGC_GEOMETRY_CURVE_H
#define VGC_GEOMETRY_CURVE_H

#include <vgc/core/arithmetic.h>
#include <vgc/core/array.h>
#include <vgc/core/color.h>
#include <vgc/core/object.h>
#include <vgc/geometry/api.h>
#include <vgc/geometry/mat3d.h>
#include <vgc/geometry/vec2d.h>

namespace vgc::geometry {

class Curve;

class CurveSample {
public:
    constexpr CurveSample() noexcept
        : s_(0) {
    }

    VGC_WARNING_PUSH
    VGC_WARNING_MSVC_DISABLE(26495) // member variable uninitialized
    CurveSample(core::NoInit) noexcept
        : position_(core::noInit)
        , normal_(core::noInit) {
    }
    VGC_WARNING_POP

    constexpr CurveSample(Vec2d position, Vec2d normal, double halfwidth = 0.5) noexcept
        : position_(position)
        , normal_(normal)
        , width_(halfwidth, halfwidth)
        , s_(0) {
    }

    const Vec2d& position() const {
        return position_;
    }

    void setPosition(const Vec2d& position) {
        position_ = position;
    }

    // ┌─── x
    // │ ─segment─→
    // y  ↓ normal
    //
    const Vec2d& normal() const {
        return normal_;
    }

    // ┌─── x
    // │ ─segment─→
    // y  ↓ normal
    //
    void setNormal(const Vec2d& normal) {
        normal_ = normal;
    }

    double rightHalfwidth() const {
        return width_[0];
    }

    double leftHalfwidth() const {
        return width_[1];
    }

    void setWidth(double rightHalfwidth, double leftHalfwidth) {
        width_[0] = rightHalfwidth;
        width_[1] = leftHalfwidth;
    }

    // ┌─── x
    // │  ↑ left
    // │ ─segment─→
    // y  ↓ right
    //
    Vec2d rightPoint() const {
        return position_ + normal_ * width_[0];
    }

    // ┌─── x
    // │  ↑ left
    // │ ─segment─→
    // y  ↓ right
    //
    Vec2d leftPoint() const {
        return position_ - normal_ * width_[1];
    }

    double s() const {
        return s_;
    }

    void setS(double s) {
        s_ = s;
    }

private:
    Vec2d position_;
    Vec2d normal_;
    Vec2d width_;
    double s_; // total arclength from start point
};

/// Alias for `vgc::core::Array<vgc::geometry::CurveSample>`.
///
using CurveSampleArray = core::Array<CurveSample>;

/// Alias for `vgc::core::SharedConstArray<vgc::geometry::CurveSample>`.
///
using SharedConstCurveSampleArray = core::SharedConstArray<CurveSample>;

/// \class vgc::geometry::CurveSamplingParameters
/// \brief Parameters for the sampling.
///
class CurveSamplingParameters {
public:
    CurveSamplingParameters() = default;

    CurveSamplingParameters(
        double maxAngle,
        Int minIntraSegmentSamples,
        Int maxIntraSegmentSamples)

        : maxAngle_(maxAngle)
        , minIntraSegmentSamples_(minIntraSegmentSamples)
        , maxIntraSegmentSamples_(maxIntraSegmentSamples) {
    }

    double maxAngle() const {
        return maxAngle_;
    }

    void setMaxAngle(double maxAngle) {
        maxAngle_ = maxAngle;
    }

    Int minIntraSegmentSamples() const {
        return minIntraSegmentSamples_;
    }

    void setMinIntraSegmentSamples(Int minIntraSegmentSamples) {
        minIntraSegmentSamples_ = minIntraSegmentSamples;
    }

    Int maxIntraSegmentSamples() const {
        return maxIntraSegmentSamples_;
    }

    void setMaxIntraSegmentSamples(Int maxIntraSegmentSamples) {
        maxIntraSegmentSamples_ = maxIntraSegmentSamples;
    }

private:
    double maxAngle_ = 0.05; // 2PI / 0.05 ~= 125.66
    Int minIntraSegmentSamples_ = 1;
    Int maxIntraSegmentSamples_ = 64;
};

/// \class vgc::geometry::WidthProfile
/// \brief A widths profile to apply on curves.
///
class WidthProfile {
public:
    WidthProfile() = default;

    // XXX todo

private:
    core::Array<Vec2d> values_;
};

/// \class vgc::geometry::Curve
/// \brief Represents a 2D curve with variable width.
///
/// This class is the preferred class to represent 2D curves in VGC
/// applications. Currently, only Catmull-Rom curves are supported, but more
/// curve types are expected in future versions.
///
/// Example 1: curve with constant width
///
/// \code
/// double width = 3;
/// Curve c(width);
/// c.addControlPoint(0, 0);
/// c.addControlPoint(100, 100);
/// c.addControlPoint(100, 200);
/// \endcode
///
/// Example 2: curve with varying width
///
/// \code
/// Curve c;
/// c.addControlPoint(0, 0, 3);
/// c.addControlPoint(100, 100, 4);
/// c.addControlPoint(100, 200, 5);
/// \endcode
///
/// In order to render the curve, you can call triangulate(), then render
/// the triangles using OpenGL.
///
class VGC_GEOMETRY_API Curve {
public:
    /// Specifies the type of the curve, that is, how the
    /// position of its centerline is represented.
    ///
    enum class Type {

        /// Represents an open uniform Catmull-Rom spline, formatted as [ x0,
        /// y0, x1, y1, ..., xn, yn ]. The curve starts at (x0, y0), ends at
        /// (xn, yn), and go through all control points p[i] = (xi, yi).
        ///
        /// In general, a Catmull-Rom spline is a cubic Hermite spline where the
        /// derivative m[i] at each control point p[i] is not user-specified,
        /// but is instead automatically computed based on the position of its
        /// two adjacent control points.
        ///
        /// In the specific case of a *uniform* Catmull-Rom spline, we have:
        ///
        /// m[i] = (p[i+1] - p[i-1]) / 2
        ///
        /// In the specific case of an *open* uniform Catmull-Rom spline, we
        /// also assume by convention that p[-1] = p[0] and p[n+1] = p[n], that
        /// is, we duplicate the end control points.
        ///
        /// Other types of Catmull-Rom splines exist but are not currently
        /// supported.
        ///
        OpenUniformCatmullRom
    };

    /// Specifies the type of a variable attribute along the curve, that is,
    /// how it is represented.
    ///
    enum class AttributeVariability {

        /// Represents a constant attribute. For example, a 2D attribute
        /// would be formatted as [ u, v ].
        ///
        Constant,

        /// Represents a varying attribute specified per control point. For
        /// example, a 2D attribute would be formatted as [ u0, v0, u1, v1,
        /// ..., un, vn ].
        ///
        PerControlPoint
    };

    /// Creates an empty curve of the given \p type and PerControlPoint width
    /// variability.
    ///
    Curve(Type type = Type::OpenUniformCatmullRom);

    /// Creates an empty curve of the given \p type and the given constant
    /// width variability.
    ///
    Curve(double constantWidth, Type type = Type::OpenUniformCatmullRom);

    /// Returns the Type of the curve.
    ///
    Type type() const {
        return type_;
    }

    /// Returns the number of control points of the curve.
    ///
    Int numPoints() const {
        return positionData_.get().length();
    }

    /// Returns the position data of the curve.
    ///
    const Vec2dArray& positionData() const {
        return positionData_;
    }

    /// Sets the position data of the curve.
    ///
    void setPositionData(const SharedConstVec2dArray& positionData) {
        positionData_ = positionData;
    }

    /// Returns the AttributeVariability of the width attribute.
    ///
    AttributeVariability widthVariability() const {
        return widthVariability_;
    }

    /// Returns the width data of the curve.
    ///
    const core::DoubleArray& widthData() const {
        return widthData_;
    }

    /// Sets the width data of the curve.
    ///
    void setWidthData(const core::SharedConstDoubleArray& widthData) {
        widthData_ = widthData;
        onWidthDataChanged_();
    }

    /// Returns the width of the curve. If width is varying, then returns
    /// the average width;
    ///
    double width() const;

    /// Computes and returns a triangulation of this curve as a triangle strip
    /// [ p0, p1, ..., p_{2n}, p_{2n+1} ]. The even indices are on the "left"
    /// of the centerline, while the odd indices are on the "right", assuming a
    /// right-handed 2D coordinate system.
    ///
    /// Representing pairs of triangles as a single quad, it looks like this:
    ///
    /// \verbatim
    /// Y ^
    ///   |
    ///   o---> X
    ///
    /// p0  p2                                p_{2n}
    /// o---o-- ............................ --o
    /// |   |                                  |
    /// |   |    --- curve direction --->      |
    /// |   |                                  |
    /// o---o-- ............................ --o
    /// p1  p3                                p_{2n+1}
    /// \endverbatim
    ///
    /// Using the default parameters, the triangulation is "adaptive". This
    /// means that the number of quads generated between two control points
    /// depends on the curvature of the curve. The higher the curvature, the
    /// more quads are generated to ensure that consecutive quads never have an
    /// angle more than \p maxAngle (expressed in radian). This is what makes
    /// sure that the curve looks "smooth" at any zoom level.
    ///
    /// \verbatim
    ///                    _o p4
    ///                 _-` |
    /// p0        p2 _-`    |
    /// o----------o`       |
    /// |          |       _o p5
    /// |          |    _-`
    /// |          | _-` } maxAngle
    /// o----------o`- - - - - - - - -
    /// p1         p3
    /// \endverbatim
    ///
    /// In the case where the curve is a straight line between two control
    /// points, a single quad is enough. However, you can use use \p minQuads
    /// if you wish to have at least a certain number of quads uniformly
    /// generated between any two control points. Also, you can use \p maxQuads
    /// to limit how many quads are generated between any two control points.
    /// This is necessary to break infinite loops in case the curve contains a
    /// cusp between two control points.
    ///
    /// If you wish to uniformly generate a fixed number of quads between
    /// control points, simply set maxAngle to any value, and set minQuads =
    /// maxQuads = number of desired quads.
    ///
    Vec2dArray
    triangulate(double maxAngle = 0.05, Int minQuads = 1, Int maxQuads = 64) const;

    /// Computes and returns a sampling of this curve as a sequence of samples
    /// with position and normal.
    ///
    /// Using the default parameters, the sampling is "adaptive". This means
    /// that the number of samples generated between two control points depends
    /// on the curvature of the curve and curve outline. The higher the curvature,
    /// the more samples are generated to ensure that consecutive segments never
    /// have an angle more than \p maxAngle (expressed in radian).
    /// This is what makes sure that the curve looks "smooth" at any zoom level.
    ///
    /// \verbatim
    ///                    _o p3
    ///                 _-`
    ///              _-` } maxAngle
    /// o----------o`- - - - - - - - -
    /// p1         p2
    /// \endverbatim
    ///
    /// In the case where the curve is a straight line between two control points,
    /// no samples are needed. However, you can use use \p minIntraSegmentSamples_
    /// if you wish to have at least a certain number of quads uniformly generated
    /// between any two control points. Also, you can use \p maxIntraSegmentSamples_
    /// to limit how many samples are generated between any two control points.
    /// This is necessary to break infinite loops in case the curve contains a
    /// cusp between two control points.
    ///
    /// If you wish to uniformly generate a fixed number of samples between
    /// control points, simply set maxAngle to any value, and set minQuads =
    /// maxQuads = number of desired samples in the parameters.
    ///
    void sampleRange(
        const CurveSamplingParameters& parameters,
        core::Array<CurveSample>& outAppend,
        Int start = 0,
        Int end = -1) const;

    /// Sets the color of the curve.
    ///
    // XXX Think aboutvariability for colors too. Does it make sense
    // to also have PerControlPoint variability? or only "SpatialLinear"?
    // Does it make sense to allow SpatialLinear for width too? and Circular?
    // It is a tradeoff between flexibility, supporting typical use case,
    // performance, and cognitive complexity (don't confuse users...). It
    // makes it complex for implementers too if too many features are supported.
    // For now, we only support constant colors, and postpone this discussion.
    //
    void setColor(const core::Color& color) {
        color_ = color;
    }

    /// Returns the color of the curve.
    ///
    core::Color color() const {
        return color_;
    }

private:
    // Representation of the centerline of the curve
    Type type_;
    SharedConstVec2dArray positionData_;

    // Representation of the width of the curve
    AttributeVariability widthVariability_;
    core::SharedConstDoubleArray widthData_;
    double averageWidth_ = 0;
    //double maxWidth_ = 0;

    // Color of the curve
    core::Color color_;

    void onWidthDataChanged_();
};

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_CURVE_H
