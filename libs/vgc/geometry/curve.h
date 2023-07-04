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

#include <array>

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

class VGC_GEOMETRY_API StrokeSample2d {
public:
    constexpr StrokeSample2d() noexcept
        : tangent_(0, 1)
        , s_(0)
        , isCornerStart_(false) {
    }

    VGC_WARNING_PUSH
    VGC_WARNING_MSVC_DISABLE(26495) // member variable uninitialized
    StrokeSample2d(core::NoInit) noexcept
        : position_(core::noInit)
        , tangent_(core::noInit)
        , halfwidths_(core::noInit) {
    }
    VGC_WARNING_POP

    StrokeSample2d(
        const Vec2d& position,
        const Vec2d& tangent,
        const Vec2d& halfwidths,
        double s = 0) noexcept

        : position_(position)
        , tangent_(tangent)
        , halfwidths_(halfwidths)
        , s_(s) {
    }

    StrokeSample2d(
        const Vec2d& position,
        const Vec2d& tangent,
        double halfwidth,
        double s = 0) noexcept

        : StrokeSample2d(position, tangent, Vec2d(halfwidth, halfwidth), s) {
    }

    const Vec2d& position() const {
        return position_;
    }

    void setPosition(const Vec2d& position) {
        position_ = position;
    }

    const Vec2d& tangent() const {
        return tangent_;
    }

    void setTangent(const Vec2d& tangent) {
        tangent_ = tangent;
    }

    void reverseTangent() {
        tangent_ = -tangent_;
    }

    // ┌─── x
    // │ ─segment─→
    // y  ↓ normal
    //
    Vec2d normal() const {
        return tangent().orthogonalized();
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

    // ┌─── x
    // │  ↑ halfwidth1
    // │ ─segment─→
    // y  ↓ halfwidth0
    //
    void setHalfwidths(double halfwidth0, double halfwidth1) {
        halfwidths_[0] = halfwidth0;
        halfwidths_[1] = halfwidth1;
    }

    // ┌─── x
    // │  ↑ side 1
    // │ ─segment─→
    // y  ↓ side 0
    //
    void setHalfwidth(Int side, double halfwidth) {
        halfwidths_[side] = halfwidth;
    }

    void swapHalfwidths() {
        std::swap(halfwidths_[0], halfwidths_[1]);
    }

    // ┌─── x
    // │  ↑ offsetPoints[1]
    // │ ─segment─→
    // y  ↓ offsetPoints[0]
    //
    std::array<Vec2d, 2> offsetPoints() const {
        Vec2d normal = this->normal();
        return {position_ + normal * halfwidths_[0], position_ - normal * halfwidths_[1]};
    }

    // ┌─── x
    // │  ↑ side 1
    // │ ─segment─→
    // y  ↓ side 0
    //
    Vec2d offsetPoint(Int side) const {
        if (side) {
            return position_ - normal() * halfwidths_[1];
        }
        else {
            return position_ + normal() * halfwidths_[0];
        }
    }

    double s() const {
        return s_;
    }

    void setS(double s) {
        s_ = s;
    }

    bool isCornerStart() const {
        return isCornerStart_;
    }

    void setCornerStart(bool isCornerStart) {
        isCornerStart_ = isCornerStart;
    }

private:
    Vec2d position_;
    Vec2d tangent_;
    Vec2d halfwidths_;
    double s_; // arclength from stroke start point.
    // isCornerStart_ is true only for the first sample of the two that makes
    // a corner (hard turn).
    // TODO: add enum/flags for corner kind ? knot corner, centerline cusp, offsetline cusp..
    bool isCornerStart_;
};

/// Returns a new sample with each attribute linearly interpolated.
///
/// Please note that due to the linear interpolation the new normal may
/// no longer be of length 1. Use `nlerp()` if you want it re-normalized.
///
inline StrokeSample2d lerp(const StrokeSample2d& a, const StrokeSample2d& b, double t) {
    const double ot = (1 - t);
    StrokeSample2d result(
        a.position() * ot + b.position() * t,
        a.tangent() * ot + b.tangent() * t,
        a.halfwidths() * ot + b.halfwidths() * t,
        a.s() * ot + b.s() * t);
    return result;
}

/// Returns a new sample with each attribute linearly interpolated and
/// the normal also re-normalized.
///
/// Use `lerp()` if you don't need the re-normalization.
///
inline StrokeSample2d nlerp(const StrokeSample2d& a, const StrokeSample2d& b, double t) {
    StrokeSample2d result = lerp(a, b, t);
    result.setTangent(result.tangent().normalized());
    return result;
}

/// Alias for `vgc::core::Array<vgc::geometry::StrokeSample2d>`.
///
using StrokeSample2dArray = core::Array<StrokeSample2d>;

/// Alias for `vgc::core::SharedConstArray<vgc::geometry::StrokeSample2d>`.
///
using SharedConstStrokeSample2dArray = core::SharedConstArray<StrokeSample2d>;

class VGC_GEOMETRY_API StrokeSampleEx2d {
public:
    constexpr StrokeSampleEx2d() noexcept
        : sample_()
        , speed_(0)
        , segmentIndex_(-1)
        , u_(-1) {
    }

    VGC_WARNING_PUSH
    VGC_WARNING_MSVC_DISABLE(26495) // member variable uninitialized
    StrokeSampleEx2d(core::NoInit) noexcept
        : sample_(core::noInit) {
    }
    VGC_WARNING_POP

    StrokeSampleEx2d(
        const Vec2d& position,
        const Vec2d& tangent,
        const Vec2d& halfwidths,
        double speed,
        Int segmentIndex = -1,
        double u = 0) noexcept

        : sample_(position, tangent, halfwidths)
        , speed_(speed)
        , segmentIndex_(segmentIndex)
        , u_(u) {

        updateOffsetPoints_();
    }

    StrokeSampleEx2d(
        const Vec2d& position,
        const Vec2d& tangent,
        double halfwidth,
        double speed,
        Int segmentIndex = -1,
        double u = 0) noexcept

        : StrokeSampleEx2d(
            position,
            tangent,
            Vec2d(halfwidth, halfwidth),
            speed,
            segmentIndex,
            u) {
    }

    operator const StrokeSample2d&() const {
        return sample_;
    }

    const Vec2d& position() const {
        return sample_.position();
    }

    void setPosition(const Vec2d& position) {
        sample_.setPosition(position);
        updateOffsetPoints_();
    }

    const Vec2d& tangent() const {
        return sample_.tangent();
    }

    void setTangent(const Vec2d& tangent) {
        sample_.setTangent(tangent);
        updateOffsetPoints_();
    }

    double speed() const {
        return speed_;
    }

    Vec2d velocity() const {
        return sample_.tangent() * speed_;
    }

    void setVelocity(const Vec2d& velocity) {
        speed_ = velocity.length();
        if (speed_ > 0) {
            sample_.setTangent(velocity / speed_);
        }
        else {
            sample_.setTangent(Vec2d(0, 1));
        }
        updateOffsetPoints_();
    }

    void setVelocity(const Vec2d& direction, double speed) {
        sample_.setTangent(direction);
        speed_ = speed;
        updateOffsetPoints_();
    }

    void reverseVelocity() {
        sample_.reverseTangent();
        std::swap(offsetPoints_[0], offsetPoints_[1]);
    }

    // ┌─── x
    // │ ─segment─→
    // y  ↓ normal
    //
    Vec2d normal() const {
        return sample_.normal();
    }

    // ┌─── x
    // │  ↑ halfwidths[1]
    // │ ─segment─→
    // y  ↓ halfwidths[0]
    //
    const Vec2d& halfwidths() const {
        return sample_.halfwidths();
    }

    // ┌─── x
    // │  ↑ halfwidth(1)
    // │ ─segment─→
    // y  ↓ halfwidth(0)
    //
    double halfwidth(Int side) const {
        return sample_.halfwidth(side);
    }

    // ┌─── x
    // │  ↑ halfwidths[1]
    // │ ─segment─→
    // y  ↓ halfwidths[0]
    //
    void setHalfwidths(const Vec2d& halfwidths) {
        sample_.setHalfwidths(halfwidths);
        updateOffsetPoints_();
    }

    // ┌─── x
    // │  ↑ halfwidth1
    // │ ─segment─→
    // y  ↓ halfwidth0
    //
    void setHalfwidths(double halfwidth0, double halfwidth1) {
        sample_.setHalfwidths(halfwidth0, halfwidth1);
        updateOffsetPoints_();
    }

    // ┌─── x
    // │  ↑ side 1
    // │ ─segment─→
    // y  ↓ side 0
    //
    void setHalfwidth(Int side, double halfwidth) {
        sample_.setHalfwidth(side, halfwidth);
        updateOffsetPoints_();
    }

    void swapHalfwidths() {
        sample_.swapHalfwidths();
        updateOffsetPoints_();
    }

    // ┌─── x
    // │  ↑ offsetPoints[1]
    // │ ─segment─→
    // y  ↓ offsetPoints[0]
    //
    const std::array<Vec2d, 2>& offsetPoints() const {
        return offsetPoints_;
    }

    // ┌─── x
    // │  ↑ side 1
    // │ ─segment─→
    // y  ↓ side 0
    //
    Vec2d offsetPoint(Int side) const {
        return offsetPoints_[side];
    }

    double s() const {
        return sample_.s();
    }

    void setS(double s) {
        sample_.setS(s);
    }

    bool isCornerStart() const {
        return sample_.isCornerStart();
    }

    void setCornerStart(bool isCornerStart) {
        sample_.setCornerStart(isCornerStart);
    }

    Int segmentIndex() const {
        return segmentIndex_;
    }

    void setSegmentIndex(Int segmentIndex) {
        segmentIndex_ = segmentIndex;
    }

    double u() const {
        return u_;
    }

    void setU(double u) {
        u_ = u;
    }

private:
    StrokeSample2d sample_;
    std::array<Vec2d, 2> offsetPoints_;
    double speed_;
    Int segmentIndex_;
    double u_; // parameter in stroke segment.

    void updateOffsetPoints_() {
        offsetPoints_ = sample_.offsetPoints();
    }
};

/// Alias for `vgc::core::Array<vgc::geometry::StrokeSampleEx2d>`.
///
using StrokeSampleEx2dArray = core::Array<StrokeSampleEx2d>;

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
DistanceToCurve
distanceToCurve(const StrokeSample2dArray& samples, const Vec2d& position);

VGC_GEOMETRY_API
DistanceToCurve
distanceToCurve(const StrokeSampleEx2dArray& samples, const Vec2d& position);

enum class CurveSamplingQuality {
    Disabled,
    UniformVeryLow,
    AdaptiveLow,
    UniformHigh,
    AdaptiveHigh,
    UniformVeryHigh,
    Max_ = UniformVeryHigh
};

VGC_GEOMETRY_API
VGC_DECLARE_ENUM(CurveSamplingQuality)

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
        , cosMaxAngle_(std::cos(maxAngle))
        , minIntraSegmentSamples_(minIntraSegmentSamples)
        , maxIntraSegmentSamples_(maxIntraSegmentSamples) {

        cosMaxAngle_ = std::cos(maxAngle);
    }

    double maxDs() const {
        return maxDs_;
    }

    void setMaxDs(double maxDs) {
        maxDs_ = maxDs;
    }

    double maxAngle() const {
        return maxAngle_;
    }

    void setMaxAngle(double maxAngle) {
        maxAngle_ = maxAngle;
        cosMaxAngle_ = std::cos(maxAngle);
    }

    double cosMaxAngle() const {
        return cosMaxAngle_;
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
    double maxDs_ = core::DoubleInfinity;
    double maxAngle_ = 0.05; // 2PI / 0.05 ~= 125.66
    double cosMaxAngle_;
    Int minIntraSegmentSamples_ = 0;
    Int maxIntraSegmentSamples_ = 63;
    bool isScreenspace_ = false;
};

/// \class vgc::geometry::WidthProfile
/// \brief A widths profile to apply on curves.
///
class VGC_GEOMETRY_API WidthProfile {
public:
    WidthProfile() = default;

    // XXX todo

private:
    core::Array<Vec2d> values_;
};

namespace detail {

// TODO: We may want to have a lean version of AbstractStroke2d dedicated to
//       curves only (without thickness nor varying attributes). This class
//       is a draft of such interface.
//
/// \class vgc::geometry::AbstractCurve2d
/// \brief An abstract model of 2D curve (no thickness or other attributes).
///
class VGC_GEOMETRY_API AbstractCurve2d {
public:
    virtual ~AbstractCurve2d() = default;

    /// Returns whether the curve is closed.
    ///
    virtual bool isClosed() const = 0;

    /// Returns the number of segments of the curve.
    ///
    /// \sa `eval()`.
    ///
    virtual Int numSegments() const = 0;

    /// Returns the position of the curve point from segment `segmentIndex` at
    /// parameter `u`.
    ///
    virtual Vec2d eval(Int segmentIndex, double u) const = 0;

    /// Returns the position of the curve point from segment `segmentIndex` at
    /// parameter `u`. It additionally sets the value of `velocity` as the
    /// position derivative at `u` with respect to the parameter u.
    ///
    virtual Vec2d eval(Int segmentIndex, double u, Vec2d& velocity) const = 0;
};

} // namespace detail

/// \class vgc::geometry::AbstractStroke2d
/// \brief An abstract model of 2D stroke.
///
class VGC_GEOMETRY_API AbstractStroke2d {
public:
    AbstractStroke2d(bool isClosed)
        : isClosed_(isClosed) {
    }

    virtual ~AbstractStroke2d() = default;

    /// Returns whether the stroke is closed.
    ///
    bool isClosed() const {
        return isClosed_;
    }

    /// Returns the number of knots of the stroke.
    ///
    Int numKnots() const {
        return numKnots_();
    }

    /// Returns the number of segments of the stroke.
    ///
    /// \sa `eval()`.
    ///
    Int numSegments() const {
        Int n = numKnots_();
        return (isClosed_ || n == 0) ? n : n - 1;
    }

    /// Returns whether the stroke segment at `segmentIndex` has a length of 0.
    ///
    /// Throws `IndexError` if `segmentIndex` is not in the range
    /// `[0, numSegments() - 1]`.
    ///
    bool isZeroLengthSegment(Int segmentIndex) const {
        return isZeroLengthSegment_(segmentIndex);
    }

    /// Returns the position of the centerline point from segment `segmentIndex` at
    /// parameter `u`.
    ///
    /// Throws `IndexError` if `segmentIndex` is not in the range
    /// `[0, numSegments() - 1]`.
    ///
    Vec2d evalCenterline(Int segmentIndex, double u) const;

    /// Returns the position of the centerline point from segment `segmentIndex` at
    /// parameter `u`. It additionally sets the value of `velocity` as the
    /// position derivative at `u` with respect to the parameter u.
    ///
    /// Throws `IndexError` if `segmentIndex` is not in the range
    /// `[0, numSegments() - 1]`.
    ///
    Vec2d evalCenterline(Int segmentIndex, double u, Vec2d& velocity) const;

    /// Returns a `StrokeSample` from the segment `segmentIndex` at
    /// parameter `u`. The attribute `s` of the sample is left to 0.
    ///
    /// Throws `IndexError` if `segmentIndex` is not in the range
    /// `[0, numSegments() - 1]`.
    ///
    StrokeSampleEx2d eval(Int segmentIndex, double u) const;

    // TODO: add variants of sampleSegment() and sampleRange() for CurveSample2d ?

    /// Computes a sampling of the segment at `segmentIndex` in this stroke.
    ///
    /// Throws `IndexError` if `segmentIndex` is not in the range
    /// `[0, numSegments() - 1]`.
    ///
    void sampleSegment(
        StrokeSampleEx2dArray& out,
        Int segmentIndex,
        const CurveSamplingParameters& params) const;

    /// Computes a sampling of the subset of this curve consisting of
    /// `numSegments` segments starting at the knot at index `startKnot`.
    ///
    /// \verbatim
    /// INPUT
    /// -----
    /// startKnot   = 1
    /// numSegments = 2
    /// knots       = 0------1-----------2---------3---------4--------5
    ///                      |                     |
    ///                      |                     |
    ///                      |                     |
    ///                      |                     |
    /// OUTPUT               |                     |
    /// ------               v                     v
    /// samples     =        x-x-x-x-x-x-x-x-x-x-x-x
    /// \endverbatim
    ///
    /// The result is appended to the output parameter `out`.
    ///
    /// The value of `startKnot` must be in the range [-m, m-1] with `m =
    /// numKnots()`. Negative values can be used for indexing from the end:
    /// `-1` represents the last knot, and `-m` represents the first knot.
    ///
    /// The value of `numSegments` must be in the range [-n-1, n] with `n =
    /// numSegments()`. Negative values can be used for specifying "all except
    /// k segments": `-1` represents all segments, and `-n-1` represents zero
    /// segments.
    ///
    /// This function throws `IndexError` if:
    /// - the curve is empty (`numKnots() == 0`), or
    /// - `startKnot` is not in the range [-m, m-1], or
    /// - `numSegments` is not in the range [-n-1, n], or
    /// - the curve is open and the requested number of segments (after wrapping
    /// negative values) is larger than the remaining number of segments when
    /// starting at `startKnot`. For example, if the curve has 4 knots and
    /// `startKnot == 1`, then the maximum value for `numSegments` is 2
    /// (segments from knot index 1 to knot index 3 which is the last knot).
    ///
    /// The start and end samples of the range are both included. This means
    /// that if this function does not throw, it is guaranteed to return a
    /// non-empty sampling (i.e., with at least one sample), even when
    /// the given `numSegments` is equal to zero.
    ///
    /// This also means that calling `sampleRange(out, params, 0, 1)` followed
    /// by `sampleRange(out, params, 1, 1)` would result in having two times
    /// the sample corresponding to knot index `1`. If you wish to do such chaining
    /// meaningfully, you have to manually discard the last point:
    ///
    /// ```cpp
    /// sampleRange(out, params, 0, 1);
    /// out.removeLast();
    /// sampleRange(out, params, 1, 1);
    /// ```
    ///
    /// If `withArclengths = true` (the default), then arclengths are computed
    /// starting from `s = 0` (if `out` is initially empty) or `s =
    /// out.last().s()` (if `out` is not initially empty).
    ///
    /// If `withArclengths = false` (the default), then all arclengths of the
    /// computed samples are left uninitialized.
    ///
    /// If the curve is open and `numKnot() == 1`, this function returns a
    /// unique sample with a normal set to zero.
    ///
    void sampleRange(
        StrokeSampleEx2dArray& out,
        const CurveSamplingParameters& params,
        Int startKnotIndex = 0,
        Int numSegments = -1,
        bool computeArcLengths = true) const;

    /// Returns the normalized tangents of the two offset lines at the given
    /// segment endpoint, given by its segment index and endpoint index (0 for
    /// the start of the segment, and 1 for the end of the segment).
    ///
    /// Note that the tangents just before and just after a knot are not
    /// necessarily equal in case of "corner" knots. Therefore,
    /// `computeOffsetLineTangentsAtSegmentEndpoint(i - 1, 1)` and
    /// `computeOffsetLineTangentsAtSegmentEndpoint(i, 0)` may not be equal.
    ///
    /// Throws `IndexError` if the given `segmentIndex` is not in the range
    /// `[0, numSegments() - 1]`.
    ///
    /// Throws `IndexError` if the given `endpointIndex` is neither `0` or `1`.
    ///
    std::array<Vec2d, 2>
    computeOffsetLineTangentsAtSegmentEndpoint(Int segmentIndex, Int endpointIndex) const;

protected:
    virtual Int numKnots_() const = 0;

    virtual bool isZeroLengthSegment_(Int segmentIndex) const = 0;

    virtual Vec2d evalNonZeroCenterline(Int segmentIndex, double u) const = 0;

    virtual Vec2d evalNonZeroCenterline(Int segmentIndex, double u, Vec2d& dp) const = 0;

    virtual StrokeSampleEx2d evalNonZero(Int segmentIndex, double u) const = 0;

    virtual void sampleNonZeroSegment(
        StrokeSampleEx2dArray& out,
        Int segmentIndex,
        const CurveSamplingParameters& params) const = 0;

    // Handle cases where:
    // - open curve with numKnots == 1: there are no segments at all in the curve
    // - closed curve with numKnots == 1: there is one segment but whose
    //   start knot is equal to its end knot
    // - There is more than 1 knot but they are all equal.
    //
    // Note that this is different from `numSegmentsToSample == 0` with at
    // least one non-corner segment in the curve, in which case we still
    // need to evaluate one of the non-corner segments in order to provide
    // a meaningful normal.
    //
    virtual StrokeSampleEx2d zeroLengthStrokeSample() const = 0;

    virtual std::array<Vec2d, 2> computeOffsetLineTangentsAtSegmentEndpoint_(
        Int segmentIndex,
        Int endpointIndex) const = 0;

private:
    const bool isClosed_;

    StrokeSampleEx2d sampleKnot_(Int knotIndex) const;
    bool fixEvalLocation_(Int& segmentIndex, double& u) const;
};

/// Specifies the type of the curve, that is, how the
/// position of its centerline is represented.
///
enum class CurveType {

    /// Represents an open uniform Catmull-Rom spline.
    ///
    /// With this curve type, we have `numSegment() == numKnots() - 1`.
    ///
    /// Each position p in `positions()` represent a control points (= knot
    /// in this case) of the spline. The curve starts at the first control
    /// point, ends at the last control point, and go through all control
    /// points.
    ///
    /// Each curve segment between two control points p[i] and p[i+1] is a
    /// cubic curve P_i(u) parameterized by u in [0, 1]. The derivative
    /// P_i'(u) at each control point (except end points) is automatically
    /// determined from its two adjacent control points:
    ///
    /// P_{i-1}'(1) = P_i'(0) = (p[i+1] - p[i-1]) / 2
    ///
    /// At end control points, we use a tangent mirrored from the adjacent control
    /// point, see: https://github.com/vgc/vgc/pull/1341.
    ///
    /// In addition, tangents are capped based on the distance between control points
    /// to avoid loops.
    ///
    /// Note: "uniform" refers here to the fact that this corresponds to a
    /// Catmull-Rom spline with knot values uniformly spaced, that is: [0,
    /// 1, 2, ..., n-1]. There exist other types of Catmull-Rom splines
    /// using different knot values, such as the Cardinal or Chordal
    /// Catmull-Rom, that uses the distance between control points to
    /// determine more suitable knot values to avoid loops, see:
    ///
    /// https://en.wikipedia.org/wiki/Centripetal_Catmull%E2%80%93Rom_spline
    ///
    OpenUniformCatmullRom,

    /// Represents an open centripetal Catmull-Rom spline.
    ///
    /// Similar to `OpenUniformCatmullRom` but using centripetal
    /// parametrization. This prevents cusps and loops.
    ///
    OpenCentripetalCatmullRom,

    /// Represents a closed uniform Catmull-Rom spline.
    ///
    /// This is similar to `OpenUniformCatmullRom` except that it forms a loop.
    ///
    /// With this curve type, we have `numSegment() == numKnots()`: the last segment
    /// goes from the knot at index `numKnots() - 1` (the last knot) back to the knot
    /// at index `0` (the first knot).
    ///
    /// Unlike `OpenUniformCatmullRom`, there is no special handling of tangents
    /// for the first/last segment, since all knots have adjacent knots. In particular,
    /// the tangent at the first/last control point is determined by:
    ///
    /// P' = (p[1] - p[n-1]) / 2
    ///
    /// where n = numKnots().
    ///
    ClosedUniformCatmullRom,

    /// Represents a closed centripetal Catmull-Rom spline.
    ///
    /// Similar to `ClosedUniformCatmullRom` but using centripetal
    /// parametrization. This prevents cusps and loops.
    ///
    ClosedCentripetalCatmullRom
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

namespace detail {

void checkSegmentIndex(Int segmentIndex, Int numSegments);

using AdaptiveSamplingParameters = CurveSamplingParameters;

template<typename TSample>
class AdaptiveSampler {
private:
    struct Node {
    public:
        Node() = default;

        TSample sample;
        double u;
        Node* previous = nullptr;
        Node* next = nullptr;
    };

public:
    // Samples the segment [data.segmentIndex, data.segmentIndex + 1], and appends the
    // result to outAppend.
    //
    // KeepPredicate signature must match:
    //   `bool(const TSample& previousSample,
    //         const TSample& sample,
    //         const TSample& nextSample)`
    //
    // The first sample of the segment is appended only if the cache `data` is new.
    // The last sample is always appended.
    //
    template<typename USample, typename Evaluator, typename KeepPredicate>
    void sample(
        Evaluator evaluator,
        KeepPredicate keepPredicate,
        const AdaptiveSamplingParameters& params,
        core::Array<USample>& outAppend) {

        const Int minISS = params.minIntraSegmentSamples(); // 0 -> 2 samples minimum
        const Int maxISS = params.maxIntraSegmentSamples(); // 1 -> 3 samples maximum
        const Int minSamples = std::max<Int>(0, minISS) + 2;
        const Int maxSamples = std::max<Int>(minSamples, maxISS + 2);

        resetSampleTree_(maxSamples);

        // Setup first and last sample nodes of segment.
        Node* s0 = &sampleTree_[0];
        s0->sample = evaluator(0);
        s0->u = 0;
        Node* sN = &sampleTree_[1];
        sN->sample = evaluator(1);
        sN->u = 1;
        s0->previous = nullptr;
        s0->next = sN;
        sN->previous = s0;
        sN->next = nullptr;

        Int nextNodeIndex = 2;

        // Compute `minIntraSegmentSamples` uniform samples.
        Node* previousNode = s0;
        for (Int i = 0; i < minISS; ++i) {
            Node* node = &sampleTree_[nextNodeIndex];
            double u = static_cast<double>(i + 1) / (minISS + 1);
            node->sample = evaluator(u);
            node->u = u;
            ++nextNodeIndex;
            linkNode_(node, previousNode);
            previousNode = node;
        }

        const Int sampleTreeLength = sampleTree_.length();
        Int previousLevelStartIndex = 2;
        Int previousLevelEndIndex = nextNodeIndex;

        // Fallback to using the last sample as previous level sample
        // when we added no uniform samples.
        if (previousLevelEndIndex == 2) {
            previousLevelStartIndex = 1;
        }

        while (nextNodeIndex < sampleTreeLength) {
            // Since we create a candidate on the left and right of each previous level node,
            // each pass can add as much as twice the amount of nodes of the previous level.
            for (Int i = previousLevelStartIndex; i < previousLevelEndIndex; ++i) {
                Node* previousLevelNode = &sampleTree_[i];
                // Try subdivide left.
                if (trySubdivide_(
                        evaluator,
                        keepPredicate,
                        nextNodeIndex,
                        previousLevelNode->previous,
                        previousLevelNode)
                    && nextNodeIndex == sampleTreeLength) {
                    break;
                }
                // We subdivide right only if it is not the last point.
                if (!previousLevelNode->next) {
                    continue;
                }
                // Try subdivide right.
                if (trySubdivide_(
                        evaluator,
                        keepPredicate,
                        nextNodeIndex,
                        previousLevelNode,
                        previousLevelNode->next)
                    && nextNodeIndex == sampleTreeLength) {
                    break;
                }
            }
            if (nextNodeIndex == previousLevelEndIndex) {
                // No new candidate, let's stop here.
                break;
            }
            previousLevelStartIndex = previousLevelEndIndex;
            previousLevelEndIndex = nextNodeIndex;
        }

        Node* node = &sampleTree_[0];
        while (node) {
            outAppend.emplaceLast(node->sample);
            node = node->next;
        }
    }

private:
    core::Span<Node> sampleTree_;
    std::unique_ptr<Node[]> sampleTreeStorage_;

    void resetSampleTree_(Int newStorageLength) {
        if (newStorageLength > sampleTree_.length()) {
            sampleTreeStorage_ = std::make_unique<Node[]>(newStorageLength);
            sampleTree_ = core::Span<Node>(sampleTreeStorage_.get(), newStorageLength);
        }
    }

    template<typename Evaluator, typename KeepPredicate>
    bool trySubdivide_(
        Evaluator evaluator,
        KeepPredicate keepPredicate,
        Int& nodeIndex,
        Node* n0,
        Node* n1) {

        Node* node = &sampleTree_[nodeIndex];
        double u = 0.5 * (n0->u + n1->u);
        node->sample = evaluator(u);
        if (keepPredicate(n0->sample, node->sample, n1->sample)) {
            ++nodeIndex;
            node->u = u;
            linkNode_(node, n0);
            return true;
        }
        return false;
    };

    static void linkNode_(Node* node, Node* previous) {
        Node* next = previous->next;
        next->previous = node;
        previous->next = node;
        node->previous = previous;
        node->next = next;
    };
};

bool isCenterlineSegmentUnderTolerance(
    const StrokeSampleEx2d& s0,
    const StrokeSampleEx2d& s1,
    double cosMaxAngle);

bool areOffsetLinesAnglesUnderTolerance(
    const StrokeSampleEx2d& s0,
    const StrokeSampleEx2d& s1,
    const StrokeSampleEx2d& s2,
    double cosMaxAngle);

bool shouldKeepNewSample(
    const StrokeSampleEx2d& previousSample,
    const StrokeSampleEx2d& sample,
    const StrokeSampleEx2d& nextSample,
    const CurveSamplingParameters& params);

class AdaptiveStrokeSampler : public AdaptiveSampler<StrokeSampleEx2d> {
public:
    template<typename USample, typename Evaluator>
    void sample(
        Evaluator&& evaluator,
        const AdaptiveSamplingParameters& params,
        core::Array<USample>& out) {

        AdaptiveSampler<StrokeSampleEx2d>::sample(
            std::forward<Evaluator>(evaluator),
            [&params](
                const StrokeSampleEx2d& previousSample,
                const StrokeSampleEx2d& sample,
                const StrokeSampleEx2d& nextSample) {
                return shouldKeepNewSample(previousSample, sample, nextSample, params);
            },
            params,
            out);
    }
};

} // namespace detail

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_CURVE_H
