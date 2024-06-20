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

class VGC_TOOLS_API EmptyPass : public SketchPass {
protected:
    void doUpdateFrom(const SketchPointBuffer& input, SketchPointBuffer& output) override;
};

class VGC_TOOLS_API TransformPass : public SketchPass {
protected:
    void doUpdateFrom(const SketchPointBuffer& input, SketchPointBuffer& output) override;
};

class VGC_TOOLS_API SmoothingPass : public SketchPass {
protected:
    void doUpdateFrom(const SketchPointBuffer& input, SketchPointBuffer& output) override;
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
    /// This is usually a good choice for non-interactive use cases.
    ///
    Furthest,

    /// Split at an index relative to the start point of the current fit.
    ///
    /// This is useful for use with fixed-sized window, e.g., if you want
    /// to exaustively compute all overlapping fits of a given size, you
    /// can use RelativeToStart with an offset of 1.
    ///
    /// Using RelativeToStart with an offset of 0 has the special meaning to
    /// allow a fit to start at the same point than the previous fit, as long
    /// as the fit ends strictly after the previous fit and is a good fit.
    ///
    RelativeToStart,

    /// Split at an index relative to the end point of the current fit.
    ///
    /// This can be a good choice in interactive use cases where input points
    /// are added one by one, since in this case, having a bad fit for the input
    /// points [j, ... n] typically means that [j, ... n-1] will be a good fit
    /// (otherwise it would have already been split before). Therefore, this
    /// tends to minimize the amount of "changes" (flickering) that the user can
    /// see. The tradeoff is that the end tangent of the new fit might not be
    /// the best (since after splitting, the fit is *barely* a good fit),
    /// possibly resulting in a slightly worse final result.
    ///
    RelativeToEnd,

    /// Split at a given ratio in terms number of points.
    ///
    /// For example, using an `indexRatio` of 0.5, it will use half of the input
    /// points for a first fit, and the other half for a second fit. This tends
    /// to be a good compromise between minimizing flickering and providing good
    /// final results.
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

    /// The number of output points (excluding the first) to generate
    /// for each quadratic Bézier segment in the spline.
    ///
    Int numOutputPointsPerBezier = 8;

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

struct BlendFitSettings {

    /// The number of output points (excluding the first) to generate
    /// for each quadratic Bézier segment in the spline.
    ///
    Int numOutputPointsPerBezier = 8;

    /// How far from a Bézier fit are the input points allowed to be
    /// for the fit to be considered a good fit.
    ///
    /// The distance is expressed in the same unit as the input points
    /// coordinates, which is typically screen physical pixels.
    ///
    double distanceThreshold = 1.2;

    /*
    /// Whether to always pre-emptively split the last good fit into two fits.
    ///
    /// This tends to reduce flickering when sketching, since when a new input
    /// point is added, if the last fit now needs to be split, the new result
    /// will be more similar to the previous result.
    ///
    /// It is recommended to set this to false if `splitStrategy` is
    /// `SecondLast`.
    ///
    bool splitLastGoodFitOnce = false;
    */

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
    FitSplitStrategy splitStrategy = FitSplitStrategy::relativeToStart(0);

    /// The minimal number of input points used for each local fit. If the
    /// input has fewer points than this, then the output consists of a single
    /// fit.
    ///
    // TODO: minFitLength for minimal arc-length per local fit?
    //
    Int minFitPoints = 5;

    /// The maximal number of input points used for each local fit. If the
    /// input has fewer points than this, then the output consists of a single
    /// fit.
    ///
    // TODO: maxFitLength for minimal arc-length per local fit?
    //
    Int maxFitPoints = 50;
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

    // Mapping with output points
    Int firstOutputIndex = 0;
    Int lastOutputIndex = 0;
};

} // namespace detail

} // namespace vgc::tools

template<>
struct fmt::formatter<vgc::tools::detail::BlendFitInfo> : fmt::formatter<vgc::Int> {
    template<typename FormatContext>
    auto format(const vgc::tools::detail::BlendFitInfo& i, FormatContext& ctx) {
        return format_to(
            ctx.out(),
            "(inFirst={}, inLast={}, outFirst={}, outLast={})",
            i.firstInputIndex,
            i.lastInputIndex,
            i.firstOutputIndex,
            i.lastOutputIndex);
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

    // more buffers
    core::DoubleArray lastGoodParams;
};

} // namespace vgc::tools

#endif // VGC_TOOLS_SKETCHPASSES_H
