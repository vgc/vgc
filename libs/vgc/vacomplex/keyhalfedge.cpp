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

#include <vgc/vacomplex/keyhalfedge.h>
#include <vgc/vacomplex/keyvertex.h>

namespace vgc::vacomplex {

KeyHalfedge KeyHalfedge::next() const {

    if (!endVertex()) {
        return *this;
    }

    core::Array<RingKeyHalfedge> ring = endVertex()->computeRingHalfedges();

    KeyHalfedge opposite = this->opposite();

    Int i = 0;
    Int n = ring.length();
    for (; i < n; ++i) {
        if (ring[i] == opposite) {
            break;
        }
    }

    // first smaller angle is next halfedge
    i = (i - 1 + n) % n;

    return ring[i];
}

KeyHalfedge KeyHalfedge::previous() const {

    if (!startVertex()) {
        return *this;
    }

    core::Array<RingKeyHalfedge> ring = startVertex()->computeRingHalfedges();

    Int i = 0;
    Int n = ring.length();
    for (; i < n; ++i) {
        if (ring[i] == *this) {
            break;
        }
    }

    // first bigger angle is opposite of previous halfedge
    i = (i + 1) % n;

    return ring[i].halfedge().opposite();
}

Int KeyHalfedge::computeWindingContributionAt(const geometry::Vec2d& position) const {
    const KeyEdge* ke = edge();
    Int contribution = ke->computeWindingContributionAt(position);
    return direction() ? contribution : -contribution;
}

} // namespace vgc::vacomplex
