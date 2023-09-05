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
    if (data_) {
        detail::CellPropertiesPrivateInterface::setOwningCell(&data_->properties_, this);
    }
}

// TODO: make it an operation, otherwise it won't get saved in dom.
bool KeyEdge::snapGeometry() {
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
    data_->snap(snapStartPosition, snapEndPosition);
    return true;
}

std::shared_ptr<const geometry::StrokeSampling2d> KeyEdge::strokeSamplingShared() const {
    updateStrokeSampling_();
    return sampling_;
}

const geometry::StrokeSampling2d& KeyEdge::strokeSampling() const {
    updateStrokeSampling_();
    VGC_ASSERT(sampling_ != nullptr);
    return *sampling_;
}

const geometry::Rect2d& KeyEdge::centerlineBoundingBox() const {
    updateStrokeSampling_();
    return sampling_ ? sampling_->centerlineBoundingBox() : geometry::Rect2d::empty;
}

/// Computes and returns a new array of samples for this edge according to the
/// given `parameters`.
///
/// Unlike `sampling()`, this function does not cache the result unless
/// `quality == edge->samplingQuality()`.
///
geometry::StrokeSampling2d
KeyEdge::computeStrokeSampling(geometry::CurveSamplingQuality quality) const {

    if (samplingQuality_ == quality && sampling_) {
        // return copy of cached sampling
        return *sampling_;
    }

    geometry::StrokeSampling2d sampling = computeStrokeSampling_(samplingQuality_);

    if (samplingQuality_ == quality) {
        sampling_ = std::make_shared<const geometry::StrokeSampling2d>(sampling);
        onMeshQueried();
    }

    return sampling;
}

double KeyEdge::startAngle() const {
    updateStrokeSampling_();
    // TODO: guarantee at least one sample
    if (!sampling_->samples().isEmpty()) {
        return sampling_->samples().first().tangent().angle();
    }
    return 0;
}

double KeyEdge::endAngle() const {
    updateStrokeSampling_();
    // TODO: guarantee at least one sample
    if (!sampling_->samples().isEmpty()) {
        geometry::Vec2d endTangent = sampling_->samples().last().tangent();
        return (-endTangent).angle();
    }
    return 0;
}

geometry::StrokeSampling2d
KeyEdge::computeStrokeSampling_(geometry::CurveSamplingQuality quality) const {
    // TODO: define guarantees.
    // - what about a closed edge without points data ?
    // - what about an open edge without points data and same end points ?
    // - what about an open edge without points data but different end points ?
    return data_->stroke()->computeSampling(quality);
}

void KeyEdge::updateStrokeSampling_() const {
    if (!sampling_) {
        geometry::StrokeSampling2d sampling = computeStrokeSampling_(samplingQuality_);
        sampling_ =
            std::make_shared<const geometry::StrokeSampling2d>(std::move(sampling));
    }
    onMeshQueried();
}

void KeyEdge::dirtyMesh_() {
    sampling_.reset();
}

bool KeyEdge::updateGeometryFromBoundary_() {
    return snapGeometry();
}

// Assumes `oldVertex != nullptr`.
void KeyEdge::substituteKeyVertex_(KeyVertex* oldVertex, KeyVertex* newVertex) {
    if (!isClosed()) {
        if (startVertex_ == oldVertex) {
            startVertex_ = newVertex;
        }
        if (endVertex_ == oldVertex) {
            endVertex_ = newVertex;
        }
    }
}

void KeyEdge::substituteKeyHalfedge_(
    const class KeyHalfedge& /*oldHalfedge*/,
    const class KeyHalfedge& /*newHalfedge*/) {
    // no-op
}

namespace {

std::string vertexId_(KeyVertex* v) {
    if (v) {
        return core::format("{}", v->id());
    }
    else {
        return "_";
    }
}

} // namespace

void KeyEdge::debugPrint_(core::StringWriter& out) {
    out << core::format( //
        "{:<12} startVertex={}, endVertex={}",
        "KeyEdge",
        vertexId_(startVertex_),
        vertexId_(endVertex_));
}

} // namespace vgc::vacomplex
