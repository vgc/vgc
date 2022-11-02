// Copyright 2021 The VGC Developers
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

#include <vgc/core/array.h>

#include <vgc/core/wraps/array.h>
#include <vgc/core/wraps/common.h>

void wrap_arrays(py::module& m) {
    vgc::core::wraps::wrap_array<double>(m, "Double");
    vgc::core::wraps::wrap_array<float>(m, "Float");
    vgc::core::wraps::wrap_array<vgc::Int>(m, "Int");
}

namespace vgc::core::wraps::detail {

uintptr_t intArrayByCopyDataAddress(IntArray array) {
    return reinterpret_cast<uintptr_t>(array.data());
}

uintptr_t intArrayDataAddress(const IntArray& array) {
    return reinterpret_cast<uintptr_t>(array.data());
}

uintptr_t sharedConstIntArrayDataAddress(const SharedConstIntArray& array) {
    return reinterpret_cast<uintptr_t>(array.get().data());
}

} // namespace vgc::core::wraps::detail

void wrap_arrays_detail(py::module& m) {
    namespace detail = vgc::core::wraps::detail;
    m.def("intArrayByCopyDataAddress", &detail::intArrayByCopyDataAddress);
    m.def("intArrayDataAddress", &detail::intArrayDataAddress);
    m.def("sharedConstIntArrayDataAddress", detail::sharedConstIntArrayDataAddress);
}
