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

#include <vgc/vacomplex/keycycle.h>

namespace vgc::vacomplex {

KeyCycle::KeyCycle(core::Span<const KeyHalfedge> halfedges) noexcept
    : halfedges_(halfedges) {

    if (halfedges_.isEmpty()) {
        // invalid cycle
    }
    else if (halfedges_.first().isClosed()) {
        KeyHalfedge h0 = halfedges_.first();
        for (const KeyHalfedge& h : halfedges_) {
            if (h0 != h) {
                // invalid cycle
                halfedges_.clear();
                break;
            }
        }
    }
    else {
        // Note: there is no need to check that all halfedges
        // have the same key time since each consecutive pair
        // of halfedges share a vertex, and its time.
        auto it = halfedges_.begin();
        KeyVertex* firstStartVertex = it->startVertex();
        KeyVertex* previousEndVertex = it->endVertex();
        for (++it; it != halfedges_.end(); ++it) {
            if (previousEndVertex != it->startVertex()) {
                // invalid cycle
                halfedges_.clear();
                break;
            }
            previousEndVertex = it->endVertex();
        }
        if (previousEndVertex != firstStartVertex) {
            // invalid cycle
            halfedges_.clear();
        }
    }
}

} // namespace vgc::vacomplex
