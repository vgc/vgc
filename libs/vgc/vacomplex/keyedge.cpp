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

KeyEdge::~KeyEdge() {
    if (geometry_) {
        geometry_->edge_ = nullptr;
    }
}

void KeyEdge::snapGeometry() {
    geometry::Vec2d snapStartPosition = {};
    geometry::Vec2d snapEndPosition = {};

    if (!isClosed()) {
        snapStartPosition = startVertex_->position();
        snapEndPosition = endVertex_->position();
    }
    else {
        // todo: set snapStartPosition
        snapEndPosition = snapStartPosition;
    }
    geometry_->snap(snapStartPosition, snapEndPosition);
}

std::shared_ptr<const EdgeSampling> KeyEdge::samplingShared() const {
    updateSampling_();
    return sampling_;
}

const EdgeSampling& KeyEdge::sampling() const {
    updateSampling_();
    VGC_ASSERT(sampling_ != nullptr);
    return *sampling_;
}

const geometry::Rect2d& KeyEdge::centerlineBoundingBox() const {
    updateSampling_();
    return sampling_ ? sampling_->centerlineBoundingBox() : geometry::Rect2d::empty;
}

/// Computes and returns a new array of samples for this edge according to the
/// given `parameters`.
///
/// Unlike `sampling()`, this function does not cache the result unless
/// `quality == edge->samplingQuality()`.
///
EdgeSampling KeyEdge::computeSampling(geometry::CurveSamplingQuality quality) const {

    if (samplingQuality_ == quality && sampling_) {
        // return copy of cached sampling
        return *sampling_;
    }

    EdgeSampling sampling = computeSampling_(samplingQuality_);

    if (samplingQuality_ == quality) {
        sampling_ = std::make_shared<const EdgeSampling>(sampling);
        hasMeshBeenQueriedSinceLastDirtyEvent_ = true;
    }

    return sampling;
}

double KeyEdge::startAngle() const {
    updateSampling_();
    // TODO: guarantee at least one sample
    if (!sampling_->samples().isEmpty()) {
        return sampling_->samples().first().tangent().angle();
    }
    return 0;
}

double KeyEdge::endAngle() const {
    updateSampling_();
    // TODO: guarantee at least one sample
    if (!sampling_->samples().isEmpty()) {
        geometry::Vec2d endTangent = sampling_->samples().last().tangent();
        return (-endTangent).angle();
    }
    return 0;
}

EdgeSampling KeyEdge::computeSampling_(geometry::CurveSamplingQuality quality) const {
    // TODO: define guarantees.
    // - what about a closed edge without points data ?
    // - what about an open edge without points data and same end points ?
    // - what about an open edge without points data but different end points ?

    if (!isClosed()) {
        geometry::Vec2d snapStartPosition = startVertex_->position();
        geometry::Vec2d snapEndPosition = endVertex_->position();
        return geometry_->computeSampling(quality, snapStartPosition, snapEndPosition);
    }
    else {
        return geometry_->computeSampling(quality, true);
    }
}

void KeyEdge::updateSampling_() const {
    if (!sampling_) {
        EdgeSampling sampling = computeSampling_(samplingQuality_);
        sampling_ = std::make_shared<const EdgeSampling>(std::move(sampling));
    }
    hasMeshBeenQueriedSinceLastDirtyEvent_ = true;
}

void KeyEdge::dirtyMesh_() {
    sampling_.reset();
}

void KeyEdge::onBoundaryMeshChanged_() {
    // possible optimization: check if positions really changed.
    dirtyMesh_();
}

} // namespace vgc::vacomplex
