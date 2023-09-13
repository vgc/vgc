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

#ifndef VGC_GEOMETRY_INTERPOLATINGSTROKE_H
#define VGC_GEOMETRY_INTERPOLATINGSTROKE_H

#include <vgc/core/arithmetic.h>
#include <vgc/core/assert.h>
#include <vgc/core/span.h>
#include <vgc/geometry/api.h>
#include <vgc/geometry/stroke.h>

namespace vgc::geometry {

enum class CurveSegmentType : UInt8 {
    Simple,
    Corner,
    AfterCorner,
    BeforeCorner,
    BetweenCorners,
};

class FreehandStrokePoint {
public:
    VGC_WARNING_PUSH
    VGC_WARNING_MSVC_DISABLE(26495) // member variable uninitialized
    FreehandStrokePoint(core::NoInit)
        : pos_(core::noInit) {
    }
    VGC_WARNING_POP

    FreehandStrokePoint(const geometry::Vec2d& position, double width)
        : pos_(position)
        , width_(width) {
    }

    FreehandStrokePoint(const geometry::StrokeSample2d& sample)
        : pos_(sample.position())
        , width_(sample.halfwidth(0) * 2) {
    }

    FreehandStrokePoint lerp(const FreehandStrokePoint& b, double u) {
        FreehandStrokePoint result = *this;
        result.pos_ += u * (b.pos_ - pos_);
        result.width_ += u * (b.width_ - width_);
        return result;
    }

    FreehandStrokePoint average(const FreehandStrokePoint& b) {
        return FreehandStrokePoint(0.5 * (pos_ + b.pos_), 0.5 * (width_ + b.width_));
    }

    geometry::Vec2d position() const {
        return pos_;
    }

    double width() const {
        return width_;
    }

private:
    geometry::Vec2d pos_;
    double width_ = 0;
};

class VGC_GEOMETRY_API AbstractInterpolatingStroke2d : public AbstractStroke2d {
protected:
    AbstractInterpolatingStroke2d(bool isClosed)
        : AbstractStroke2d(isClosed) {
    }

    AbstractInterpolatingStroke2d(bool isClosed, double constantWidth)

        : AbstractStroke2d(isClosed)
        , widths_(1, constantWidth)
        , hasConstantWidth_(true) {
    }

    template<typename TRangePositions, typename TRangeWidths>
    AbstractInterpolatingStroke2d(
        bool isClosed,
        TRangePositions&& positions,
        TRangeWidths&& widths)

        : AbstractStroke2d(isClosed)
        , positions_(std::forward<TRangePositions>(positions))
        , widths_(std::forward<TRangeWidths>(widths)) {

        hasConstantWidth_ = widths_.length() != positions_.length();
    }

public:
    const Vec2dArray& positions() const {
        return positions_;
    }

    template<typename TRange>
    void setPositions(TRange&& positions) {
        positions_ = Vec2dArray(std::forward<TRange>(positions));
        onPositionsChanged_();
    }

    const core::DoubleArray& widths() const {
        return widths_;
    }

    template<typename TRange>
    void setWidths(TRange&& widths) {
        widths_ = core::DoubleArray(std::forward<TRange>(widths));
        hasConstantWidth_ = false;
        onWidthsChanged_();
    }

    void setConstantWidth(double width) {
        hasConstantWidth_ = true;
        widths_ = core::DoubleArray{width};
        onWidthsChanged_();
    }

    bool hasConstantWidth() const {
        return hasConstantWidth_;
    }

    double constantWidth() const {
        return widths_.isEmpty() ? 0. : widths_.getUnchecked(0);
    }

protected:
    struct SegmentComputeData {
        std::array<Int, 4> knotIndices;
        std::array<Vec2d, 3> chords;
        std::array<double, 3> chordLengths;
    };

    const core::DoubleArray& chordLengths() const {
        updateCache();
        return chordLengths_;
    }

    const core::Array<CurveSegmentType>& segmentTypes() const {
        updateCache();
        return segmentTypes_;
    }

    void updateCache() const;

protected:
    const StrokeModelInfo& modelInfo_() const override = 0;

    std::unique_ptr<AbstractStroke2d> cloneEmpty_() const override = 0;
    std::unique_ptr<AbstractStroke2d> clone_() const override = 0;
    std::unique_ptr<AbstractStroke2d>
    convert_(const AbstractStroke2d* source) const override;

    bool copyAssign_(const AbstractStroke2d* other) override = 0;
    bool moveAssign_(AbstractStroke2d* other) override = 0;
    bool convertAssign_(const AbstractStroke2d* other) override;

    double approximateLength_() const override;

    Int numKnots_() const override;

    bool isZeroLengthSegment_(Int segmentIndex) const override;

    std::array<Vec2d, 2> endPositions_() const override;

    StrokeBoundaryInfo computeBoundaryInfo_() const override = 0;

    CurveParameter
    resolveSampledLocation_(const SampledCurveLocation& location) const override;

    void translate_(const geometry::Vec2d& delta) override;

    void transform_(const geometry::Mat3d& transformation) override;

    void close_(bool smoothJoin) override;

    void open_(bool keepJoinAsBestAsPossible) override;

    std::unique_ptr<AbstractStroke2d> subStroke_(
        const CurveParameter& p1,
        const CurveParameter& p2,
        Int numWraps) const override;

    void reverse_() override;

    void assignFromConcat_(
        const AbstractStroke2d* a,
        bool directionA,
        const AbstractStroke2d* b,
        bool directionB,
        bool smoothJoin) override;

    virtual void assignFromAverage_(
        core::ConstSpan<const AbstractStroke2d*> strokes,
        core::ConstSpan<bool> directions,
        core::ConstSpan<double> uOffsets,
        bool areClosed) override;

    bool snap_(
        const geometry::Vec2d& snapStartPosition,
        const geometry::Vec2d& snapEndPosition,
        CurveSnapTransformationMode mode) override;

    Vec2d sculptGrab_(
        const Vec2d& startPosition,
        const Vec2d& endPosition,
        double radius,
        double strength,
        double tolerance,
        bool isClosed) override;

    Vec2d sculptWidth_(
        const Vec2d& position,
        double delta,
        double radius,
        double tolerance,
        bool isClosed) override;

    Vec2d sculptSmooth_(
        const Vec2d& position,
        double radius,
        double strength,
        double tolerance,
        bool isClosed) override;

protected:
    virtual void
    updateCache_(const core::Array<SegmentComputeData>& baseComputeDataArray) const = 0;

private:
    Vec2dArray positions_;
    core::DoubleArray widths_;

    // It has the same number of elements as of positions_.
    // Last chord is the closure if closed, zero otherwise.
    mutable core::DoubleArray chordLengths_;
    mutable double totalChordalLength_ = 0;
    mutable core::Array<CurveSegmentType> segmentTypes_;

    bool hasConstantWidth_ = false;

    mutable bool isCacheDirty_ = true;

    void computePositionsS_(core::DoubleArray& positionsS) const;

    void onPositionsChanged_();
    void onWidthsChanged_();
};

namespace detail {

VGC_GEOMETRY_API
void checkSegmentIndexIsValid(Int segmentIndex, Int numSegments);

template<std::size_t I, typename... T, std::size_t... Is>
constexpr std::tuple<std::tuple_element_t<I + Is, std::tuple<T...>>...>
SubPackAsTuple_(std::index_sequence<Is...>) {
}

template<typename T, size_t n, size_t... Is>
std::array<T, n> getElementsUnchecked_(
    const core::Array<T>& arr,
    const std::array<Int, n>& indices,
    std::index_sequence<Is...>) {

    return std::array<T, n>{arr.getUnchecked(indices[Is])...};
}

template<typename T, size_t n>
std::array<T, n>
getElementsUnchecked(const core::Array<T>& arr, const std::array<Int, n>& indices) {
    return getElementsUnchecked_(arr, indices, std::make_index_sequence<n>());
}

} // namespace detail

} // namespace vgc::geometry

#endif // VGC_GEOMETRY_INTERPOLATINGSTROKE_H
