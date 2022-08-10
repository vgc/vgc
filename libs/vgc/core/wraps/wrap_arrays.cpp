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

#include <vgc/core/wraps/array.h>

namespace {

template<typename This>
void wrap_1darray(py::module& m, const std::string& valueTypeName) {
    using T = typename This::value_type;
    std::string thisTypeName = valueTypeName + "Array";
    py::class_<This> c(m, thisTypeName.c_str());
    vgc::core::wraps::defineArrayCommonMethods(c);
    c.def(py::init([](py::sequence s) {
        This res;
        for (auto x : s) {
            res.append(x.cast<T>());
        }
        return res;
    }));
}

} // namespace

void wrap_arrays(py::module& m) {
    wrap_1darray<vgc::core::DoubleArray>(m, "Double");
    wrap_1darray<vgc::core::FloatArray>(m, "Float");
    wrap_1darray<vgc::core::IntArray>(m, "Int");
}
