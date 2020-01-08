// Copyright 2020 The VGC Developers
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

#include <pybind11/pybind11.h>

namespace py = pybind11;

void wrap_document(py::module& m);
void wrap_element(py::module& m);
void wrap_exceptions(py::module& m);
void wrap_node(py::module& m);
void wrap_xmlformattingstyle(py::module& m);

PYBIND11_MODULE(dom, m) {
    wrap_exceptions(m);
    wrap_node(m);
    wrap_xmlformattingstyle(m);

    wrap_document(m); // dependencies: node, xmlformattingstyle
    wrap_element(m); // dependencies: node
}
