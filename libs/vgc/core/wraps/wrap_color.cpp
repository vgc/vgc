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

#include <vgc/core/color.h>

#include <vgc/core/wraps/array.h>
#include <vgc/core/wraps/common.h>

void wrap_color(py::module& m) {

    using This = vgc::core::Color;

    auto self2 = py::self; // Fix https://github.com/pybind/pybind11/issues/1893

    vgc::core::wraps::Class<This>(m, "Color")
        .def(py::init([]() { return This(); }))
        .def(py::init<float, float, float>())
        .def(py::init<float, float, float, float>())
        .def(py::init([](const std::string& s) { return vgc::core::parse<This>(s); }))
        .def(py::init<This>())

        .def(
            "__getitem__",
            [](const This& v, int i) {
                if (i < 0 || i >= 4)
                    throw py::index_error();
                return v[i];
            })
        .def(
            "__setitem__",
            [](This& v, int i, float x) {
                if (i < 0 || i >= 4)
                    throw py::index_error();
                v[i] = x;
            })

        .def_property("r", &This::r, &This::setR)
        .def_property("g", &This::g, &This::setG)
        .def_property("b", &This::b, &This::setB)
        .def_property("a", &This::a, &This::setA)

        .def(py::self += py::self)
        .def(py::self + py::self)
        .def(self2 -= py::self)
        .def(py::self - py::self)
        .def(py::self *= float())
        .def(float() * py::self)
        .def(py::self * float())
        .def(py::self /= float())
        .def(py::self / float())
        .def(py::self == py::self)
        .def(py::self != py::self)
        .def(py::self < py::self)
        .def(py::self <= py::self)
        .def(py::self > py::self)
        .def(py::self >= py::self)

        .def("__repr__", [](const This& c) { return vgc::core::toString(c); })

        ;

    vgc::core::wraps::wrap_array<vgc::core::Color>(m, "Color");
}
