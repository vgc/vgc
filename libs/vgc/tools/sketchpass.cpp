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

#include <vgc/tools/sketchpass.h>

namespace vgc::tools {

void SketchPass::reset() {
    buffer_.clear();
    lastNumStablePoints_ = 0;
    lastNumStableInputPoints_ = 0;
    doReset();
}

void SketchPass::updateFrom(const SketchPointBuffer& input) {
    areCumulativeChordalDistancesUpdated_ = false;
    Int numStablePoints = doUpdateFrom(input, lastNumStableInputPoints_);
    if (!areCumulativeChordalDistancesUpdated_) {
        updateCumulativeChordalDistances();
    }
    buffer_.setNumStablePoints(numStablePoints);
    lastNumStablePoints_ = numStablePoints;
    lastNumStableInputPoints_ = input.numStablePoints();
}

void SketchPass::updateCumulativeChordalDistances() {

    Int n = numPoints();
    if (n == 0) {
        return;
    }

    Int start = std::max<Int>(lastNumStablePoints_, 1);
    const SketchPoint* p1 = &getPoint(start - 1);
    double s = p1->s();
    for (Int i = start; i < n; ++i) {
        SketchPoint& p2 = getPointRef(i);
        s += (p2.position() - p1->position()).length();
        p2.setS(s);
        p1 = &p2;
    }

    areCumulativeChordalDistancesUpdated_ = true;
}

void SketchPass::doReset() {
}

} // namespace vgc::tools
