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

#ifndef VGC_TOOLS_SKETCHPASSES_H
#define VGC_TOOLS_SKETCHPASSES_H

#include <vgc/geometry/bezier.h>
#include <vgc/tools/api.h>
#include <vgc/tools/sketchpass.h>

namespace vgc::tools {

/// \class vgc::tools::EmptyPass
/// \brief A sketch pass that does nothing: the output becomes equal to the input.
///
class VGC_TOOLS_API EmptyPass : public SketchPass {
protected:
    void doUpdateFrom(const SketchPointBuffer& input, SketchPointBuffer& output) override;
};

/// \class vgc::tools::TransformPass
/// \brief A sketch pass that applies its transformMatrix() to all points.
///
class VGC_TOOLS_API TransformPass : public SketchPass {
protected:
    void doUpdateFrom(const SketchPointBuffer& input, SketchPointBuffer& output) override;
};

/// \class vgc::tools::RemoveDuplicatesSettings
/// \brief Settings for the RemoveDuplicatesPass sketch pass
///
class VGC_TOOLS_API RemoveDuplicatesSettings {
public:
    /// Creates a `RemoveDuplicatesSettings` with default settings.
    ///
    RemoveDuplicatesSettings()
        : distanceThreshold_(1.5) {
    }

    /// Creates a `RemoveDuplicatesSettings` with the given settings.
    ///
    explicit RemoveDuplicatesSettings(double distanceThreshold)
        : distanceThreshold_(distanceThreshold) {
    }

    /// Returns the distance threshold below which points are considered duplicates.
    ///
    /// More precisely, two consecutive input points are consider to *not* be
    /// duplicate if and only if their distance is strictly greater than the
    /// threshold.
    ///
    /// A negative threshold can be used to preserve all input points, even if
    /// exactly at the same position.
    ///
    /// A threshold of zero can be used to only consider as duplicate points
    /// those that have exactly the same position.
    ///
    double distanceThreshold() const {
        return distanceThreshold_;
    };

    /// Sets the distance threshold.
    ///
    void setDistanceThreshold(double distanceThreshold) {
        distanceThreshold_ = distanceThreshold;
    }

private:
    double distanceThreshold_;
};

/// \class vgc::tools::RemoveDuplicatesSettings
/// \brief A sketch pass that removes duplicate points.
///
/// This sketch pass removes input points that are within
/// `RemoveDuplicatesSettings::distanceThreshold()` of their previous point.
///
/// When input points are considered duplicates, then the corresponding output
/// point has the following properties:
///
/// - Its position and timestamp is the same as the first duplicate input
/// point (except for the last output point, see below).
///
/// - Its pressure and width is the same as the duplicate input point that has
/// the greater pressure. This more closely matches what the stroke would look
/// like if the duplicates were not removed.
///
/// If the output has at least two points ([..., p, q]), then the position and
/// timestamp of the last output point q is the same as the input point r,
/// among all input points merged into q, whose position is further away from
/// the second-last output point p.
///
/// Example (distance threshold of 1):
///
/// ```
/// Input:  [(0, 0), (0, 5), (0, 10), (0, 10.1), (0, 9.9)]
/// Output: [(0, 0), (0, 5), (0, 10.1)]
/// ```
///
/// Note that while this pass guarantees that the first output point has the
/// same position as the first input point, it does NOT guarantee that the last
/// output point has the same position as the last input point. Indeed, in the
/// general case, this would be impossible to satisfy (at least not without
/// inventing points) while also satisfying the distance threshold between all
/// output points.
///
/// Example (distance threshold of 1):
///
/// ```
/// Input:  [(0, 0), (0, 0.1)]
/// Output:  [(0, 0)]
/// ```
///
/// In the example above, we do not want the output to be `[(0, 0), (0, 0.1)]`,
/// since the points would not satisfy the distance threshold between all
/// output points.
///
/// Also consider the following example (distance threshold of 1):
///
/// ```
/// Input:  [(0, 0), (0, 1.1), (0, 0.9)]
/// Output:  [(0, 0), (0, 1.1)]
/// ```
///
/// In the example above, we do not want the output to be `[(0, 0), (0, 0.9)]`
/// since it would not satisfy the distance threshold either.
///
class VGC_TOOLS_API RemoveDuplicatesPass : public SketchPass {
public:
    RemoveDuplicatesPass();
    explicit RemoveDuplicatesPass(const RemoveDuplicatesSettings& settings);

    /// Changes the settings for this pass.
    ///
    /// Throws `LogicError` if `output().numStablePoints()` is not zero,
    /// as settings can affect the number of stable points and therefore should not
    /// be called while points are being processed.
    ///
    void setSettings(const RemoveDuplicatesSettings& settings);

protected:
    void doReset() override;
    void doUpdateFrom(const SketchPointBuffer& input, SketchPointBuffer& output) override;

private:
    RemoveDuplicatesSettings settings_;
    Int startInputIndex_ = 0; // see comment in implementation
};

class VGC_TOOLS_API SmoothingPass : public SketchPass {
protected:
    void doUpdateFrom(const SketchPointBuffer& input, SketchPointBuffer& output) override;

private:
    core::DoubleArray widthsBuffer_;
};

class VGC_TOOLS_API DouglasPeuckerPass : public SketchPass {
protected:
    void doUpdateFrom(const SketchPointBuffer& input, SketchPointBuffer& output) override;
};

class VGC_TOOLS_API SingleLineSegmentWithFixedEndpointsPass : public SketchPass {
protected:
    void doUpdateFrom(const SketchPointBuffer& input, SketchPointBuffer& output) override;
};

class VGC_TOOLS_API SingleLineSegmentWithFreeEndpointsPass : public SketchPass {
protected:
    void doUpdateFrom(const SketchPointBuffer& input, SketchPointBuffer& output) override;
};

namespace detail {

// Buffer used to minimize dynamic allocations across multiple fits.
//
struct FitBuffer {
    geometry::Vec2dArray positions;
    core::DoubleArray params;
};

} // namespace detail

class VGC_TOOLS_API SingleQuadraticSegmentWithFixedEndpointsPass : public SketchPass {
protected:
    void doUpdateFrom(const SketchPointBuffer& input, SketchPointBuffer& output) override;

private:
    detail::FitBuffer buffer_;
};

namespace experimental {

/// Where to split a Bézier segment that isn't a good-enough fit.
///
enum class FitSplitType {

    /// Split at the input point which is the furthest away from the best fit.
    ///
    /// This can be a good choice for non-interactive use cases.
    ///
    Furthest,

    /// Split at an index relative to the start point of the current fit.
    ///
    /// This can be a good choice for spline fits when you want most of the
    /// previous fit to be refitted, or for blend fits when you want to have a
    /// lot of overlap between fits, but not as much as when using a dense
    /// blend fit.
    ///
    RelativeToStart,

    /// Split at an index relative to the end point of the current fit.
    ///
    /// This can be a good choice for spline fits in interactive use cases
    /// where input points are added one by one, since in this case, having a
    /// bad fit for the input points [j, ... n] typically means that [j, ...
    /// n-1] will be a good fit (otherwise it would have already been split
    /// before). Therefore, this tends to minimize the amount of "changes"
    /// (flickering) that the user can see. The tradeoff is that the end
    /// tangent of the new fit might not be the best (since after splitting,
    /// the fit is *barely* a good fit), possibly resulting in a slightly worse
    /// final result.
    ///
    RelativeToEnd,

    /// Split at a given ratio in terms number of points.
    ///
    /// For example, using an `indexRatio` of 0.5, it will use half of the
    /// input points for a first fit, and the other half for a second fit. This
    /// tends to be a good compromise between minimizing flickering and
    /// providing good final results.
    ///
    IndexRatio
};

class FitSplitStrategy {
public:
    /// Creates a split strategy of type `Furthest`.
    ///
    static constexpr FitSplitStrategy furthest() {
        return FitSplitStrategy(FitSplitType::Furthest);
    }

    /// Creates a split strategy of type `RelativeToStart`.
    ///
    static constexpr FitSplitStrategy relativeToStart(Int offset) {
        return FitSplitStrategy(FitSplitType::RelativeToStart, offset);
    }

    /// Creates a split strategy of type `RelativeToEnd`.
    ///
    static constexpr FitSplitStrategy relativeToEnd(Int offset) {
        return FitSplitStrategy(FitSplitType::RelativeToEnd, offset);
    }

    /// Creates a split strategy of type `IndexRatio`.
    ///
    static constexpr FitSplitStrategy indexRatio(double ratio) {
        return FitSplitStrategy(FitSplitType::IndexRatio, 0, ratio);
    }

    /// The type of the split strategy.
    ///
    FitSplitType type() const {
        return type_;
    }

    /// The offset to use when `type()` is `RelativeToStart` or `RelativeToEnd`.
    ///
    Int offset() const {
        return offset_;
    }

    /// The ratio to use when `type()` is `IndexRatio`.
    ///
    double ratio() const {
        return ratio_;
    }

    /// Returns the split index based on this split strategy.
    ///
    Int getSplitIndex(Int firstIndex, Int lastIndex, Int furthestIndex) const;

    /// Returns whether two `FitSplitStrategy` are equal.
    ///
    bool operator==(const FitSplitStrategy& other) const {
        return type_ == other.type_        //
               && offset_ == other.offset_ //
               && ratio_ == other.ratio_;
    }

    /// Returns two `FitSplitStrategy` are different.
    ///
    bool operator!=(const FitSplitStrategy& other) const {
        return !(*this == other);
    }

private:
    FitSplitType type_;
    Int offset_;
    double ratio_;

    constexpr FitSplitStrategy(FitSplitType type, Int offset = 0, double ratio = 0.5)
        : type_(type)
        , offset_(offset)
        , ratio_(ratio) {
    }
};

struct SplineFitSettings {

    /// How far from a Bézier fit are the input points allowed to be
    /// for the fit to be considered a good fit.
    ///
    /// The distance is expressed in the same unit as the input points
    /// coordinates, which is typically screen physical pixels.
    ///
    double distanceThreshold = 1.8;

    /// Whether to always pre-emptively split the last good fit into two fits.
    ///
    /// This tends to reduce flickering when sketching, since when a new input
    /// point is added, if the last fit now needs to be split, the new result
    /// will be more similar to the previous result.
    ///
    /// It is recommended to set this to false if `splitStrategy` is
    /// `SecondLast`.
    ///
    bool splitLastGoodFitOnce = true;

    /// How "flat" should a quadratic Bézier segment be in order to be considered
    /// a good fit. It is computed as the ratio between the length of (B2-B0) and the
    /// length of 2*(B0-2B1+B2) (= the second derivative of the quadratic Bézier).
    ///
    /// This prevents outputting quadratic Bézier segments with too high a
    /// curvature, which may be undesirable.
    ///
    /// A value of 1 corresponds to the following quadratic Bézier segment:
    /// - B0 = (0, 0)
    /// - B1 = (2, 1)
    /// - B2 = (4, 0)
    ///
    /// A value of -1 disables the flatness threshold.
    ///
    double flatnessThreshold = -1;

    /// The minimum number of input points required before the flatness
    /// threshold is used.
    ///
    /// This is useful since when there are very few input points (e.g., 3
    /// points), then it is typically preferable to have one high-curvature
    /// segment rather than splitting it further into several segments.
    ///
    Int flatnessThresholdMinPoints = 4;

    /// Where to split a Bézier segment that isn't a good-enough fit.
    ///
    FitSplitStrategy splitStrategy = FitSplitStrategy::indexRatio(0.67);

    /// The number of output points (excluding the first) to generate
    /// for each quadratic Bézier segment in the spline.
    ///
    Int numOutputPointsPerBezier = 8;
};

} // namespace experimental

namespace detail {

// Info about the mapping between input points and output points
// of one of the fit part of a recursive fit.
//
struct SplineFitInfo {
    Int lastInputIndex;
    Int lastOutputIndex;
    geometry::QuadraticBezier2d bezier;
};

} // namespace detail

} // namespace vgc::tools

template<>
struct fmt::formatter<vgc::tools::detail::SplineFitInfo> : fmt::formatter<vgc::Int> {
    template<typename FormatContext>
    auto format(const vgc::tools::detail::SplineFitInfo& i, FormatContext& ctx) {
        return format_to(ctx.out(), "({}, {})", i.lastInputIndex, i.lastOutputIndex);
    }
};

namespace vgc::tools {

class VGC_TOOLS_API QuadraticSplinePass : public SketchPass {
public:
    QuadraticSplinePass();

    /// A constructor with manually specified experimental settings.
    ///
    /// This is not considered stable API and may change at any time.
    ///
    explicit QuadraticSplinePass(const experimental::SplineFitSettings& settings);

protected:
    void doReset() override;
    void doUpdateFrom(const SketchPointBuffer& input, SketchPointBuffer& output) override;

private:
    experimental::SplineFitSettings settings_;
    core::Array<detail::SplineFitInfo> info_;
    detail::FitBuffer buffer_;
};

namespace experimental {

enum class BlendFitType {

    /// Use a dense number of local fits, where two consecutive local fits have
    /// their first input index (and last input index) differ by no more than
    /// by one.
    ///
    /// With this type of blend fit, the `FitSplitStrategy` setting is ignored.
    ///
    Dense,

    /// Use a sparse number of local fits, where two consecutive local fits can
    /// have their first input index (and last input index) differ by more than
    /// by one.
    ///
    /// The offset between one local fit to the next is controlled by the
    /// `FitSplitStrategy` setting.
    ///
    Sparse

    // Future work: Bidirectional?
    // See comment in .cpp/getSparseBlendIndexRange()
};

struct BlendFitSettings {

    /// How far from a Bézier fit are the input points allowed to be
    /// for the fit to be considered a good fit.
    ///
    /// The distance is expressed in the same unit as the input points
    /// coordinates, which is typically screen physical pixels.
    ///
    /// A value around 1.2 tends to work well for input rounded to integer
    /// pixel values (typically mouse input) as it is large enough to smooth
    /// out quantization artifacts. A smaller value (e.g., 0.5) can be used
    /// when the input has sub-pixel precision, resulting in a more precise
    /// output preserving more detail.
    ///
    double distanceThreshold = 1.2;

    /// How "flat" should a quadratic Bézier segment be in order to be considered
    /// a good fit. It is computed as the ratio between the length of (B2-B0) and the
    /// length of 2*(B0-2B1+B2) (= the second derivative of the quadratic Bézier).
    ///
    /// This prevents outputting quadratic Bézier segments with too high a
    /// curvature, which may be undesirable.
    ///
    /// A value of 1 corresponds to the following quadratic Bézier segment:
    /// - B0 = (0, 0)
    /// - B1 = (2, 1)
    /// - B2 = (4, 0)
    ///
    /// A value of -1 disables the flatness threshold.
    ///
    double flatnessThreshold = -1;

    /// The minimum number of input points required before the flatness
    /// threshold is used.
    ///
    /// This is useful since when there are very few input points (e.g., 3
    /// points), then it is typically preferable to have one high-curvature
    /// segment rather than splitting it further into several segments.
    ///
    Int flatnessThresholdMinPoints = 4;

    /// The type of blend fit.
    ///
    BlendFitType type = BlendFitType::Dense;

    /// Where to split a Bézier segment that isn't a good-enough fit.
    ///
    /// This is only used if `type` is `Sparse`.
    ///
    FitSplitStrategy splitStrategy = FitSplitStrategy::indexRatio(0.25);

    /// The minimal number of input points used for each local fit. If the
    /// input has fewer points than this, then the output consists of a single
    /// fit.
    ///
    /// Using a value of 4 or greater is recommended to avoid overfitting
    /// (there always exists a quadratic going exactly through 3 given points).
    ///
    Int minFitPoints = 5;

    /// The maximal number of input points used for each local fit. If the
    /// input has more points than this, then several local fits are used even
    /// if the whole input can be well-approximated by a single fit.
    ///
    /// This ensures that the unstable part of the curve stays under a
    /// reasonable size, improving performance and locality (each input point
    /// should not affect input points that are far away).
    ///
    Int maxFitPoints = 50;

    // This is only used if `type` is `Dense`.
    //
    // Having the first fit be the largest possible good fit is usually not a
    // good idea for dense fits, since this means that there would only be a
    // single fit covering the first input point, and a possibly unaesthetic
    // transition between the first and second fit (which would start at the
    // second input point). The same reasoning also applies at the end of the
    // curve.
    //
    // This setting solves this problem by enforcing that the first input point
    // and the last input point are covered by at least a certain number of
    // fits, whenever possible. A value of at least 3 typically reduce
    // flickering and makes the curve ends look smoother.
    //
    // It is also possible to set numStartFits = IntMax to enforce that the
    // first and last fit always have a size equal to `minFitPoints` (or equal
    // to the number of input points, whichever is smaller), but this is
    // usually not recommended since using small fits is prone to overfitting,
    // which also tends to cause flickering and cause curve ends to be less
    // smooth than the middle of the curve.
    //
    Int numStartFits = 5;

    /// The target arclength distance between samples that is used when
    /// computing the blend between local fits as a uniform sampling.
    ///
    double ds = 3.0;
};

} // namespace experimental

namespace detail {

// Info about the mapping between input points and output points
// of one of the fit part of a recursive fit.
//
struct BlendFitInfo {

    // Input points
    Int firstInputIndex = 0;
    Int lastInputIndex = 0;

    // Chord-length of first and last input points
    double s1 = 0;
    double s2 = 0;

    // Best fit
    geometry::QuadraticBezier2d bezier;
    Int furthestIndex = 0;
    bool isGoodFit = false;
};

} // namespace detail

} // namespace vgc::tools

template<>
struct fmt::formatter<vgc::tools::detail::BlendFitInfo> : fmt::formatter<vgc::Int> {
    template<typename FormatContext>
    auto format(const vgc::tools::detail::BlendFitInfo& i, FormatContext& ctx) {
        return format_to(
            ctx.out(), "(i1={}, i2={})", i.firstInputIndex, i.lastInputIndex);
    }
};

namespace vgc::tools {

class VGC_TOOLS_API QuadraticBlendPass : public SketchPass {
public:
    QuadraticBlendPass();

    /// A constructor with manually specified experimental settings.
    ///
    /// This is not considered stable API and may change at any time.
    ///
    explicit QuadraticBlendPass(const experimental::BlendFitSettings& settings);

protected:
    void doReset() override;
    void doUpdateFrom(const SketchPointBuffer& input, SketchPointBuffer& output) override;

private:
    experimental::BlendFitSettings settings_;
    core::Array<detail::BlendFitInfo> fits_;
    detail::FitBuffer buffer_;
    Int numStableFits_ = 0;

    // more buffers
    core::DoubleArray lastGoodParams;
};

} // namespace vgc::tools

#endif // VGC_TOOLS_SKETCHPASSES_H
