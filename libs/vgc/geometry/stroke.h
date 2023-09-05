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

#ifndef VGC_GEOMETRY_STROKE_H
#define VGC_GEOMETRY_STROKE_H

#include <algorithm> // std::swap
#include <array>

#include <vgc/core/arithmetic.h>
#include <vgc/core/array.h>
#include <vgc/core/span.h>
#include <vgc/core/stringid.h>
#include <vgc/geometry/api.h>
#include <vgc/geometry/curve.h>
#include <vgc/geometry/mat3d.h>
#include <vgc/geometry/rect2d.h>
#include <vgc/geometry/vec2d.h>

namespace vgc::geometry {

/// Note: normal and tangent are not necessarily orthogonal, for instance
/// when using relaxed normals.
///
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
        , normal_(core::noInit)
        , halfwidths_(core::noInit) {
    }
    VGC_WARNING_POP

    StrokeSample2d(
        const Vec2d& position,
        const Vec2d& tangent,
        const Vec2d& normal,
        const Vec2d& halfwidths,
        double s = 0) noexcept

        : position_(position)
        , tangent_(tangent)
        , normal_(normal)
        , halfwidths_(halfwidths)
        , s_(s) {
    }

    StrokeSample2d(
        const Vec2d& position,
        const Vec2d& tangent,
        const Vec2d& normal,
        double halfwidth,
        double s = 0) noexcept

        : StrokeSample2d(position, tangent, normal, Vec2d(halfwidth, halfwidth), s) {
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

    void reverseDirection() {
        tangent_ = -tangent_;
        normal_ = -normal_;
    }

    // ┌─── x
    // │ ─segment─→
    // y  ↓ normal
    //
    Vec2d normal() const {
        return normal_;
    }

    void setNormal(const Vec2d& normal) {
        normal_ = normal;
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

    void offsetS(double offset) {
        s_ += offset;
    }

    bool isCornerStart() const {
        return isCornerStart_;
    }

    void setCornerStart(bool isCornerStart) {
        isCornerStart_ = isCornerStart;
    }

private:
    Vec2d position_ = {};
    Vec2d tangent_ = {};
    Vec2d normal_ = {};
    Vec2d halfwidths_ = {};
    double s_ = 0; // arclength from stroke start point.
    // isCornerStart_ is true only for the first sample of the two that makes
    // a corner (hard turn).
    // TODO: add enum/flags for corner kind ? knot corner, centerline cusp, offsetline cusp..
    bool isCornerStart_ = false;
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
        a.normal() * ot + b.normal() * t,
        a.halfwidths() * ot + b.halfwidths() * t,
        a.s() * ot + b.s() * t);
    return result;
}

/// Returns a new sample with each attribute linearly interpolated except
/// the normal and the tangent that also re-normalized.
///
/// Use `lerp()` if you don't need the re-normalization.
///
inline StrokeSample2d nlerp(const StrokeSample2d& a, const StrokeSample2d& b, double t) {
    StrokeSample2d result = lerp(a, b, t);
    result.setTangent(result.tangent().normalized());
    result.setNormal(result.normal().normalized());
    return result;
}

/// Alias for `vgc::core::Array<vgc::geometry::StrokeSample2d>`.
///
using StrokeSample2dArray = core::Array<StrokeSample2d>;

/// Alias for `vgc::core::SharedConstArray<vgc::geometry::StrokeSample2d>`.
///
using SharedConstStrokeSample2dArray = core::SharedConstArray<StrokeSample2d>;

/// Note: normal and tangent are not necessarily orthogonal, for instance
/// when using relaxed normals.
///
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
        const Vec2d& normal,
        const Vec2d& halfwidths,
        double speed,
        Int segmentIndex = -1,
        double u = 0) noexcept

        : sample_(position, tangent, normal, halfwidths)
        , speed_(speed)
        , segmentIndex_(segmentIndex)
        , u_(u) {

        updateOffsetPoints_();
    }

    StrokeSampleEx2d(
        const Vec2d& position,
        const Vec2d& tangent,
        const Vec2d& normal,
        double halfwidth,
        double speed,
        Int segmentIndex = -1,
        double u = 0) noexcept

        : StrokeSampleEx2d(
            position,
            tangent,
            normal,
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
        sample_.reverseDirection();
        std::swap(offsetPoints_[0], offsetPoints_[1]);
    }

    // ┌─── x
    // │ ─segment─→
    // y  ↓ normal
    //
    Vec2d normal() const {
        return sample_.normal();
    }

    void setNormal(const Vec2d& normal) {
        sample_.setNormal(normal);
        updateOffsetPoints_();
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

    void offsetS(double offset) {
        sample_.offsetS(offset);
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

VGC_GEOMETRY_API
DistanceToCurve
distanceToCurve(const StrokeSample2dArray& samples, const Vec2d& position);

VGC_GEOMETRY_API
DistanceToCurve
distanceToCurve(const StrokeSampleEx2dArray& samples, const Vec2d& position);

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

class VGC_GEOMETRY_API StrokeEndInfo {
public:
    StrokeEndInfo() noexcept = default;

    StrokeEndInfo(const Vec2d& position, const Vec2d& tangent, const Vec2d& halfwidths)
        : position_(position)
        , tangent_(tangent)
        , halfwidths_(halfwidths) {
    }

    Vec2d position() const {
        return position_;
    }

    void setPosition(Vec2d position) {
        position_ = position;
    }

    Vec2d tangent() const {
        return tangent_;
    }

    void setTangent(Vec2d tangent) {
        tangent_ = tangent;
    }

    Vec2d halfwidths() const {
        return halfwidths_;
    }

    void setHalfwidths(Vec2d halfwidths) {
        halfwidths_ = halfwidths;
    }

    const std::array<Vec2d, 2>& offsetLineTangents() const {
        return offsetLineTangents_;
    }

    template<size_t Side, VGC_REQUIRES(Side == 0 || Side == 1)>
    const Vec2d& getOffsetLineTangent() const {
        return offsetLineTangents_[Side];
    }

    void setOffsetLineTangents(const std::array<Vec2d, 2>& offsetLineTangents) {
        offsetLineTangents_[0] = offsetLineTangents[0];
        offsetLineTangents_[1] = offsetLineTangents[1];
    }

private:
    Vec2d position_;
    Vec2d tangent_;
    Vec2d halfwidths_;
    std::array<Vec2d, 2> offsetLineTangents_;
};

using StrokeBoundaryInfo = std::array<StrokeEndInfo, 2>;

/// \class vgc::geometry::StrokeSampling2d
/// \brief Sampling of a 2d stroke.
///
class VGC_GEOMETRY_API StrokeSampling2d {
public:
    StrokeSampling2d() noexcept = default;

    explicit StrokeSampling2d(const StrokeSample2dArray& samples)
        : samples_(samples) {

        computeCenterlineBoundingBox();
    }

    explicit StrokeSampling2d(StrokeSample2dArray&& samples)
        : samples_(std::move(samples)) {

        computeCenterlineBoundingBox();
    }

    const StrokeSample2dArray& samples() const {
        return samples_;
    }

    StrokeSample2dArray stealSamples() {
        return std::move(samples_);
    }

    const Rect2d& centerlineBoundingBox() const {
        return centerlineBoundingBox_;
    }

    const StrokeBoundaryInfo& boundaryInfo() const {
        return boundaryInfo_;
    }

    void setBoundaryInfo(const StrokeBoundaryInfo& boundaryInfo) {
        boundaryInfo_ = boundaryInfo;
    }

private:
    StrokeSample2dArray samples_ = {};
    StrokeBoundaryInfo boundaryInfo_;
    Rect2d centerlineBoundingBox_ = Rect2d::empty;

    void computeCenterlineBoundingBox() {
        centerlineBoundingBox_ = Rect2d::empty;
        for (const StrokeSample2d& cs : samples_) {
            centerlineBoundingBox_.uniteWith(cs.position());
        }
    }
};

namespace detail {

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

/// \class vgc::geometry::StrokeModelInfo
/// \brief Describes a model of 2D stroke.
///
class VGC_GEOMETRY_API StrokeModelInfo {
public:
    StrokeModelInfo(core::StringId name, Int defaultConversionRank)
        : name_(name)
        , defaultConversionRank_(defaultConversionRank) {
    }

    /// Returns the name of the model (concrete implementation of AbstractStroke2d).
    ///
    core::StringId name() const {
        return name_;
    }

    /// Returns the default conversion rank of this stroke model.
    ///
    Int defaultConversionRank() const {
        return defaultConversionRank_;
    }

private:
    core::StringId name_;
    Int defaultConversionRank_;
    // TODO: add other flags such as supported operations.
};

/// \class vgc::geometry::AbstractStroke2d
/// \brief An abstract model of 2D stroke.
///
class VGC_GEOMETRY_API AbstractStroke2d {
protected:
    AbstractStroke2d(bool isClosed)
        : isClosed_(isClosed) {
    }

public:
    virtual ~AbstractStroke2d() = default;

    const StrokeModelInfo& modelInfo() const {
        return modelInfo_();
    }

    std::unique_ptr<AbstractStroke2d> cloneEmpty() const {
        return cloneEmpty_();
    }

    std::unique_ptr<AbstractStroke2d> clone() const {
        return clone_();
    }

    std::unique_ptr<AbstractStroke2d> convert(const AbstractStroke2d* source) const {
        return convert_(source);
    }

    bool copyAssign(const AbstractStroke2d* other) {
        return copyAssign_(other);
    }

    bool moveAssign(AbstractStroke2d* other) {
        return moveAssign_(other);
    }

    /// Returns whether the stroke is closed.
    ///
    bool isClosed() const {
        return isClosed_;
    }

    double approximateLength() const {
        return approximateLength_();
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

    /// Returns the centerline end positions of the stroke.
    ///
    std::array<Vec2d, 2> endPositions() const {
        return endPositions_();
    }

    /// Returns geometric information for both ends of the stroke.
    ///
    StrokeBoundaryInfo computeBoundaryInfo() const {
        return computeBoundaryInfo_();
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

    /// Variant of sampleSegment() accepting a sampler to reuse its storage.
    ///
    void sampleSegment(
        StrokeSampleEx2dArray& out,
        Int segmentIndex,
        const CurveSamplingParameters& params,
        detail::AdaptiveStrokeSampler& sampler) const;

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

    // TODO: We will later need a variant of computeSampling() that accepts a target
    // view matrix.
    // Ideally, for inbetweening we would like a sampling that is good in 2 spaces:
    // - the common ancestor group space for best morphing.
    // - the canvas space for best rendering.

    /// Expects positions in object space.
    ///
    StrokeSampling2d computeSampling(const CurveSamplingParameters& params) const;

    /// Expects delta in object space.
    ///
    void translate(const Vec2d& delta) {
        translate_(delta);
    }

    /// Expects transformation in object space.
    ///
    void transform(const Mat3d& transformation) {
        transform_(transformation);
    }

    void reverse() {
        reverse_();
    }

    void prepend(const AbstractStroke2d* other, bool direction, bool smoothJoin) {
        assignFromConcat_(other, direction, this, true, smoothJoin);
    }

    void append(const AbstractStroke2d* other, bool direction, bool smoothJoin) {
        assignFromConcat_(this, true, other, direction, smoothJoin);
    }

    void assignFromConcat(
        const AbstractStroke2d* a,
        bool directionA,
        const AbstractStroke2d* b,
        bool directionB,
        bool smoothJoin) {

        assignFromConcat_(a, directionA, b, directionB, smoothJoin);
    }

    /// Assigns to `this` the average of the `strokes`.
    /// Prior to averaging and for each stroke:
    /// - parameterization is reversed according to its given direction.
    ///
    void assignFromAverageOpen(
        core::ConstSpan<const AbstractStroke2d*> strokes,
        core::ConstSpan<bool> directions) {

        assignFromAverage_(strokes, directions, core::ConstSpan<double>(), false);
    }

    /// Assigns to `this` the average of the `strokes`.
    /// Prior to averaging and for each closed stroke in order:
    /// - parameterization is reversed according to its given direction.
    /// - parameterization [0, 1] is rotated by the given offset.
    ///
    void assignFromAverageClosed(
        core::ConstSpan<const AbstractStroke2d*> strokes,
        core::ConstSpan<bool> directions,
        core::ConstSpan<double> uOffsets) {

        assignFromAverage_(strokes, directions, uOffsets, true);
    }

    /// Expects positions in object space.
    /// Returns whether a snap actually happened.
    ///
    bool snap(
        const Vec2d& snapStartPosition,
        const Vec2d& snapEndPosition,
        CurveSnapTransformationMode mode =
            CurveSnapTransformationMode::LinearInArclength) {

        return snap_(snapStartPosition, snapEndPosition, mode);
    }

    bool isSnapped(const Vec2d& startPosition, const Vec2d& endPosition) const {
        std::array<Vec2d, 2> endPositions = this->endPositions();
        return endPositions[0] == startPosition && endPositions[1] == endPosition;
    }

    /// Returns the new position of the grabbed point (center of deformation falloff).
    ///
    // TODO: choose properly between tolerance/samplingDelta/quality.
    // TODO: later add falloff kind, arclength/spatial, keep vertices.
    //
    Vec2d sculptGrab(
        const Vec2d& startPosition,
        const Vec2d& endPosition,
        double radius,
        double strength,
        double tolerance,
        bool isClosed = false) {

        return sculptGrab_(
            startPosition, endPosition, radius, strength, tolerance, isClosed);
    }

    /// Returns the position of the grabbed point (center of deformation falloff).
    ///
    // TODO: choose properly between tolerance/samplingDelta/quality.
    // TODO: later add falloff kind, arclength/spatial, keep vertices.
    //
    Vec2d sculptWidth(
        const Vec2d& position,
        double delta,
        double radius,
        double tolerance,
        bool isClosed = false) {

        return sculptWidth_(position, delta, radius, tolerance, isClosed);
    }

    /// Returns the new position of the smooth point.
    ///
    // TODO: later add falloff kind, arclength/spatial.
    //
    Vec2d sculptSmooth(
        const Vec2d& position,
        double radius,
        double strength,
        double tolerance,
        bool isClosed = false) {

        return sculptSmooth_(position, radius, strength, tolerance, isClosed);
    }

protected:
    virtual Vec2d evalNonZeroCenterline(Int segmentIndex, double u) const = 0;

    virtual Vec2d evalNonZeroCenterline(Int segmentIndex, double u, Vec2d& dp) const = 0;

    virtual StrokeSampleEx2d evalNonZero(Int segmentIndex, double u) const = 0;

    virtual void sampleNonZeroSegment(
        StrokeSampleEx2dArray& out,
        Int segmentIndex,
        const CurveSamplingParameters& params,
        detail::AdaptiveStrokeSampler& sampler) const = 0;

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

protected:
    virtual const StrokeModelInfo& modelInfo_() const = 0;

    virtual std::unique_ptr<AbstractStroke2d> cloneEmpty_() const = 0;
    virtual std::unique_ptr<AbstractStroke2d> clone_() const = 0;
    virtual std::unique_ptr<AbstractStroke2d>
    convert_(const AbstractStroke2d* source) const;

    virtual bool copyAssign_(const AbstractStroke2d* other) = 0;
    virtual bool moveAssign_(AbstractStroke2d* other) = 0;
    virtual bool convertAssign_(const AbstractStroke2d* other) = 0;

    virtual double approximateLength_() const = 0;

    virtual Int numKnots_() const = 0;

    virtual bool isZeroLengthSegment_(Int segmentIndex) const = 0;

    virtual std::array<Vec2d, 2> endPositions_() const = 0;

    virtual StrokeBoundaryInfo computeBoundaryInfo_() const = 0;

    virtual void translate_(const Vec2d& delta) = 0;

    virtual void transform_(const Mat3d& transformation) = 0;

    virtual void reverse_() = 0;

    virtual void assignFromConcat_(
        const AbstractStroke2d* a,
        bool directionA,
        const AbstractStroke2d* b,
        bool directionB,
        bool smoothJoin) = 0;

    virtual void assignFromAverage_(
        core::ConstSpan<const AbstractStroke2d*> strokes,
        core::ConstSpan<bool> directions,
        core::ConstSpan<double> uOffsets,
        bool areClosed) = 0;

    virtual bool snap_(
        const Vec2d& snapStartPosition,
        const Vec2d& snapEndPosition,
        CurveSnapTransformationMode mode) = 0;

    virtual Vec2d sculptGrab_(
        const Vec2d& startPosition,
        const Vec2d& endPosition,
        double radius,
        double strength,
        double tolerance,
        bool isClosed = false) = 0;

    virtual Vec2d sculptWidth_(
        const Vec2d& position,
        double delta,
        double radius,
        double tolerance,
        bool isClosed = false) = 0;

    virtual Vec2d sculptSmooth_(
        const Vec2d& position,
        double radius,
        double strength,
        double tolerance,
        bool isClosed = false) = 0;

private:
    bool isClosed_;

    StrokeSampleEx2d sampleKnot_(Int knotIndex) const;

    bool fixEvalLocation_(Int& segmentIndex, double& u) const;
};

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_STROKE_H
