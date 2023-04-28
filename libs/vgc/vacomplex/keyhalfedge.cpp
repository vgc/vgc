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

#include <vgc/vacomplex/keyhalfedge.h>

namespace vgc::vacomplex {

KeyHalfedge KeyHalfedge::next() const {
    KeyHalfedge result = opposite();
    double a = endAngle();
    double minAngle = core::DoubleInfinity;
    KeyVertex* kv = endVertex();

    auto angleDist = [](double a, double b) {
        double c = b - a;
        while (c < 0) {
            c += core::pi * 2;
        }
        return c;
    };

    for (Cell* cell : endVertex()->star()) {
        KeyEdge* ke = cell->toKeyEdge();
        if (ke) {
            bool isOtherEdge = ke != edge_;
            if (ke->isStartVertex(kv) && (isOtherEdge || direction_)) {
                double d = angleDist(ke->startAngle(), a);
                if (d < minAngle) {
                    minAngle = d;
                    result = KeyHalfedge(ke, true);
                }
            }
            if (ke->isEndVertex(kv) && (isOtherEdge || !direction_)) {
                double d = angleDist(ke->endAngle(), a);
                if (d < minAngle) {
                    minAngle = d;
                    result = KeyHalfedge(ke, false);
                }
            }
        }
    }

    return result;
}

} // namespace vgc::vacomplex
