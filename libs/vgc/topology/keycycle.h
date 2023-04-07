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

#ifndef VGC_TOPOLOGY_KEYCYCLE_H
#define VGC_TOPOLOGY_KEYCYCLE_H

#include <initializer_list>

#include <vgc/core/span.h>
#include <vgc/topology/api.h>
#include <vgc/topology/keyhalfedge.h>

namespace vgc::topology {

class VGC_TOPOLOGY_API KeyCycle {
public:
    explicit KeyCycle(core::Span<const KeyHalfedge> halfedges) noexcept
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

    explicit KeyCycle(std::initializer_list<KeyHalfedge> halfedges) noexcept
        : halfedges_(halfedges) {
    }

    explicit KeyCycle(core::Array<KeyHalfedge>&& halfedges) noexcept
        : halfedges_(std::move(halfedges)) {
    }

    explicit KeyCycle(KeyVertex* steinerVertex) noexcept
        : steinerVertex_(steinerVertex) {
    }

    KeyVertex* steinerVertex() const {
        return steinerVertex_;
    }

    const core::Array<KeyHalfedge>& halfedges() const {
        return halfedges_;
    }

    bool isValid() const {
        return steinerVertex_ || !halfedges_.isEmpty();
    }

private:
    friend detail::Operations;

    KeyVertex* steinerVertex_ = nullptr;
    core::Array<KeyHalfedge> halfedges_;
};

} // namespace vgc::topology

#endif // VGC_TOPOLOGY_KEYCYCLE_H
