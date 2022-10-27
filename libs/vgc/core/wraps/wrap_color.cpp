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

#include <pybind11/operators.h>
#include <vgc/core/wraps/array.h>
#include <vgc/core/wraps/common.h>

#include <vgc/core/color.h>

namespace py = pybind11;
using This = vgc::core::Color;
using vgc::core::Color;

void wrap_color(py::module& m) {
    auto self2 = py::self; // Fix https://github.com/pybind/pybind11/issues/1893

    py::class_<Color>(m, "Color")

        .def(py::init([]() { return Color(); }))
        .def(py::init<float, float, float>())
        .def(py::init<float, float, float, float>())
        .def(py::init([](const std::string& s) { return vgc::core::parse<This>(s); }))
        .def(py::init<Color>())

        .def(
            "__getitem__",
            [](const Color& v, int i) {
                if (i < 0 || i >= 4)
                    throw py::index_error();
                return v[i];
            })
        .def(
            "__setitem__",
            [](Color& v, int i, float x) {
                if (i < 0 || i >= 4)
                    throw py::index_error();
                v[i] = x;
            })

        .def_property("r", &Color::r, &Color::setR)
        .def_property("g", &Color::g, &Color::setG)
        .def_property("b", &Color::b, &Color::setB)
        .def_property("a", &Color::a, &Color::setA)

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

        .def("__repr__", [](const Color& c) { return vgc::core::toString(c); })

        ;

    vgc::core::wraps::wrap_1darray<vgc::core::ColorArray>(m, "Color");
}
