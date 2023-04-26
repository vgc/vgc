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

#ifndef VGC_TOPOLOGY_KEYFACE_H
#define VGC_TOPOLOGY_KEYFACE_H

#include <initializer_list>

#include <vgc/geometry/curve.h>
#include <vgc/geometry/curves2d.h>
#include <vgc/geometry/vec2d.h>
#include <vgc/topology/api.h>
#include <vgc/topology/cell.h>
#include <vgc/topology/keycycle.h>

namespace vgc::topology {

namespace detail {

VGC_TOPOLOGY_API
core::Array<KeyCycle> computeKeyFaceCandidateAt(
    geometry::Vec2d position,
    VacGroup* group,
    core::FloatArray& trianglesBuffer,
    geometry::WindingRule windingRule = geometry::WindingRule::Odd,
    core::AnimTime t = {});

VGC_TOPOLOGY_API
bool computeKeyFaceFillTriangles(
    const core::Array<KeyCycle>& cycles,
    core::FloatArray& trianglesBuffer,
    geometry::WindingRule windingRule = geometry::WindingRule::Odd);

VGC_TOPOLOGY_API
bool computeKeyFaceFillTriangles(
    const core::Array<KeyCycle>& cycles,
    core::FloatArray& trianglesBuffer,
    const geometry::CurveSamplingParameters& parameters,
    geometry::WindingRule windingRule = geometry::WindingRule::Odd);

} // namespace detail

class VGC_TOPOLOGY_API KeyFace final : public SpatioTemporalCell<FaceCell, KeyCell> {
private:
    friend detail::Operations;

    explicit KeyFace(core::Id id, core::AnimTime t) noexcept
        : SpatioTemporalCell(id, t) {
    }

public:
    VGC_TOPOLOGY_DEFINE_SPATIOTEMPORAL_CELL_CAST_METHODS(Key, Face)

    const core::Array<KeyCycle>& cycles() const {
        return cycles_;
    }

private:
    core::Array<KeyCycle> cycles_;
};

} // namespace vgc::topology

#endif // VGC_TOPOLOGY_KEYFACE_H
