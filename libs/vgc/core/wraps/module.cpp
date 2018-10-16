// Copyright 2018 The VGC Developers
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

void wrap_charutil(py::module& m);
void wrap_color(py::module& m);
void wrap_doublearray(py::module& m);
void wrap_exceptions(py::module& m);
void wrap_io(py::module& m);
void wrap_object(py::module& m);
void wrap_stopwatch(py::module& m);
void wrap_stringutil(py::module& m);
void wrap_vec2d(py::module& m);
void wrap_vec2darray(py::module& m);

PYBIND11_MODULE(core, m) {
    wrap_charutil(m);
    wrap_color(m);
    wrap_doublearray(m);
    wrap_exceptions(m);
    wrap_io(m);
    wrap_object(m);
    wrap_stopwatch(m);
    wrap_stringutil(m);
    wrap_vec2d(m);
    wrap_vec2darray(m);
}
