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

template<typename TVec>
TVec vecFromTuple(py::tuple t) {
    using T = typename TVec::ScalarType;
    constexpr Int dimension = TVec::dimension;
    if (t.size() != dimension) {
        throw py::value_error("Tuple length does not match dimension of Vec type.");
    }
    if constexpr (dimension == 2) {
        return TVec(t[0].cast<T>(), t[1].cast<T>());
    }
    else if constexpr (dimension == 3) {
        return TVec(t[0].cast<T>(), t[1].cast<T>(), t[2].cast<T>());
    }
    else if constexpr (dimension == 4) {
        return TVec(t[0].cast<T>(), t[1].cast<T>(), t[2].cast<T>(), t[3].cast<T>());
    }
    else {
        static_assert("Vec type not supported");
    }
}

template<typename TVec>
TVec vecFromSequence(py::sequence s) {
    using T = typename TVec::ScalarType;
    constexpr Int dimension = TVec::dimension;
    if (s.size() != dimension) {
        throw py::value_error("Sequence length does not match dimension of Vec type.");
    }
    if constexpr (dimension == 2) {
        return TVec(s[0].cast<T>(), s[1].cast<T>());
    }
    else if constexpr (dimension == 3) {
        return TVec(s[0].cast<T>(), s[1].cast<T>(), s[2].cast<T>());
    }
    else if constexpr (dimension == 4) {
        return TVec(s[0].cast<T>(), s[1].cast<T>(), s[2].cast<T>(), s[3].cast<T>());
    }
    else {
        static_assert("Vec type not supported");
    }
}

} // namespace vgc::geometry::wraps

#endif // VGC_GEOMETRY_WRAPS_VEC_H
