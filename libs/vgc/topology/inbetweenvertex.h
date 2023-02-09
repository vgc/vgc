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

#ifndef VGC_TOPOLOGY_INBETWEENVERTEX_H
#define VGC_TOPOLOGY_INBETWEENVERTEX_H

#include <vgc/topology/api.h>
#include <vgc/topology/cell.h>

namespace vgc::topology {

class VGC_TOPOLOGY_API InbetweenVertex final
    : public SpatioTemporalCell<VertexCell, InbetweenCell> {

private:
    friend detail::Operations;

    explicit InbetweenVertex(core::Id id) noexcept
        : SpatioTemporalCell(id) {
    }

public:
    VGC_TOPOLOGY_DEFINE_SPATIOTEMPORAL_CELL_CAST_METHODS(Inbetween, Vertex)

    geometry::Vec2d position(core::AnimTime /*t*/) const override {
        // XXX todo interp
        return geometry::Vec2d();
    }
};

} // namespace vgc::topology

#endif // VGC_TOPOLOGY_INBETWEENVERTEX_H
