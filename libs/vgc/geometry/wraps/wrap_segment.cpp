// Copyright 2024 The VGC Developers
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

#include <vgc/geometry/segment2d.h>
#include <vgc/geometry/segment2f.h>
#include <vgc/geometry/vec.h>

#include <vgc/core/wraps/class.h>
#include <vgc/core/wraps/common.h>
#include <vgc/geometry/wraps/vec.h>

namespace {

template<typename T>
struct Segment2_ {};

template<>
struct Segment2_<float> {
    using type = vgc::geometry::Segment2f;
};

template<>
struct Segment2_<double> {
    using type = vgc::geometry::Segment2d;
};

template<typename T>
using Segment2 = typename Segment2_<T>::type;

template<typename T>
void wrap_segment(py::module& m, const std::string& name) {

    // Fix https://github.com/pybind/pybind11/issues/1893
    auto self2 = py::self;

    using This = Segment2<T>;
    using Vec2x = vgc::geometry::Vec<2, T>;

    vgc::core::wraps::Class<This>(m, name.c_str())

        .def(py::init<>())
        .def(py::init<Vec2x, Vec2x>())
        .def(py::init<T, T, T, T>())
        .def(py::init<This>())
        .def(py::init([](const std::string& s) { return vgc::core::parse<This>(s); }))

        .def(
            "__getitem__",
            [](const This& t, int i) {
                if (i < 0 || i >= 2) {
                    throw py::index_error();
                }
                return t[i];
            })

        .def(
            "__setitem__",
            [](This& t, int i, const Vec2x& v) {
                if (i < 0 || i >= 2) {
                    throw py::index_error();
                }
                t[i] = v;
            })

        .def_property("a", &This::a, py::overload_cast<const Vec2x&>(&This::setA))
        .def_property("b", &This::b, py::overload_cast<const Vec2x&>(&This::setB))

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

        .def("isDegenerate", &This::isDegenerate)

        .def("__repr__", [](const This& t) { return vgc::core::toString(t); });
}

} // namespace

void wrap_segment(py::module& m) {
    wrap_segment<double>(m, "Segment2d");
    wrap_segment<float>(m, "Segment2f");
}