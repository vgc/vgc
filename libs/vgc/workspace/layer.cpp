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

#include <vgc/workspace/layer.h>
#include <vgc/workspace/workspace.h>

namespace vgc::workspace {

geometry::Rect2d Layer::boundingBox(core::AnimTime /*t*/) const {
    // todo, union of children
    return geometry::Rect2d::empty;
}

ElementError Layer::updateFromDom_(Workspace* /*workspace*/) {
    dom::Element* const domElement = this->domElement();

    topology::VacGroup* g = nullptr;
    if (!vacNode_) {
        g = topology::ops::createVacGroup(
            domElement->internalId(), parentVacElement()->vacNode()->toGroupUnchecked());
        vacNode_ = g;
    }
    else {
        g = vacNode()->toCellUnchecked()->toGroupUnchecked();
    }

    // todo: set attributes
    // ...

    return ElementError::None;
}

void Layer::paint_(
    graphics::Engine* /*engine*/,
    core::AnimTime /*t*/,
    PaintOptions /*flags*/) const {
}

} // namespace vgc::workspace
