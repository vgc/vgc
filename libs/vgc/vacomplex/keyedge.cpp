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
    data_.snap(snapStartPosition, snapEndPosition);
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

    if (samplingQuality_ == quality) {
        updateStrokeSampling_();
        // return copy of cached sampling
        return *sampling_;
    }

    return computeStrokeSampling_(samplingQuality_);
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

Int KeyEdge::computeWindingContributionAt(const geometry::Vec2d& position) const {

    const geometry::StrokeSampling2d& sampling = strokeSampling();
    const geometry::StrokeSample2dArray& samples = sampling.samples();
    const geometry::Rect2d& bbox = sampling.centerlineBoundingBox();

    // The winding number of a closed curve C (cycle) at position P = (Px, Py)
    // is the total number of times C travels positively around P.
    //
    // If Y points down and X points right, positive means clockwise.
    //
    // Example: the winding number of the trigonometric circle defined as
    // C(t) = (cos(t), sin(t)), for t from 0 to 1, is 1 for any position inside,
    // and 0 for positions outside.
    //
    // It can be computed by looking for all intersections of C with a
    // half line L starting at P. When C crosses L positively as seen from P
    // we count +1, when it crosses L the other way we count -1.
    //
    // We use an axis-aligned half-line that simplifies the computation of
    // intersections.
    //
    // In particular, for a vertical half-line L defined as
    // {(x, y): x = Px, y >= Py}:
    //
    // - If the bounding box of a halfedge H is strictly on one side of P in X,
    // or below P (box.maxY() <= Py), then it cannot cross the half-line and
    // does not participate in the winding number (apart from being part of the
    // cycle) whatever the shape of the halfedge H inside that box.
    //
    // - If the bounding box of a halfedge H is above P (box.minY() >= Py),
    // then we know that the winding number would remain the same whatever the
    // shape of the interior of the curve inside that box. We can thus use
    // the straight line between its endpoints to compute the crossing with L.
    // In fact, only the X component of its endpoints is relevant.
    //
    // - Otherwise, we have to intersect the halfedge curve with the half-line
    // the usual way.
    //
    // To not miss a crossing nor count one twice, care must be taken about
    // samples exactly on H. To deal with this problem we consider Hx to be
    // between Px's floating point value and the next (core::nextafter(Px)).
    //
    // As future optimization, curves could implement an acceleration structure
    // for intersections in the form of a tree of sub-spans and bounding boxes
    // that we could benefit from. Such sub-spans would follow the same rules
    // as halfedges (listed above).

    double px = position.x();
    double py = position.y();

    if (bbox.xMax() + core::epsilon < px) {
        return 0;
    }

    if (bbox.xMin() - core::epsilon > px) {
        return 0;
    }

    if (bbox.yMax() < py) {
        return 0;
    }

    // from here, bbox overlaps the half-line

    Int contribution = 0;

    if (bbox.yMin() > py) {
        // bbox overlaps the half-line but does not contain P.
        // => only X component of endpoints matters.

        // Note: We assume first and last sample positions to exactly match the
        // positions of start and end vertices.
        geometry::Vec2d p1 = samples.first().position();
        geometry::Vec2d p2 = samples.last().position();
        double x1 = p1.x();
        double x2 = p2.x();

        // winding contribution:
        // x1 <= px && x2 > px: -1
        // x2 <= px && x1 > px: +1
        // otherwise: 0
        bool x1Side = x1 <= px;
        bool x2Side = x2 <= px;
        if (x1Side == x2Side) {
            return 0;
        }
        else {
            //       P┌────────→ x
            //  true  │  false
            //  Side  H  Side
            //        │
            //        │    winding
            //        │    contribution:
            //      ──│─→    -1
            //      ←─│──    +1
            //        │
            //        y
            //
            contribution = 1 - 2 * x1Side;
        }
    }
    else {
        // bbox contains P.
        // => compute intersections with curve.

        // Note: We assume first and last sample positions to exactly match the
        // positions of start and end vertices.
        auto it = samples.begin();
        geometry::Vec2d p1 = it->position();
        geometry::Vec2d p2 = {};
        bool x1Side = (p1.x() <= px);
        bool x2Side = {};
        for (++it; it != samples.end(); ++it) {
            p2 = it->position();
            x2Side = (p2.x() <= px);
            if (x1Side != x2Side) {
                bool y1Side = p1.y() < py;
                bool y2Side = p2.y() < py;
                if (y1Side == y2Side) {
                    if (!y1Side) {
                        contribution += (1 - 2 * x1Side);
                    }
                }
                else {
                    geometry::Vec2d p1p2 = p2 - p1;
                    geometry::Vec2d pp1 = p1 - position;
                    double t = pp1.det(p1p2);
                    bool xDir = std::signbit(p1p2.x());
                    bool tSign = std::signbit(t);
                    // If p lies exactly on p1p2, the result does not matter,
                    // thus we do not test for (t == 0).
                    if (tSign != xDir) {
                        contribution += (1 - 2 * x1Side);
                    }
                }
            }
            p1 = p2;
            x1Side = x2Side;
        }
    }

    return contribution;
}

geometry::Rect2d KeyEdge::boundingBox() const {
    if (!bbox_.has_value()) {
        updateStrokeSampling_();
        bbox_ = sampling_->centerlineBoundingBox();
    }
    return bbox_.value();
}

void KeyEdge::dirtyMesh() {
    sampling_.reset();
    bbox_ = std::nullopt;
}

bool KeyEdge::updateGeometryFromBoundary() {
    return snapGeometry();
}

geometry::StrokeSampling2d
KeyEdge::computeStrokeSampling_(geometry::CurveSamplingQuality quality) const {
    // TODO: define guarantees.
    // - what about a closed edge without points data ?
    // - what about an open edge without points data and same end points ?
    // - what about an open edge without points data but different end points ?
    geometry::StrokeSampling2d sampling = data_.stroke()->computeSampling(quality);
    onMeshQueried();
    return sampling;
}

void KeyEdge::updateStrokeSampling_() const {
    if (!sampling_) {
        geometry::StrokeSampling2d sampling = computeStrokeSampling_(samplingQuality_);
        sampling_ =
            std::make_shared<const geometry::StrokeSampling2d>(std::move(sampling));
    }
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

void KeyEdge::substituteKeyEdge_(
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
