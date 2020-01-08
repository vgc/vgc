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

#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <vgc/core/color.h>

namespace py = pybind11;
using vgc::core::Color;

void wrap_color(py::module& m)
{
    py::class_<Color>(m, "Color")

        // Note: in Python, Color() does (0,0,0,1)-initialization, unlike in C++
        .def(py::init([]() { return Color(0, 0, 0); } ))
        .def(py::init<double, double, double>())
        .def(py::init<double, double, double, double>())
        .def(py::init<Color>())

        .def("__getitem__", [](const Color& v, int i) {
            if (i < 0 || i >= 4) throw py::index_error();
            return v[i]; })
        .def("__setitem__", [](Color& v, int i, double x) {
            if (i < 0 || i >= 4) throw py::index_error();
            v[i] = x; })

        .def_property("r", &Color::r, &Color::setR)
        .def_property("g", &Color::g, &Color::setG)
        .def_property("b", &Color::b, &Color::setB)
        .def_property("a", &Color::a, &Color::setA)

        .def(py::self += py::self)
        .def(py::self + py::self)
        .def(py::self -= py::self)
        .def(py::self - py::self)
        .def(py::self *= double())
        .def(double() * py::self)
        .def(py::self * double())
        .def(py::self /= double())
        .def(py::self / double())
        .def(py::self == py::self)
        .def(py::self != py::self)
        .def(py::self < py::self)
        .def(py::self <= py::self)
        .def(py::self > py::self)
        .def(py::self >= py::self)

        .def("__repr__", [](const Color& c) { return vgc::core::toString(c); })

    ;

    m.def("toColor", &vgc::core::toColor);
}
