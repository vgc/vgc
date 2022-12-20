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

#include <vgc/workspace/element.h>

namespace vgc::workspace {

geometry::Rect2d Element::boundingBox() {
    return geometry::Rect2d::empty;
}

void Element::onDomElementChanged() {
}

void Element::onVacNodeChanged() {
}

void Element::prepareForFrame_(core::AnimTime /*t*/) {
}

void Element::paint_(
    graphics::Engine* /*engine*/,
    core::AnimTime /*t*/,
    PaintOptions /*flags*/) {

    // XXX make it pure virtual once the factory is in.
}

} // namespace vgc::workspace
