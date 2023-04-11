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

#include <vgc/topology/keyedge.h>

namespace vgc::topology {

const std::shared_ptr<const geometry::CurveSampleArray>&
KeyEdge::computeSampling(const geometry::CurveSamplingParameters& parameters) const {

    if (!sampling_ || parameters != lastSamplingParameters_) {
        geometry::Curve curve;
        geometry::CurveSampleArray sampling;
        curve.setPositions(points());
        curve.setWidths(widths());
        curve.sampleRange(parameters, sampling);
        if (sampling.length()) {
            auto it = sampling.begin();
            geometry::Vec2d lastPoint = it->position();
            double s = 0;
            for (++it; it != sampling.end(); ++it) {
                geometry::Vec2d point = it->position();
                s += (point - lastPoint).length();
                it->setS(s);
                lastPoint = point;
            }
        }
        sampling_ =
            std::make_shared<const geometry::CurveSampleArray>(std::move(sampling));
        snappedSampling_.reset();
    }

    // TODO: transform and snap

    if (isGeometryDirty_ || !snappedSampling_) {
        snappedSampling_ = sampling_;
        isGeometryDirty_ = false;
    }

    return snappedSampling_;
}

void KeyEdge::dirtyInputSampling_() {
    snappedSampling_.reset();
    sampling_.reset();
}

} // namespace vgc::topology
