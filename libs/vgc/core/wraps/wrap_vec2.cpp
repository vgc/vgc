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

#include <vgc/core/wraps/common.h>
#include <pybind11/operators.h>
#include <vgc/core/vec2d.h>
#include <vgc/core/vec2f.h>

namespace  {

template<typename This, typename T>
typename std::enable_if<std::is_same<typename This::value_type, T>::value>::type
wrap_vec2x(py::module& m, const std::string& thisTypeName, T relTol)
{
    py::class_<This>(m, thisTypeName.c_str())

        // Note: in Python, Vec2x() does zero-initialization, unlike in C++
        .def(py::init([]() { return This(0, 0); } ))
        .def(py::init<T, T>())
        .def(py::init([](const std::string& s) { return vgc::core::parse<This>(s); } ))
        .def(py::init([thisTypeName](py::tuple t) {
            if (t.size() != 2) throw py::value_error("Tuple length must be 2 for conversion to " + thisTypeName);
            return This(t[0].cast<T>(), t[1].cast<T>()); } ))
        .def(py::init<This>())

        .def("__getitem__", [](const This& v, int i) {
            if (i < 0 || i >= 2) throw py::index_error();
            return v[i]; })
        .def("__setitem__", [](This& v, int i, T x) {
            if (i < 0 || i >= 2) throw py::index_error();
            v[i] = x; })

        .def_property("x", &This::x, &This::setX)
        .def_property("y", &This::y, &This::setY)

        .def(py::self += py::self)
        .def(py::self + py::self)
        .def(+ py::self)
        .def(py::self -= py::self)
        .def(py::self - py::self)
        .def(- py::self)
        .def(py::self *= T())
        .def(T() * py::self)
        .def(py::self * T())
        .def(py::self /= T())
        .def(py::self / T())
        .def(py::self == py::self)
        .def(py::self != py::self)
        .def(py::self < py::self)
        .def(py::self <= py::self)
        .def(py::self > py::self)
        .def(py::self >= py::self)

        .def("length", &This::length)
        .def("squaredLength", &This::squaredLength)
        .def("normalize", &This::normalize)
        .def("normalized", &This::normalized)
        .def("orthogonalize", &This::orthogonalize)
        .def("orthogonalized", &This::orthogonalized)
        .def("dot", &This::dot)
        .def("det", &This::det)
        .def("angle", &This::angle)
        .def("isClose",  &This::isClose,  "b"_a, "relTol"_a = relTol, "absTol"_a = 0)
        .def("allClose", &This::allClose, "b"_a, "relTol"_a = relTol, "absTol"_a = 0)
        .def("isNear",   &This::isNear,   "b"_a, "absTol"_a)
        .def("allNear",  &This::allNear,  "b"_a, "absTol"_a)

        .def("__repr__", [](const This& v) { return toString(v); })
    ;
}

} // namespace

void wrap_vec2(py::module& m)
{
    wrap_vec2x<vgc::core::Vec2d>(m, "Vec2d", 1e-9);
    wrap_vec2x<vgc::core::Vec2f>(m, "Vec2f", 1e-5f);
}
