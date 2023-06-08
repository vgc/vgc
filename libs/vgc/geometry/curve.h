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
#include <vgc/core/enum.h>
#include <vgc/core/object.h>
#include <vgc/core/span.h>
#include <vgc/geometry/api.h>
#include <vgc/geometry/mat3d.h>
#include <vgc/geometry/vec2d.h>

namespace vgc::geometry {

class Curve;

enum class CurveSamplingQuality {
    Disabled,
    UniformLow,
    AdaptiveLow,
    UniformHigh,
    AdaptiveHigh,
    UniformVeryHigh,
    Max_ = UniformVeryHigh
};

VGC_GEOMETRY_API
VGC_DECLARE_ENUM(CurveSamplingQuality)

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

    constexpr CurveSample(
        const Vec2d& position,
        const Vec2d& normal,
        double halfwidth = 0.5) noexcept

        : position_(position)
        , normal_(normal)
        , halfwidths_(halfwidth, halfwidth)
        , s_(0) {
    }

    constexpr CurveSample(
        const Vec2d& position,
        const Vec2d& normal,
        const Vec2d& halfwidths,
        double s) noexcept

        : position_(position)
        , normal_(normal)
        , halfwidths_(halfwidths)
        , s_(s) {
    }

    const Vec2d& position() const {
        return position_;
    }

    void setPosition(const Vec2d& position) {
        position_ = position;
    }

    Vec2d tangent() const {
        return -(normal_.orthogonalized());
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

    // ┌─── x
    // │ ─segment─→
    // │  ↓ right
    // y
    //
    double rightHalfwidth() const {
        return halfwidths_[0];
    }

    // ┌─── x
    // │  ↑ left
    // │ ─segment─→
    // y
    //
    double leftHalfwidth() const {
        return halfwidths_[1];
    }

    // ┌─── x
    // │  ↑ halfwidths[1]
    // │ ─segment─→
    // y  ↓ halfwidths[0]
    //
    const Vec2d& halfwidths() const {
        return halfwidths_;
    }

    // ┌─── x
    // │  ↑ halfwidth(1)
    // │ ─segment─→
    // y  ↓ halfwidth(0)
    //
    double halfwidth(Int side) const {
        return halfwidths_[side];
    }

    // ┌─── x
    // │  ↑ halfwidths[1]
    // │ ─segment─→
    // y  ↓ halfwidths[0]
    //
    void setHalfwidths(const Vec2d& halfwidths) {
        halfwidths_ = halfwidths;
    }

    void setWidth(double rightHalfwidth, double leftHalfwidth) {
        halfwidths_[0] = rightHalfwidth;
        halfwidths_[1] = leftHalfwidth;
    }

    // ┌─── x
    // │ ─segment─→
    // │  ↓ right
    // y
    //
    Vec2d rightPoint() const {
        return position_ + normal_ * halfwidths_[0];
    }

    // ┌─── x
    // │  ↑ left
    // │ ─segment─→
    // y
    //
    Vec2d leftPoint() const {
        return position_ - normal_ * halfwidths_[1];
    }

    // ┌─── x
    // │  ↑ side 1
    // │ ─segment─→
    // y  ↓ side 0
    //
    Vec2d sidePoint(Int side) const {
        return side ? leftPoint() : rightPoint();
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
    Vec2d halfwidths_;
    double s_; // total arclength from start point
};

/// Returns a new sample with each attribute linearly interpolated.
///
/// Please note that due to the linear interpolation the new normal may
/// no longer be of length 1. Use `nlerp()` if you want it re-normalized.
///
inline geometry::CurveSample
lerp(const geometry::CurveSample& a, const geometry::CurveSample& b, double t) {
    const double ot = (1 - t);
    return geometry::CurveSample(
        a.position() * ot + b.position() * t,
        a.normal() * ot + b.normal() * t,
        a.halfwidths() * ot + b.halfwidths() * t,
        a.s() * ot + b.s() * t);
}

/// Returns a new sample with each attribute linearly interpolated and
/// the normal also re-normalized.
///
/// Use `lerp()` if you don't need the re-normalization.
///
inline geometry::CurveSample
nlerp(const geometry::CurveSample& a, const geometry::CurveSample& b, double t) {
    geometry::CurveSample ret = lerp(a, b, t);
    ret.setNormal(ret.normal().normalized());
    return ret;
}

/// Alias for `vgc::core::Array<vgc::geometry::CurveSample>`.
///
using CurveSampleArray = core::Array<CurveSample>;

class VGC_GEOMETRY_API DistanceToCurve {
public:
    DistanceToCurve() noexcept = default;

    DistanceToCurve(
        double distance,
        double angleFromTangent,
        Int segmentIndex,
        double segmentParameter) noexcept

        : distance_(distance)
        , angleFromTangent_(angleFromTangent)
        , segmentParameter_(segmentParameter)
        , segmentIndex_(segmentIndex) {
    }

    double distance() const {
        return distance_;
    }

    void setDistance(double distance) {
        distance_ = distance;
    }

    double angleFromTangent() const {
        return angleFromTangent_;
    }

    /// Returns the index of:
    /// - a segment containing a closest point, or
    /// - the index of the last sample.
    ///
    Int segmentIndex() const {
        return segmentIndex_;
    }

    /// Returns the parameter t between 0 and 1 such that
    /// `lerp(samples[segmentIndex], samples[segmentIndex + 1], t)`
    /// is a closest point.
    ///
    double segmentParameter() const {
        return segmentParameter_;
    }

private:
    double distance_ = 0;
    double angleFromTangent_ = 0;
    double segmentParameter_ = 0;
    Int segmentIndex_ = 0;
};

VGC_GEOMETRY_API
DistanceToCurve distanceToCurve(const CurveSampleArray& samples, const Vec2d& position);

/// Alias for `vgc::core::SharedConstArray<vgc::geometry::CurveSample>`.
///
using SharedConstCurveSampleArray = core::SharedConstArray<CurveSample>;

/// \class vgc::geometry::CurveSamplingParameters
/// \brief Parameters for the sampling.
///
/// Using the default parameters, the sampling is "adaptive". This means that
/// the number of samples generated between two control points depends on the
/// curvature of the curve: the higher the curvature, the more samples are
/// generated. This ensures that consecutive segments never have an angle more
/// than `maxAngle` (expressed in radian), which ensures that the curve looks
/// "smooth" at any zoom level.
///
/// \verbatim
///                    _o p3
///                 _-`
///              _-` } maxAngle
/// o----------o`- - - - - - - - -
/// p1         p2
/// \endverbatim
///
/// Note that some algorithms may only take into account the angle between
/// consecutive segments of the centerline, while other may be width-aware,
/// that is, also ensure that the angle between consecutive segments of the
/// offset curves is less than `maxAngle`.
///
/// When the curve is a straight line between two control points, no intra
/// segment samples are needed. However, you can use `minIntraSegmentSamples`
/// if you wish to have at least a certain number of samples uniformly
/// generated between any two control points. Also, you can use
/// `maxIntraSegmentSamples` to limit how many samples are generated between
/// any two control points. This is necessary to break infinite loops in case
/// the curve contains a cusp between two control points.
///
/// If you wish to uniformly generate a fixed number of samples between control
/// points, simply set `maxAngle` to any value, and set
/// `minIntraSegmentSamples` and `maxIntraSegmentSamples` to the same value.
///
class VGC_GEOMETRY_API CurveSamplingParameters {
public:
    CurveSamplingParameters() = default;

    CurveSamplingParameters(CurveSamplingQuality quality);

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

    friend bool
    operator==(const CurveSamplingParameters& a, const CurveSamplingParameters& b) {
        return (a.maxAngle_ == b.maxAngle_)
               && (a.minIntraSegmentSamples_ == b.minIntraSegmentSamples_)
               && (a.maxIntraSegmentSamples_ == b.maxIntraSegmentSamples_);
    }

    friend bool
    operator!=(const CurveSamplingParameters& a, const CurveSamplingParameters& b) {
        return !(a == b);
    }

private:
    double maxAngle_ = 0.05; // 2PI / 0.05 ~= 125.66
    Int minIntraSegmentSamples_ = 0;
    Int maxIntraSegmentSamples_ = 63;
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
/// \brief A helper class to sample a 2D curve given external (non-owned) data.
///
/// This class can be used to sample a 2D curve given external (non-owned) data.
///
/// Note that this class does not own the data provided, for example via the
/// `setPositions(positions)` and `setWidths(widths)` function. It is the
/// responsability of the programmer to ensure that the data referred to by the
/// given `Span` outlives the `Curve`.
///
/// Currently, only Catmull-Rom curves are supported, but more curve types are
/// expected in future versions.
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
        OpenUniformCatmullRom,
        ClosedUniformCatmullRom
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
        return positions_.length();
    }

    /// Returns the position data of the curve.
    ///
    core::ConstSpan<Vec2d> positions() const {
        return positions_;
    }

    /// Sets the position data of the curve.
    ///
    /// Note that this `Curve` does not make a copy of the data. It is your
    /// responsibility to ensure that the data outlives this `Curve`.
    ///
    void setPositions(core::ConstSpan<Vec2d> positions) {
        positions_ = positions;
    }

    /// Returns the AttributeVariability of the width attribute.
    ///
    AttributeVariability widthVariability() const {
        return widthVariability_;
    }

    /// Returns the width data of the curve.
    ///
    core::ConstSpan<double> widths() const {
        return widths_;
    }

    /// Sets the width data of the curve.
    ///
    /// Note that this `Curve` does not make a copy of the data. It is your
    /// responsibility to ensure that the data outlives this `Curve`.
    ///
    void setWidths(core::ConstSpan<double> widths) {
        widths_ = widths;
        onWidthsChanged_();
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

    /// Computes a sampling of the subset of this curve between `startIndex`
    /// and `endIndex`.
    ///
    /// \verbatim
    ///                    startIndex             endIndex
    ///              0         1           2         3         4        5
    /// input      = x---------x-----------x---------x---------x--------x
    ///                        |                     |
    ///                        |                     |
    ///                        |                     |
    ///                        v                     v
    /// output     =           x-x-x-x-x-x-x-x-x-x-x-x
    ///
    /// \endverbatim
    ///
    /// The result is appended to the output parameter `outAppend`.
    ///
    /// The values of `startIndex` and `endIndex` must be in the range [-n,
    /// n-1] with `n = numPoints()`. Negative values can be used for indexing
    /// from the end: `-1` represents the last point, and `-n` represents the
    /// first point.
    ///
    /// This function throws `IndexError` if:
    /// - the curve is empty (`numPoints() == 0`), or
    /// - `startIndex` is not in the range [-n, n-1], or
    /// - `endIndex` is not in the range [-n, n-1], or
    /// - the start index is greater than the end index (after wrapping
    ///   negative input indices to their corresponding positive index).
    ///
    /// The start and end points of the range are both included. This means
    /// that if this function does not throw, it is guaranteed to return a
    /// non-empty sampling (i.e., with at least one sample).
    ///
    /// This also means that calling `sampleRange(out, params, 0, 1)` followed
    /// by `sampleRange(out, params, 1, 2)` would result in having two times
    /// the sample corresponding to index `1`. If you wish to do such chaining
    /// meaningfully, you have to manually discard the last point:
    ///
    /// ```cpp
    /// sampleRange(out, params, 0, 1);
    /// out.removeLast();
    /// sampleRange(out, params, 1, 2);
    /// ```
    ///
    /// If `withArclengths = true` (the default), then arclengths are computed
    /// starting from `s = 0` (if `outAppend` is initially empty) or `s =
    /// outAppend.last().s()` (if `outAppend` is not initially empty).
    ///
    /// If `withArclengths = false` (the default), then all arclengths of the
    /// computed samples are left uninitialized.
    ///
    /// If `numPoints() == 1`, this function returns a unique sample with a
    /// normal set to zero.
    ///
    void sampleRange(
        core::Array<CurveSample>& outAppend,
        const CurveSamplingParameters& parameters,
        Int start = 0,
        Int end = -1,
        bool withArclengths = true) const;

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
    core::Span<const Vec2d> positions_ = {};

    // Representation of the width of the curve
    AttributeVariability widthVariability_;
    core::Span<const double> widths_ = {};
    double widthConstant_ = 0;
    double averageWidth_ = 0;
    //double maxWidth_ = 0;

    // Color of the curve
    core::Color color_;

    void onWidthsChanged_();
};

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_CURVE_H
