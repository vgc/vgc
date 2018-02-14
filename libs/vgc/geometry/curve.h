// Copyright 2017 The VGC Developers
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

#include <vector>
#include <vgc/core/object.h>
#include <vgc/geometry/api.h>

namespace vgc {
namespace geometry {

class Vec2d;

VGC_CORE_DECLARE_PTRS(Curve);

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
class VGC_GEOMETRY_API Curve: public core::Object<Curve>
{
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
    Type type() const { return type_; }

    /// Returns the position data of the curve.
    ///
    const std::vector<double>& positionData() const { return positionData_; }

    /// Returns the AttributeVariability of the width attribute.
    ///
    AttributeVariability widthVariability() const { return widthVariability_; }

    /// Returns the width data of the curve.
    ///
    const std::vector<double>& widthData() const { return widthData_; }

    /// Returns the width of the curve. If width is varying, then returns
    /// the average width;
    ///
    double width() const;

    /// Convenient function to add a sample to the curve. This overload is
    /// intended to be used when widthVariability() == Constant.
    ///
    /// If widthVariability() == PerControlPoint, then the width of this sample
    /// is set to be the same as the width of the previous sample (or 1.0 if
    /// there is no previous sample).
    ///
    void addControlPoint(double x, double y);

    /// \overload addControlPoint(double x, double y)
    ///
    void addControlPoint(const Vec2d& position);

    /// Convenient function to add a sample to the curve. This overload is
    /// intended to be used when widthVariability() == PerControlPoint.
    ///
    /// If widthVariability() == Constant, then the provided \p width is ignored.
    ///
    void addControlPoint(double x, double y, double width);

    /// \overload addControlPoint(double x, double y, double width)
    ///
    void addControlPoint(const Vec2d& position, double width);

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
    std::vector<Vec2d> triangulate(
            double maxAngle = 0.05,
            int minQuads = 1,
            int maxQuads = 64) const;

private:
    // Representation of the centerline of the curve
    Type type_;
    std::vector<double> positionData_;

    // Representation of the width of the curve
    AttributeVariability widthVariability_;
    std::vector<double> widthData_;
};

} // namespace geometry
} // namespace vgc

#endif // VGC_GEOMETRY_CURVE_H
