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

#include <vgc/vacomplex/keyvertex.h>

namespace vgc::vacomplex {

geometry::Rect2d KeyVertex::boundingBox() const {
    return geometry::Rect2d(position_, position_);
}

void KeyVertex::substituteKeyVertex_(KeyVertex*, KeyVertex*) {
    // no-op
}

void KeyVertex::substituteKeyEdge_(const KeyHalfedge&, const KeyHalfedge&) {
    // no-op
}

void KeyVertex::debugPrint_(core::StringWriter& out) {
    out << core::format( //
        "{:<12} position=({:.2f}, {:.2f})",
        "KeyVertex",
        position_[0],
        position_[1]);
}

} // namespace vgc::vacomplex
