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

namespace vgc::vacomplex {

KeyHalfedge KeyHalfedge::next() const {

    struct Candidate {
        Candidate(KeyEdge* ke, bool direction, double angle)
            : khe(ke, direction)
            , angle(angle)
            , id(ke->id()) {
        }

        KeyHalfedge khe;
        double angle;
        Int id;
    };

    KeyVertex* kv = this->endVertex();
    core::Array<Candidate> candidates;

    for (Cell* cell : kv->star()) {
        KeyEdge* ke = cell->toKeyEdge();
        if (ke) {
            if (ke->isStartVertex(kv)) {
                candidates.emplaceLast(ke, true, ke->startAngle());
            }
            if (ke->isEndVertex(kv)) {
                candidates.emplaceLast(ke, false, ke->endAngle());
            }
        }
    }

    std::sort(
        candidates.begin(),
        candidates.end(),
        [=](const Candidate& a, const Candidate& b) {
            if (a.angle != b.angle) {
                return a.angle < b.angle;
            }
            if (a.id != b.id) {
                return a.id < b.id;
            }
            // assumes `a.khe.direction() != b.khe.direction()`
            return a.khe.direction();
        });

    KeyHalfedge opposite = this->opposite();

    Int i = 0;
    Int n = candidates.length();
    for (; i < n; ++i) {
        if (candidates[i].khe == opposite) {
            break;
        }
    }

    // first smaller angle is next halfedge
    i = (i - 1 + n) % n;

    return candidates[i].khe;
}

} // namespace vgc::vacomplex
