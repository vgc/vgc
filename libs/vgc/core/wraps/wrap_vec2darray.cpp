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
#include <vgc/core/vec2darray.h>

namespace py = pybind11;
using This = vgc::core::Vec2dArray;
using vgc::core::Vec2d;

// See wrap_doublearray.cpp for details

void wrap_vec2darray(py::module& m)
{
    using T = typename This::value_type;
    using It = typename This::iterator;

    py::class_<This>(m, "Vec2dArray")

        .def(py::init<>())
        .def(py::init([](int size) { return This(size, Vec2d(0, 0)); } )) // Important: Vec2d(0, 0), otherwise uninitialized
        .def(py::init([](int size, const T& value) { return This(size, value); } ))
        .def(py::init([](py::sequence s) {
            This res;
            for (auto it : s) {
                auto t = py::reinterpret_borrow<py::tuple>(it);
                if (t.size() != 2) throw py::value_error("size of tuple must be 2 for conversion to Vec2d");
                res.append(Vec2d(t[0].cast<double>(), t[1].cast<double>()));
            }
            return res; } ))
        .def(py::init<const This&>())

        // TODO: Use Python semantics: a[i] OK for i in [-n, n-1]
        .def("__getitem__", [](const This& a, int i) { return a[i]; })
        .def("__setitem__", [](This& a, int i, const T& value) { a[i] = value; })
        .def("__len__", &This::size)
        .def("__iter__",
            [](This& a) {
                return py::make_iterator<py::return_value_policy::reference_internal, It, It, T&>(a.begin(), a.end());
            },
            py::keep_alive<0, 1>()
        )

        .def("append", [](This& a, const T& value) { a.append(value); })

        .def(py::self == py::self)
        .def(py::self != py::self)

        .def("__repr__", [](const This& a) { return toString(a); })
    ;

    m.def("toVec2dArray", &vgc::core::toVec2dArray);
}
