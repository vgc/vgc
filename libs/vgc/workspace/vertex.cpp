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

#include <vgc/workspace/edge.h>
#include <vgc/workspace/vertex.h>
#include <vgc/workspace/workspace.h>

namespace vgc::workspace {

void Vertex::updateJoinsAndCaps(core::AnimTime /*t*/) {
}

void KeyVertex::updateJoinsAndCaps() {
    updateJoinsAndCaps_();
}

void KeyVertex::updateJoinsAndCaps(core::AnimTime /*t*/) {
    updateJoinsAndCaps_();
}

geometry::Rect2d KeyVertex::boundingBox(core::AnimTime /*t*/) const {
    geometry::Vec2d pos = vacKeyVertex()->position({});
    return geometry::Rect2d(pos, pos);
}

ElementError KeyVertex::updateFromDom_(Workspace* /*workspace*/) {
    namespace ds = dom::strings;
    dom::Element* const domElement = this->domElement();

    topology::KeyVertex* kv = nullptr;
    if (!vacNode_) {
        kv = topology::ops::createKeyVertex(
            domElement->internalId(), parentVacElement()->vacNode()->toGroupUnchecked());
        vacNode_ = kv;
    }
    else {
        kv = vacNode_->toCellUnchecked()->toKeyVertexUnchecked();
    }

    const auto& position = domElement->getAttribute(ds::position).getVec2d();
    topology::ops::setKeyVertexPosition(kv, position);

    notifyChanges();

    return ElementError::None;
}

void KeyVertex::paint_(
    graphics::Engine* /*engine*/,
    core::AnimTime /*t*/,
    PaintOptions /*flags*/) const {
}

void KeyVertex::updateJoinsAndCaps_() {
    //VGC_DEBUG_TMP("updateJoinsAndCaps_()");

    topology::KeyVertex* kv = vacNode()->toCellUnchecked()->toKeyVertexUnchecked();
    if (!kv) {
        return;
    }

    // get the KeyEdges that we have to join (later we'll have to do that by join group index)
    struct IncidentEdge {
        topology::KeyEdge* vacKe;
        KeyEdge* ke;
        double angle;
        bool isReverse;
        Int numSamples;
    };
    core::Array<IncidentEdge> edges;
    for (topology::VacCell* cell : kv->star()) {
        topology::KeyEdge* vacKe = cell->toKeyEdge();
        // XXX replace dynamic_cast
        KeyEdge* ke = dynamic_cast<KeyEdge*>(workspace()->find(vacKe->id()));
        if (ke) {
            ke->updateGeometry({});
            bool isReverse = (vacKe->startVertex() != kv);
            const geometry::CurveSampleArray& samples = ke->geometry_.samples_;
            if (samples.length() < 2) {
                continue;
            }
            geometry::CurveSample sample = samples[isReverse ? samples.size() - 2 : 1];

            // XXX add xAxisAngle to Vec class
            double angle =
                geometry::Vec2d(1.f, 0).angle(sample.position() - kv->position({}));
            edges.emplaceLast(
                IncidentEdge{vacKe, ke, angle, isReverse, samples.length()});
        }
    }

    // sort by incident angle
}

geometry::Rect2d InbetweenVertex::boundingBox(core::AnimTime t) const {
    geometry::Vec2d pos = vacInbetweenVertex()->position(t);
    return geometry::Rect2d(pos, pos);
}

ElementError InbetweenVertex::updateFromDom_(Workspace* /*workspace*/) {
    return ElementError::None;
}

void InbetweenVertex::preparePaint_(core::AnimTime /*t*/, PaintOptions /*flags*/) {
}

void InbetweenVertex::paint_(
    graphics::Engine* /*engine*/,
    core::AnimTime /*t*/,
    PaintOptions /*flags*/) const {
}

} // namespace vgc::workspace
