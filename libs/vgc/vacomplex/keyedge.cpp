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

#include <vgc/vacomplex/keyedge.h>

namespace vgc::vacomplex {

std::shared_ptr<const EdgeSampling> KeyEdge::samplingShared() const {
    sampling();
    return snappedSampling_;
}

const EdgeSampling& KeyEdge::sampling() const {

    if (snappedSampling_) {
        return *snappedSampling_;
    }

    geometry::CurveSampleArray snappedSamples;

    if (inputSamples_.isEmpty()) {
        inputSamples_ = computeInputSamples_(samplingParameters_);
    }
    hasGeometryBeenQueriedSinceLastDirtyEvent_ = true;

    // TODO: transform and snap

    snappedSamples = inputSamples_;

    // TODO: define guarantees.
    // - what about a closed edge without points data ?
    // - what about an open edge without points data and same end points ?
    // - what about an open edge without points data but different end points ?
    if (!isClosed() && !snappedSamples.isEmpty()) {
        snappedSamples.first().setPosition(startVertex_->position());
        snappedSamples.last().setPosition(endVertex_->position());
    }

    snappedSamplingBbox_ = geometry::Rect2d::empty;
    for (const auto& sample : snappedSamples) {
        snappedSamplingBbox_.uniteWith(sample.sidePoint(0));
        snappedSamplingBbox_.uniteWith(sample.sidePoint(1));
    }

    snappedSampling_ = std::make_shared<EdgeSampling>(std::move(snappedSamples));

    return *snappedSampling_;
}

const geometry::Rect2d& KeyEdge::samplingBoundingBox() const {
    // update sampling
    sampling();
    return snappedSamplingBbox_;
}

/// Computes and returns a new array of samples for this edge according to the
/// given `parameters`.
///
/// Unlike `sampling()`, this function does not cache the result.
///
geometry::CurveSampleArray
KeyEdge::computeSampling(const geometry::CurveSamplingParameters& parameters) const {

    if (samplingParameters_ == parameters) {
        // return copy of cached sampling
        return sampling().samples();
    }

    geometry::CurveSampleArray sampling = computeInputSamples_(parameters);

    // TODO: transform and snap

    return sampling;
}

double KeyEdge::startAngle() const {
    sampling();
    // TODO: guarantee at least one sample
    if (!snappedSampling_->samples().isEmpty()) {
        return snappedSampling_->samples().first().tangent().angle();
    }
    return 0;
}

double KeyEdge::endAngle() const {
    sampling();
    // TODO: guarantee at least one sample
    if (!snappedSampling_->samples().isEmpty()) {
        geometry::Vec2d endTangent = snappedSampling_->samples().last().tangent();
        return (-endTangent).angle();
    }
    return 0;
}

void KeyEdge::dirtyInputSampling_() {
    snappedSampling_.reset();
    inputSamples_.clear();
}

void KeyEdge::onBoundaryGeometryChanged_() {
    snappedSampling_.reset();
}

geometry::CurveSampleArray
KeyEdge::computeInputSamples_(const geometry::CurveSamplingParameters& parameters) const {

    //VGC_DEBUG_TMP(
    //    "[{}]::computeInputSampling_({{{}, {}, {}}})",
    //    (void*)this,
    //    parameters.maxAngle(),
    //    parameters.minIntraSegmentSamples(),
    //    parameters.maxIntraSegmentSamples());
    geometry::Curve curve;
    geometry::CurveSampleArray sampling;
    curve.setPositions(points());
    curve.setWidths(widths());
    curve.sampleRange(parameters, sampling);
    if (sampling.length()) {
        auto it = sampling.begin();
        geometry::Vec2d lastPoint = it->position();
        double s = 0;
        for (++it; it != sampling.end(); ++it) {
            geometry::Vec2d point = it->position();
            s += (point - lastPoint).length();
            it->setS(s);
            lastPoint = point;
        }
    }
    hasGeometryBeenQueriedSinceLastDirtyEvent_ = true;
    return sampling;
}

} // namespace vgc::vacomplex
