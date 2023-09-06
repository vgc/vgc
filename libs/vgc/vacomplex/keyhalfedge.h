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

#ifndef VGC_VACOMPLEX_KEYHALFEDGE_H
#define VGC_VACOMPLEX_KEYHALFEDGE_H

#include <functional>
#include <initializer_list>

#include <vgc/vacomplex/api.h>
#include <vgc/vacomplex/keyedge.h>

namespace vgc::vacomplex {

class VGC_VACOMPLEX_API KeyHalfedge {
public:
    KeyHalfedge() noexcept = default;

    KeyHalfedge(KeyEdge* edge, bool direction) noexcept
        : edge_(edge)
        , direction_(direction) {
    }

    KeyEdge* edge() const {
        return edge_;
    }

    void setEdge(KeyEdge* edge) {
        edge_ = edge;
    }

    bool direction() const {
        return direction_;
    }

    void setDirection(bool direction) {
        direction_ = direction;
    }

    KeyVertex* startVertex() const {
        return direction_ ? edge_->startVertex() : edge_->endVertex();
    }

    KeyVertex* endVertex() const {
        return direction_ ? edge_->endVertex() : edge_->startVertex();
    }

    /// Returns the angle between the x-axis and the start tangent.
    double startAngle() const {
        return direction_ ? edge_->startAngle() : edge_->endAngle();
    }

    /// Returns the angle between the x-axis and the reversed end tangent.
    double endAngle() const {
        return direction_ ? edge_->endAngle() : edge_->startAngle();
    }

    bool isClosed() const {
        return edge_->isClosed();
    }

    KeyHalfedge next() const;
    //KeyHalfedge previous() const;

    KeyHalfedge opposite() const {
        return KeyHalfedge(edge_, !direction_);
    }

    friend bool operator==(const KeyHalfedge& h1, const KeyHalfedge& h2) {
        return h1.edge_ == h2.edge_ && h1.direction_ == h2.direction_;
    }

    friend bool operator!=(const KeyHalfedge& h1, const KeyHalfedge& h2) {
        return !(h1 == h2);
    }

private:
    friend detail::Operations;

    KeyEdge* edge_ = nullptr;
    bool direction_ = false;
};

} // namespace vgc::vacomplex

template<>
struct std::hash<vgc::vacomplex::KeyHalfedge> {
    std::size_t operator()(const vgc::vacomplex::KeyHalfedge& kh) const noexcept {
        std::size_t h = std::hash<void*>{}(kh.edge());
        return (h << 1) & (kh.direction() ? 1 : 0);
    }
};

#endif // VGC_VACOMPLEX_KEYHALFEDGE_H
