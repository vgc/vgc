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

#include <vgc/core/wraps/common.h>

void wrap_curve(py::module& m);
void wrap_intersect(py::module& m);
void wrap_mat(py::module& m);
void wrap_range(py::module& m);
void wrap_rect(py::module& m);
void wrap_triangle(py::module& m);
void wrap_vec(py::module& m);

PYBIND11_MODULE(geometry, m) {
    wrap_curve(m);
    wrap_intersect(m);
    wrap_mat(m);
    wrap_range(m);
    wrap_rect(m);
    wrap_triangle(m);
    wrap_vec(m);
}
