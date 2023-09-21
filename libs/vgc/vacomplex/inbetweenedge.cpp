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

#include <vgc/vacomplex/inbetweenedge.h>

namespace vgc::vacomplex {

bool InbetweenEdge::isStartVertex(const VertexCell* /*v*/) const {
    // TODO: check whether v is one of the start vertices of this inbetween edge.
    return false;
}

bool InbetweenEdge::isEndVertex(const VertexCell* /*v*/) const {
    // TODO: check whether v is one of the end vertices of this inbetween edge.
    return false;
}

bool InbetweenEdge::isClosed() const {
    // TODO: return whether this inbetween edge is open or closed.
    return false;
}

geometry::Rect2d InbetweenEdge::boundingBoxAt(core::AnimTime /*t*/) const {
    // TODO
    return geometry::Rect2d::empty;
}

void InbetweenEdge::substituteKeyVertex_(KeyVertex*, KeyVertex*) {
    // TODO
}

void InbetweenEdge::substituteKeyEdge_(const KeyHalfedge&, const KeyHalfedge&) {
    // TODO
}

void InbetweenEdge::debugPrint_(core::StringWriter& out) {
    // TODO: more debug info
    out << core::format( //
        "{:<12}",
        "InbetweenEdge");
}

} // namespace vgc::vacomplex
