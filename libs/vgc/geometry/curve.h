// Copyright 2017 The VGC Developers
// See the COPYRIGHT file at the top-level directory of this distribution
// and at https://github.com/vgc.io/vgc/blob/master/COPYRIGHT
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
#include <vgc/geometry/api.h>

namespace vgc {
namespace geometry {

class Vec2d;

/// \class vgc::geometry::Curve
/// \brief Represents a 2D curve with variable width.
///
//
// Note: In the future, we may want to extend this class with:
//     - more curve type (e.g., bezier, bspline, nurbs, ellipticalarc. etc.)
//     - variable color
//     - variable custom attributes (e.g., that can be passed to shaders)
//     - dimension other than 2? Probably not.
//
// Supporting other types of curves in the future is why we use a
// std::vector<double> of size 2*n instead of a std::vector<Vec2d> of size n.
// Indeed, other types of curve may need additional data, such as knot values,
// homogeneous coordinates, etc.
//
// A "cleaner" approach with more type-safety would be to have different
// classes for different types of curves. Unfortunately, this has other
// drawbacks, in particular, switching from one curve type to the other
// dynamically would be harder. Also, it is quite useful to have a continuous
// array of doubles that can directly be passed to C-style functions, such as
// OpenGL, etc.
//
class VGC_GEOMETRY_API Curve
{
public:
    /// Specifies the type of the curve, that is, how the
    /// position of its centerline is represented.
    ///
    enum class Type {

        /// Represents an open Catmull-Rom curve, formatted as [ x0, y0, x1,
        /// y1, ..., xn, yn ]. The curve starts at (x0, y0), ends at (xn, yn),
        /// and go through all control points (xi, yi).
        ///
        /// It is a special case of composite cubic Bezier curve where the tangents
        /// are automatically computed from neighbour control points.
        ///
        CatmullRom
    };

    /// Specifies the type of a variable attribute along the curve, that is,
    /// how it is represented.
    ///
    enum class AttributeVariability {

        /// Represents a constant attribute. For example, a 2D attribute
        /// would be formatted as [ u, v ].
        ///
        Constant,

        /// Represents a varying attribute specified per sample. For example, a
        /// 2D attribute would be formatted as [ u0, v0, u1, v1, ..., un, vn ].
        ///
        PerSample
    };

    /// Creates an empty curve of the given \p type and PerSample width
    /// variability.
    ///
    Curve(Type type = Type::CatmullRom);

    /// Creates an empty curve of the given \p type and the given constant
    /// width variability.
    ///
    Curve(double constantWidth, Type type = Type::CatmullRom);

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
    /// If widthVariability() == PerSample, then the width of this sample is
    /// set to be the same as the width of the previous sample (or 1.0 if there
    /// is no previous sample).
    ///
    void addSample(double x, double y);

    /// \overload addSample(double x, double y)
    ///
    void addSample(const Vec2d& position);

    /// Convenient function to add a sample to the curve. This overload is
    /// intended to be used when widthVariability() == PerSample.
    ///
    /// If widthVariability() == Constant, then the provided \p width is ignored.
    ///
    void addSample(double x, double y, double width);

    /// \overload addSample(double x, double y, double width)
    ///
    void addSample(const Vec2d& position, double width);

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
