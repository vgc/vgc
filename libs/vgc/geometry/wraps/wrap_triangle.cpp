// Copyright 2022 The VGC Developers
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

#include <vgc/geometry/triangle2d.h>
#include <vgc/geometry/triangle2f.h>

#include <vgc/core/wraps/class.h>
#include <vgc/core/wraps/common.h>
#include <vgc/geometry/wraps/vec.h>

namespace {

template<typename T>
struct Triangle2_ {};

template<>
struct Triangle2_<float> {
    using type = vgc::geometry::Triangle2f;
};

template<>
struct Triangle2_<double> {
    using type = vgc::geometry::Triangle2d;
};

template<typename T>
using Triangle2 = typename Triangle2_<T>::type;

template<typename T>
struct Vec2_ {};

template<>
struct Vec2_<float> {
    using type = vgc::geometry::Vec2f;
};

template<>
struct Vec2_<double> {
    using type = vgc::geometry::Vec2d;
};

template<typename T>
using Vec2 = typename Vec2_<T>::type;

template<typename T>
void wrap_triangle(py::module& m, const std::string& name) {

    // Fix https://github.com/pybind/pybind11/issues/1893
    auto self2 = py::self;

    using This = Triangle2<T>;
    using Vec2x = Vec2<T>;

    vgc::core::wraps::Class<This>(m, name.c_str())

        .def(py::init<>())
        .def(py::init<Vec2x, Vec2x, Vec2x>())
        .def(py::init<This>())
        .def(py::init([](const std::string& s) { return vgc::core::parse<This>(s); }))

        .def(
            "__getitem__",
            [](const This& t, int i) {
                if (i < 0 || i >= 3) {
                    throw py::index_error();
                }
                return t[i];
            })

        .def(
            "__setitem__",
            [](This& t, int i, const Vec2x& v) {
                if (i < 0 || i >= 3) {
                    throw py::index_error();
                }
                t[i] = v;
            })

        .def_property("a", &This::a, py::overload_cast<const Vec2x&>(&This::setA))
        .def_property("b", &This::b, py::overload_cast<const Vec2x&>(&This::setB))
        .def_property("c", &This::c, py::overload_cast<const Vec2x&>(&This::setC))

        .def("setA", py::overload_cast<T, T>(&This::setA))
        .def("setB", py::overload_cast<T, T>(&This::setB))
        .def("setC", py::overload_cast<T, T>(&This::setC))

        .def(py::self += py::self)
        .def(py::self + py::self)
        .def(+py::self)
        .def(self2 -= py::self)
        .def(py::self - py::self)
        .def(-py::self)
        .def(py::self *= T())
        .def(T() * py::self)
        .def(py::self * T())
        .def(py::self /= T())
        .def(py::self / T())
        .def(py::self == py::self)
        .def(py::self != py::self)

        .def("__repr__", [](const This& t) { return vgc::core::toString(t); });
}

} // namespace

void wrap_triangle(py::module& m) {
    wrap_triangle<double>(m, "Triangle2d");
    wrap_triangle<float>(m, "Triangle2f");
}
