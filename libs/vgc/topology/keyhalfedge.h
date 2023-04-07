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

#ifndef VGC_TOPOLOGY_KEYHALFEDGE_H
#define VGC_TOPOLOGY_KEYHALFEDGE_H

#include <initializer_list>

#include <vgc/topology/api.h>
#include <vgc/topology/keyedge.h>

namespace vgc::topology {

class VGC_TOPOLOGY_API KeyHalfedge {
public:
    explicit KeyHalfedge(KeyEdge* edge, bool direction) noexcept
        : edge_(edge)
        , direction_(direction) {
    }

    KeyEdge* edge() const {
        return edge_;
    }

    bool direction() const {
        return direction_;
    }

    KeyVertex* startVertex() const {
        return direction_ ? edge_->endVertex() : edge_->startVertex();
    }

    KeyVertex* endVertex() const {
        return direction_ ? edge_->startVertex() : edge_->endVertex();
    }

    bool isClosed() const {
        return edge_->isClosed();
    }

    friend bool operator==(const KeyHalfedge& h1, const KeyHalfedge& h2) {
        return h1.edge_ == h2.edge_ && h1.direction_ == h2.direction_;
    }

    friend bool operator!=(const KeyHalfedge& h1, const KeyHalfedge& h2) {
        return !(h1 == h2);
    }

private:
    friend detail::Operations;

    KeyEdge* edge_;
    bool direction_;
};

} // namespace vgc::topology

#endif // VGC_TOPOLOGY_KEYHALFEDGE_H
