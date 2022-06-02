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

#ifndef VGC_GEOMETRY_WRAPS_VEC_H
#define VGC_GEOMETRY_WRAPS_VEC_H

#include <vgc/core/templateutil.h>
#include <vgc/core/wraps/common.h>
#include <vgc/geometry/vec2d.h>
#include <vgc/geometry/vec2f.h>

namespace vgc::geometry::wraps {

template<typename TVec, VGC_REQUIRES(TVec::dimension == 2)>
TVec vecFromTuple(py::tuple t) {
    using T = typename TVec::value_type;
    if (t.size() != 2) {
        throw py::value_error("Tuple length must be 2 to be convertible to a Vec2 type.");
    }
    return TVec(t[0].cast<T>(), t[1].cast<T>());
}

} // namespace vgc::geometry::wraps

#endif // VGC_GEOMETRY_WRAPS_VEC_H
