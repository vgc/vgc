// Copyright 2022 The VGC Developers
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

#ifndef VGC_VACOMPLEX_KEYEDGEDATA_H
#define VGC_VACOMPLEX_KEYEDGEDATA_H

#include <array>
#include <memory>
#include <optional>

#include <vgc/core/arithmetic.h>
#include <vgc/core/assert.h>
#include <vgc/geometry/mat3d.h>
#include <vgc/geometry/stroke.h>
#include <vgc/vacomplex/api.h>
#include <vgc/vacomplex/celldata.h>

// how to share edge shape correctly ?
// an inbetween edge that doesn't change should have the same shape for all times
// we also need edge shape source/def, which can be different curve types
// -> EdgeParameters ?

namespace vgc::vacomplex {

class Complex;
class KeyEdge;
class KeyEdgeData;

namespace detail {

class Operations;
struct DefaultSamplingQualitySetter;

struct KeyEdgePrivateKey {
private:
    friend KeyEdge;
    constexpr KeyEdgePrivateKey() noexcept = default;
};

} // namespace detail

/// \class vgc::vacomplex::KeyEdgeData
/// \brief Authored model of the edge geometry.
///
/// It can be translated from dom or set manually.
///
// Dev Notes:
// Edge geometry is relative to end vertices position.
// We want to snap the source geometry in its own space when:
//    - releasing a dragged end vertex
//    - right before sculpting
//    - right before control point dragging
// We have to snap output geometry (sampling) when the source
// geometry is not already snapped (happens in many cases).
//
// In which space do we sample ?
// inbetweening -> common ancestor for best identification of interest points
//
class VGC_VACOMPLEX_API KeyEdgeData final : public CellData {
private:
    friend detail::Operations;
    friend detail::DefaultSamplingQualitySetter;
    friend KeyEdge;

    void moveInit_(KeyEdgeData&& other) noexcept;
    void moveInitDerivedOnly_(KeyEdgeData&& other) noexcept;

public:
    KeyEdgeData() noexcept = default;
    ~KeyEdgeData() = default;

    KeyEdgeData(detail::KeyEdgePrivateKey, KeyEdge* owner) noexcept;

    KeyEdgeData(const KeyEdgeData& other);
    KeyEdgeData(KeyEdgeData&& other) noexcept;
    KeyEdgeData& operator=(const KeyEdgeData& other);
    KeyEdgeData& operator=(KeyEdgeData&& other);

    // Note: the move-constructor is not noexcept since it calls setStroke()
    // which can do allocations and emissions of signals.

    KeyEdge* keyEdge() const;

    bool isClosed() const;

    const geometry::AbstractStroke2d* stroke() const;

    void setStroke(const geometry::AbstractStroke2d* stroke);
    void setStroke(std::unique_ptr<geometry::AbstractStroke2d>&& stroke);

    /// Returns the sampling quality currently used to sample this stroke.
    ///
    /// If no override is set, then this is equal to
    /// `Complex::samplingQuality()`, otherwise this is equal to the override
    /// set.
    ///
    /// \sa `hasSamplingQualityOverride()`,
    ///     `setSamplingQualityOverride()`,
    ///     `clearSamplingQualityOverride()`.
    ///
    geometry::CurveSamplingQuality samplingQuality() const {
        return samplingQuality_;
    }

    /// Returns whether there is currently an explicit override
    /// of sampling quality for this edge.
    ///
    /// This is true if and only if `setSamplingQualityOverride()` has been
    /// called without a subsequent call to `clearSamplingQualityOverride()`
    /// yet.
    ///
    /// \sa `Complex::samplingQuality()`.
    ///
    bool hasSamplingQualityOverride() const {
        return hasSamplingQualityOverride_;
    }

    /// Overrides the default sampling quality with the given `quality`.
    ///
    /// \sa `samplingQuality()`,
    ///     `hasSamplingQualityOverride()`,
    ///     `clearSamplingQualityOverride()`.
    ///
    void setSamplingQualityOverride(geometry::CurveSamplingQuality quality);

    /// Removes the sampling quality override for this edge.
    ///
    /// \sa `samplingQuality()`,
    ///     `hasSamplingQualityOverride()`,
    ///     `clearSamplingQualityOverride()`.
    ///
    void clearSamplingQualityOverride();

    std::shared_ptr<const geometry::StrokeSampling2d> strokeSamplingShared() const {
        updateStrokeSampling_();
        return strokeSampling_;
    }

    const geometry::StrokeSampling2d& strokeSampling() const {
        updateStrokeSampling_();
        VGC_ASSERT(strokeSampling_ != nullptr);
        return *strokeSampling_;
    }

    const geometry::StrokeSample2dArray& strokeSamples() const {
        return strokeSampling().samples();
    }

    /// Computes and returns a new array of samples for the stroke according
    /// to the given `quality`.
    ///
    /// Unlike `strokeSampling()`, this function does not cache the result.
    ///
    geometry::StrokeSampling2d
    computeStrokeSampling(geometry::CurveSamplingQuality quality) const {
        if (samplingQuality_ == quality) {
            // return copy of cached sampling
            return strokeSampling();
        }
        return computeStrokeSampling_(samplingQuality_);
    }

    const geometry::Rect2d& centerlineBoundingBox() const {
        if (!centerlineBBox_.has_value()) {
            updateStrokeSampling_();
            centerlineBBox_ = strokeSampling_->centerlineBoundingBox();
        }
        return centerlineBBox_.value();
    }

    void closeStroke(bool smoothJoin);

    /// Expects positions in object space.
    ///
    void snapGeometry(
        const geometry::Vec2d& snapStartPosition,
        const geometry::Vec2d& snapEndPosition,
        geometry::CurveSnapTransformationMode mode =
            geometry::CurveSnapTransformationMode::LinearInArclength);

    /// Expects delta in object space.
    ///
    void translate(const geometry::Vec2d& delta);

    /// Expects transformation in object space.
    ///
    void transform(const geometry::Mat3d& transformation);

    static KeyEdgeData fromConcatStep(
        const KeyHalfedgeData& khd1,
        const KeyHalfedgeData& khd2,
        bool smoothJoin);

    void finalizeConcat();

    static KeyEdgeData fromGlueOpen(core::ConstSpan<KeyHalfedgeData> khds);

    static KeyEdgeData fromGlueClosed(
        core::ConstSpan<KeyHalfedgeData> khds,
        core::ConstSpan<double> uOffsets);

    static KeyEdgeData fromGlue(
        core::ConstSpan<KeyHalfedgeData> khds,
        std::unique_ptr<geometry::AbstractStroke2d>&& gluedStroke);

    /// The stroke of the returned data is always an open stroke.
    ///
    static KeyEdgeData fromSlice(
        const KeyEdgeData& ked,
        const geometry::CurveParameter& start,
        const geometry::CurveParameter& end,
        Int numWraps);

private:
    std::unique_ptr<geometry::AbstractStroke2d> stroke_;

    // Whether there is a sampling quality override
    bool hasSamplingQualityOverride_ = false;

    // The current sampling quality (either from Complex or from the override)
    geometry::CurveSamplingQuality samplingQuality_ =
        geometry::CurveSamplingQuality::AdaptiveLow;

    // Helpers to change the current sampling quality
    void setSamplingQuality_(geometry::CurveSamplingQuality quality);
    void updateSamplingQuality_(geometry::CurveSamplingQuality quality);
    void updateSamplingQuality_();
    void initSamplingQuality_(const KeyEdgeData& other);
    void copySamplingQuality_(const KeyEdgeData& other);

    mutable std::shared_ptr<const geometry::StrokeSampling2d> strokeSampling_;
    mutable std::optional<geometry::Rect2d> centerlineBBox_;

    void updateStrokeSampling_() const;
    void dirtyStrokeSampling_() const {
        strokeSampling_.reset();
        centerlineBBox_.reset();
    }

    geometry::StrokeSampling2d
    computeStrokeSampling_(geometry::CurveSamplingQuality quality) const;
};

namespace detail {

struct DefaultSamplingQualitySetter {
private:
    friend Complex;
    friend detail::Operations;
    static void set(KeyEdgeData& data, geometry::CurveSamplingQuality quality) {
        if (!data.hasSamplingQualityOverride()) {
            data.setSamplingQuality_(quality);
        }
    }
};

} // namespace detail

//std::shared_ptr<const EdgeSampling> snappedSampling_;
//virtual EdgeSampling computeSampling() = 0;

// key edge
//   geometry as pointer or type, but if it's a type it could be integrated to key edge...
//   if pointer then poly or inner pointer again ? poly is more efficient..

} // namespace vgc::vacomplex

#endif // VGC_VACOMPLEX_KEYEDGEDATA_H
