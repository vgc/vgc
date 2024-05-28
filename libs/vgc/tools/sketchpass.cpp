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

void SketchPointBuffer::updateChordLengths() {

    // Get all the unstable points, except the first if it doesn't have a previous point
    auto points = unstablePoints();
    if (numStablePoints() == 0 && !points.isEmpty()) {
        points = points.subspan(1);
    }

    // Compute chord-lengths
    for (SketchPoint& p2 : points) {
        const SketchPoint& p1 = *(&p2 - 1);
        double ds = (p2.position() - p1.position()).length();
        p2.setS(p1.s() + ds);
    }
}

void SketchPass::reset() {
    output_.reset();
    doReset();
}

void SketchPass::updateFrom(const SketchPointBuffer& input) {
    SketchPointBuffer& output = output_;
    doUpdateFrom(input, output);
}

void SketchPass::doReset() {
}

} // namespace vgc::tools
