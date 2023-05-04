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

#ifndef VGC_VACOMPLEX_KEYVERTEX_H
#define VGC_VACOMPLEX_KEYVERTEX_H

#include <vgc/geometry/vec2d.h>
#include <vgc/vacomplex/api.h>
#include <vgc/vacomplex/cell.h>

namespace vgc::vacomplex {

// dev note: position could be a variant<vec2d, func, provider>
//           provider could have a dirty flag to not update data, especially important for
//           big value types like curve geometry in edges.

class VGC_VACOMPLEX_API KeyVertex final : public SpatioTemporalCell<VertexCell, KeyCell> {
private:
    friend detail::Operations;

    explicit KeyVertex(core::Id id, core::AnimTime t) noexcept
        : SpatioTemporalCell(id, t) {
    }

public:
    VGC_VACOMPLEX_DEFINE_SPATIOTEMPORAL_CELL_CAST_METHODS(Key, Vertex)

    constexpr geometry::Vec2d position() const {
        hasGeometryBeenQueriedSinceLastDirtyEvent_ = true;
        return position_;
    }

    geometry::Vec2d position(core::AnimTime /*t*/) const override {
        hasGeometryBeenQueriedSinceLastDirtyEvent_ = true;
        return position_;
    }

private:
    geometry::Vec2d position_;
};

} // namespace vgc::vacomplex

#endif // VGC_VACOMPLEX_KEYVERTEX_H
