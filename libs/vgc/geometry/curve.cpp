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

#include <vgc/geometry/curve.h>

#include <vgc/core/exceptions.h>
#include <vgc/geometry/logcategories.h>

namespace vgc::geometry {

VGC_DEFINE_ENUM(
    CurveSamplingQuality,

    (Disabled, "Disabled"),

    (UniformVeryLow, "Uniform Very Low"),
    (UniformLow, "Uniform Low"),
    (UniformMedium, "Uniform Medium"),
    (UniformHigh, "Uniform High"),
    (UniformVeryHigh, "Uniform Very High"),

    (AdaptiveVeryLow, "Adaptive Very Low"),
    (AdaptiveLow, "Adaptive Low"),
    (AdaptiveMedium, "Adaptive Medium"),
    (AdaptiveHigh, "Adaptive High"),
    (AdaptiveVeryHigh, "Adaptive Very High"))

CurveSamplingQuality getSamplingQuality(Int8 level, bool adaptiveSampling) {
    static constexpr Int8 maxLevel = 5;
    if (level < 0 || level > maxLevel) {
        Int8 oldLevel = level;
        level = core::clamp(oldLevel, 0, maxLevel);
        VGC_WARNING(
            LogVgcGeometry,
            core::format(
                "Invalid CurveSamplingQuality level ({}): must be an integer between 0 "
                "and {}. Using {} instead.",
                oldLevel,
                maxLevel,
                level));
    }

    // If level is 0, then this means `Disabled`, which is considered a uniform
    // sampling. Note that in this case, it is important not to return 0x10
    // which is an invalid enum value, which is why we need this code block.
    //
    // For now, we decided not to emit a warning in this case, but maybe we should
    // consider it?
    //
    if (level == 0) {
        return CurveSamplingQuality::Disabled;
    }

    UInt8 adaptiveBit = adaptiveSampling ? 0x10 : 0x00;
    UInt8 levelBits = static_cast<UInt8>(level);
    return static_cast<CurveSamplingQuality>(adaptiveBit | levelBits);
}

VGC_DEFINE_ENUM(CurveSnapTransformationMode, (LinearInArclength, "LinearInArclength"))

CurveSamplingParameters::CurveSamplingParameters(CurveSamplingQuality quality)
    : maxAngle_(1)
    , minIntraSegmentSamples_(1)
    , maxIntraSegmentSamples_(1) {

    switch (quality) {
    case CurveSamplingQuality::Disabled:
        maxAngle_ = 100;
        minIntraSegmentSamples_ = 0;
        maxIntraSegmentSamples_ = 0;
        break;
    case CurveSamplingQuality::UniformVeryLow:
        maxAngle_ = 100;
        minIntraSegmentSamples_ = 3;
        maxIntraSegmentSamples_ = 3;
        break;
    case CurveSamplingQuality::UniformLow:
        maxAngle_ = 100;
        minIntraSegmentSamples_ = 7;
        maxIntraSegmentSamples_ = 7;
        break;
    case CurveSamplingQuality::UniformMedium:
        maxAngle_ = 100;
        minIntraSegmentSamples_ = 15;
        maxIntraSegmentSamples_ = 15;
        break;
    case CurveSamplingQuality::UniformHigh:
        maxAngle_ = 100;
        minIntraSegmentSamples_ = 31;
        maxIntraSegmentSamples_ = 31;
        break;
    case CurveSamplingQuality::UniformVeryHigh:
        maxAngle_ = 100;
        minIntraSegmentSamples_ = 63;
        maxIntraSegmentSamples_ = 63;
        break;
    case CurveSamplingQuality::AdaptiveVeryLow:
        maxAngle_ = 0.150;
        minIntraSegmentSamples_ = 0;
        maxIntraSegmentSamples_ = 127;
        break;
    case CurveSamplingQuality::AdaptiveLow:
        maxAngle_ = 0.075;
        minIntraSegmentSamples_ = 0;
        maxIntraSegmentSamples_ = 255;
        break;
    case CurveSamplingQuality::AdaptiveMedium:
        maxAngle_ = 0.050;
        minIntraSegmentSamples_ = 0;
        maxIntraSegmentSamples_ = 511;
        break;
    case CurveSamplingQuality::AdaptiveHigh:
        maxAngle_ = 0.025;
        minIntraSegmentSamples_ = 0;
        maxIntraSegmentSamples_ = 1023;
        break;
    case CurveSamplingQuality::AdaptiveVeryHigh:
        maxAngle_ = 0.0125;
        minIntraSegmentSamples_ = 0;
        maxIntraSegmentSamples_ = 2047;
        break;
    }

    cosMaxAngle_ = std::cos(maxAngle_);
}

namespace detail {

void checkSegmentIndex(Int segmentIndex, Int numSegments) {
    if (segmentIndex < 0 || segmentIndex > numSegments - 1) {
        throw core::IndexError(core::format(
            "Parameter segmentIndex ({}) out of range [{}, {}] (numSegments() == {})",
            segmentIndex,
            0,
            numSegments - 1,
            numSegments));
    }
}

} // namespace detail

} // namespace vgc::geometry
