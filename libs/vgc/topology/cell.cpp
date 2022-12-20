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

#include <vgc/topology/cell.h>

namespace vgc::topology {

geometry::Mat3d VacGroup::computeInverseTransformTo(VacGroup* ancestor) const {
    geometry::Mat3d t = inverseTransform_;
    VacGroup* g = parentGroup();
    while (g && g != ancestor) {
        t *= g->inverseTransform();
        g = parentGroup();
    }
    return t;
}

void VacGroup::setTransform_(const geometry::Mat3d& transform) {
    transform_ = transform;
    // todo: handle non-invertible case.
    inverseTransform_ = transform_.inverted();
    updateTransformFromRoot_();
}

void VacGroup::updateTransformFromRoot_() {
    const VacGroup* p = parentGroup();
    if (p) {
        transformFromRoot_ = p->transformFromRoot() * transform_;
    }
    else {
        transformFromRoot_ = transform_;
    }
}

} // namespace vgc::topology