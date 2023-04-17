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

#include <vgc/topology/keyface.h>
#include <vgc/topology/vac.h>

#include <unordered_set>

#include <vgc/geometry/curves2d.h>

namespace vgc::topology {

namespace detail {

namespace {

bool computeKeyFaceFillTriangles(
    const core::Array<KeyCycle>& cycles,
    core::FloatArray& trianglesBuffer,
    const geometry::CurveSamplingParameters* parameters,
    geometry::WindingRule windingRule) {

    trianglesBuffer.clear();

    geometry::Curves2d curves2d;
    for (const vacomplex::KeyCycle& cycle : cycles) {
        KeyVertex* kv = cycle.steinerVertex();
        if (kv) {
            geometry::Vec2d p = kv->position();
            curves2d.moveTo(p[0], p[1]);
        }
        else {
            bool isFirst = true;
            for (const KeyHalfedge& khe : cycle.halfedges()) {
                const KeyEdge* ke = khe.edge();
                if (ke) {
                    const geometry::CurveSampleArray& samples =
                        parameters ? ke->computeSampling(*parameters)
                                   : ke->sampling().samples();
                    if (khe.direction()) {
                        for (const geometry::CurveSample& s : samples) {
                            geometry::Vec2d p = s.position();
                            if (isFirst) {
                                curves2d.moveTo(p[0], p[1]);
                                isFirst = false;
                            }
                            else {
                                curves2d.lineTo(p[0], p[1]);
                            }
                        }
                    }
                    else {
                        for (auto it = samples.rbegin(); it != samples.rend(); ++it) {
                            const geometry::CurveSample& s = *it;
                            geometry::Vec2d p = s.position();
                            if (isFirst) {
                                curves2d.moveTo(p[0], p[1]);
                                isFirst = false;
                            }
                            else {
                                curves2d.lineTo(p[0], p[1]);
                            }
                        }
                    }
                }
                else {
                    // TODO: log error
                    return false;
                }
            }
        }
        curves2d.close();
    }

    auto params = geometry::Curves2dSampleParams::adaptive();
    curves2d.fill(trianglesBuffer, params, windingRule);

    return true;
}

} // namespace

bool computeKeyFaceFillTriangles(
    const core::Array<KeyCycle>& cycles,
    core::FloatArray& trianglesBuffer,
    geometry::WindingRule windingRule) {

    return computeKeyFaceFillTriangles(cycles, trianglesBuffer, nullptr, windingRule);
}

bool computeKeyFaceFillTriangles(
    const core::Array<KeyCycle>& cycles,
    core::FloatArray& trianglesBuffer,
    const geometry::CurveSamplingParameters& parameters,
    geometry::WindingRule windingRule) {

    return computeKeyFaceFillTriangles(cycles, trianglesBuffer, &parameters, windingRule);
}

} // namespace detail

} // namespace vgc::topology
