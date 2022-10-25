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

void wrap_arithmetic(py::module& m);
void wrap_arrays(py::module& m);
void wrap_color(py::module& m);
void wrap_exceptions(py::module& m);
void wrap_format(py::module& m);
void wrap_io(py::module& m);
void wrap_object(py::module& m);
void wrap_parse(py::module& m);
void wrap_paths(py::module& m);
void wrap_signal(py::module& m);
void wrap_stopwatch(py::module& m);
void wrap_stringid(py::module& m);

PYBIND11_MODULE(core, m) {
    wrap_arithmetic(m);
    wrap_arrays(m);
    wrap_color(m);
    wrap_exceptions(m);
    wrap_format(m);
    wrap_io(m);
    wrap_object(m);
    wrap_parse(m);
    wrap_paths(m);
    wrap_signal(m);
    wrap_stopwatch(m);
    wrap_stringid(m);
}
