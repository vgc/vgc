// Copyright 2017 The VGC Developers
// See the COPYRIGHT file at the top-level directory of this distribution
// and at https://github.com/vgc.io/vgc/blob/master/COPYRIGHT
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
#include <pybind11/operators.h>
#include <vgc/geometry/vec2d.h>

namespace py = pybind11;
using vgc::geometry::Vec2d;

void wrap_vec2d(py::module& m)
{
    py::class_<Vec2d>(m, "Vec2d")

        .def(py::init<>())
        .def(py::init<double, double>())

        .def("__getitem__", [](const Vec2d& v, int i) {
            if (i < 0 || i >= 2) throw py::index_error();
            return v[i]; })
        .def("__setitem__", [](Vec2d& v, int i, double x) {
            if (i < 0 || i >= 2) throw py::index_error();
            v[i] = x; })

        .def_property("x", &Vec2d::x, &Vec2d::setX)
        .def_property("y", &Vec2d::y, &Vec2d::setY)

        .def(py::self += py::self)
        .def(py::self + py::self)
        .def(py::self *= float())
        .def(double() * py::self)
        .def(py::self * double())

        .def("length", &Vec2d::length)
        .def("squaredLength", &Vec2d::squaredLength)

        .def("__repr__", [](const Vec2d& v) {
            return "("
                   + std::to_string(v[0]) + ", "
                   + std::to_string(v[1]) + ")"; })
    ;
}
